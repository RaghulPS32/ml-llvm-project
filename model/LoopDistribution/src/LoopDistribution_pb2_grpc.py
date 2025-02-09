# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

import LoopDistribution_pb2 as LoopDistribution__pb2


class LoopDistributionStub(object):
    """Missing associated documentation comment in .proto file."""

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.distributeLoopAndGetLoopCost = channel.unary_unary(
                '/loopdistribution.LoopDistribution/distributeLoopAndGetLoopCost',
                request_serializer=LoopDistribution__pb2.LoopDistributionRequest.SerializeToString,
                response_deserializer=LoopDistribution__pb2.LoopDistributionResponse.FromString,
                )


class LoopDistributionServicer(object):
    """Missing associated documentation comment in .proto file."""

    def distributeLoopAndGetLoopCost(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_LoopDistributionServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'distributeLoopAndGetLoopCost': grpc.unary_unary_rpc_method_handler(
                    servicer.distributeLoopAndGetLoopCost,
                    request_deserializer=LoopDistribution__pb2.LoopDistributionRequest.FromString,
                    response_serializer=LoopDistribution__pb2.LoopDistributionResponse.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'loopdistribution.LoopDistribution', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class LoopDistribution(object):
    """Missing associated documentation comment in .proto file."""

    @staticmethod
    def distributeLoopAndGetLoopCost(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/loopdistribution.LoopDistribution/distributeLoopAndGetLoopCost',
            LoopDistribution__pb2.LoopDistributionRequest.SerializeToString,
            LoopDistribution__pb2.LoopDistributionResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)
