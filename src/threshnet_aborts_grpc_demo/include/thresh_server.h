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

#include "openfhe.h"

#include "thresh_server_crypto_context_request.h"
#include "thresh_server_eval_key_request.h"
#include "thresh_server_eval_key_to_client_request.h"
#include "thresh_server_eval_mix_key_request.h"
#include "thresh_server_eval_mix_key_to_client_request.h"
#include "thresh_server_eval_sum_key_request.h"
#include "thresh_server_eval_sum_key_to_client_request.h"
#include "thresh_server_public_key_request.h"
#include "thresh_server_public_key_to_client_request.h"
#include "thresh_server_cipher_text_request.h"
#include "thresh_server_cipher_text_to_client_request.h"
#include "thresh_server_partial_cipher_text_request.h"
#include "thresh_server_partial_cipher_text_to_client_request.h"
#include "thresh_server_secret_shares_request.h"
#include "thresh_server_secret_shares_to_client_request.h"
#include "thresh_server_aborts_partial_cipher_text_request.h"
#include "thresh_server_aborts_partial_cipher_text_to_client_request.h"
#include "thresh_server_partial_cipher_text_wait_client_request.h"

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
    
	void initializeCC() {
		TimeVar t;
        TIC(t);

		int plaintextModulus = 65537;
		double sigma = 3.2;
		lbcrypto::SecurityLevel securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;

		usint batchSize = 16;
		usint multDepth = 4;
		usint digitSize = 30;
		usint dcrtBits = 60;

		lbcrypto::CCParams<lbcrypto::CryptoContextBFVRNS> parameters;		

        parameters.SetPlaintextModulus(plaintextModulus);
		parameters.SetSecurityLevel(securityLevel);
		parameters.SetStandardDeviation(sigma);
		parameters.SetSecretKeyDist(lbcrypto::UNIFORM_TERNARY);
		parameters.SetMultiplicativeDepth(multDepth);
		parameters.SetBatchSize(batchSize);
		parameters.SetDigitSize(digitSize);
		parameters.SetScalingModSize(dcrtBits);
		parameters.SetMultiplicationTechnique(lbcrypto::HPS);

		cc = GenCryptoContext(parameters);

		cc->Enable(lbcrypto::PKE);
		cc->Enable(lbcrypto::KEYSWITCH);
		cc->Enable(lbcrypto::LEVELEDSHE);
		cc->Enable(lbcrypto::ADVANCEDSHE);
		cc->Enable(lbcrypto::MULTIPARTY);
        
		auto elapsed_seconds = TOC_MS(t);
        
		if (TEST_MODE)
            std::cout << "Cryptocontext generation time: " << elapsed_seconds << " ms\n";

		std::cout << "n = " << cc->GetCryptoParameters()->GetElementParams()->GetCyclotomicOrder() / 2 << std::endl;
  		std::cout << "log2 q = " << log2(cc->GetCryptoParameters() ->GetElementParams() ->GetModulus().ConvertToDouble()) << std::endl;
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
		initializeCC();
		// server's main loop.
		HandleRpcs();
	}

private:
	// This can be run in multiple threads if needed.
	void HandleRpcs() {
		// Spawn different instances derived from CallData to serve clients.
		new CryptoContextRequestServicer(&service_, cq_.get(), cc);

        new SecretSharesRequestServicer(&service_, cq_.get(), SecretShares);
		new SecretSharesToClientRequestServicer(&service_, cq_.get(), SecretShares);

		new PublicKeyRequestServicer(&service_, cq_.get(), PublicKeys, PublicKeysReady);
		new PublicKeyToClientRequestServicer(&service_, cq_.get(), PublicKeys, PublicKeysReady);

		//request servicers for eval key generation
        new EvalMultKeyRequestServicer(&service_, cq_.get(), EvalKeyMults, EvalKeyMultsReady);
		new EvalMixMultKeyRequestServicer(&service_, cq_.get(), EvalKeyMixMults, EvalKeyMixMultsReady);
		new EvalSumKeyRequestServicer(&service_, cq_.get(), EvalKeySums, EvalKeySumsReady);

		new EvalMultKeyToClientRequestServicer(&service_, cq_.get(), EvalKeyMults, EvalKeyMultsReady);
		new EvalMixMultKeyToClientRequestServicer(&service_, cq_.get(), EvalKeyMixMults, EvalKeyMixMultsReady);
		new EvalSumKeyToClientRequestServicer(&service_, cq_.get(), EvalKeySums, EvalKeySumsReady);

		new CipherTextRequestServicer(&service_, cq_.get(), Ciphertexts);
		new PartialCipherTextRequestServicer(&service_, cq_.get(), PartialCiphertexts, CTRecdFlag);
		new AbortsPartialCipherTextRequestServicer(&service_, cq_.get(), AbortsPartialCiphertexts);

		new CipherTextToClientRequestServicer(&service_, cq_.get(), Ciphertexts);
		new PartialCipherTextToClientRequestServicer(&service_, cq_.get(), PartialCiphertexts);
		new PartialCipherTextWaitClientRequestServicer(&service_, cq_.get(), CTRecdFlag);
		new AbortsPartialCipherTextToClientsRequestServicer(&service_, cq_.get(), AbortsPartialCiphertexts);
		

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
			static_cast<CallData*>(tag)->Proceed();
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
	std::unordered_map<uint32_t, std::unordered_map<uint32_t, lbcrypto::DCRTPoly>> SecretShares;
	std::unordered_map<uint32_t, std::string> EvalKeyMixMults;
	std::unordered_map<pair, std::string, boost::hash<pair>> EvalKeySums;
	std::unordered_map<pair, bool, boost::hash<pair>> EvalKeySumsReady;
	
};

#endif // __THRESH_SERVER__

