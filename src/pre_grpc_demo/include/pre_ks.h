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
#ifndef __PRE_KS__
#define __PRE_KS__

#include <iostream>
#include <climits>

#include "scheme/bgvrns/cryptocontext-bgvrns.h"
#include "cryptocontext-ser.h"
#include "scheme/bgvrns/bgvrns-ser.h"
#include "gen-cryptocontext.h"

#include "utils.h"
#include "ks_crypto_context_to_producer_consumer.h"
#include "ks_cryptocontext_to_broker.h"
#include "ks_cryptocontext_to_ks.h"
#include "ks_private_key_from_producer.h"
#include "ks_public_key_from_producer_consumer.h"
#include "ks_broker_public_key_from_ks.h"
#include "ks_broker_public_key_to_broker.h"
#include "ks_broker_public_key_to_ks.h"
#include "ks_reencryption_key_to_broker.h"
#include "ks_reencryption_key_to_ks.h"
#include "ks_ks_internal_client.h"

using grpc::Server;
using grpc::ServerBuilder;

class ServerImpl final {
public:
	ServerImpl() : cc(nullptr) {}//, clientPubKey(nullptr) {}//, PublicKeys(nullptr) {}

	~ServerImpl() {
		server_->Shutdown();
		// Always shutdown the completion queue after the server.
		cq_->Shutdown();
	}

    void initializeCC(Params& params) {
		std::string security_model = params.security_model.c_str();
		std::string upstream_key_server = params.upstream_key_server_socket_address.c_str();
        //todomultipletz: if condition to generate cc 
		if (!upstream_key_server.size()) {
			lbcrypto::CCParams<lbcrypto::CryptoContextBGVRNS> parameters;		
			//default if security model is not passed
			int plaintextModulus = 2;
			uint32_t multDepth = 0;
			double sigma = 3.2;		
			lbcrypto::SecurityLevel securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;
			
			usint ringDimension = 1024;
			usint digitSize = 1;//1;
			//usint modulus_bits = 27;
			usint dcrtbits = 0;

			usint qmodulus = 27;
			usint firstqmod = 27;
			parameters.SetPREMode(lbcrypto::INDCPA);

			if (security_model == "INDCPA") {
				plaintextModulus = 2;
				multDepth = 0;
				sigma = 3.2;		
				securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;
				ringDimension = 1024;
				digitSize = 1;
				dcrtbits = 0;

				qmodulus = 27;
				firstqmod = 27;
				parameters.SetPREMode(lbcrypto::INDCPA);
				parameters.SetKeySwitchTechnique(lbcrypto::BV);
			} else if (security_model == "FIXED_NOISE_HRA") {
				plaintextModulus = 2;
				multDepth = 0;
				sigma = 3.2;		
				securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;
				ringDimension = 2048;
				digitSize = 18;
				//modulus_bits = 54;
				dcrtbits = 0;

				qmodulus = 54;
				firstqmod = 54;
				parameters.SetPREMode(lbcrypto::FIXED_NOISE_HRA);
				parameters.SetKeySwitchTechnique(lbcrypto::BV);
			} else if (security_model == "NOISE_FLOODING_HRA"){
				plaintextModulus = 2;
				multDepth = 0;
				sigma = 3.2;		
				securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;
				ringDimension = 16384;
				digitSize = 1;
				//modulus_bits = 438;
				dcrtbits = 30;

				qmodulus = 438;
				firstqmod = 60;
				parameters.SetPREMode(lbcrypto::NOISE_FLOODING_HRA);
				parameters.SetKeySwitchTechnique(lbcrypto::BV);
			} else if (security_model == "NOISE_FLOODING_HRA_HYBRID") {
				plaintextModulus = 2;
				ringDimension = 16384;
				digitSize = 0;
				dcrtbits = 30;

				qmodulus = 438;
				firstqmod = 60;
				uint32_t dnum = 3;
				parameters.SetPREMode(lbcrypto::NOISE_FLOODING_HRA);
				parameters.SetKeySwitchTechnique(lbcrypto::HYBRID);
				parameters.SetNumLargeDigits(dnum);
				}

			parameters.SetMultiplicativeDepth(multDepth);
			parameters.SetPlaintextModulus(plaintextModulus);
			parameters.SetSecurityLevel(securityLevel);
			parameters.SetStandardDeviation(sigma);
			parameters.SetSecretKeyDist(lbcrypto::UNIFORM_TERNARY);
			
			parameters.SetRingDim(ringDimension);
			parameters.SetFirstModSize(firstqmod);
			parameters.SetScalingModSize(dcrtbits);
			parameters.SetDigitSize(digitSize);
			parameters.SetScalingTechnique(lbcrypto::FIXEDMANUAL);
			parameters.SetMultiHopModSize(qmodulus);
			parameters.SetStatisticalSecurity(24);
  			parameters.SetNumAdversarialQueries(1048576); //2^20

			cc = GenCryptoContext(parameters);
			
			// Enable features that you wish to use
			cc->Enable(lbcrypto::PKE);
			cc->Enable(lbcrypto::KEYSWITCH);
			cc->Enable(lbcrypto::LEVELEDSHE);
			cc->Enable(lbcrypto::PRE);
		} else {
            //create internal client instance and get cryptocontext from upstream key server
            KeyServerInternalClient internalClient_pre_server(params);

            std::string client_name = params.process_name;
		    auto reply_cryptocontext = internalClient_pre_server.CryptoContextRequestFromServer_Server();
		    if (!reply_cryptocontext.size())
		        OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized CryptoContext");
	
		    std::istringstream is(reply_cryptocontext);
		    lbcrypto::Serial::Deserialize(cc, is, GlobalSerializationType);
		    if (!cc)
			    OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized CryptoContext is NULL");
	    }
		
		std::cout << "p = "
            << cc->GetCryptoParameters()->GetPlaintextModulus()
            << std::endl;
		
	}

	// There is no shutdown handling in this code.
	void Run(Params& params) {
		ServerBuilder builder;
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
		if(params.disableSSLAuthentication )
			creds = grpc::InsecureServerCredentials();
		else {
			grpc::SslServerCredentialsOptions::PemKeyCertPair keyCert = {
				file2String(params.server_private_key_file),
				file2String(params.server_cert_chain_file) };
			grpc::SslServerCredentialsOptions opts;
			opts.pem_root_certs = file2String(params.client_cert_chain_file);
			opts.pem_key_cert_pairs.push_back(keyCert);
			creds = grpc::SslServerCredentials(opts);
		}
		builder.AddListeningPort(params.key_server_socket_address, creds);

		builder.RegisterService(&service_);
		cq_ = builder.AddCompletionQueue();
		server_ = builder.BuildAndStart();
		// std::cout << "Server listening on " << socket_address << std::endl;

        //check if accessmap path is specified
		if (!params.access_map_path.size())
			OPENFHE_THROW(lbcrypto::openfhe_error, "access map path not specified with -a argument");

		authorizedConsumers = ParseAccessMap(params.access_map_path);

        //server initialize cryptocontext
        initializeCC(params);

		// server's main loop.
		HandleRpcs();
	}

private:
	// This can be run in multiple threads if needed.
	void HandleRpcs() {
		// Spawn different instances derived from CallData to serve clients.

		// send cryptocontext to clients - producer or consumer
		new CryptoContextRequestServicer(&service_, cq_.get(), cc);
		
		// send cryptocontext to broker and generate key pair for each broker and 
		// save them to the maps PrivateKeys and PublicKeys
        new CryptoContextRequest_BrokerServicer(&service_, cq_.get(), cc, PrivateKeys, PublicKeys);

        // send cryptocontext to downstream key server
		new CryptoContextRequest_ServerServicer(&service_, cq_.get(), cc);

		// receive private key from producer and save it in the PrivateKeys map with the producer name
		new PrivateKeyRequestServicer(&service_, cq_.get(), PrivateKeys);

		// receive public key from producer/consumer and save it in the PrivateKeys map with the producer/consumer name/type
		new PublicKeyRequestServicer(&service_, cq_.get(), PublicKeys);
	
	    // send public key of upstream process (broker/producer) to downstream broker
	    new PublicKeyToBrokerRequestServicer(&service_, cq_.get(), localParams, PublicKeys);	
		
		// send reencryption key to broker that is computed using the private key of sender and public key of reciever
		new ReencryptionKeyRequestServicer(&service_, cq_.get(), localParams, cc, PrivateKeys, PublicKeys, authorizedConsumers);

        // send public key of upstream broker of upstream key server to downstream key server
		new PublicKeyToServerRequestServicer(&service_, cq_.get(), localParams, PublicKeys);	

        // receive public key of upstream broker of upstream key server from upstream key server
		new PublicKeyServerRequestServicer(&service_, cq_.get(), RecdPublicKeys);

        // send reencryption key for downstream broker to downstream key server
		new ReencryptionKeyServerRequestServicer(&service_, cq_.get(), cc, PrivateKeys, RecdPublicKeys, authorizedConsumers);

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
			static_cast<CallData<pre_net::PreNetServer::AsyncService>*>(tag)->Proceed();
		}
	}

    Params localParams;
	std::unique_ptr<grpc::ServerCompletionQueue> cq_;
	pre_net::PreNetServer::AsyncService service_;
	std::unique_ptr<Server> server_;

    // cryptocontext initialized at first key server or received from upstream key server
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc;

    //access map for authorized consumers
	std::vector<std::string> authorizedConsumers;

    // Public and private keys of broker and producer with client type and client name pair
	std::unordered_map<pair, lbcrypto::PublicKey<lbcrypto::DCRTPoly>, boost::hash<pair>> PublicKeys;
	std::unordered_map<pair, lbcrypto::PrivateKey<lbcrypto::DCRTPoly>, boost::hash<pair>> PrivateKeys;

    // Public keys of broker/producer received from upstream key server with client type/name pair
	std::unordered_map<pair, lbcrypto::PublicKey<lbcrypto::DCRTPoly>, boost::hash<pair>> RecdPublicKeys;
};

#endif // __PRE_KS__
