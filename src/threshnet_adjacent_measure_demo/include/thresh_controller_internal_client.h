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

#ifndef __THRESH_CONTROLLER_INTERNAL_CLIENT_H__
#define __THRESH_CONTROLLER_INTERNAL_CLIENT_H__


#include "utils/exception.h"
#include "utils.h"
#include <iostream>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "thresh_net/protos/thresh_net.grpc.pb.h"
#else
#include "thresh_net.grpc.pb.h"
#endif


class ThreshControllerInternalClient
{
public:
    ThreshControllerInternalClient(Params params) {
        grpc::ChannelArguments channel_args;
        channel_args.SetMaxReceiveMessageSize(-1);

        std::shared_ptr<grpc::Channel> channel = nullptr;
        if (params.disableSSLAuthentication) {
            channel = grpc::CreateCustomChannel(params.socket_address, grpc::InsecureChannelCredentials(), channel_args);
        }
        else {
            grpc::SslCredentialsOptions opts = {
                file2String(params.root_cert_file),
                file2String(params.client_private_key_file),
                file2String(params.client_cert_chain_file) };

            auto channel_creds = grpc::SslCredentials(grpc::SslCredentialsOptions(opts));
            channel = grpc::CreateCustomChannel(params.socket_address, channel_creds, channel_args);
        }

        stub_ = thresh_net::ThreshServer::NewStub(channel);
    }

	//************************************************************
	std::string CryptoContextRequestFromServer_Controller() {
		thresh_net::CryptoContextRequest request;     // data to be sent to the server
		
		grpc::ClientContext context;
		grpc::CompletionQueue cq;
		std::unique_ptr<grpc::ClientAsyncResponseReader<thresh_net::CryptoContextReturn> > rpc(
			stub_->PrepareAsyncCryptoContextRequestFromServer_Controller(&context, request, &cq));

		rpc->StartCall();
		thresh_net::CryptoContextReturn reply;      // data returned from the server
		grpc::Status status;          // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)thresh_net::RequestTag::CRYPTO_CONTEXT_FOR_CONTROLLER_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)thresh_net::RequestTag::CRYPTO_CONTEXT_FOR_CONTROLLER_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.cryptocontext();
	}

    //-----------------------------------------------------
    bool JointPublicKeyToController(std::string& pkey, Params params) {
		thresh_net::PublicKeyRequest request;    // data to be sent to the server

        request.set_client_id(atoi(params.Num_of_parties.c_str()));

		grpc::ClientContext context;
		grpc::CompletionQueue cq;

		std::unique_ptr<grpc::ClientAsyncResponseReader<thresh_net::PublicKeyReturn> > rpc(
			stub_->PrepareAsyncJointPublicKeyToController(&context, request, &cq));

		rpc->StartCall();
		thresh_net::PublicKeyReturn reply;   // data returned from the server
		grpc::Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)thresh_net::RequestTag::JOINT_PUBLIC_KEY_TO_CONTROLLER_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)thresh_net::RequestTag::JOINT_PUBLIC_KEY_TO_CONTROLLER_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.keyavailable()) {
			pkey = reply.pkey();
		}

		return reply.keyavailable();
	}


    //**************************************************************
    bool EvalMultKeyToController(std::string& eKey) {
		thresh_net::EvalMultKeyRequest request;    // data to be sent to the server

		grpc::ClientContext context;
		grpc::CompletionQueue cq;
		
		std::unique_ptr<grpc::ClientAsyncResponseReader<thresh_net::EvalMultKeyReturn> > rpc(
			stub_->PrepareAsyncEvalMultKeyToController(&context, request, &cq));

		rpc->StartCall();
		thresh_net::EvalMultKeyReturn reply;   // data returned from the server
		grpc::Status status;         // status of the RPC upon completion

		rpc->Finish(&reply, &status, (void*)thresh_net::RequestTag::EVALMULT_KEY_TO_CONTROLLER_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)thresh_net::RequestTag::EVALMULT_KEY_TO_CONTROLLER_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.keyavailable())
			eKey = reply.ekey();

		return reply.keyavailable();
	}	


    //**************************************************************
private:
    std::unique_ptr<thresh_net::ThreshServer::Stub> stub_;
};

#endif // __THRESH_CONTROLLER_INTERNAL_CLIENT_H__

