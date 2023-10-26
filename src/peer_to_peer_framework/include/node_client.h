// @file node_client.h
// @author TPOC: info@DualityTech.com
//
// @copyright Copyright (c) 2021, Duality Technologies Inc.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution. THIS SOFTWARE IS
// PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// This file defines the class for internal client of the node and method sendmessage 
// to handle the RPC sendmessage in the proto file to send messages from the node to other nodes.

#ifndef __NODE_CLIENT_H__
#define __NODE_CLIENT_H__


#include "utils/exception.h"
#include <iostream>
#include "node_request.h"


#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "peer_to_peer/protos/peer_to_peer.grpc.pb.h"
#else
#include "peer_to_peer.grpc.pb.h"
#endif


class NodeClient
{
public:
    NodeClient(std::string receiver_node_socket_address, CommParams& params) {

        grpc::ChannelArguments channel_args;

        channel_args.SetMaxReceiveMessageSize(-1);

        std::shared_ptr<grpc::Channel> channel = nullptr;
        if (params.disableSSLAuthentication) {
            channel = grpc::CreateCustomChannel(receiver_node_socket_address, grpc::InsecureChannelCredentials(), channel_args);
        }
        else {
            grpc::SslCredentialsOptions opts = {
                file2String(params.root_cert_file),
                file2String(params.client_private_key_file),
                file2String(params.client_cert_chain_file) };

            auto channel_creds = grpc::SslCredentials(grpc::SslCredentialsOptions(opts));
            channel = grpc::CreateCustomChannel(receiver_node_socket_address, channel_creds, channel_args);
        }

        stub_ = peer_to_peer::NodeServer::NewStub(channel);
    }
	
    //**************************************************************
    peer_to_peer::ReturnMessage sendGRPCMsg(const std::string& NodeName, message_format& Msg) {
        peer_to_peer::RequestMessage request;    // data to be sent to the server
        peer_to_peer::ReturnMessage reply;   // data returned from the server

		request.set_data(Msg.Data);
        request.set_data_type(Msg.msgType);
        request.set_node_name(Msg.NodeName);
		
		grpc::ClientContext context;
		grpc::CompletionQueue cq;
		std::unique_ptr<grpc::ClientAsyncResponseReader<peer_to_peer::ReturnMessage>> rpc(
			stub_->PrepareAsyncsendGRPCMsg(&context, request, &cq));

		rpc->StartCall();
		grpc::Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)peer_to_peer::RequestTag::NODE_MESSAGE);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)peer_to_peer::RequestTag::NODE_MESSAGE);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
            reply.set_error_msg(status.error_message());
			//OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());
            
		return reply;
    }


	


    //**************************************************************
private:
    std::unique_ptr<peer_to_peer::NodeServer::Stub> stub_;
};

#endif // __NODE_CLIENT_H__

