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

constexpr unsigned SLEEP_TIME_SECS = 500;

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
	std::shared_ptr<grpc::Channel> channel_server = nullptr;
	std::shared_ptr<grpc::Channel> channel_controller = nullptr;
	if (params.disableSSLAuthentication) {
		channel_server = grpc::CreateCustomChannel(params.socket_address, grpc::InsecureChannelCredentials(), channel_args);
		channel_controller = grpc::CreateCustomChannel(params.controller_socket_address, grpc::InsecureChannelCredentials(), channel_args);
	}
	else {
        grpc::SslCredentialsOptions opts = {
            file2String(params.root_cert_file),
            file2String(params.client_private_key_file),
            file2String(params.client_cert_chain_file) };

        auto channel_creds = grpc::SslCredentials(grpc::SslCredentialsOptions(opts));
        channel_server = grpc::CreateCustomChannel(params.socket_address, channel_creds, channel_args);
		channel_controller = grpc::CreateCustomChannel(params.controller_socket_address, channel_creds, channel_args);
	}
	
	ThreshClient threshClient_server(channel_server);
	ThreshClient threshClient_controller(channel_controller, 1);

    uint32_t client_id = atoi(params.client_id.c_str());
	uint32_t measure_id = atoi(params.measure_id.c_str());
	uint32_t Num_of_parties = atoi(params.Num_of_parties.c_str());
	std::string compute_operation = params.computation;

	// get CryptoContext from the server
	std::string serializedCryptoContext(threshClient_server.CryptoContextFromServer());
	if(!serializedCryptoContext.size())
		OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized CryptoContext");

    if (TEST_MODE)
		write2File(serializedCryptoContext, "./client_ryptocontext_received_serialized.txt");

	//DBC added: this works without this but it is standard practice when deserializing CC
	lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();

	lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc(nullptr);
	std::istringstream is(serializedCryptoContext);
	lbcrypto::Serial::Deserialize(cc, is, GlobalSerializationType);

	
	if(!cc)
		OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized CryptoContext is NULL");

	//DBC added: this works without this but it is standard practice when deserializing CC
	// It is possible that the keys are carried over in the cryptocontext
	// serialization so clearing the keys is important
	/////////////////////////////////////////////////////////////////
	cc->ClearEvalMultKeys();
	cc->ClearEvalAutomorphismKeys();
	
    //send public key to the server

	//possibly wait for clients to start public key gen phase
	Debug_pause(PAUSE_FLAG, "public key wait", params);

	TimeVar t;
	
	TIC(t);
    
    std::cout << "----Interactive Joint public Key Generation-----" << std::endl;
    
	lbcrypto::KeyPair<lbcrypto::DCRTPoly> keyPair (nullptr);
	if (client_id == 1) {
		keyPair = cc->KeyGen();
	} else {
		//get the public key of the previous client
        std::string serializedPrevPubKey;
		std::cout << "\n Wait for previous client's public key" << std::endl;
	    while (!threshClient_server.PublicKeyToClient(serializedPrevPubKey, client_id-1)) {
		    std::cout << "." << std::flush;
		    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_SECS));
	    }
	    if (!serializedPrevPubKey.size())
		    OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized public key");
	
    
	    lbcrypto::PublicKey<lbcrypto::DCRTPoly> prevPubKey(nullptr);
	    std::istringstream isPrevPubKey(serializedPrevPubKey);
	    lbcrypto::Serial::Deserialize(prevPubKey, isPrevPubKey, GlobalSerializationType);

		if (TEST_MODE)
		    write2File(serializedPrevPubKey, "./client_"+ std::to_string(client_id) +"_publickey_received_serialized.txt");

		//RecvdpublicKey
		//compute public key from
		keyPair = cc->MultipartyKeyGen(prevPubKey);
	}


	std::ostringstream ospKey;
	lbcrypto::Serial::Serialize(keyPair.publicKey, ospKey, GlobalSerializationType);
	if (!ospKey.str().size())
		OPENFHE_THROW(lbcrypto::serialize_error, "Serialized public key is empty");
	
	std::string pubKeyAck(threshClient_server.PublicKeyFromClient(ospKey.str(), client_id));
	// print the reply buffer
	if (TEST_MODE) {
		std::cerr << "Server's acknowledgement public key: " << buffer2PrintableString(pubKeyAck) << "]" << std::endl;
        write2File(ospKey.str(), "./client_"+ std::to_string(client_id) +"_publickey_sent_serialized.txt");
	}
    
	//request and receive final public key from the final client
	lbcrypto::PublicKey<lbcrypto::DCRTPoly> FinalPubKey(nullptr);
	std::string serializedFinalPubKey;
	if (client_id != Num_of_parties) {
		std::cout << "\n Wait for final public key" << std::endl;
		while (!threshClient_server.PublicKeyToClient(serializedFinalPubKey, Num_of_parties)) {
			std::cout << "." << std::flush;
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_SECS));
		}
		if (!serializedFinalPubKey.size())
			OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized final public key");

		std::istringstream isFinalPubKey(serializedFinalPubKey);
		lbcrypto::Serial::Deserialize(FinalPubKey, isFinalPubKey, GlobalSerializationType);
	} else {
		FinalPubKey = keyPair.publicKey;
	}

    auto elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
    	std::cout << "Joint Public Key generation time: " << elapsed_seconds << " ms\n";


    //Eval mult key generation multiple rounds
    TIC(t);

	std::string serializedevalMultFinalKey;
	std::ostringstream evmmKey;

	if (client_id == 1) {
		// Generate evalmult key part for A
		Debug_pause(PAUSE_FLAG, "eval key wait", params);

		auto evalMultKey = cc->KeySwitchGen(keyPair.secretKey, keyPair.secretKey);	
	
		//send evalMultkey to server with round number (same as client number)
		std::ostringstream evKey;
		lbcrypto::Serial::Serialize(evalMultKey, evKey, GlobalSerializationType);
		if (!evKey.str().size())
			OPENFHE_THROW(lbcrypto::serialize_error, "Serialized public key is empty");
		std::string evalMultKeyAck(threshClient_server.evalMultKeyFromClient(evKey.str(), client_id));//, round_num));
		// print the reply buffer
		if (TEST_MODE)
			std::cerr << "Server's acknowledgement evalmult key: " << buffer2PrintableString(evalMultKeyAck) << "]" << std::endl;

		if (TEST_MODE)
			write2File(evKey.str(), "./client1_evalmult_sent_serialized.txt");


	} else if ((client_id <= Num_of_parties) && (client_id > 1)) {

		Debug_pause(PAUSE_FLAG, "next eval key wait", params);

		//get previous eval multkey
		std::string serializedPrevevalMultKey;
		std::cout << "\n Wait for previous client's evalmult key" << std::endl;
		while (!threshClient_server.evalMultKeyToClient(serializedPrevevalMultKey, client_id-1)) {//, client_id -1)) {
			std::cout << "." << std::flush;
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_SECS));
		}
		if (!serializedPrevevalMultKey.size())
			OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized previous client evalmult key");
		
		if (TEST_MODE)
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
		std::string evalMultKeyAck(threshClient_server.evalMultKeyFromClient(evKey.str(), client_id));//, round_num));
		// print the reply buffer
		if (TEST_MODE)
			std::cerr << "Server's acknowledgement evalmult key: " << buffer2PrintableString(evalMultKeyAck) << "]" << std::endl;
		
		if (TEST_MODE)
				write2File(evKey.str(), "./client_" + std::to_string(client_id) + "_evalmult_multall_sent_serialized.txt");

		if (client_id == Num_of_parties) {
			auto evalMultskAll = cc->MultiMultEvalKey(keyPair.secretKey, evalMultPrevandCurrent,
											FinalPubKey->GetKeyTag());


			//std::ostringstream evmmKey;
			lbcrypto::Serial::Serialize(evalMultskAll, evmmKey, GlobalSerializationType);
			if (!evmmKey.str().size())
				OPENFHE_THROW(lbcrypto::serialize_error, "Serialized evalmultfinal key is empty");
			
			if (TEST_MODE)
				write2File(evmmKey.str(), "./client_" + std::to_string(client_id) + "_evalmixmult_sent_serialized.txt");

			std::string evalMixMultKeyAck(threshClient_server.evalMixMultKeyFromClient(evmmKey.str(), client_id));//, round_num));
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
		while (!threshClient_server.evalMultKeyToClient(serializedevalMultAll, Num_of_parties)) {//, Num_of_parties)) {
			std::cout << "." << std::flush;
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_SECS));
		}
		if (!serializedevalMultAll.size())
			OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized evalmultall key");
	
		if (TEST_MODE)
				write2File(serializedevalMultAll, "./client_" + std::to_string(client_id) + "_evalmultall_received_serialized.txt");

		lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultAll(nullptr), PrevevalMixMultKey (nullptr);
		std::istringstream isevalMultAll(serializedevalMultAll);
		lbcrypto::Serial::Deserialize(evalMultAll, isevalMultAll, GlobalSerializationType);

		std::cerr << "\n Wait for PrevevalMixMultKey" << std::endl;
		while (!threshClient_server.evalMixMultKeyToClient(serializedPrevevalMixMultKey, client_id + 1)) {//, round_num - 1)) {
			std::cout << "." << std::flush;    
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_SECS));
		}
		if (!serializedPrevevalMixMultKey.size())
			OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized PrevevalMixMultKey key");
	
		if (TEST_MODE)
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

		std::string evalMixMultKeyAck(threshClient_server.evalMixMultKeyFromClient(evmmKey.str(), client_id));//, round_num));

		if (TEST_MODE)
			write2File(evmmKey.str(), "./client_"+ std::to_string(client_id) + "_evalmixmult_sent_serialized.txt");

		if (client_id == 1) {
			cc->InsertEvalMultKey({evalMultAddskAll});
		}
	}


	//get the final evaluation mult key if client id is not 1
	if (client_id != 1) {
		std::cout << "\n Wait for evalmultfinal key" << std::endl;
		while (!threshClient_server.evalMixMultKeyToClient(serializedevalMultFinalKey, 1)) {
			std::cout << "." << std::flush;
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_SECS));
		}
		if (!serializedevalMultFinalKey.size())
			OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized evalmultfinal key");
		
		if (TEST_MODE)
			write2File(serializedevalMultFinalKey, "./client_" + std::to_string(client_id) + "_evalmmultfinal_received_serialized.txt");
		
		lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultFinal(nullptr);
		std::istringstream isevalMultFinal(serializedevalMultFinalKey);
		lbcrypto::Serial::Deserialize(evalMultFinal, isevalMultFinal, GlobalSerializationType);

		cc->InsertEvalMultKey({evalMultFinal});
	}

	elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
    	std::cout << "Eval Mult key generation time: " << elapsed_seconds << " ms\n";
    //####################################################################################

	// generate and send cipher text to the server
	TIC(t);

	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> cipherText(threshClient_server.genCT(cc, FinalPubKey, params));

	elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
	    std::cout << "Ciphertext generation time: " << elapsed_seconds << " ms\n";

	std::ostringstream osCT;
	lbcrypto::Serial::Serialize(cipherText, osCT, GlobalSerializationType);
	if (!osCT.str().size())
		OPENFHE_THROW(lbcrypto::serialize_error, "Serialized cipher text is empty");
	
	std::string controllerct(threshClient_controller.CipherTextFromClientToController(osCT.str(), measure_id));

    //deserialize ciphertext from controller
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> controllerCipherText (nullptr);
	std::istringstream iscontrollerct(controllerct);
	lbcrypto::Serial::Deserialize(controllerCipherText, iscontrollerct, GlobalSerializationType);

	std::string ctAck(threshClient_server.CipherTextFromClient(controllerct, client_id));
	if (TEST_MODE)
		std::cout << "Server's acknowledgement: " << buffer2PrintableString(ctAck) << "]" << std::endl;

	std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> CipherTexts(Num_of_parties);

	//CipherTexts.push_back(cipherText);
	for (uint32_t i = 1; i <= Num_of_parties;i++) {
        if (client_id != i) {
			std::string serializedOtherCipherText;

			std::cout << "\n Wait for CipherText result of client " << i << std::endl;
			while (!threshClient_server.CipherTextToClient(serializedOtherCipherText, i)) {
				std::cout << "." << std::flush;
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_SECS));
			}
			if (!serializedOtherCipherText.size())
				OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized cipher text result");
			lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ClientCT(nullptr);
			std::istringstream isClientCText(serializedOtherCipherText);
			lbcrypto::Serial::Deserialize(ClientCT, isClientCText, GlobalSerializationType);
			if (!ClientCT)
				OPENFHE_THROW(lbcrypto::deserialize_error, "De-serialized client ciphertext is NULL");
			
			//CipherTexts.push_back(ClientCT);
			CipherTexts[i-1] = ClientCT;
		} else {
		    CipherTexts[i-1] = controllerCipherText;
		}
	}

	//compute multiplication of the ciphertexts
	std::cout << "Number of ciphertexts: " << CipherTexts.size() << std::endl;
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> resultCT (nullptr), temp(nullptr);


    resultCT = CipherTexts[0];

    for (uint32_t i = 1; i < Num_of_parties;i++) {
		temp = cc->EvalSub(resultCT, CipherTexts[i]);
		resultCT = temp;
	}    
    
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

	std::ostringstream osPartialCT;
	lbcrypto::Serial::Serialize(ciphertextPartial, osPartialCT, GlobalSerializationType);
	if (!osPartialCT.str().size())
		OPENFHE_THROW(lbcrypto::serialize_error, "Serialized partial cipher text is empty");
	std::string partialctAck(threshClient_server.PartialCipherTextFromClient(osPartialCT.str(), client_id)); 
	bool PartialCTSent(threshClient_server.PartialCipherTextsReceivedWait(client_id));
	
	if (TEST_MODE) {
		write2File(osPartialCT.str(), "./client_" + std::to_string(client_id) + "_patialciphertext_sent_serialized.txt");
		std::cout << "Server's acknowledgement: " << buffer2PrintableString(partialctAck) << "]" << std::endl;
	}

    if (PartialCTSent) {
		for (uint32_t i = 1; i <= Num_of_parties;i++) {
			if (client_id != i) {
				std::string serializedPartialCipherText;
				bool PartialCipherTextRecd(threshClient_server.PartialCipherTextToClient(serializedPartialCipherText, i));

               std::cout << "\n Wait for "<< compute_operation << " partial CipherText result of client " << i << std::endl;
				while (!PartialCipherTextRecd) {
					PartialCipherTextRecd = threshClient_server.PartialCipherTextToClient(serializedPartialCipherText, i);
					std::cout << "." << std::flush;
					std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_SECS));
				}
				if (!serializedPartialCipherText.size())
					OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized mult partial cipher text result");
				
				std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> ciphertextPartial1;//(nullptr);
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
	
	lbcrypto::Plaintext plaintextMultipartyNew;
    cc->MultipartyDecryptFusion(partialCiphertextVec, &plaintextMultipartyNew);

    elapsed_seconds = TOC_MS(t);

    if (TEST_MODE)
        std::cout << "Distributed decryption time: " << elapsed_seconds << " ms\n";

    std::cout << "\n Resulting Fused " << compute_operation << " Plaintext: \n" << std::endl;

	plaintextMultipartyNew->SetLength(12);

    auto plaintextvec = plaintextMultipartyNew->GetPackedValue();
	bool flag = true;
    for (int i = 0; i < 12; i++) {
		if (plaintextvec[i] != 0) {
			std::cerr << "Measurements of the clients are not the same at index " << i << std::endl;
			flag = false;
			//exit(EXIT_FAILURE);
		}
	}

	if (flag == true) {
        std::cout << "Measurements of the clients are the same" << std::endl;
	}

    std::cout << "\n";

	//********************************************************************    

	if (TEST_MODE) {
	    if (client_id != Num_of_parties) {
	        std::cout << "Final Public key received size: " << serializedFinalPubKey.size() << std::endl;
	    } else {
			std::cout << "Public key sent size: " << ospKey.str().size() << std::endl;
		}
        
		if (client_id == 1) {
			std::cout << "EvalMult key size size: " << evmmKey.str().size() << std::endl;	
		} else {
			std::cout << "EvalMult key size size: " << serializedevalMultFinalKey.size() << std::endl;
		}

        std::cout << "Ciphertext sent size: " << osCT.str().size() << std::endl;
        std::cout << "Partial Ciphertext sent size: " << osPartialCT.str().size() << std::endl;
		
	}
	return 0;
}
