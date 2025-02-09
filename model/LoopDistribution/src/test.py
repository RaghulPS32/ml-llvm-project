import pandas as pd
import numpy as np
import json
from distributeEnv import DistributeLoopEnv
from dqn_agent import Agent
import glob
import json
import torch
from collections import deque
import os
import utils
from utils import get_parse_args
import logging

def run(agent, config):

    eps=0
    max_t=1000
    scores = []                        # list containing scores from each episode
    scores_window = deque(maxlen=100)  # last 100 scores

    dataset= config.dataset
    #Load the envroinment
    env = DistributeLoopEnv(config)    
    score = 0
    count = 1
    # logging.info(glob.glob(os.path.join(dataset, 'graphs/test/*.json')))
    for path in glob.glob(os.path.join(dataset, 'graphs/json/I_520*.json')): # Number of the iterations
        
        with open(path) as f:
            graph = json.load(f)
        logging.info('New graph to the env. {} '.format(path))
        # state, topology = env.reset_env(graph, path)
        # Updated 
        state, topology, focusNode = env.reset_env(graph, path)
        while(True):
            possibleNodes_emb, possibleNodes = state

            # pass the state and  topology to get the action
            # action is 
            nextNodeIndex, merge_distribute = agent.act(state, topology, focusNode, eps)
            logging.info("action choosed : {} {} {}".format(nextNodeIndex, possibleNodes[nextNodeIndex],merge_distribute))
            # Get the next the next state from the action
            # reward is 0 till we reach the end node
            # reward will be -negative, maximize  the reward
            #
            action=(possibleNodes[nextNodeIndex], merge_distribute)
            next_state, reward, done, distribute, focusNode = env.step(action)
            next_possibleNodes_emb, next_possibleNodes = next_state

            logging.info('Distribution till now : {}'.format(distribute))
            
            state = (next_possibleNodes_emb, next_possibleNodes)
            score += reward
 
            logging.info('DLOOP Goto to Next.................')
            scores_window.append(score)       # save most recent score
            scores.append(score)              # save most recent score
            logging.info('\n------------------------------------------------------------------------------------------------')
            if done:
               break
 
        agent.writer.add_scalar('test/rewardStep', score, count)
        agent.writer.add_scalar('test/rewardWall', reward)

        def speedup(reward):
            return reward
            # if reward > 0:
            #     return reward - 5
            # else:
            #     return reward + 0.5
        agent.writer.add_scalar('test/speedup', speedup(reward))
 
        count+=1
    # utils.plot(range(1, len(scores_window)+1), scores_window, 'Last 100 rewards',location=config.distributed_data)
    # utils.plot(range(1, len(scores)+1), scores, 'Total Rewards per time instant',location=config.distributed_data)


if __name__ == '__main__':

    config = get_parse_args()
    logger = logging.getLogger('test.py') 
    logging.basicConfig(filename=os.path.join(config.logdir, 'running.log'), format='%(levelname)s - %(filename)s - %(message)s', level=logging.DEBUG)

    logging.info(config)
    dqn_agent = Agent(config, seed=0)

    trained_model = config.trained_model
    
    if not os.path.exists(trained_model):
        raise Exception('Path Not Exists: {}'.format(trained_model))
    if os.path.isdir(trained_model):
        trained_model = os.path.join(trained_model, 'final-model.pth')

    logging.info('model selected for training :{}'.format(trained_model))

    dqn_agent.qnetwork_local.load_state_dict(torch.load(trained_model, map_location=torch.device("cuda" if torch.cuda.is_available() else "cpu")))
    # dqn_agent.writer.add_graph(dqn_agent.qnetwork_local)
    run(dqn_agent, config)

    # dqn_agent.writer.add_graph(dqn_agent.qnetwork_local)
    dqn_agent.writer.flush()
    dqn_agent.writer.close()

    logging.info('Testing Completed..... ')
    
