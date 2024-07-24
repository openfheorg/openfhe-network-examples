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
#include "utils.h"
#include "thresh_client.h"
#include <string.h>
#include <iostream>
#include <sstream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::milliseconds
#include <cmath>

constexpr unsigned SLEEP_TIME_MSECS = 1;
constexpr unsigned ABORTS_SLEEP_TIME_MSECS = 10;
constexpr unsigned TIME_OUT = 1000;

void Debug_pause(bool pause_flag, std::string msg, Params params){
  if (pause_flag) {
	//wait for carrage return input to continue
	std::cout << "client "<< params.client_id << " " << msg << std::endl;
	std::cin.get();
  }
  return;
}


int main(int argc, char** argv) {
	Params params;
	//char optstring[] = "i:p:l:n:W:h";
	if (!processInputParams(argc, argv, params)) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	// from https://github.com/grpc/grpc/blob/master/include/grpc/impl/codegen/grpc_types.h:
	// GRPC doesn't set size limit for sent messages by default. However, the max size of received messages
	// is limited by "#define GRPC_DEFAULT_MAX_RECV_MESSAGE_LENGTH (4 * 1024 * 1024)", which is only 4194304.
	// for server and client the size must be set individually:
	//		for a client it is (-1): SetMaxReceiveMessageSize(-1)
	//		for a server INT_MAX (from #include <climits>) should do it: SetMaxReceiveMessageSize(INT_MAX)
	// see https://nanxiao.me/en/message-length-setting-in-grpc/ for more information on the message size
	grpc::ChannelArguments channel_args;
	channel_args.SetMaxReceiveMessageSize(-1);

	// Instantiate the client. It requires a channel, out of which the actual RPCs
	// are created. This channel models a connection to an endpoint (in this case,
	// localhost:50051) using client-side SSL/TLS
	std::shared_ptr<grpc::Channel> channel = nullptr;
	if (params.disableSSLAuthentication) {
		channel = grpc::CreateCustomChannel(params.socket_address, grpc::InsecureChannelCredentials(), channel_args);
	}
	else {
        grpc::SslCredentialsOptions opts = {
            file2String(params.root_cert_file),
            file2String(params.client_private_key_file),
            file2String(params.client_cert_chain_file) };

        auto channel_creds = grpc::SslCredentials(grpc::SslCredentialsOptions(opts));
        channel = grpc::CreateCustomChannel(params.socket_address, channel_creds, channel_args);
	}
	ThreshClient threshClient(channel);

    uint32_t recovering_client_id = 1;
    uint32_t client_id = atoi(params.client_id.c_str());
	uint32_t aborts = atoi(params.aborts.c_str());
	uint32_t Num_of_parties = atoi(params.Num_of_parties.c_str());
	std::string compute_operation = params.computation;

	TimeVar t;

	TIC(t);
	// get CryptoContext from the server
	std::string serializedCryptoContext(threshClient.CryptoContextFromServer());
	if(!serializedCryptoContext.size())
		OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized CryptoContext");

	//DBC added: this works without this but it is standard practice when deserializing CC
	// It is possible that the keys are carried over in the cryptocontext
	// serialization so clearing the keys is important
	lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();

	lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc(nullptr);
	std::istringstream is(serializedCryptoContext);
	lbcrypto::Serial::Deserialize(cc, is, GlobalSerializationType);

	
	if(!cc)
		OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized CryptoContext is NULL");

	auto elapsed_seconds = TOC_MS(t);
	if (TEST_MODE)
    	std::cout << "Receive cryptocontext from server: " << elapsed_seconds << " ms\n";
	
	/////////////////////////////////////////////////////////////////
	cc->ClearEvalMultKeys();
	cc->ClearEvalAutomorphismKeys();

    //send public key to the server

	//possibly wait for clients to start public key gen phase
	Debug_pause(PAUSE_FLAG, "public key wait", params);

    std::cout << "----Interactive Joint public Key Generation-----" << std::endl;
    
	TIC(t);
	lbcrypto::KeyPair<lbcrypto::DCRTPoly> keyPair (nullptr);
	if (client_id == 1) {
		keyPair = cc->KeyGen();
	} else {
		//get the public key of the previous client
        std::string serializedPrevPubKey;
		std::cout << "\n Wait for previous client's public key" << std::endl;
	    while (!threshClient.PublicKeyToClient(serializedPrevPubKey, client_id-1)) {
		    std::cout << "." << std::flush;
		    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
	    }
	    if (!serializedPrevPubKey.size())
		    OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized public key");
	
    
	    lbcrypto::PublicKey<lbcrypto::DCRTPoly> prevPubKey(nullptr);
	    std::istringstream isPrevPubKey(serializedPrevPubKey);
	    lbcrypto::Serial::Deserialize(prevPubKey, isPrevPubKey, GlobalSerializationType);

		if (debug_flag_val)
		    write2File(serializedPrevPubKey, "./client_"+ std::to_string(client_id) +"_publickey_received_serialized.txt");

		//RecvdpublicKey
		//compute public key from
		keyPair = cc->MultipartyKeyGen(prevPubKey);
	}


	std::ostringstream ospKey;
	lbcrypto::Serial::Serialize(keyPair.publicKey, ospKey, GlobalSerializationType);
	if (!ospKey.str().size())
		OPENFHE_THROW(lbcrypto::serialize_error, "Serialized public key is empty");
	
	std::string pubKeyAck(threshClient.PublicKeyFromClient(ospKey.str(), client_id));
	// print the reply buffer
	if (debug_flag_val) {
		std::cerr << "Server's acknowledgement public key: " << buffer2PrintableString(pubKeyAck) << "]" << std::endl;
        write2File(ospKey.str(), "./client_"+ std::to_string(client_id) +"_publickey_sent_serialized.txt");
	}
    
	//request and receive final public key from the final client
	lbcrypto::PublicKey<lbcrypto::DCRTPoly> FinalPubKey(nullptr);
	std::string serializedFinalPubKey;
	if (client_id != Num_of_parties) {
		std::cout << "\n Wait for final public key" << std::endl;
		while (!threshClient.PublicKeyToClient(serializedFinalPubKey, Num_of_parties)) {
			std::cout << "." << std::flush;
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
		}
		if (!serializedFinalPubKey.size())
			OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized final public key");

		std::istringstream isFinalPubKey(serializedFinalPubKey);
		lbcrypto::Serial::Deserialize(FinalPubKey, isFinalPubKey, GlobalSerializationType);
	} else {
		FinalPubKey = keyPair.publicKey;
	}

    elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
    	std::cout << "Joint Public Key generation time: " << elapsed_seconds << " ms\n";

    //Secret sharing secret key for aborts protocol
	TIC(t);

    usint thresh = floor(Num_of_parties/2) + 1;
    //share the secret key to other parties
	auto secretshares = cc->ShareKeys(keyPair.secretKey, Num_of_parties, thresh, client_id, "shamir");

	elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
	    std::cout << "Generate secret shares for Aborts protocol time: " << elapsed_seconds << " ms\n";

	TIC(t);
	std::ostringstream keyshares;
	lbcrypto::Serial::Serialize(secretshares, keyshares, GlobalSerializationType);
	if (!keyshares.str().size())
		OPENFHE_THROW(lbcrypto::serialize_error, "Serialized secret shares vector is empty");
	
	elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
	    std::cout << "Serialize secret shares for Aborts protocol time: " << elapsed_seconds << " ms\n";
	
	TIC(t);
	std::string secretSharesAck(threshClient.SecretSharesFromClient(keyshares.str(), client_id));

    elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
	    std::cout << "Send secret shares for Aborts protocol time: " << elapsed_seconds << " ms\n";

    //Eval mult key generation multiple rounds
    TIC(t);

	std::string serializedevalMultFinalKey;
	std::ostringstream evmmKey;
	if (compute_operation == "multiply") {
		if (client_id == 1) {
			// Generate evalmult key part for A
			Debug_pause(PAUSE_FLAG, "eval key wait", params);

			auto evalMultKey = cc->KeySwitchGen(keyPair.secretKey, keyPair.secretKey);	
		
			//send evalMultkey to server with round number (same as client number)
			std::ostringstream evKey;
			lbcrypto::Serial::Serialize(evalMultKey, evKey, GlobalSerializationType);
			if (!evKey.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "Serialized public key is empty");
			std::string evalMultKeyAck(threshClient.evalMultKeyFromClient(evKey.str(), client_id));
			// print the reply buffer
			if (TEST_MODE)
				std::cerr << "Server's acknowledgement evalmult key: " << buffer2PrintableString(evalMultKeyAck) << "]" << std::endl;

			if (debug_flag_val)
				write2File(evKey.str(), "./client1_evalmult_sent_serialized.txt");
	

		} else if ((client_id <= Num_of_parties) && (client_id > 1)) {

			Debug_pause(PAUSE_FLAG, "next eval key wait", params);

			//get previous eval multkey
			std::string serializedPrevevalMultKey;
			std::cout << "\n Wait for previous client's evalmult key" << std::endl;
			while (!threshClient.evalMultKeyToClient(serializedPrevevalMultKey, client_id-1)) {
				std::cout << "." << std::flush;
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
			}
			if (!serializedPrevevalMultKey.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized previous client evalmult key");
			
			if (debug_flag_val)
				write2File(serializedPrevevalMultKey, "./client_"+ std::to_string(client_id) + "_evalmult_received_serialized.txt");
			
			lbcrypto::EvalKey<lbcrypto::DCRTPoly> PrevevalMultKey(nullptr);
			std::istringstream isPrevevalMultKey(serializedPrevevalMultKey);
			lbcrypto::Serial::Deserialize(PrevevalMultKey, isPrevevalMultKey, GlobalSerializationType);


			//compute evalmultkey for this client
			auto evalMultKey = cc->MultiKeySwitchGen(keyPair.secretKey, keyPair.secretKey, PrevevalMultKey);

			//compute evalmultAB/evalmultABC (adding up with prev evalmultkey)
			auto evalMultPrevandCurrent = cc->MultiAddEvalKeys(PrevevalMultKey, evalMultKey, keyPair.publicKey->GetKeyTag());

			std::ostringstream evKey;
			lbcrypto::Serial::Serialize(evalMultPrevandCurrent, evKey, GlobalSerializationType);
			if (!evKey.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "Serialized eval key is empty");
			std::string evalMultKeyAck(threshClient.evalMultKeyFromClient(evKey.str(), client_id));
			// print the reply buffer
			if (TEST_MODE)
				std::cerr << "Server's acknowledgement evalmult key: " << buffer2PrintableString(evalMultKeyAck) << "]" << std::endl;
			
			if (debug_flag_val)
					write2File(evKey.str(), "./client_" + std::to_string(client_id) + "_evalmult_multall_sent_serialized.txt");

			if (client_id == Num_of_parties) {
				auto evalMultskAll = cc->MultiMultEvalKey(keyPair.secretKey, evalMultPrevandCurrent,
												FinalPubKey->GetKeyTag());


				//std::ostringstream evmmKey;
				lbcrypto::Serial::Serialize(evalMultskAll, evmmKey, GlobalSerializationType);
				if (!evmmKey.str().size())
					OPENFHE_THROW(lbcrypto::serialize_error, "Serialized evalmultfinal key is empty");
				
				if (debug_flag_val)
					write2File(evmmKey.str(), "./client_" + std::to_string(client_id) + "_evalmixmult_sent_serialized.txt");

				std::string evalMixMultKeyAck(threshClient.evalMixMultKeyFromClient(evmmKey.str(), client_id));
			}

		} else {
			std::cerr << "client id out of range" <<std::endl;
		}
		
		if (client_id != Num_of_parties) {

			//request and receive the evalmultAll key and evalmult computed from all parties
			//round_num -1
			Debug_pause(PAUSE_FLAG, "eval key round 2 wait", params);

			std::string serializedevalMultAll, serializedPrevevalMixMultKey;
			std::cout << "\n Wait for evalmultall key" << std::endl;
			while (!threshClient.evalMultKeyToClient(serializedevalMultAll, Num_of_parties)) {
				std::cout << "." << std::flush;
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
			}
			if (!serializedevalMultAll.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized evalmultall key");
		
			if (debug_flag_val)
					write2File(serializedevalMultAll, "./client_" + std::to_string(client_id) + "_evalmultall_received_serialized.txt");

			lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultAll(nullptr), PrevevalMixMultKey (nullptr);
			std::istringstream isevalMultAll(serializedevalMultAll);
			lbcrypto::Serial::Deserialize(evalMultAll, isevalMultAll, GlobalSerializationType);

			std::cerr << "\n Wait for PrevevalMixMultKey" << std::endl;
			while (!threshClient.evalMixMultKeyToClient(serializedPrevevalMixMultKey, client_id + 1)) {
				std::cout << "." << std::flush;    
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
			}
			if (!serializedPrevevalMixMultKey.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized PrevevalMixMultKey key");
		
			if (debug_flag_val)
					write2File(serializedPrevevalMixMultKey, "./client_" + std::to_string(client_id) + "_evalmixmul_received_serialized.txt");

			std::istringstream isPrevevalMixMultKey(serializedPrevevalMixMultKey);
			lbcrypto::Serial::Deserialize(PrevevalMixMultKey, isPrevevalMixMultKey, GlobalSerializationType);

			//Add evalmultkeys to compute final evalmultkey
			auto evalMultKeyskAll = cc->MultiMultEvalKey(keyPair.secretKey, evalMultAll,
											FinalPubKey->GetKeyTag());
			
			auto evalMultAddskAll = cc->MultiAddEvalMultKeys(evalMultKeyskAll, PrevevalMixMultKey,
													FinalPubKey->GetKeyTag());


			lbcrypto::Serial::Serialize(evalMultAddskAll, evmmKey, GlobalSerializationType);
			if (!evmmKey.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "Serialized evalmultfinal key is empty");

			std::string evalMixMultKeyAck(threshClient.evalMixMultKeyFromClient(evmmKey.str(), client_id));//, round_num));

			if (debug_flag_val)
				write2File(evmmKey.str(), "./client_"+ std::to_string(client_id) + "_evalmixmult_sent_serialized.txt");

			if (client_id == 1) {
				cc->InsertEvalMultKey({evalMultAddskAll});
			}
		}


		//get the final evaluation mult key if client id is not 1
		if (client_id != 1) {
			std::cout << "\n Wait for evalmultfinal key" << std::endl;
			while (!threshClient.evalMixMultKeyToClient(serializedevalMultFinalKey, 1)) {
				std::cout << "." << std::flush;
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
			}
			if (!serializedevalMultFinalKey.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized evalmultfinal key");
			
			if (debug_flag_val)
				write2File(serializedevalMultFinalKey, "./client_" + std::to_string(client_id) + "_evalmmultfinal_received_serialized.txt");
			
			lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultFinal(nullptr);
			std::istringstream isevalMultFinal(serializedevalMultFinalKey);
			lbcrypto::Serial::Deserialize(evalMultFinal, isevalMultFinal, GlobalSerializationType);

			cc->InsertEvalMultKey({evalMultFinal});
		}
    }
	elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
    	std::cout << "Eval Mult key generation time: " << elapsed_seconds << " ms\n";
    //####################################################################################
	//Eval sum key generation multiple rounds
    TIC(t);
	std::ostringstream esKey0, esKey1;
	std::string serializedevalSumFinalKey;
	if (compute_operation == "vectorsum") {
		//generate evalSumKey
		if (client_id == 1) {
			// Generate evalmult key part for A
			Debug_pause(PAUSE_FLAG, "eval sum key wait", params);

			cc->EvalSumKeyGen(keyPair.secretKey);
			auto evalSumKeys = std::make_shared<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>>(cc->GetEvalSumKeyMap(keyPair.secretKey->GetKeyTag()));	
		
			//send evalSumkeys to server with round number (same as client number)
			std::ostringstream esKey;
			lbcrypto::Serial::Serialize(evalSumKeys, esKey, GlobalSerializationType);
			if (!esKey.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "Serialized eval sum key is empty");
			std::string evalSumKeysAck0(threshClient.evalSumKeyFromClient(esKey.str(), client_id, 0));
			std::string evalSumKeysAck1(threshClient.evalSumKeyFromClient(esKey.str(), client_id, 1));
			// print the reply buffer
			if (TEST_MODE) {
				std::cerr << "Server's acknowledgement evalSumKeys key: " << buffer2PrintableString(evalSumKeysAck0) << "]" << std::endl;
				std::cerr << "Server's acknowledgement evalSumKeys key: " << buffer2PrintableString(evalSumKeysAck1) << "]" << std::endl;
			}

			if (debug_flag_val)
				write2File(esKey.str(), "./client1_evalsumkeys_sent_serialized.txt");
			

		} else if ((client_id <= Num_of_parties) && (client_id > 1)) {
			Debug_pause(PAUSE_FLAG, "next eval key wait", params);

			//get previous eval multkey
			std::string serializedPrevevalSumKey0, serializedPrevevalSumKey1;
			std::cout << "\n Wait for previous client evalsum0 key" << std::endl;
			while (!threshClient.evalSumKeyToClient(serializedPrevevalSumKey0, client_id-1, 0)) {
				std::cout << "." << std::flush;
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
			}
			if (!serializedPrevevalSumKey0.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized previous client evalsum0 key");
			
			if (debug_flag_val)
				write2File(serializedPrevevalSumKey0, "./client_"+ std::to_string(client_id) + "_evalsum0_received_serialized.txt");
			
			std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> PrevevalSumKey0;
			std::istringstream isPrevevalSumKey0(serializedPrevevalSumKey0);
			lbcrypto::Serial::Deserialize(PrevevalSumKey0, isPrevevalSumKey0, GlobalSerializationType);
			
			std::cout << "\n Wait for previous client evalsum1 key" << std::endl;
			while (!threshClient.evalSumKeyToClient(serializedPrevevalSumKey1, client_id-1, 1)) {
				std::cout << "." << std::flush;
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
			}
			if (!serializedPrevevalSumKey1.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized previous client evalsum1 key");
			
			if (debug_flag_val)
				write2File(serializedPrevevalSumKey1, "./client_"+ std::to_string(client_id) + "_evalsum1_received_serialized.txt");
			
			std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> PrevevalSumKey1;
			std::istringstream isPrevevalSumKey1(serializedPrevevalSumKey1);
			lbcrypto::Serial::Deserialize(PrevevalSumKey1, isPrevevalSumKey1, GlobalSerializationType);


			//compute evalmultkey for this client
			auto evalSumKeys0 = cc->MultiEvalSumKeyGen(keyPair.secretKey, PrevevalSumKey0, keyPair.publicKey->GetKeyTag());

			//compute evalSumKeys1 (adding up with prev evalsumkey)
			auto evalSumKeys1 = cc->MultiAddEvalSumKeys(PrevevalSumKey1, evalSumKeys0, keyPair.publicKey->GetKeyTag());

			lbcrypto::Serial::Serialize(evalSumKeys0, esKey0, GlobalSerializationType);
			lbcrypto::Serial::Serialize(evalSumKeys1, esKey1, GlobalSerializationType);
			if (!esKey0.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "Serialized evalsumkey0 is empty");
			if (!esKey1.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "Serialized evalsumkey1 is empty");

			std::string evalSumKeyAck0(threshClient.evalSumKeyFromClient(esKey0.str(), client_id, 0));
			std::string evalSumKeyAck1(threshClient.evalSumKeyFromClient(esKey1.str(), client_id, 1));
			// print the reply buffer
			if (debug_flag_val) {
				std::cerr << "Server's acknowledgement evalsumkey0: " << buffer2PrintableString(evalSumKeyAck0) << "]" << std::endl;
				std::cerr << "Server's acknowledgement evalsumkey1: " << buffer2PrintableString(evalSumKeyAck1) << "]" << std::endl;
				write2File(esKey0.str(), "./client_" + std::to_string(client_id) + "_evalsumkey0_sent_serialized.txt");
				write2File(esKey1.str(), "./client_" + std::to_string(client_id) + "_evalsumkey1_sent_serialized.txt");
			}
				

			if (client_id == Num_of_parties) {
				cc->InsertEvalSumKey(evalSumKeys1);
			}

		} else {
			std::cerr << "yet to decide if this needs to be an error" <<std::endl;
		}

		//get the final evalsums key from the last client
		if (client_id != Num_of_parties) {
			std::cout << "\n Wait for seconds as evalsumfinal key" << std::endl;
			while (!threshClient.evalSumKeyToClient(serializedevalSumFinalKey, Num_of_parties, 1)) {
				std::cout << "." << std::flush;
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
			}
			if (!serializedevalSumFinalKey.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized evalsumfinal key");
			
			if (debug_flag_val)
				write2File(serializedevalSumFinalKey, "./client_" + std::to_string(client_id) + "_evalsumfinal_received_serialized.txt");
			
			std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> evalSumFinal;
			std::istringstream isevalSumFinal(serializedevalSumFinalKey);
			lbcrypto::Serial::Deserialize(evalSumFinal, isevalSumFinal, GlobalSerializationType);

			cc->InsertEvalSumKey({evalSumFinal});
		}
	}
	elapsed_seconds = TOC_MS(t);

        if (TEST_MODE)
            std::cout << "Eval Sum key generation time: " << elapsed_seconds << " ms\n";
    //####################################################################################

	// generate and send cipher text to the server
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> cipherText(threshClient.genCT(cc, FinalPubKey, params));

	std::ostringstream osCT;
	lbcrypto::Serial::Serialize(cipherText, osCT, GlobalSerializationType);
	if (!osCT.str().size())
		OPENFHE_THROW(lbcrypto::serialize_error, "Serialized cipher text is empty");
	
	std::string ctAck(threshClient.CipherTextFromClient(osCT.str(), client_id)); //todo: include producer_id in parameters to create a vector of ciphertexts being reencrypted.
	if (TEST_MODE)
		std::cout << "Server's acknowledgement: " << buffer2PrintableString(ctAck) << "]" << std::endl;

	std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> CipherTexts(Num_of_parties);

    
	for (uint32_t i = 1; i <= Num_of_parties;i++) {
        if (client_id != i) {
			std::string serializedOtherCipherText;

			std::cout << "\n Wait for CipherText result of client " << i << std::endl;
			while (!threshClient.CipherTextToClient(serializedOtherCipherText, i)) {
				std::cout << "." << std::flush;
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MSECS));
			}
			if (!serializedOtherCipherText.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized cipher text result");
			lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ClientCT(nullptr);
			std::istringstream isClientCText(serializedOtherCipherText);
			lbcrypto::Serial::Deserialize(ClientCT, isClientCText, GlobalSerializationType);
			if (!ClientCT)
				OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized client ciphertext is NULL");
			
			CipherTexts[i-1] = ClientCT;
		} else {
		    CipherTexts[i-1] = cipherText;
		}
	}

	//compute multiplication of the ciphertexts
	std::cout << "Number of ciphertexts: " << CipherTexts.size() << std::endl;

	if (aborts == 1) {
	  std::cout << "This client is Aborting the protocol and exiting." <<std::endl;
	  exit(EXIT_FAILURE);
	}

	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> resultCT (nullptr), temp(nullptr);
    
	TIC(t);
	if (compute_operation == "multiply") {
		resultCT = cc->EvalMultMany(CipherTexts);
	}
	else if (compute_operation == "add") {
        resultCT = cc->EvalAddMany(CipherTexts);
	}
    else if (compute_operation == "vectorsum") {
        //compute evalsum
	    usint batchSize = 16;
        resultCT = cc->EvalSum(CipherTexts[0], batchSize);
	} else {
		std::cerr << "operation not valid" << std::endl;
	}
    
    
	elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
	    std::cout << "Compute time: " << elapsed_seconds << " ms\n";

    //vector for all partially decrypted mult ciphertexts
	TIC(t);

	std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> partialCiphertextVec(Num_of_parties);

	//compute partially decrypted ciphertext
	//client id
	std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> ciphertextPartial;
	if (client_id == 1) {
        ciphertextPartial = cc->MultipartyDecryptLead({resultCT}, keyPair.secretKey);

	} else {
        ciphertextPartial = cc->MultipartyDecryptMain({resultCT}, keyPair.secretKey);
	}

	elapsed_seconds = TOC_MS(t);

    if (TEST_MODE) {
        std::cout << "Distributed decryption partial ct time: " << elapsed_seconds << " ms\n";
	}

	TIC(t);
	std::ostringstream osPartialCT;
	lbcrypto::Serial::Serialize(ciphertextPartial, osPartialCT, GlobalSerializationType);
	if (!osPartialCT.str().size())
		OPENFHE_THROW(lbcrypto::serialize_error, "Serialized partial cipher text is empty");
	std::string partialctAck(threshClient.PartialCipherTextFromClient(osPartialCT.str(), client_id)); 

	elapsed_seconds = TOC_MS(t);

    if (TEST_MODE) {
        std::cout << "Distributed decryption send partial ct to server time: " << elapsed_seconds << " ms\n";
	}
		
	TIC(t);
	bool PartialCTSent(threshClient.PartialCipherTextsReceivedWait(client_id));
	
	if (debug_flag_val) {
		write2File(osPartialCT.str(), "./client_" + std::to_string(client_id) + "_patialciphertext_sent_serialized.txt");
		std::cout << "Server's acknowledgement: " << buffer2PrintableString(partialctAck) << "]" << std::endl;
	}

	elapsed_seconds = TOC_MS(t);

    if (TEST_MODE) {
        std::cout << "Distributed decryption verify partial ct sent to server time: " << elapsed_seconds << " ms\n";
	}

	TIC(t);
    if (PartialCTSent) {
		for (uint32_t i = 1; i <= Num_of_parties;i++) {
			if (client_id != i) {
				std::string serializedPartialCipherText;
				usint ctr = 0;
				bool PartialCipherTextRecd(threshClient.PartialCipherTextToClient(serializedPartialCipherText, i));

               std::cout << "\n Wait for "<< compute_operation << " partial CipherText result of client " << i << std::endl;
				while (!PartialCipherTextRecd) {
					PartialCipherTextRecd = threshClient.PartialCipherTextToClient(serializedPartialCipherText, i);
					std::cout << "." << std::flush;
					std::this_thread::sleep_for(std::chrono::milliseconds(ABORTS_SLEEP_TIME_MSECS));
					bool PartialCTRecd(threshClient.PartialCipherTextsReceivedWait(i));

					ctr += 1;
					if ((ctr*ABORTS_SLEEP_TIME_MSECS > TIME_OUT) && (!PartialCTRecd)) {
						std::cout << "Party " << i << " unavailable for sending partial decryption share, initiating aborts protocol" << std::endl;
						std::cout << "recovering client id " << recovering_client_id << std::endl;

						if (client_id == recovering_client_id) {
						//receive the threshold t number of secret shares of party i
						std::string serializedSecretShares;
						threshClient.SecretSharesToClient(serializedSecretShares, recovering_client_id, i, thresh, Num_of_parties);
						
						std::unordered_map<uint32_t, lbcrypto::DCRTPoly> SecretSharesRecd;
						std::istringstream isSecretSharesRecd(serializedSecretShares);
						lbcrypto::Serial::Deserialize(SecretSharesRecd, isSecretSharesRecd, GlobalSerializationType);

						//recover the party i's secret key 
						lbcrypto::PrivateKey<lbcrypto::DCRTPoly> recovered_sk = std::make_shared<lbcrypto::PrivateKeyImpl<lbcrypto::DCRTPoly>>(cc);
						cc->RecoverSharedKey(recovered_sk, SecretSharesRecd, Num_of_parties, thresh, "shamir");//"additive");
						
						//compute partial decryption share with recovered secret key
						std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> AbortsciphertextPartial;
						AbortsciphertextPartial = cc->MultipartyDecryptMain({resultCT}, recovered_sk);//multCT, addCT, sumCT});

						//send AbortsciphertextPartial to other clients via the server
						std::ostringstream osAbortsPartialCT;
						lbcrypto::Serial::Serialize(AbortsciphertextPartial, osAbortsPartialCT, GlobalSerializationType);
						serializedPartialCipherText = osAbortsPartialCT.str();
						
						if (!serializedPartialCipherText.size())
							OPENFHE_THROW(lbcrypto::serialize_error, "Serialized aborts partial cipher text is empty");
						
						std::string abortspartialctAck(threshClient.AbortsPartialCipherTextFromClient(serializedPartialCipherText, recovering_client_id, i));
						PartialCipherTextRecd = true;
						if (TEST_MODE)
							std::cout << "Server's acknowledgement: " << buffer2PrintableString(abortspartialctAck) << "]" << std::endl;

						} else {

						std::cout << "\n Wait for "<< compute_operation <<" aborts partial CipherText result of client " << i << " from client " << recovering_client_id << std::endl;	
						while (!threshClient.AbortsPartialCipherTextToClients(serializedPartialCipherText, recovering_client_id, i)) {
							std::cout << "." << std::flush;
							std::this_thread::sleep_for(std::chrono::milliseconds(ABORTS_SLEEP_TIME_MSECS));
						}
						PartialCipherTextRecd = true;
						}
					}

				}
				if (!serializedPartialCipherText.size())
					OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized mult partial cipher text result");
				
				std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> ciphertextPartial1;
				std::istringstream ispartialCText(serializedPartialCipherText);
				lbcrypto::Serial::Deserialize(ciphertextPartial1, ispartialCText, GlobalSerializationType);
				if (!ciphertextPartial1[0])
					OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized mult partial ciphertext is NULL");

				partialCiphertextVec[i-1] = ciphertextPartial1[0];
			}
			else {
				partialCiphertextVec[i-1] = ciphertextPartial[0];
			}
		}
	}
	
    elapsed_seconds = TOC_MS(t);

    if (TEST_MODE) {
        std::cout << "Distributed decryption receive and abort partial ct time: " << elapsed_seconds << " ms\n";
	}
		
	TIC(t);
	lbcrypto::Plaintext plaintextMultipartyNew;
    cc->MultipartyDecryptFusion(partialCiphertextVec, &plaintextMultipartyNew);

    elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
        std::cout << "Distributed decryption time: " << elapsed_seconds << " ms\n";

    std::cout << "\n Resulting Fused " << compute_operation << " Plaintext: \n" << std::endl;

	plaintextMultipartyNew->SetLength(24);
    std::cout << plaintextMultipartyNew << std::endl;
    std::cout << "\n";

	//********************************************************************    

	if (TEST_MODE) {
	    if (client_id != Num_of_parties) {
	        std::cout << "Final Public key received size: " << serializedFinalPubKey.size() << std::endl;
	    } else {
			std::cout << "Public key sent size: " << ospKey.str().size() << std::endl;
		}
	    std::cout << "Aborts secret shares sent size: " << keyshares.str().size() << std::endl;
        
		if (compute_operation == "multiply"){
			if (client_id == 1) {
			    std::cout << "EvalMult key size size: " << evmmKey.str().size() << std::endl;	
			} else {
				std::cout << "EvalMult key size size: " << serializedevalMultFinalKey.size() << std::endl;
			}
		}

		if (compute_operation == "vectorsum"){
			if (client_id == Num_of_parties) {
			    std::cout << "EvalSum key size size: " << esKey1.str().size() << std::endl;	
			} else {
				std::cout << "EvalSum key size size: " << serializedevalSumFinalKey.size() << std::endl;
			}
		}

        std::cout << "Ciphertext sent size: " << osCT.str().size() << std::endl;
        std::cout << "Partial Ciphertext sent size: " << osPartialCT.str().size() << std::endl;
		
	}
	return 0;
}
