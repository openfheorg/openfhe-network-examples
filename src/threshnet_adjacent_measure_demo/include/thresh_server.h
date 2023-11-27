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
#ifndef __THRESH_SERVER__
#define __THRESH_SERVER__

#include "thresh_server_crypto_context_request.h"
#include "thresh_server_eval_key_request.h"
#include "thresh_server_eval_key_to_client_request.h"
#include "thresh_server_eval_mix_key_request.h"
#include "thresh_server_eval_mix_key_to_client_request.h"
#include "thresh_server_public_key_request.h"
#include "thresh_server_public_key_to_client_request.h"
#include "thresh_server_cipher_text_request.h"
#include "thresh_server_cipher_text_to_client_request.h"
#include "thresh_server_partial_cipher_text_request.h"
#include "thresh_server_partial_cipher_text_to_client_request.h"
#include "thresh_server_partial_cipher_text_wait_client_request.h"
#include "thresh_server_cryptocontext_request_to_controller.h"
#include "thresh_server_public_key_request_to_controller.h"
#include "thresh_server_eval_mult_key_request_to_controller.h"

#include <iostream>
#include <climits>
#include "utils.h"

using grpc::Server;
using grpc::ServerBuilder;

class ServerImpl final {
public:
	ServerImpl() : cc(nullptr) {}

	~ServerImpl() {
		server_->Shutdown();
		// Always shutdown the completion queue after the server.
		cq_->Shutdown();
	}

	// There is no shutdown handling in this code.
	void Run(Params& params) {
		ServerBuilder builder;
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
		builder.AddListeningPort(params.socket_address, creds);

		builder.RegisterService(&service_);
		cq_ = builder.AddCompletionQueue();
		server_ = builder.BuildAndStart();
		// std::cout << "Server listening on " << socket_address << std::endl;

		// server's main loop.
		HandleRpcs();
	}

private:
	// This can be run in multiple threads if needed.
	void HandleRpcs() {
		// Spawn different instances derived from CallData to serve clients.
		new CryptoContextRequestServicer(&service_, cq_.get(), cc);

		new PublicKeyRequestServicer(&service_, cq_.get(), PublicKeys, PublicKeysReady);
		new PublicKeyToClientRequestServicer(&service_, cq_.get(), PublicKeys, PublicKeysReady);

		//request servicers for eval key generation
        new EvalMultKeyRequestServicer(&service_, cq_.get(), EvalKeyMults, EvalKeyMultsReady);
		new EvalMixMultKeyRequestServicer(&service_, cq_.get(), EvalKeyMixMults, EvalKeyMixMultsReady);

		new EvalMultKeyToClientRequestServicer(&service_, cq_.get(), EvalKeyMults, EvalKeyMultsReady);
		new EvalMixMultKeyToClientRequestServicer(&service_, cq_.get(), EvalKeyMixMults, EvalKeyMixMultsReady);

		new CipherTextRequestServicer(&service_, cq_.get(), Ciphertexts);
		new PartialCipherTextRequestServicer(&service_, cq_.get(), PartialCiphertexts, CTRecdFlag);

		new CipherTextToClientRequestServicer(&service_, cq_.get(), Ciphertexts);
		new PartialCipherTextToClientRequestServicer(&service_, cq_.get(), PartialCiphertexts);
		new PartialCipherTextWaitClientRequestServicer(&service_, cq_.get(), CTRecdFlag);		


        //*********************
        new CryptoContextToControllerRequestServicer(&service_, cq_.get(), cc);

		new PublicKeyToControllerRequestServicer(&service_, cq_.get(), PublicKeys, PublicKeysReady);

		//request servicers for eval key generation
        new EvalMultKeyToControllerRequestServicer(&service_, cq_.get(), EvalKeyMixMults, EvalKeyMixMultsReady);

		//*********************
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
			static_cast<CallData<thresh_net::ThreshServer::AsyncService>*>(tag)->Proceed();
		}
	}

	std::unique_ptr<ServerCompletionQueue> cq_;
	ThreshServer::AsyncService service_;
	std::unique_ptr<Server> server_;

	lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc;

    std::unordered_map<uint32_t, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> Ciphertexts;
	std::unordered_map<uint32_t, std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>> PartialCiphertexts;
	std::unordered_map<uint32_t, bool> CTRecdFlag;
	std::unordered_map<pair, std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>, boost::hash<pair>> AbortsPartialCiphertexts;

	std::unordered_map<uint32_t, std::string> PublicKeys;
	std::unordered_map<uint32_t, bool> PublicKeysReady;
	std::unordered_map<uint32_t, bool> EvalKeyMultsReady;
	std::unordered_map<uint32_t, bool> EvalKeyMixMultsReady;

    std::unordered_map<uint32_t, std::string> EvalKeyMults;
	std::unordered_map<uint32_t, std::string> EvalKeyMixMults;
	std::unordered_map<pair, std::string, boost::hash<pair>> EvalKeySums;
	std::unordered_map<pair, bool, boost::hash<pair>> EvalKeySumsReady;
	
};

#endif // __THRESH_SERVER__

