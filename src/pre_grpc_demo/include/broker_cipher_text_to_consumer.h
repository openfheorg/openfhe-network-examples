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

#ifndef __BROKER_CIPHER_TEXT_TO_CONSUMER_H__
#define __BROKER_CIPHER_TEXT_TO_CONSUMER_H__

#include <iostream>

#include "pre_server_call_data.h"
#include "utils/exception.h"

#include "utils.h"
#include "broker_ks_internal_client.h"
#include "broker_broker_internal_client.h"

// all instances of this class created dynamically
class CipherTextToConsumerRequestServicer : public CallData<pre_net::PreNetBroker::AsyncService> {
public:
    CipherTextToConsumerRequestServicer(pre_net::PreNetBroker::AsyncService* service, grpc::ServerCompletionQueue* cq, const Params& params0,
            lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cryptoContext,
            std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& consumerCipherTexts,
            std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& brokerCipherTexts)
            : CallData(service, cq, CREATE), params(params0), responder_(&ctx_), bcc(cryptoContext), consumerCTs(consumerCipherTexts), brokerCTs(brokerCipherTexts) {
        Proceed();
    }

    void Proceed() override {
        if (status_ == CREATE) {
            status_ = PROCESS;
            service_->RequestCipherTextToConsumer(&ctx_, &request_, &responder_, cq_, cq_, this);
        }
        else if (status_ == PROCESS) {
            new CipherTextToConsumerRequestServicer(service_, cq_, params, bcc, consumerCTs, brokerCTs);

            std::string upstream_client_name = params.upstream_broker_name;
            std::string client_type = request_.client_type();
            std::string client_name = request_.client_name();
            std::string current_client_name = params.process_name;
            std::string current_client_type = "broker";
            std::string requested_channel_name = request_.channel_name();

            std::string producer_name = SplitString(requested_channel_name, "-")[0];
            std::string consumer_in_channel = SplitString(requested_channel_name, "-")[1];

            // check if consumer name in channel and consumer name in request are the same
            if (client_name != consumer_in_channel)
                OPENFHE_THROW(lbcrypto::openfhe_error, "consumer name in channel requested doesn't match the actual consumer name");

            std::string security_model = params.security_model;
            std::string upstream_client_type;
            std::string reply_publickeyclient, reply_reencryptionkey, reply_reencryptedCT;

            bool source_sink_flag = false;
            bool producerRegistered = false;

            lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ct(nullptr);

            if (!params.upstream_broker_name.size()) {
                upstream_client_type = "producer";
                upstream_client_name = producer_name;
                source_sink_flag = true;
            } else {
                upstream_client_type = "broker";
            }

            // gets channel name, source, destination and source/sink flag from routing table.
            auto routingTableData = ParseRoutingTable(params.routingtablepath, params.process_name, client_name);
            bool routingTableEmpty = true, channelFound = false;

            if (routingTableData.empty()) {
                routingTableEmpty = true;
            } else {
                routingTableEmpty = false;
                // else if routing table not empty
                if (routingTableData.find(requested_channel_name) != routingTableData.end()) {
                    channelFound = true;
                }
            }

            // Reencrypt the ciphertext from upstream broker to reencrypt to the consumer
            // if routing table is empty or does not have the requested channel
            lbcrypto::Ciphertext<lbcrypto::DCRTPoly> reencryptedCText, reencryptedCTextConsumer;
            BrokerKeyServerInternalClient internalClient_pre_server(params);

            // receive public key of upstream broker/producer (sender)
            auto replypk = internalClient_pre_server.PublicKeyToBroker(reply_publickeyclient, upstream_client_type, upstream_client_name);

            // receive reencryption key from upstream to current broker
            auto replyrk = internalClient_pre_server.ReEncryptionKeyToBroker(reply_reencryptionkey, current_client_type, current_client_name, upstream_client_name, upstream_client_type);

            lbcrypto::PublicKey<lbcrypto::DCRTPoly> senderPubKey(nullptr);
            // get ciphertext from upstream
            // check if the routing table is empty
            if (routingTableEmpty || !channelFound) {
                if (brokerCTs.find(producer_name) == brokerCTs.end()) {
                    if (!params.upstream_broker_name.size()) {
                        // check if the broker has the ciphertext received from the producer
                        if (replypk.producerregistered() && replyrk.producerregistered()) {
                            std::cerr << "Producer registered at key server but CT not found at source broker" << std::endl;
                        } else {
                            std::cerr << "Producer CT not found at source broker" << std::endl;
                        }
                    } else {
                        BrokerUpstreamInternalClient internalClient_brokerserver(params);
                        auto replyct = internalClient_brokerserver.ReEncryptedCipherTextToBroker(reply_reencryptedCT, upstream_client_type, params.process_name, requested_channel_name);
                        if (replyct.producerregistered()) {
                            producerRegistered = true;

                            if (!reply_reencryptedCT.size())
                                OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized cipher text");

                            std::istringstream isCText(reply_reencryptedCT);
                            lbcrypto::Serial::Deserialize(ct, isCText, GlobalSerializationType);
                            if (!ct)
                                OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized ciphertext is NULL");
                            if (TEST_MODE)
                                write2File(reply_reencryptedCT, "./client_ciphertext_received_serialized.txt");

                            if (!reply_publickeyclient.size())
                                OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized public key");
                            if (TEST_MODE)
                                write2File(reply_publickeyclient, "./Sender_public_key_received_serialized.txt");

                            std::istringstream isSenderPubKey(reply_publickeyclient);
                            lbcrypto::Serial::Deserialize(senderPubKey, isSenderPubKey, GlobalSerializationType);

                            if (!reply_reencryptionkey.size())
                                OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized re-encryption key");

                            lbcrypto::EvalKey<lbcrypto::DCRTPoly> reencryptionKey (nullptr);
                            std::istringstream isReencrKey(reply_reencryptionkey);

                            lbcrypto::Serial::Deserialize(reencryptionKey, isReencrKey, GlobalSerializationType);
                            if (TEST_MODE)
                                write2File(reply_reencryptionkey, "./client_reencryption_key_received_serialized.txt");

                            if (reencryptionKey) {
                                if (security_model == "INDCPA") {
                                    reencryptedCText = bcc->ReEncrypt(ct, reencryptionKey); //IND-CPA security
                                } else if (security_model == "FIXED_NOISE_HRA") {
                                    reencryptedCText = bcc->ReEncrypt(ct, reencryptionKey, senderPubKey); //Fixed noise security
                                } else if (security_model == "NOISE_FLOODING_HRA") {
                                    reencryptedCText = bcc->ReEncrypt(ct, reencryptionKey, senderPubKey);
                                    if (reencryptedCText->GetElements()[0].GetNumOfElements() > 1)
                                        reencryptedCText = bcc->ModReduce(reencryptedCText);
                                } else if (security_model == "NOISE_FLOODING_HRA_HYBRID") {
                                    reencryptedCText = bcc->ReEncrypt(ct, reencryptionKey, senderPubKey);
                                    if (reencryptedCText->GetElements()[0].GetNumOfElements() > 1)
                                        reencryptedCText = bcc->ModReduce(reencryptedCText);
                                } else {
                                    OPENFHE_THROW(lbcrypto::openfhe_error, "Bad security model");
                                }
                            } else {
                                reencryptedCText = ct;
                            }
                            brokerCTs[producer_name] = reencryptedCText;
                        } else {
                            std::cerr << "Producer CT not found at source broker" << std::endl;
                        }
                    }
                } else {
                    reencryptedCText = brokerCTs[producer_name];
                    producerRegistered = true;
                }
            } else {
                reencryptedCText = brokerCTs[producer_name];
                producerRegistered = true;
            }

            //*********
            // receive the reencryption key from current broker to consumer
            // receive public key of upstream broker/producer (sender)
            replypk = internalClient_pre_server.PublicKeyToBroker(reply_publickeyclient, current_client_type, current_client_name);
            replyrk = internalClient_pre_server.ReEncryptionKeyToBroker(reply_reencryptionkey, client_type, client_name, current_client_name, current_client_type);
            if (producerRegistered) {
                // write the channel to routing table
                UpdateRoutingTable(params.routingtablepath, requested_channel_name, params.process_name, upstream_client_name, client_name, source_sink_flag);

                if (!reply_publickeyclient.size())
                    OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized public key");

                std::istringstream isSenderPubKey(reply_publickeyclient);
                lbcrypto::Serial::Deserialize(senderPubKey, isSenderPubKey, GlobalSerializationType);

                if (!reply_reencryptionkey.size())
                    OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized re-encryption key");

                lbcrypto::EvalKey<lbcrypto::DCRTPoly> reencryptionKey (nullptr);
                std::istringstream isReencrKey(reply_reencryptionkey);

                lbcrypto::Serial::Deserialize(reencryptionKey, isReencrKey, GlobalSerializationType);
                if (TEST_MODE)
                    write2File(reply_reencryptionkey, "./client_reencryption_key_received_serialized.txt");

                if (reencryptionKey) {
                    if (security_model == "INDCPA") {
                        reencryptedCTextConsumer = bcc->ReEncrypt(reencryptedCText, reencryptionKey); //IND-CPA security
                    } else if (security_model == "FIXED_NOISE_HRA") {
                        reencryptedCTextConsumer = bcc->ReEncrypt(reencryptedCText, reencryptionKey, senderPubKey); //Fixed noise security
                    } else if (security_model == "NOISE_FLOODING_HRA") {
                        reencryptedCTextConsumer = bcc->ReEncrypt(reencryptedCText, reencryptionKey, senderPubKey);
                        if (reencryptedCTextConsumer->GetElements()[0].GetNumOfElements() > 1)
                        reencryptedCTextConsumer = bcc->ModReduce(reencryptedCTextConsumer);
                    } else if (security_model == "NOISE_FLOODING_HRA_HYBRID") {
                        reencryptedCTextConsumer = bcc->ReEncrypt(reencryptedCText, reencryptionKey, senderPubKey);
                        if (reencryptedCTextConsumer->GetElements()[0].GetNumOfElements() > 1)
                            reencryptedCTextConsumer = bcc->ModReduce(reencryptedCTextConsumer);
                    } else {
                        OPENFHE_THROW(lbcrypto::openfhe_error, "Bad security model");
                    }
                } else {
                    reencryptedCTextConsumer = reencryptedCText;
                }
            } else {
                std::cerr << "Producer " << producer_name << " has not registered with the key server" << std::endl;
            }

            if (producerRegistered) {
                // consumerCTs[client_name] = reencryptedCText;

                std::ostringstream os;
                lbcrypto::Serial::Serialize(reencryptedCTextConsumer, os, GlobalSerializationType);
                if (!os.str().size())
                    OPENFHE_THROW(lbcrypto::serialize_error, "CipherText serialization error");
                if (TEST_MODE)
                    write2File(os.str(), "./server_ciphertext_sent_serialized.txt");

                reply_.set_textavailable(true);
                reply_.set_producerregistered(true);
                reply_.set_ciphertext(os.str());
            } else {
                reply_.set_producerregistered(false);
                reply_.set_textavailable(false);
            }
            status_ = FINISH;
            responder_.Finish(reply_, grpc::Status::OK, this);
        } else {
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
    std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& consumerCTs;
    std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>>& brokerCTs;
};

#endif // __BROKER_CIPHER_TEXT_TO_CONSUMER_H__
