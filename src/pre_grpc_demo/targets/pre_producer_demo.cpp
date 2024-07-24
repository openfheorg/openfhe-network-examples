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

#include <iostream>
#include <sstream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include "utils.h"
#include "pre_producer.h"

[[maybe_unused]] constexpr unsigned SLEEP_TIME_SECS = 10;

int main(int argc, char** argv) {
    Params params;
    // char optstring[] = "i:p:l:n:W:h";
    if (!processInputParams(argc, argv, params)) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // from https://github.com/grpc/grpc/blob/master/include/grpc/impl/codegen/grpc_types.h:
    // GRPC doesn't set size limit for sent messages by default. However, the max size of received messages
    // is limited by "#define GRPC_DEFAULT_MAX_RECV_MESSAGE_LENGTH (4 * 1024 * 1024)", which is only 4194304.
    // for server and client the size must be set individually:
    //  for a client it is (-1): SetMaxReceiveMessageSize(-1)
    //  for a server INT_MAX (from #include <climits>) should do it: SetMaxReceiveMessageSize(INT_MAX)
    // see https://nanxiao.me/en/message-length-setting-in-grpc/ for more information on the message size
    grpc::ChannelArguments channel_args;
    channel_args.SetMaxReceiveMessageSize(-1);

    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost:50051) using client-side SSL/TLS
    std::shared_ptr<grpc::Channel> channel_key_server = nullptr;
    std::shared_ptr<grpc::Channel> channel_broker_server = nullptr;
    if (params.disableSSLAuthentication) {
        channel_key_server = grpc::CreateCustomChannel(params.key_server_socket_address, grpc::InsecureChannelCredentials(), channel_args);
        channel_broker_server = grpc::CreateCustomChannel(params.broker_socket_address, grpc::InsecureChannelCredentials(), channel_args);
    }
    else {
        grpc::SslCredentialsOptions opts = {
            file2String(params.root_cert_file),
            file2String(params.client_private_key_file),
            file2String(params.client_cert_chain_file)
        };

        auto channel_creds = grpc::SslCredentials(grpc::SslCredentialsOptions(opts));
        channel_key_server = grpc::CreateCustomChannel(params.key_server_socket_address, channel_creds, channel_args);
        channel_broker_server = grpc::CreateCustomChannel(params.broker_socket_address, channel_creds, channel_args);
    }
    PreNetProducer preProducer_to_server(channel_key_server);
    PreNetProducer preProducer_to_broker(channel_broker_server, 1);

    std::string client_name = params.process_name;

    // get CryptoContext from the server
    std::string serializedCryptoContext(preProducer_to_server.CryptoContextFromServer());
    if (!serializedCryptoContext.size())
        OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized CryptoContext");

    lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc(nullptr);
    std::istringstream is(serializedCryptoContext);
    lbcrypto::Serial::Deserialize(cc, is, GlobalSerializationType);
    if (!cc)
        OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized CryptoContext is NULL");
    if (TEST_MODE)
        write2File(serializedCryptoContext, "./client_cc_received_serialized.txt");

    // generate and send private key to the server
    lbcrypto::KeyPair<lbcrypto::DCRTPoly> keyPair = cc->KeyGen();
    std::ostringstream osKey;
    lbcrypto::Serial::Serialize(keyPair.secretKey, osKey, GlobalSerializationType);
    if (!osKey.str().size())
        OPENFHE_THROW(lbcrypto::serialize_error, "Serialized private key is empty");
    if (TEST_MODE)
        write2File(osKey.str(), "./client_private_key_sent_serialized.txt");

    std::string pKeyAck(preProducer_to_server.PrivateKeyFromProducer(osKey.str(), client_name));

    // print the reply buffer
    if (TEST_MODE)
        std::cout << "Server's acknowledgement: " << buffer2PrintableString(pKeyAck) << "]" << std::endl;

    // send public key to the server

    std::ostringstream ospKey;
    lbcrypto::Serial::Serialize(keyPair.publicKey, ospKey, GlobalSerializationType);
    if (!ospKey.str().size())
        OPENFHE_THROW(lbcrypto::serialize_error, "Serialized public key is empty");
    if (TEST_MODE)
        write2File(ospKey.str(), "./client_producer_public_key_sent_serialized.txt");

    std::string pubKeyAck(preProducer_to_server.PublicKeyFromClient(ospKey.str(), client_name));
    // print the reply buffer
    if (TEST_MODE)
        std::cerr << "Server's acknowledgement public key: " << buffer2PrintableString(pubKeyAck) << "]" << std::endl;

    // generate and send cipher text to the server
    // std::vector<int64_t> vShorts; // must be a vector of int64_t for MakePlaintext
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> cipherText(preProducer_to_server.genCT(cc, keyPair.publicKey, params));
    std::ostringstream osCT;
    lbcrypto::Serial::Serialize(cipherText, osCT, GlobalSerializationType);
    if (!osCT.str().size())
        OPENFHE_THROW(lbcrypto::serialize_error, "Serialized cipther text is empty");
    if (TEST_MODE)
        write2File(osCT.str(), "./client_ciphertext_sent_serialized.txt");

    std::string ctAck(preProducer_to_broker.CipherTextFromProducer(osCT.str(), client_name)); //todo: include producer_id in parameters to create a vector of ciphertexts being reencrypted.

    if (TEST_MODE)
        std::cout << "Server's acknowledgement: " << buffer2PrintableString(ctAck) << "]" << std::endl;

    return 0;
}
