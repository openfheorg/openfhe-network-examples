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
#ifndef __KS_CRYPTO_CONTEXT_TO_PRODUCER_CONSUMER_H__
#define __KS_CRYPTO_CONTEXT_TO_PRODUCER_CONSUMER_H__

#include <sstream>

#include "cryptocontext-ser.h"
#include "scheme/bgvrns/bgvrns-ser.h"
#include "utils/exception.h"

#include "utils.h"
#include "pre_server_call_data.h"

// all instances of this class created dynamically
class CryptoContextRequestServicer : public CallData<pre_net::PreNetServer::AsyncService> {
public:
	CryptoContextRequestServicer(pre_net::PreNetServer::AsyncService* service, grpc::ServerCompletionQueue* cq,
		lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cryptoContext)
		: CallData(service, cq, CREATE), responder_(&ctx_), cc(cryptoContext) {
		//initializeCC();
		Proceed();
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
			responder_.Finish(reply_, grpc::Status::OK, this);
		}
		else {
			GPR_ASSERT(status_ == FINISH);
			// Once in the FINISH state, deallocate ourselves (CallData).
			delete this;
		}
	}

private:
	pre_net::MessageRequest request_;
	pre_net::MessageReturn reply_;

	grpc::ServerAsyncResponseWriter<pre_net::MessageReturn> responder_;
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc;
};

#endif // __KS_BROKER_CRYPTO_CONTEXT_TO_PRODUCER_CONSUMER_H__
