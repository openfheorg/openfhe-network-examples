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
#ifndef __THRESH_SERVER_CRYPTO_CONTEXT_REQUEST__
#define __THRESH_SERVER_CRYPTO_CONTEXT_REQUEST__

#include "thresh_server_call_data.h"
#include "utils.h"
#include "gen-cryptocontext.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "scheme/bfvrns/cryptocontext-bfvrns.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <sstream>


// all instances of this class created dynamically
class CryptoContextRequestServicer : public CallData<thresh_net::ThreshServer::AsyncService> {
public:
	CryptoContextRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
		lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cryptoContext)
		: CallData(service, cq, CREATE), responder_(&ctx_), cc(cryptoContext) {
		initializeCC();
		Proceed();
	}

	void initializeCC() {
		//usint multDepth = 4;
		//usint ring_dimension = 8192;
		TimeVar t;
		TIC(t);

		int plaintextModulus = 65537;
		double sigma = 3.2;
		lbcrypto::SecurityLevel securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;

		usint batchSize = 1024;
		usint multDepth = 4;

		lbcrypto::CCParams<lbcrypto::CryptoContextBFVRNS> parameters;		

        parameters.SetPlaintextModulus(plaintextModulus);
		parameters.SetSecurityLevel(securityLevel);
		parameters.SetStandardDeviation(sigma);
		parameters.SetSecretKeyDist(lbcrypto::UNIFORM_TERNARY);
		parameters.SetMultiplicativeDepth(multDepth);
		parameters.SetBatchSize(batchSize);

		cc = GenCryptoContext(parameters);

		cc->Enable(lbcrypto::PKE);
		cc->Enable(lbcrypto::LEVELEDSHE);
		cc->Enable(lbcrypto::MULTIPARTY);
		
		auto elapsed_seconds = TOC_MS(t);
        
		if (TEST_MODE)
            std::cout << "Cryptocontext generation time: " << elapsed_seconds << " ms\n";

	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestCryptoContextFromServer(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			// Spawn a new CryptoContextRequestServicer instance to serve new clients while we process
			// the one for this CryptoContextRequestServicer. The instance will deallocate itself as
			// part of its FINISH state.
			// TODO (dsuponit): do we really need this "new" instance in every Proceed()??? I am still not sure
			new CryptoContextRequestServicer(service_, cq_, cc);

			std::ostringstream os;
			lbcrypto::Serial::Serialize(cc, os, GlobalSerializationType);
			if (!os.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "CryptoContext serialization error");
			if (TEST_MODE)
				write2File(os.str(), "./server_cc_sent_serialized.txt"); // TODO: added for testing only

			reply_.set_cryptocontext(os.str());
			status_ = FINISH;
			responder_.Finish(reply_, Status::OK, this);
		}
		else {
			GPR_ASSERT(status_ == FINISH);
			// Once in the FINISH state, deallocate ourselves (CallData).
			delete this;
		}
	}

private:
	thresh_net::CryptoContextRequest request_;
	thresh_net::CryptoContextReturn reply_;

	ServerAsyncResponseWriter<thresh_net::CryptoContextReturn> responder_;
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc;
};

#endif // __THRESH_SERVER_CRYPTO_CONTEXT_REQUEST__
