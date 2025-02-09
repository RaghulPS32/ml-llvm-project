//===- InlineSimple.cpp - Code to perform simple function inlining --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements bottom-up inlining of functions into callees.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/InitializePasses.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

#define DEBUG_TYPE "inline"

static cl::opt<bool> EnableLoopInlining
    ("enable-loop-inline", cl::init(false), cl::Hidden);

namespace {

/// Actual inliner pass implementation.
///
/// The common implementation of the inlining logic is shared between this
/// inliner pass and the always inliner pass. The two passes use different cost
/// analyses to determine when to inline.
class SimpleInliner : public LegacyInlinerBase {

  InlineParams Params;

public:
  SimpleInliner() : LegacyInlinerBase(ID), Params(llvm::getInlineParams()) {
    initializeSimpleInlinerPass(*PassRegistry::getPassRegistry());
  }

  explicit SimpleInliner(InlineParams Params)
      : LegacyInlinerBase(ID), Params(std::move(Params)) {
    initializeSimpleInlinerPass(*PassRegistry::getPassRegistry());
  }

  static char ID; // Pass identification, replacement for typeid

  InlineCost getInlineCost(CallSite CS) override {
    Function *Callee = CS.getCalledFunction();
    TargetTransformInfo &TTI = TTIWP->getTTI(*Callee);

    bool RemarksEnabled = false;
    const auto &BBs = CS.getCaller()->getBasicBlockList();
    if (!BBs.empty()) {
      auto DI = OptimizationRemark(DEBUG_TYPE, "", DebugLoc(), &BBs.front());
      if (DI.isEnabled())
        RemarksEnabled = true;
    }
    OptimizationRemarkEmitter ORE(CS.getCaller());

    std::function<AssumptionCache &(Function &)> GetAssumptionCache =
        [&](Function &F) -> AssumptionCache & {
      return ACT->getAssumptionCache(F);
    };

    if (EnableLoopInlining) {
      Function *CallFn = CS.getInstruction()->getParent()->getParent();
      if (CallFn) {
        const DominatorTree DT(*CallFn);
        LoopInfo LI(DT);
        if (isa<CallInst>(CS.getInstruction())) {
          Loop *L = LI.getLoopFor(CS.getInstruction()->getParent());
          if (L && isInlineViable(*Callee)) {
            // Rule#1: Inline all the callsite inside the innermost loop
            if (L->empty()) {
              LLVM_DEBUG(dbgs() << "IITH: Inlined call in innermost loop\n");
              LLVM_DEBUG(dbgs() << "Caller : " << CallFn->getName() << "\n");
              LLVM_DEBUG(dbgs() << "Callee : " << Callee->getName() << "\n\n");
              return llvm::InlineCost::
                          getAlways("IITH: Inlined call in innermost loop");
            } 
            /* else {
            //Rule#2: Inline the callsites in the loop if there is an innmost loop adjacent to it
              bool EmptySubloop = false;
              for (auto SubLoop : L->getSubLoops()) {
                if (SubLoop->empty()) {
                  EmptySubloop = true;
                  break;
                }
              }
              if (EmptySubloop) {
                LLVM_DEBUG(dbgs() << "IITH: Inlined call in non-inner-most loop\n");
                LLVM_DEBUG(dbgs() << "Caller : " << CallFn->getName() << "\n");
                LLVM_DEBUG(dbgs() << "Callee : " << Callee->getName() << "\n\n");
                return llvm::InlineCost::
                            getAlways("IITH: Inlined call in non-inner-most loop");
              }
            } */
          }
        }
      }
    }

    return llvm::getInlineCost(
        cast<CallBase>(*CS.getInstruction()), Params, TTI, GetAssumptionCache,
        /*GetBFI=*/None, PSI, RemarksEnabled ? &ORE : nullptr);
  }

  bool runOnSCC(CallGraphSCC &SCC) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  TargetTransformInfoWrapperPass *TTIWP;

};

} // end anonymous namespace

char SimpleInliner::ID = 0;
INITIALIZE_PASS_BEGIN(SimpleInliner, "inline", "Function Integration/Inlining",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ProfileSummaryInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_END(SimpleInliner, "inline", "Function Integration/Inlining",
                    false, false)

Pass *llvm::createFunctionInliningPass() { return new SimpleInliner(); }

Pass *llvm::createFunctionInliningPass(int Threshold) {
  return new SimpleInliner(llvm::getInlineParams(Threshold));
}

Pass *llvm::createFunctionInliningPass(unsigned OptLevel,
                                       unsigned SizeOptLevel,
                                       bool DisableInlineHotCallSite) {
  auto Param = llvm::getInlineParams(OptLevel, SizeOptLevel);
  if (DisableInlineHotCallSite)
    Param.HotCallSiteThreshold = 0;
  return new SimpleInliner(Param);
}

Pass *llvm::createFunctionInliningPass(InlineParams &Params) {
  return new SimpleInliner(Params);
}

bool SimpleInliner::runOnSCC(CallGraphSCC &SCC) {
  TTIWP = &getAnalysis<TargetTransformInfoWrapperPass>();
  return LegacyInlinerBase::runOnSCC(SCC);
}

void SimpleInliner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetTransformInfoWrapperPass>();
  LegacyInlinerBase::getAnalysisUsage(AU);
}
