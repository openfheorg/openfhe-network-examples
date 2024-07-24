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
#include "pre_consumer.h"

constexpr unsigned SLEEP_TIME_SECS = 10;

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
    // for every GRPC application the received message size must be set individually:
    //  for a client it may be "-1" (unlimited): SetMaxReceiveMessageSize(-1)
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
        channel_broker_server = grpc::CreateCustomChannel(params.upstream_broker_socket_address, grpc::InsecureChannelCredentials(), channel_args);
    }
    else {
        grpc::SslCredentialsOptions opts = {
        file2String(params.root_cert_file),
        file2String(params.client_private_key_file),
        file2String(params.client_cert_chain_file) };

        auto channel_creds = grpc::SslCredentials(grpc::SslCredentialsOptions(opts));
        channel_key_server = grpc::CreateCustomChannel(params.key_server_socket_address, channel_creds, channel_args);
        channel_broker_server = grpc::CreateCustomChannel(params.upstream_broker_socket_address, channel_creds, channel_args);
    }
    PreNetConsumer preConsumer_to_server(channel_key_server);
    PreNetConsumer preConsumer_to_broker(channel_broker_server, 1);

    std::string client_name = params.process_name;
    std::string upstream_client_name = params.upstream_broker_name;
    std::string channel_name = params.channel_name;

    if (!channel_name.size())
        OPENFHE_THROW(lbcrypto::openfhe_error, "Channel name not available");

    // get CryptoContext from the server
    std::string serializedCryptoContext(preConsumer_to_server.CryptoContextFromServer());
    if (!serializedCryptoContext.size())
        OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized CryptoContext");
    lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc(nullptr);
    std::istringstream is(serializedCryptoContext);
    lbcrypto::Serial::Deserialize(cc, is, GlobalSerializationType);

    if (!cc)
        OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized CryptoContext is NULL");
    if (TEST_MODE)
        write2File(serializedCryptoContext, "./client_cc_received_serialized.txt");

    // generate and send public key to the server
    lbcrypto::KeyPair<lbcrypto::DCRTPoly> keyPair = cc->KeyGen();
    std::ostringstream osKey;
    lbcrypto::Serial::Serialize(keyPair.publicKey, osKey, GlobalSerializationType);
    if (!osKey.str().size())
        OPENFHE_THROW(lbcrypto::serialize_error, "Serialized public key is empty");
    if (TEST_MODE)
        write2File(osKey.str(), "./client_consumer_public_key_sent_serialized.txt");
    std::string pKeyAck(preConsumer_to_server.PublicKeyFromClient(osKey.str(), client_name));
    // print the reply buffer
    if (TEST_MODE)
        std::cerr << "Server's acknowledgement: " << buffer2PrintableString(pKeyAck) << "]" << std::endl;

    // get ciphertext from the broker
    std::string serializedCipherText;
    auto replyct = preConsumer_to_broker.CipherTextToConsumer(serializedCipherText, "consumer", client_name, upstream_client_name, channel_name);

    if (!replyct.producerregistered())
        OPENFHE_THROW(lbcrypto::openfhe_error, "Producer not registered with the key server");

    while (!replyct.textavailable()) {
        std::cerr << "Sleep for " << SLEEP_TIME_SECS << " seconds as CipherText is not available" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME_SECS));
        replyct = preConsumer_to_broker.CipherTextToConsumer(serializedCipherText, "consumer", client_name, upstream_client_name, channel_name);
    }

    if (!serializedCipherText.size())
        OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized cipher text");
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ct(nullptr);
    std::istringstream isCText(serializedCipherText);
    lbcrypto::Serial::Deserialize(ct, isCText, GlobalSerializationType);

    if (!ct)
        OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized ciphertext is NULL");
    if (TEST_MODE)
        write2File(serializedCipherText, "./client_ciphertext_received_serialized.txt");

    lbcrypto::Plaintext pt;

    // decrypt the ciphertext received from the broker using the secret key and write to file
    cc->Decrypt(keyPair.secretKey, ct, &pt);
    auto decryptedVec = pt->GetCoefPackedValue();

    // convert decrypted vector of ints to
    unsigned int plaintextModulus = cc->GetCryptoParameters()->GetPlaintextModulus();
    std::vector<int64_t> decryptedkeyVec;
    for(int i = 0; i < 256; i++) {
        decryptedkeyVec.push_back(decryptedVec[i]);
        // std::cout << "decryptedvec[" << i << "]: " << decryptedVec [i] << std::endl;
    }

    // vector with only the first 256 bits to get the 256 bits AES key
    std::string unpackedConsumer = intvec2hexstr(decryptedkeyVec, plaintextModulus);
    std::ofstream keyoutfile;
    keyoutfile.open(params.consumer_aes_key + "_" + channel_name);
    if (!keyoutfile) {
        std::cout << "Unable to open key file";
        exit(1);  // terminate with error
    }
    keyoutfile << unpackedConsumer;
    keyoutfile.close();

    return 0;
}
