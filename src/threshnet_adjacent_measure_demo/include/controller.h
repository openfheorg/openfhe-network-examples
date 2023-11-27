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
#ifndef __CONTROLLER__
#define __CONTROLLER__

#include "controller_cipher_text_from_client_request.h"
#include "thresh_controller_internal_client.h"

#include <iostream>
#include <climits>
#include "utils.h"

using grpc::Server;
using grpc::ServerBuilder;

class ControllerImpl final {
public:
	ControllerImpl() : controllercc(nullptr) {}

	~ControllerImpl() {
		server_->Shutdown();
		// Always shutdown the completion queue after the server.
		cq_->Shutdown();
	}

	// There is no shutdown handling in this code.
	void Run(Params& params) {
		ServerBuilder builder;
		localParams = params;
		// from https://github.com/grpc/grpc/blob/master/include/grpc/impl/codegen/grpc_types.h:
		// GRPC doesn't set size limit for sent messages by default. However, the max size of received messages
		// is limited by "#define GRPC_DEFAULT_MAX_RECV_MESSAGE_LENGTH (4 * 1024 * 1024)", which is only 4194304.
		// for every GRPC application the received message size must be set individually:
		//		for a client it may be "-1" (unlimited): SetMaxReceiveMessageSize(-1)
		//		for a server INT_MAX (from #include <climits>) should do it: SetMaxReceiveMessageSize(INT_MAX)
		// see https://nanxiao.me/en/message-length-setting-in-grpc/ for more information on the message size
		builder.SetMaxReceiveMessageSize(INT_MAX);

		std::shared_ptr<grpc::ServerCredentials> creds = nullptr;
		if(params.disableSSLAuthentication )
			creds = grpc::InsecureServerCredentials();
		else {
			grpc::SslServerCredentialsOptions::PemKeyCertPair keyCert = {
				file2String(params.server_private_key_file),
				file2String(params.server_cert_chain_file) };
			grpc::SslServerCredentialsOptions opts;
			opts.pem_root_certs = file2String(params.root_cert_file);
			opts.pem_key_cert_pairs.push_back(keyCert);
			creds = grpc::SslServerCredentials(opts);
		}
		builder.AddListeningPort(params.controller_socket_address, creds);

		builder.RegisterService(&service_);
		cq_ = builder.AddCompletionQueue();
		server_ = builder.BuildAndStart();
		// std::cout << "Server listening on " << socket_address << std::endl;
        ThreshControllerInternalClient internalClient_controller(params);
        
		std::string reply_cryptocontext(internalClient_controller.CryptoContextRequestFromServer_Controller());
		if (!reply_cryptocontext.size())
		        OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized CryptoContext");
	
		std::istringstream is(reply_cryptocontext);
		lbcrypto::Serial::Deserialize(controllercc, is, GlobalSerializationType);
		if (!controllercc)
			OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized CryptoContext is NULL");

		if (TEST_MODE)
			write2File(reply_cryptocontext, "./controller_cc_received_serialized.txt");

		// server's main loop.
		HandleRpcs();
	}

private:
	// This can be run in multiple threads if needed.
	void HandleRpcs() {
		// Spawn different instances derived from CallData to serve clients.
		new ControllerCipherTextRequestServicer(&service_, cq_.get(), localParams, controllercc, random_s);

		void* tag;  // uniquely identifies a request.
		bool ok;
		while (true) {
			// Block waiting to read the next event from the completion queue. The
			// event is uniquely identified by its tag, which in this case is the
			// memory address of a CallData instance.
			// The return value of Next should always be checked. This return value
			// tells us whether there is any kind of event or cq_ is shutting down.
			GPR_ASSERT(cq_->Next(&tag, &ok));
			GPR_ASSERT(ok);
			static_cast<CallData<thresh_net::ThreshController::AsyncService>*>(tag)->Proceed();
		}
	}

    Params localParams;
	std::unique_ptr<ServerCompletionQueue> cq_;
	thresh_net::ThreshController::AsyncService service_;
	std::unique_ptr<Server> server_;

	lbcrypto::CryptoContext<lbcrypto::DCRTPoly> controllercc;
	
	std::unordered_map<uint32_t, lbcrypto::Plaintext> random_s;
};

#endif // __CONTROLLER__

