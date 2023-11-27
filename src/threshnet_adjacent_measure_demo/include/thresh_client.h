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
using thresh_net::ThreshController;
using thresh_net::RequestTag;

class ThreshClient {
public:
	explicit ThreshClient(std::shared_ptr<grpc::Channel> channel)
		: stubserver_(ThreshServer::NewStub(channel)) {}
	
	explicit ThreshClient(std::shared_ptr<grpc::Channel> channel, usint controller_connect)
		: stubcontroller_(ThreshController::NewStub(channel)) {}

	// ============================================================================================
	std::string CryptoContextFromServer() {
		thresh_net::CryptoContextRequest request;     // data to be sent to the server

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::CryptoContextReturn> > rpc(
			stubserver_->PrepareAsyncCryptoContextFromServer(&context, request, &cq));

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
			stubserver_->PrepareAsyncPublicKeyFromClient(&context, request, &cq));

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
			stubserver_->PrepareAsyncPublicKeyToClient(&context, request, &cq));

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

	// ============================================================================================
	std::string evalMultKeyFromClient(const std::string& ekey, uint32_t client_id) {
		thresh_net::EvalMultKey request;    // data to be sent to the server
		request.set_ekey(ekey);
        
		request.set_client_id(client_id);
		//request.set_round(round_num);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalMultKeyAck> > rpc(
			stubserver_->PrepareAsyncevalMultKeyFromClient(&context, request, &cq));

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
		//request.set_round(round_num);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalMultKeyAck> > rpc(
			stubserver_->PrepareAsyncevalMixMultKeyFromClient(&context, request, &cq));

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
		//request.set_round(round_num);

		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalMultKeyReturn> > rpc(
			stubserver_->PrepareAsyncevalMultKeyToClient(&context, request, &cq));

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
		//request.set_round(round_num);

		std::unique_ptr<ClientAsyncResponseReader<thresh_net::EvalMultKeyReturn> > rpc(
			stubserver_->PrepareAsyncevalMixMultKeyToClient(&context, request, &cq));

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
	std::string CipherTextFromClient(const std::string& cText, uint32_t client_id) {
		thresh_net::CipherText request;    // data to be sent to the server
		request.set_ctext(cText);
		request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::CipherTextAck> > rpc(
			stubserver_->PrepareAsyncCipherTextFromClient(&context, request, &cq));

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
	std::string CipherTextFromClientToController(const std::string& cText, uint32_t measure_id) {
		thresh_net::ControllerCipherText request;    // data to be sent to the server
		request.set_ctext(cText);
		request.set_measure_id(measure_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::ControllerCipherTextReturn> > rpc(
			stubcontroller_->PrepareAsyncCipherTextFromClientToController(&context, request, &cq));

		rpc->StartCall();
		thresh_net::ControllerCipherTextReturn reply;   // data returned from the server
		Status status;         // status of the RPC upon completion
		rpc->Finish(&reply, &status, (void*)RequestTag::CIPHER_TEXT_FROM_CLIENT_TO_CONTROLLER_REQUEST);

		void* got_tag;
		bool success = false;
		GPR_ASSERT(cq.Next(&got_tag, &success));

		GPR_ASSERT(got_tag == (void*)RequestTag::CIPHER_TEXT_FROM_CLIENT_TO_CONTROLLER_REQUEST);
		GPR_ASSERT(success);

		// Act upon the status of the actual RPC.
		if (!status.ok())
			OPENFHE_THROW(lbcrypto::openfhe_error, status.error_message());

		return reply.maskedctext();
	}

	// ============================================================================================
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> genCT(lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc,
		lbcrypto::PublicKey<lbcrypto::DCRTPoly> publicKey,
		Params &params) {
		//unsigned int ringsize = cc->GetRingDimension();
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
		
		//load vector of ints from input file
		std::ifstream fin;
		fin.open(params.input_file + "_" + params.client_id.c_str());
    
        int64_t data;

		if (!fin) {
            std::cout << "Unable to open input file";
            exit(1);  // terminate with error
        }

        while (fin >> data)
        {
            vectorOfInts.push_back(data);
        }
		fin.close();
		//pack them into a packed plaintext (vector encryption)
		lbcrypto::Plaintext pt = cc->MakePackedPlaintext(vectorOfInts);

        std::cout << "\n Original Plaintext: \n" << std::endl;
        std::cout << pt << std::endl;

		if( TEST_MODE ) {
			//std::cerr << "Sending plaintext: " << pt << std::endl;
			std::ostringstream osPt;
			osPt << pt;
		}

        return cc->Encrypt(publicKey, pt);  // Encrypt
	}


	// ============================================================================================
	bool CipherTextToClient(std::string& cText, uint32_t client_id) {
		thresh_net::CipherTextRequest request;    // data to be sent to the server

        request.set_client_id(client_id);

		ClientContext context;
		CompletionQueue cq;
		std::unique_ptr<ClientAsyncResponseReader<thresh_net::CipherTextReturn> > rpc(
			stubserver_->PrepareAsyncCipherTextToClient(&context, request, &cq));

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
			stubserver_->PrepareAsyncPartialCipherTextFromClient(&context, request, &cq));

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
			stubserver_->PrepareAsyncPartialCipherTextsReceivedWait(&context, request, &cq));

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
			stubserver_->PrepareAsyncPartialCipherTextToClient(&context, request, &cq));

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

private:

	std::unique_ptr<ThreshServer::Stub> stubserver_;
	std::unique_ptr<ThreshController::Stub> stubcontroller_;
};

#endif // __THRESH_CLIENT__
