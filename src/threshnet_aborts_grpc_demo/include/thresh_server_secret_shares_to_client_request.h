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
#ifndef __THRESH_SERVER_SECRET_SHARES_TO_CLIENT_REQUEST__
#define __THRESH_SERVER_SECRET_SHARES_TO_CLIENT_REQUEST__

#include "thresh_server_call_data.h"
#include "utils.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include "utils/serial.h"
#include "utils/exception.h"
#include <sstream>


// all instances of this class created dynamically
class SecretSharesToClientRequestServicer : public CallData {
public:
	SecretSharesToClientRequestServicer(ThreshServer::AsyncService* service, ServerCompletionQueue* cq,
		std::unordered_map<uint32_t, std::unordered_map<uint32_t, lbcrypto::DCRTPoly>>& SecretShares)
		: CallData(service, cq, CREATE), responder_(&ctx_), ssm(SecretShares) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestSecretSharesToClient(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new SecretSharesToClientRequestServicer(service_, cq_, ssm);

            //uint32_t requesting_client_id = request_.requesting_client_id();
			uint32_t recover_client_id = request_.recover_client_id();
			uint32_t threshold = request_.threshold();
			uint32_t num_of_parties = request_.num_of_parties();

            ss = ssm[recover_client_id];

            usint mapsize = 1;
			for (usint j = 1;j <= num_of_parties; j++) {
				if ((j != recover_client_id) && (mapsize <= threshold)) {
					sst[j] = ss[j];
					mapsize++;
				}
			}
			
			if (sst.size() == 0) {
				reply_.set_sharesavailable(false);
			}
			else {
				std::ostringstream os;
				lbcrypto::Serial::Serialize(sst, os, GlobalSerializationType);
				if (!os.str().size())
					OPENFHE_THROW(lbcrypto::serialize_error, "SecretShares serialization error");
				if (TEST_MODE)
					write2File(os.str(), "./server_secretshares_sent_serialized.txt");

				reply_.set_sharesavailable(true);
				reply_.set_secretshares(os.str());
			}
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
	thresh_net::SecretKeySharesRequest request_;
	thresh_net::SecretKeySharesReturn reply_;

	ServerAsyncResponseWriter<thresh_net::SecretKeySharesReturn> responder_;

    std::unordered_map<uint32_t, lbcrypto::DCRTPoly> ss, sst;
	std::unordered_map<uint32_t, std::unordered_map<uint32_t, lbcrypto::DCRTPoly>>& ssm;
};

#endif // __THRESH_SERVER_SECRET_SHARES_TO_CLIENT_REQUEST__
