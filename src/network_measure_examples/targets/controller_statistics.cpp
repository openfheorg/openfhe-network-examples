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
//#include "../src/peer_to_peer_framework/include/node.h"
#include "node.h"
#include "register_functions.h"


int main(int argc, char** argv) {

    //OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements
	
	CommParams params;
	//char optstring[] = "i:p:l:n:W:h";
	if (!processInputParams(argc, argv, params)) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	std::string node_name = params.NodeName;
	NodeImpl mynode;
   
	message_format Msgsent;
    Msgsent.NodeName = params.NodeName;

	mynode.Register(params); //initiate and add nodes
    mynode.Start();
	

	std::cout << "My name " << node_name << std::endl;

    
    //cryptocontext generation.
    //For now, the cryptocontext is generated by node 1 and sent to node 2. This will be changed later to be
    //generated by a key server when a client-server communication is added to the peer to peer framework.
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc(nullptr);

	int plaintextModulus = 65537;
	double sigma = 3.2;
	lbcrypto::SecurityLevel securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;

    usint multDepth = 1;
	usint scaleFactorBits = 50;

	lbcrypto::CCParams<lbcrypto::CryptoContextCKKSRNS> parameters;		

	parameters.SetPlaintextModulus(plaintextModulus);
	parameters.SetStandardDeviation(sigma);
	parameters.SetSecurityLevel(securityLevel);
	parameters.SetSecretKeyDist(lbcrypto::UNIFORM_TERNARY);
	parameters.SetMultiplicativeDepth(multDepth);
	parameters.SetScalingModSize(scaleFactorBits);

	cc = GenCryptoContext(parameters);

	cc->Enable(lbcrypto::PKE);
	cc->Enable(lbcrypto::KEYSWITCH);
	cc->Enable(lbcrypto::LEVELEDSHE);
	cc->Enable(lbcrypto::ADVANCEDSHE);
	cc->Enable(lbcrypto::MULTIPARTY);

	std::vector<std::string> node_names = mynode.getConnectedNodes();

    for(usint i = 0; i< node_names.size(); i++) {
        mynode.sendSerialMsg(cc, Msgsent, "cryptocontext", node_names[i]);
	}
	
    std::cout << "finished sending cryptocontext" << std::endl;


    usint num_of_nodes = 3;
	std::string startNode = "Node1"; 
    std::string endNode = "Node3";//todo: how to pass number of nodes


    lbcrypto::PublicKey<lbcrypto::DCRTPoly> FinalPubKey(nullptr);
	mynode.getSerialMsgWait(FinalPubKey, "PublicKey", endNode);
   
    unsigned int ringsize = cc->GetCryptoParameters()->GetElementParams()->GetCyclotomicOrder()/2;
    initialize_map(ringsize);

    //initialize zero vector and initial zero ciphertext to send to startNode
	usint vecsize = ringsize/4;
    std::vector<std::complex<double>> zeroVector(vecsize);
	for(usint i = 0; i < vecsize; i++) {
		zeroVector.push_back(0.0);
	}

	auto zeroPT = cc->MakeCKKSPackedPlaintext(zeroVector);
	auto zeroCT = cc->Encrypt(FinalPubKey, zeroPT);

	mynode.sendSerialMsg(zeroCT, Msgsent, "zero ciphertext", startNode);

	//get the final evaluation mult key if client id is not 1
	lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultFinal(nullptr);
	mynode.getSerialMsgWait(evalMultFinal, "evalmixmultkey", startNode);
	cc->InsertEvalMultKey({evalMultFinal});

    std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> evalSumFinal;
    mynode.getSerialMsgWait(evalSumFinal, "evalsumkey1", endNode);
	cc->InsertEvalSumKey({evalSumFinal});

    //get partial ct from other node
	std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> partialCiphertextVec(num_of_nodes); //todo: get number of parties

	for (usint i = 0; i < node_names.size(); i++) {
	    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> nodePartialCT(nullptr);
	    mynode.getSerialMsgWait(nodePartialCT, "partialct", node_names[i]);
		partialCiphertextVec[i] = nodePartialCT;
	}
	
	lbcrypto::Plaintext plaintextMultipartyNew;
    cc->MultipartyDecryptFusion(partialCiphertextVec, &plaintextMultipartyNew);

    plaintextMultipartyNew->SetLength(16);

    auto ptvec = plaintextMultipartyNew->GetCKKSPackedValue();

	//#########################register###########
	// now on the controller you need to eventually unpack that data to process it
	
	std::map<std::string,std::vector<double>> unpacked_vectors;

	unpacked_vectors = unpackPT(ptvec); //pt is the plaintext vector that was decrypted and unpacked

	auto x1 = unpacked_vectors.find("v1");
	auto x2 = unpacked_vectors.find("v2");
	auto x3 = unpacked_vectors.find("v3");
	auto n = unpacked_vectors.find("n");

	auto num_of_elements = n->second[0];

	double sum_x1 = 0.0;
	for(auto& v : x1->second){
		sum_x1 += v;
	}
	auto ave_x1 = sum_x1 / num_of_elements;

	double sum_x2 = 0.0;
	for(auto& v : x2->second){
		sum_x2 += v;
	}
	auto ave_x2 = sum_x2 / num_of_elements;

	double sum_x3 = 0.0;
	for(auto& v : x3->second){
		sum_x3 += v;
	}
	auto ave_x3 = sum_x3 / num_of_elements;

	std::cout <<" Mean: "<< ave_x1
		<<" Mean of squares: "<< ave_x2
		<<" Mean of cubes: "<< ave_x3
		<<" with " << round(num_of_elements) << " elements total"   << std::endl;

	//############################################

	mynode.Stop(); //exception thrown if not stopped


	return 0;
}
