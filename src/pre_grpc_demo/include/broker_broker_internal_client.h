// @file internal_client.h
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

#ifndef __BROKER_BROKER_INTERNAL_CLIENT_H__
#define __BROKER_BROKER_INTERNAL_CLIENT_H__

#include <iostream>

#include "utils/exception.h"

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "pre_net/protos/pre_net.grpc.pb.h"
#else
#include "pre_net.grpc.pb.h"
#endif

#include "utils.h"

class BrokerUpstreamInternalClient
{
public:
    BrokerUpstreamInternalClient(Params params) {
        grpc::ChannelArguments channel_args;
        channel_args.SetMaxReceiveMessageSize(-1);

        std::shared_ptr<grpc::Channel> channel = nullptr;
        if (params.disableSSLAuthentication) {
            channel = grpc::CreateCustomChannel(params.upstream_broker_socket_address, grpc::InsecureChannelCredentials(), channel_args);
        }
        else {
            grpc::SslCredentialsOptions opts = {
                file2String(params.root_cert_file),
                file2String(params.client_private_key_file),
                file2String(params.client_cert_chain_file) };

            auto channel_creds = grpc::SslCredentials(grpc::SslCredentialsOptions(opts));
            channel = grpc::CreateCustomChannel(params.upstream_broker_socket_address, channel_creds, channel_args);
        }

        stub_ = pre_net::PreNetBroker::NewStub(channel);
    }
	
    //**************************************************************
    pre_net::MessageReturn ReEncryptedCipherTextToBroker(std::string& CT, std::string client_type, std::string client_name, std::string channel_name) {
		pre_net::MessageRequest request;    // data to be sent to the server
        
		request.set_client_type(client_type);
		request.set_client_name(client_name);
		request.set_channel_name(channel_name);
		
		grpc::ClientContext context;
		grpc::CompletionQueue cq;
		std::unique_ptr<grpc::ClientAsyncResponseReader<pre_net::MessageReturn> > rpc(
			stub_->PrepareAsyncReEncryptedCipherTextToBroker(&context, request, &cq));

		rpc->StartCall();
		pre_net::MessageReturn reply;   // data returned from the server
		grpc::Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)pre_net::RequestTag::CIPHER_TEXT_TO_BROKER_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)pre_net::RequestTag::CIPHER_TEXT_TO_BROKER_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.textavailable())
			CT = reply.ciphertext();

		return reply;
	}


	


    //**************************************************************
private:
    std::unique_ptr<pre_net::PreNetBroker::Stub> stub_;
};

#endif // __BROKER_BROKER_INTERNAL_CLIENT_H__

