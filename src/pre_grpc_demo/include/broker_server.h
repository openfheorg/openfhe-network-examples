// @file another_server.h
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

#ifndef __BROKER_SERVER_H__
#define __BROKER_SERVER_H__

#include <iostream>

#include "utils/exception.h"

#include "utils.h"
#include "broker_cipher_text_from_producer.h"
#include "broker_reencrypted_cipher_text_to_broker.h"
#include "broker_cipher_text_to_consumer.h"

class BrokerServerImpl final {
public:
    BrokerServerImpl() {}

    ~BrokerServerImpl() {
        server_->Shutdown();
        // Always shutdown the completion queue after the server.
        cq_->Shutdown();
    }

    // There is no shutdown handling in this code.
    void Run(Params& params) {
        grpc::ServerBuilder builder;
        localParams = params;
		// from https://github.com/grpc/grpc/blob/master/include/grpc/impl/codegen/grpc_types.h:
		// GRPC doesn't set size limit for sent messages by default. However, the max size of received messages
		// is limited by "#define GRPC_DEFAULT_MAX_RECV_MESSAGE_LENGTH (4 * 1024 * 1024)", which is only 4194304.
		// for every GRPC application the received message size must be set individually:
		//		for a client it may be "-1" (unlimited): SetMaxReceiveMessageSize(-1)
		//		for a server INT_MAX (from #include <climits>) should do it: SetMaxReceiveMessageSize(INT_MAX)
		// see https://nanxiao.me/en/message-length-setting-in-grpc/ for more information on the message size
        builder.SetMaxReceiveMessageSize(INT_MAX);

        std::shared_ptr<grpc::ServerCredentials> creds = nullptr;
        if (params.disableSSLAuthentication)
            creds = grpc::InsecureServerCredentials();
        else {
            grpc::SslServerCredentialsOptions::PemKeyCertPair keyCert = { file2String(params.server_private_key_file), file2String(params.server_cert_chain_file) };
            grpc::SslServerCredentialsOptions opts;
            opts.pem_root_certs = file2String(params.root_cert_file);
            opts.pem_key_cert_pairs.push_back(keyCert);
            creds = grpc::SslServerCredentials(opts);
        }

        builder.AddListeningPort(params.broker_socket_address, creds);
        builder.RegisterService(&service_);

        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();

        //clear the routing table at start
        ClearRoutingTable(params.routingtablepath + params.process_name);

        //initialize CC from the key server
        BrokerKeyServerInternalClient internalClient_pre_server(params);

        std::string client_name = params.process_name;
        auto reply_cryptocontext = internalClient_pre_server.CryptoContextRequestFromServer_Broker(client_name);
        if (!reply_cryptocontext.size())
            OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized CryptoContext");

        std::istringstream is(reply_cryptocontext);
        lbcrypto::Serial::Deserialize(bcc, is, GlobalSerializationType);
        if (!bcc)
            OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized CryptoContext is NULL");

        if (TEST_MODE)
            write2File(reply_cryptocontext, "./broker_cc_received_serialized.txt");

        // server's main loop.
        HandleRpcs();
    }

private:
    // This can be run in multiple threads if needed.
    void HandleRpcs() {
        // Spawn different instances derived from CallData to serve clients.

        // to receive ciphertexts from the producer and saves it to brokerCipherTexts map with producer_name
        new CipherTextRequestServicer(&service_, cq_.get(), localParams, bcc, brokerCipherTexts);

        // send re-encrypted ciphertext recursively from upstream to downstream broker -
        // saving reencrypted ciphertext at each broker with producer name
        new ReEncryptedCipherTextToBrokerRequestServicer(&service_, cq_.get(), localParams, bcc, brokerCipherTexts);

        // send re-encrypted ciphertext from upstream broker to consumer and save it in consumerCipherTexts
        new CipherTextToConsumerRequestServicer(&service_, cq_.get(), localParams, bcc, consumerCipherTexts, brokerCipherTexts);


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
            static_cast<CallData<pre_net::PreNetBroker::AsyncService>*>(tag)->Proceed();
        }
    }

    Params localParams;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    pre_net::PreNetBroker::AsyncService service_;
    std::unique_ptr<grpc::Server> server_;

    // broker cryptocontext received from key server
    lbcrypto::CryptoContext<lbcrypto::DCRTPoly> bcc;

    // map that saves reencrypted ciphertext at the broker with its producer name
    std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> brokerCipherTexts;

    // map that saves reencrypted consumer ciphertexts along with producer name
    std::unordered_map<std::string, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> consumerCipherTexts;
};


#endif // __BROKER_SERVER_H__
