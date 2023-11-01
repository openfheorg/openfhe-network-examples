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
#ifndef __THRESH_CLIENT__
#define __THRESH_CLIENT__

#include "utils.h"
#include <utils/exception.h>
#include "cryptocontext-ser.h"
#include "key/key.h"
#include "key/key-ser.h"

#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include <sstream>
#include <string>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "thresh_net_aborts_grpc/protos/thresh_net.grpc.pb.h"
#else
#include "thresh_net.grpc.pb.h"
#endif

using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using thresh_net::ThreshServer;
using thresh_net::RequestTag;

class ThreshClient {
public:
	explicit ThreshClient(std::shared_ptr<grpc::Channel> channel)
		: stub_(ThreshServer::NewStub(channel)) {}

	// ============================================================================================
	std::string CryptoContextFromServer() {
		thresh_net::CryptoContextRequest request;     // data to be sent to the server

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::CryptoContextReturn> > rpc(
			stub_->PrepareAsyncCryptoContextFromServer(&context, request, &cq));

		rpc->StartCall();
        thresh_net::CryptoContextReturn reply;      // data returned from the server
		Status status;          // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::CRYPTO_CONTEXT_FOR_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::CRYPTO_CONTEXT_FOR_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.cryptocontext();
	}


	// ============================================================================================
	std::string PublicKeyFromClient(const std::string& pkey, uint32_t client_id) {
		thresh_net::PublicKey request;    // data to be sent to the server
		request.set_pkey(pkey);
		request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::PublicKeyAck> > rpc(
			stub_->PrepareAsyncPublicKeyFromClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::PublicKeyAck reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::PUBLIC_KEY_FROM_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::PUBLIC_KEY_FROM_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.acknowledgement();
	}

// ============================================================================================
	bool PublicKeyToClient(std::string& pkey, uint32_t client_id) {
		thresh_net::PublicKeyRequest request;    // data to be sent to the server

		ClientContext context;
		CompletionQueue cq;

		request.set_client_id(client_id);

		std::unique_ptr<ClientAsyncResponseReader<thresh_net::PublicKeyReturn> > rpc(
			stub_->PrepareAsyncPublicKeyToClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::PublicKeyReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::PUBLIC_KEY_TO_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::PUBLIC_KEY_TO_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.keyavailable()) {
			pkey = reply.pkey();
		}

		return reply.keyavailable();
	}

    std::string SecretSharesFromClient(const std::string& secretshares, uint32_t client_id) {
		thresh_net::SecretKeyShares request;    // data to be sent to the server
		request.set_secretshares(secretshares);
		request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::SecretKeySharesAck> > rpc(
			stub_->PrepareAsyncSecretSharesFromClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::SecretKeySharesAck reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::SECRET_SHARES_FROM_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::SECRET_SHARES_FROM_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.acknowledgement();
	}

	bool SecretSharesToClient(std::string& secretshares, uint32_t requesting_client_id, uint32_t recover_client_id, uint32_t threshold, uint32_t num_of_parties) {
		thresh_net::SecretKeySharesRequest request;    // data to be sent to the server

		ClientContext context;
		CompletionQueue cq;

		request.set_requesting_client_id(requesting_client_id);
		request.set_recover_client_id(recover_client_id);
		request.set_threshold(threshold);
		request.set_num_of_parties(num_of_parties);

		std::unique_ptr<ClientAsyncResponseReader<thresh_net::SecretKeySharesReturn> > rpc(
			stub_->PrepareAsyncSecretSharesToClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::SecretKeySharesReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::SECRET_SHARES_TO_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::SECRET_SHARES_TO_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.sharesavailable()) {
			secretshares = reply.secretshares();
		}

		return reply.sharesavailable();
	}

	// ============================================================================================
	std::string evalMultKeyFromClient(const std::string& ekey, uint32_t client_id) {//, uint32_t round_num) {
		thresh_net::EvalMultKey request;    // data to be sent to the server
		request.set_ekey(ekey);
        
		request.set_client_id(client_id);
		//request.set_round(round_num);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalMultKeyAck> > rpc(
			stub_->PrepareAsyncevalMultKeyFromClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::EvalMultKeyAck reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::EVALMULT_KEY_FROM_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::EVALMULT_KEY_FROM_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.acknowledgement();
	}

	// ============================================================================================
	std::string evalMixMultKeyFromClient(const std::string& ekey, uint32_t client_id) {
		thresh_net::EvalMultKey request;    // data to be sent to the server
		request.set_ekey(ekey);
        
		request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalMultKeyAck> > rpc(
			stub_->PrepareAsyncevalMixMultKeyFromClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::EvalMultKeyAck reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::EVALMIXMULT_KEY_FROM_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::EVALMIXMULT_KEY_FROM_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.acknowledgement();
	}
// ============================================================================================
	bool evalMultKeyToClient(std::string& ekey, uint32_t client_id) {
		thresh_net::EvalMultKeyRequest request;    // data to be sent to the server

		ClientContext context;
		CompletionQueue cq;

		request.set_client_id(client_id);

		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalMultKeyReturn> > rpc(
			stub_->PrepareAsyncevalMultKeyToClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::EvalMultKeyReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::EVALMULT_KEY_TO_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::EVALMULT_KEY_TO_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.keyavailable()) {
			ekey = reply.ekey();
		}

		return reply.keyavailable();
	}

	// ============================================================================================
	bool evalMixMultKeyToClient(std::string& ekey, uint32_t client_id) {
		thresh_net::EvalMultKeyRequest request;    // data to be sent to the server

		ClientContext context;
		CompletionQueue cq;

		request.set_client_id(client_id);

		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalMultKeyReturn> > rpc(
			stub_->PrepareAsyncevalMixMultKeyToClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::EvalMultKeyReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::EVALMIXMULT_KEY_TO_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::EVALMIXMULT_KEY_TO_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.keyavailable()) {
			ekey = reply.ekey();
		}

		return reply.keyavailable();
	}

    // ============================================================================================
	std::string evalSumKeyFromClient(const std::string& ekey, uint32_t client_id, uint32_t index_num) {
		thresh_net::EvalSumKey request;    // data to be sent to the server
		request.set_ekey(ekey);
        
		request.set_client_id(client_id);
		request.set_index(index_num);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalSumKeyAck> > rpc(
			stub_->PrepareAsyncevalSumKeyFromClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::EvalSumKeyAck reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::EVALSUM_KEY_FROM_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::EVALSUM_KEY_FROM_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.acknowledgement();
	}
// ============================================================================================
	bool evalSumKeyToClient(std::string& ekey, uint32_t client_id, uint32_t index_num) {
		thresh_net::EvalSumKeyRequest request;    // data to be sent to the server

		ClientContext context;
		CompletionQueue cq;

		request.set_client_id(client_id);
		request.set_index(index_num);

		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalSumKeyReturn> > rpc(
			stub_->PrepareAsyncevalSumKeyToClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::EvalSumKeyReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::EVALSUM_KEY_TO_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::EVALSUM_KEY_TO_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.keyavailable()) {
			ekey = reply.ekey();
		}

		return reply.keyavailable();
	}

	// ============================================================================================
	std::string CipherTextFromClient(const std::string& cText, uint32_t client_id) {
		thresh_net::CipherText request;    // data to be sent to the server
		request.set_ctext(cText);
		request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::CipherTextAck> > rpc(
			stub_->PrepareAsyncCipherTextFromClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::CipherTextAck reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::CIPHER_TEXT_FROM_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::CIPHER_TEXT_FROM_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.acknowledgement();
	}

	// ============================================================================================
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> genCT(lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc,
		lbcrypto::PublicKey<lbcrypto::DCRTPoly> publicKey,
		Params &params) {
		unsigned int plaintextModulus = cc->GetCryptoParameters()->GetPlaintextModulus();
		constexpr unsigned int minPlaintextMod = 65536;

		if (plaintextModulus < minPlaintextMod) {
			std::string errMsg(
				std::string("code is designed for plaintextModulus>=") +
				std::to_string(minPlaintextMod) +
				"; modulus is " +
				std::to_string(plaintextModulus));
			OPENFHE_THROW(lbcrypto::math_error, errMsg);
		}

		std::vector<int64_t> vectorOfInts;
		
		uint32_t client_id = atoi(params.client_id.c_str());
		vectorOfInts = {1, 2, 3, 4, 3, 2, 1, 2, 4, 3, 3, client_id};
		 
		//pack them into a packed plaintext (vector encryption)
		lbcrypto::Plaintext pt = cc->MakePackedPlaintext(vectorOfInts);

        std::cout << "\n Original Plaintext: \n" << std::endl;
        std::cout << pt << std::endl;

		if( TEST_MODE ) {
			std::ostringstream osPt;
			osPt << pt;
			write2File(osPt.str(), "./client_" + std::to_string(client_id) + "_plaintext.txt");
		}

		TimeVar t;
		TIC(t);
        auto ct = cc->Encrypt(publicKey, pt);  // Encrypt

		auto elapsed_seconds = TOC_MS(t);

    	if (TEST_MODE)
			std::cout << "Ciphertext generation time: " << elapsed_seconds << " ms\n";
		
		return ct;
	}


	// ============================================================================================
	bool CipherTextToClient(std::string& cText, uint32_t client_id) {
		thresh_net::CipherTextRequest request;    // data to be sent to the server

        request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::CipherTextReturn> > rpc(
			stub_->PrepareAsyncCipherTextToClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::CipherTextReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::CIPHER_TEXT_TO_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::CIPHER_TEXT_TO_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.textavailable())
			cText = reply.ciphertext();

		return reply.textavailable();
	}

	// ============================================================================================
	std::string PartialCipherTextFromClient(const std::string& cText, uint32_t client_id) {
		thresh_net::CipherText request;    // data to be sent to the server
		request.set_ctext(cText);
		request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::CipherTextAck> > rpc(
			stub_->PrepareAsyncPartialCipherTextFromClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::CipherTextAck reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::PARTIAL_CIPHER_TEXT_FROM_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::PARTIAL_CIPHER_TEXT_FROM_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.acknowledgement();
	}

// ============================================================================================
	bool PartialCipherTextsReceivedWait(uint32_t client_id) {
		thresh_net::PartialCTFlagRequest request;    // data to be sent to the server
		request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::PartialCTFlagReturn> > rpc(
			stub_->PrepareAsyncPartialCipherTextsReceivedWait(&context, request, &cq));

		rpc->StartCall();
		thresh_net::PartialCTFlagReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::PARTIAL_CIPHER_TEXT_WAIT_FLAG_FROM_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::PARTIAL_CIPHER_TEXT_WAIT_FLAG_FROM_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.textavailable();
	}

	// ============================================================================================
	bool PartialCipherTextToClient(std::string& cText, uint32_t client_id) {
		thresh_net::CipherTextRequest request;    // data to be sent to the server

        request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::CipherTextReturn> > rpc(
			stub_->PrepareAsyncPartialCipherTextToClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::CipherTextReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::PARTIAL_CIPHER_TEXT_TO_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::PARTIAL_CIPHER_TEXT_TO_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.textavailable())
			cText = reply.ciphertext();

		return reply.textavailable();
	}

    // ============================================================================================
	std::string AbortsPartialCipherTextFromClient(const std::string& cText, uint32_t received_client_id, uint32_t recovered_client_id) {
		thresh_net::AbortsCipherText request;    // data to be sent to the server
		request.set_ctext(cText);
		request.set_received_client_id(received_client_id);
		request.set_recovered_client_id(recovered_client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::AbortsCipherTextAck> > rpc(
			stub_->PrepareAsyncAbortsPartialCipherTextFromClient(&context, request, &cq));

		rpc->StartCall();
		thresh_net::AbortsCipherTextAck reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::ABORTS_PARTIAL_CIPHER_TEXT_FROM_CLIENT_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::ABORTS_PARTIAL_CIPHER_TEXT_FROM_CLIENT_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.acknowledgement();
	}

	// ============================================================================================
	bool AbortsPartialCipherTextToClients(std::string& cText, uint32_t received_client_id, uint32_t recovered_client_id) {
		thresh_net::AbortsCipherTextRequest request;    // data to be sent to the server

        request.set_received_client_id(received_client_id);
		request.set_recovered_client_id(recovered_client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::AbortsCipherTextReturn> > rpc(
			stub_->PrepareAsyncAbortsPartialCipherTextToClients(&context, request, &cq));

		rpc->StartCall();
		thresh_net::AbortsCipherTextReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::ABORTS_PARTIAL_CIPHER_TEXT_TO_CLIENTS_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::ABORTS_PARTIAL_CIPHER_TEXT_TO_CLIENTS_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		if (reply.textavailable())
			cText = reply.ciphertext();

		return reply.textavailable();
	}
private:

	std::unique_ptr<ThreshServer::Stub> stub_;
};

#endif // __THRESH_CLIENT__
