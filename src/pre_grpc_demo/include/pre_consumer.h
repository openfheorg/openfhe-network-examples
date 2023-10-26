/***
 * � 2021 Duality Technologies, Inc. All rights reserved.
 * This is a proprietary software product of Duality Technologies, Inc.
 *protected under copyright laws and international copyright treaties, patent
 *law, trade secret law and other intellectual property rights of general
 *applicability. Any use of this software is strictly prohibited absent a
 *written agreement executed by Duality Technologies, Inc., which provides
 *certain limited rights to use this software. You may not copy, distribute,
 *make publicly available, publicly perform, disassemble, de-compile or reverse
 *engineer any part of this software, breach its security, or circumvent,
 *manipulate, impair or disrupt its operation.
 ***/
#ifndef __PRE_CONSUMER__
#define __PRE_CONSUMER__

#include <sstream>
#include <string>

#include "utils/exception.h"
#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include "scheme/bgvrns/bgvrns-ser.h"

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "pre_net/protos/pre_net.grpc.pb.h"
#else
#include "pre_net.grpc.pb.h"
#endif

#include "utils.h"

using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using pre_net::PreNetServer;
using pre_net::PreNetBroker;
using pre_net::RequestTag;

class PreNetConsumer {
public:
	explicit PreNetConsumer(std::shared_ptr<grpc::Channel> channel)
		: stubserver_(PreNetServer::NewStub(channel)) {}

    explicit PreNetConsumer(std::shared_ptr<grpc::Channel> channel, usint broker_connect)
	: stubbroker_(PreNetBroker::NewStub(channel)) {}
	
	// ============================================================================================
	std::string CryptoContextFromServer() {
		pre_net::MessageRequest request;     // data to be sent to the server

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<pre_net::MessageReturn> > rpc(
			stubserver_->PrepareAsyncCryptoContextFromServer(&context, request, &cq));

		rpc->StartCall();
		pre_net::MessageReturn reply;      // data returned from the server
		Status status;          // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::CRYPTO_CONTEXT_FOR_CONSUMER_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::CRYPTO_CONTEXT_FOR_CONSUMER_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.cryptocontext();
	}

	// ============================================================================================
	std::string PublicKeyFromClient(const std::string& pkey, std::string client_name) {
		pre_net::MessageRequest request;    // data to be sent to the server
		request.set_pkey(pkey);
        request.set_client_type("consumer");
		request.set_client_name(client_name);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<pre_net::MessageReturn> > rpc(
			stubserver_->PrepareAsyncPublicKeyFromClient(&context, request, &cq));

		rpc->StartCall();
		pre_net::MessageReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::PUBLIC_KEY_FROM_CONSUMER_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::PUBLIC_KEY_FROM_CONSUMER_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.acknowledgement();
	}

	// ============================================================================================
	pre_net::MessageReturn CipherTextToConsumer(std::string& cText, std::string client_type, std::string client_name, std::string upstream_broker_name, std::string channel_name) {
		pre_net::MessageRequest request;    // data to be sent to the server

        request.set_client_type(client_type);
		request.set_client_name(client_name);
		request.set_upstream_client_name(upstream_broker_name);
		request.set_channel_name(channel_name);

		ClientContext context;
		CompletionQueue cq;
		
		std::unique_ptr<ClientAsyncResponseReader<pre_net::MessageReturn> > rpc(
			stubbroker_->PrepareAsyncCipherTextToConsumer(&context, request, &cq));

		rpc->StartCall();
		pre_net::MessageReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion

		rpc->Finish(&reply, &status, (void*)RequestTag::CIPHER_TEXT_TO_CONSUMER_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::CIPHER_TEXT_TO_CONSUMER_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.textavailable())
			cText = reply.ciphertext();

		return reply;
	}


private:

	std::unique_ptr<PreNetServer::Stub> stubserver_;
	std::unique_ptr<PreNetBroker::Stub> stubbroker_;
};

#endif // __PRE_CONSUMER__
