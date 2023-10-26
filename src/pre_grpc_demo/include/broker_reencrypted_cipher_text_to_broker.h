// @file another_server_test_request.h
// @author TPOC: info@DualityTech.com
//
// @copyright Copyright (c) 2021, Duality Technologies Inc.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution. THIS SOFTWARE IS
// PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef __BROKER_REENCRYPTED_CIPHER_TEXT_TO_BROKER_H__
#define __BROKER_REENCRYPTED_CIPHER_TEXT_TO_BROKER_H__


#include <iostream>

#include "utils/exception.h"
#include "key/key-ser.h"

#include "utils.h"
#include "pre_server_call_data.h"
#include "broker_ks_internal_client.h"
#include "broker_broker_internal_client.h"


// all instances of this class created dynamically
class ReEncryptedCipherTextToBrokerRequestServicer : public CallData<pre_net::PreNetBroker::AsyncService> {
public:
	ReEncryptedCipherTextToBrokerRequestServicer(pre_net::PreNetBroker::AsyncService* service, grpc::ServerCompletionQueue* cq, const Params& params0,
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cryptoContext,
	std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& brokerCipherTexts)
		: CallData(service, cq, CREATE), params(params0), responder_(&ctx_), bcc(cryptoContext), brokerCTs(brokerCipherTexts) {
		Proceed();
	}

	void Proceed() override {
		if (status_ == CREATE) {
			status_ = PROCESS;

			service_->RequestReEncryptedCipherTextToBroker(&ctx_, &request_, &responder_, cq_, cq_, this);
		}
		else if (status_ == PROCESS) {
			new ReEncryptedCipherTextToBrokerRequestServicer(service_, cq_, params, bcc, brokerCTs);
			//std::cerr << "From AnotherClient: " << request_.acknowledgement_in() << std::endl;
			// request to PreNetServer
			// 1. create a client
			//params.socket_address = "0.0.0.0:50051";


            std::string upstream_client_name = params.upstream_broker_name;
			std::string client_type = request_.client_type();
			std::string downstream_client_name = request_.client_name();
			std::string client_name = params.process_name;

            std::string requested_channel_name = request_.channel_name();

			std::string producer_name = SplitString(requested_channel_name, "-")[0];
            std::string security_model = params.security_model;

            std::string upstream_client_type;
			bool source_sink_flag = false;

			bool producerRegistered = false;

            if (!params.upstream_broker_name.size()) {
					upstream_client_name = producer_name;
					upstream_client_type = "producer";
					source_sink_flag = true;
			} else {
					upstream_client_type = "broker";
			}
            lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ct;

			//gets channel name, source, destination and source/sink flag from routing table.
			auto routingTableData = ParseRoutingTable(params.routingtablepath, params.process_name, downstream_client_name);
            
			bool routingTableEmpty, channelFound = false;
            if (routingTableData.empty()) {
                routingTableEmpty = true;
			} else {
				routingTableEmpty = false;
                //else if routing table not empty
				if (routingTableData.find(requested_channel_name) != routingTableData.end()) {
					channelFound = true;
				}
			}
            
			
			//Reencrypt the ciphertext from upstream broker to reencrypt to the consumer
			lbcrypto::Ciphertext<lbcrypto::DCRTPoly> reencryptedCText;
			//if routing table is empty or does not have the requested channel
			if (routingTableEmpty || !channelFound) {
			    //a broker connected to a producer would have the CT even when there is no entry in routing table
				if (brokerCTs.find(producer_name) == brokerCTs.end()) {	

					BrokerKeyServerInternalClient internalClient_pre_server(params);
					
					std::string reply_publickeyclient, reply_reencryptionkey;
					auto replypk = internalClient_pre_server.PublicKeyToBroker(reply_publickeyclient, upstream_client_type, upstream_client_name);//client_name);
					auto replyrk = internalClient_pre_server.ReEncryptionKeyToBroker(reply_reencryptionkey, client_type, client_name, upstream_client_name, upstream_client_type);

					//get ciphertext from upstream
					if (!params.upstream_broker_name.size()) {
						if (replypk.producerregistered() && replyrk.producerregistered()) {
							std::cerr << "Producer registered at key server but CT not found at source broker" << std::endl;
						} else {
							std::cerr << "Producer not registered at key server" << std::endl;
						}
					} else {
						BrokerUpstreamInternalClient internalClient_brokerserver(params);

						std::string strct;
						auto replyct = internalClient_brokerserver.ReEncryptedCipherTextToBroker(strct, upstream_client_type, client_name, requested_channel_name);

						if (replyct.producerregistered()) {
							producerRegistered = true;

							if (!strct.size())
								OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized cipher text");
							
							std::istringstream isCText(strct);
							lbcrypto::Serial::Deserialize(ct, isCText, GlobalSerializationType);
							if (!ct)
								OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized ciphertext is NULL");
						}
					}

					if (producerRegistered) {
						//*********
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
						if (TEST_MODE)
							write2File(reply_reencryptionkey, "./client_reencryption_key_received_serialized.txt");
						//*********
						
						if (reencryptionKey) {
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

						} else {
							reencryptedCText = ct;
						}

						brokerCTs[producer_name] = reencryptedCText;
					}
					else {
						std::cerr << "Producer " << producer_name << " has not registered with the key server" << std::endl;
					}
				} else {
					reencryptedCText = brokerCTs[producer_name];
					producerRegistered = true;
				}
			} else {
				reencryptedCText = brokerCTs[producer_name];
				producerRegistered = true;
			}

			if (producerRegistered) {
				std::ostringstream os;
				lbcrypto::Serial::Serialize(reencryptedCText, os, GlobalSerializationType);
				if (!os.str().size())
					OPENFHE_THROW(lbcrypto::serialize_error, "CipherText serialization error");
				if (TEST_MODE)
					write2File(os.str(), "./server_ciphertext_sent_serialized.txt");

				reply_.set_textavailable(true);
				reply_.set_ciphertext(os.str());
				reply_.set_producerregistered(true);

				//write the channel to routing table
				UpdateRoutingTable(params.routingtablepath, requested_channel_name, params.process_name, upstream_client_name, downstream_client_name, source_sink_flag);
			} else {
				reply_.set_textavailable(false);
				reply_.set_producerregistered(false);
			}

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
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& bcc;
	std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& brokerCTs;
};

#endif // __BROKER_REENCRYPTED_CIPHER_TEXT_TO_BROKER_H__
