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
#ifndef __BROKER_CIPHER_TEXT_FROM_PRODUCER_H__
#define __BROKER_CIPHER_TEXT_FROM_PRODUCER_H__

#include <iostream>
#include <memory>
#include <string>

#include "cryptocontext-ser.h"
#include "scheme/bgvrns/bgvrns-ser.h"
#include "utils/exception.h"

#include "utils.h"
#include "pre_server_call_data.h"
#include "broker_ks_internal_client.h"
#include "broker_broker_internal_client.h"


// ============================================================================================
// all instances of this class created dynamically
class CipherTextRequestServicer : public CallData<pre_net::PreNetBroker::AsyncService> {
public:
	CipherTextRequestServicer(pre_net::PreNetBroker::AsyncService* service, grpc::ServerCompletionQueue* cq, const Params& params0,
		lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cryptoContext,
		std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& brokerCipherTexts)
		: CallData(service, cq, CREATE), params(params0), responder_(&ctx_), bcc(cryptoContext), cts(brokerCipherTexts) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestCipherTextFromProducer(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new CipherTextRequestServicer(service_, cq_, params, bcc, cts);

            std::string client_name = request_.client_name();
			if (TEST_MODE)
				write2File(request_.ctext(), "./server_ciphertext_received_serialized.txt");
			std::istringstream is(request_.ctext());
			if (!is.str().size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty cipher text buffer");

			lbcrypto::Serial::Deserialize(ct, is, GlobalSerializationType);
			if (!ct)
				OPENFHE_THROW(lbcrypto::deserialize_error, "CipherText de-serialization error");
	
			reply_.set_acknowledgement("Received CTEXT. Thank you, Your Server");

			//==============
			std::string security_model = params.security_model;
			BrokerKeyServerInternalClient internalClient_pre_server(params);

			std::string reply_publickeyclient, reply_reencryptionkey;
			internalClient_pre_server.PublicKeyToBroker(reply_publickeyclient, "producer", client_name);
			internalClient_pre_server.ReEncryptionKeyToBroker(reply_reencryptionkey, "broker", params.process_name, client_name, "producer");

			if (!reply_publickeyclient.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized public key");

			if (TEST_MODE)
				write2File(reply_publickeyclient, "./Sender_public_key_received_serialized.txt");
				
			lbcrypto::PublicKey<lbcrypto::DCRTPoly> senderPubKey(nullptr);
			std::istringstream isSenderPubKey(reply_publickeyclient);
			lbcrypto::Serial::Deserialize(senderPubKey, isSenderPubKey, GlobalSerializationType);
		
			//********* 
			if (!reply_reencryptionkey.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized re-encryption key");
			
			lbcrypto::EvalKey<lbcrypto::DCRTPoly> reencryptionKey (nullptr);
			std::istringstream isReencrKey(reply_reencryptionkey);
			lbcrypto::Serial::Deserialize(reencryptionKey, isReencrKey, GlobalSerializationType);
			
			lbcrypto::Ciphertext<lbcrypto::DCRTPoly> reencryptedCText;
			if (TEST_MODE)
				write2File(reply_reencryptionkey, "./client_reencryption_key_received_serialized.txt");
			//*********
			if (security_model == "INDCPA") {
				reencryptedCText = bcc->ReEncrypt(ct, reencryptionKey); //IND-CPA security
			}
			else if (security_model == "FIXED_NOISE_HRA") {
				reencryptedCText = bcc->ReEncrypt(ct, reencryptionKey, senderPubKey); //Fixed noise security
			}
			else if (security_model == "NOISE_FLOODING_HRA") {
				auto reencryptedCText1 = bcc->ReEncrypt(ct, reencryptionKey, senderPubKey);//commenting temporarily, 1); //HRA security with noise flooding
				reencryptedCText = bcc->ModReduce(reencryptedCText1);
			} else {
				reencryptedCText = bcc->ReEncrypt(ct, reencryptionKey); //IND-CPA security default
			}
			cts[client_name] = reencryptedCText;
			std::cout << "Received ciphertext from producer " << client_name << std::endl;
			//-=============
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
    Params params;
	pre_net::MessageRequest request_;
	pre_net::MessageReturn reply_;

	grpc::ServerAsyncResponseWriter<pre_net::MessageReturn> responder_;
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ct;
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& bcc;
	std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& cts;
};

#endif // __BROKER_CIPHER_TEXT_FROM_PRODUCER_H__
