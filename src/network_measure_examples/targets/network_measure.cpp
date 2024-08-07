/***
 * (c) 2021 Duality Technologies, Inc. All rights reserved.
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
#include "node.h"


void Debug_pause(bool pause_flag, std::string msg, CommParams params){
    if (pause_flag) {
        //wait for carrage return input to continue
        std::cout << "client "<< params.NodeName << " " << msg << std::endl;
        std::cin.get();
    }
    return;
}

int main(int argc, char** argv) {

    OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements

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

    std::string sendtoNode, getfromNode;

    usint lead_node_index = 1;
    std::string startNode = "Node"+std::to_string(lead_node_index);
    std::string nextNode = "Node"+std::to_string(lead_node_index+1);

    if (node_name == startNode) {
        sendtoNode = nextNode;
        getfromNode = nextNode;
    } else {
        sendtoNode = startNode;
        getfromNode = startNode;
    }

    //cryptocontext generation.
    //For now, the cryptocontext is generated by node 1 and sent to node 2. This will be changed later to be
    //generated by a key server when a client-server communication is added to the peer to peer framework.
    lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc(nullptr);

    OPENFHE_DEBUG("Cryptocontext Generation");
    if (node_name == startNode) {
        int plaintextModulus = 65537;
        double sigma = 3.2;
        lbcrypto::SecurityLevel securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;
        usint batchSize = 1024;
        usint multDepth = 4;

        lbcrypto::CCParams<lbcrypto::CryptoContextBFVRNS> parameters;

        parameters.SetPlaintextModulus(plaintextModulus);
        parameters.SetSecurityLevel(securityLevel);
	parameters.SetStandardDeviation(sigma);
	parameters.SetSecretKeyDist(lbcrypto::UNIFORM_TERNARY);
	parameters.SetMultiplicativeDepth(multDepth);
	parameters.SetBatchSize(batchSize);

	cc = GenCryptoContext(parameters);

	cc->Enable(lbcrypto::PKE);
	cc->Enable(lbcrypto::LEVELEDSHE);
	cc->Enable(lbcrypto::MULTIPARTY);

	mynode.sendSerialMsg(cc, Msgsent, "cryptocontext", sendtoNode);

    } else {
	// get CryptoContext from the lead node.
        //DBC added: this works without this but it is standard practice when deserializing CC
        lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();
	mynode.getSerialMsgWait(cc, "cryptocontext", getfromNode);
	//DBC added: this works without this but it is standard practice when deserializing CC
	// It is possible that the keys are carried over in the cryptocontext
	// serialization so clearing the keys is important
	/////////////////////////////////////////////////////////////////
	cc->ClearEvalMultKeys();
	cc->ClearEvalAutomorphismKeys();
    }

    //possibly wait for clients to start public key gen phase
    Debug_pause(PAUSE_FLAG, "public key wait", params);

    TimeVar t;
    TIC(t);

    OPENFHE_DEBUG("Joint Public key Generation");

    lbcrypto::KeyPair<lbcrypto::DCRTPoly> keyPair (nullptr);
    if (node_name == startNode) {
	keyPair = cc->KeyGen();
    } else {
	//get the public key of the previous client
	std::cout << "\n Wait for previous client's public key" << std::endl;

	//get prev client's public key
	lbcrypto::PublicKey<lbcrypto::DCRTPoly> prevPubKey(nullptr);
	mynode.getSerialMsgWait(prevPubKey, "PublicKey", getfromNode);

	//compute public key from
	keyPair = cc->MultipartyKeyGen(prevPubKey);
    }

    mynode.sendSerialMsg(keyPair.publicKey, Msgsent, "PublicKey", sendtoNode);

    //request and receive final public key from the final client
    lbcrypto::PublicKey<lbcrypto::DCRTPoly> FinalPubKey(nullptr);
    if (node_name != "Node2") {
	std::cout << "\n Wait for final public key" << std::endl;
	mynode.getSerialMsgWait(FinalPubKey, "PublicKey", getfromNode);
    } else {
	FinalPubKey = keyPair.publicKey;
    }

    auto elapsed_time = TOC_MS(t);

    if (TEST_MODE)
    	std::cout << "Joint Public Key generation time: " << elapsed_time << " ms\n";


    //Eval mult key generation multiple rounds
    TIC(t);

    OPENFHE_DEBUG("EvalMult key Generation");

    //joint evalmult key generation does a round trip communication from Node1->Node2->Node1

    // joint evalmult key generation in the direction Node1 -> Node2
    if (node_name == startNode) {
	// Generate evalmult key part for A
	Debug_pause(PAUSE_FLAG, "eval key wait", params);

	auto evalMultKey = cc->KeySwitchGen(keyPair.secretKey, keyPair.secretKey);

	//send evalMultkey to server with round number (same as client number)
	mynode.sendSerialMsg(evalMultKey, Msgsent, "evalmultkey", sendtoNode);

    } else if (node_name != startNode) {

	Debug_pause(PAUSE_FLAG, "next eval key wait", params);

	//get previous eval multkey
	std::cout << "\n Wait for previous client's evalmult key" << std::endl;

        lbcrypto::EvalKey<lbcrypto::DCRTPoly> PrevevalMultKey(nullptr);
        mynode.getSerialMsgWait(PrevevalMultKey,"evalmultkey", getfromNode);

	//compute evalmultkey for this client
	auto evalMultKey = cc->MultiKeySwitchGen(keyPair.secretKey, keyPair.secretKey, PrevevalMultKey);

	//compute evalmultAB/evalmultABC (adding up with prev evalmultkey)
	auto evalMultPrevandCurrent = cc->MultiAddEvalKeys(PrevevalMultKey, evalMultKey, keyPair.publicKey->GetKeyTag());

	mynode.sendSerialMsg(evalMultPrevandCurrent, Msgsent, "evalmultkey", sendtoNode);

	if (node_name == "Node2") {
		auto evalMultskAll = cc->MultiMultEvalKey(keyPair.secretKey, evalMultPrevandCurrent, FinalPubKey->GetKeyTag());

		mynode.sendSerialMsg(evalMultskAll, Msgsent, "evalmixmultkey", sendtoNode);
	}

    } else {
	std::cerr << "client id out of range" <<std::endl;
    }

    //joint evalmult key generation in the direction Node2 -> Node1
    if (node_name != nextNode) {

	//request and receive the evalmultAll key and evalmult computed from all parties
	lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultAll(nullptr), PrevevalMixMultKey (nullptr);

	//round_num -1
	Debug_pause(PAUSE_FLAG, "eval key round 2 wait", params);

	std::cout << "\n Wait for evalmultall key" << std::endl;
        mynode.getSerialMsgWait(evalMultAll, "evalmultkey", getfromNode);

	std::cerr << "\n Wait for PrevevalMixMultKey" << std::endl;
	mynode.getSerialMsgWait(PrevevalMixMultKey, "evalmixmultkey", getfromNode);

	//Add evalmultkeys to compute final evalmultkey
	auto evalMultKeyskAll = cc->MultiMultEvalKey(keyPair.secretKey, evalMultAll, FinalPubKey->GetKeyTag());
	auto evalMultAddskAll = cc->MultiAddEvalMultKeys(evalMultKeyskAll, PrevevalMixMultKey, FinalPubKey->GetKeyTag());


        mynode.sendSerialMsg(evalMultAddskAll, Msgsent, "evalmixmultkey", sendtoNode);

	if (node_name == startNode) {
            cc->InsertEvalMultKey({evalMultAddskAll});
	}
    }


    //get the final evaluation mult key if client id is not 1
    if (node_name != startNode) {
	std::cout << "\n Wait for evalmultfinal key" << std::endl;

	lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultFinal(nullptr);

	mynode.getSerialMsgWait(evalMultFinal, "evalmixmultkey", getfromNode);
	cc->InsertEvalMultKey({evalMultFinal});
    }

    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
    	std::cout << "Eval Mult key generation time: " << elapsed_time << " ms\n";
    //####################################################################################

    // generate and send cipher text to the server
    unsigned int plaintextModulus = cc->GetCryptoParameters()->GetPlaintextModulus();

    std::vector<int64_t> vectorOfInts;

    //load vector of ints from input file
    sleep(5);

    std::string inputfile = file2String(params.ApplicationFile + "_" + params.NodeName);
    std::istringstream fin (inputfile);
    int64_t data;

	
	if (fin.fail()){
	  cout << "Error: can't find input file: " << inputfile << endl;
	  exit(EXIT_FAILURE);
	}
	
    while (fin >> data)
    {
        vectorOfInts.push_back(data);
    }

    //pack them into a packed plaintext (vector encryption)
    lbcrypto::Plaintext pt = cc->MakePackedPlaintext(vectorOfInts);

    std::cout << "\n Original Plaintext: \n" << std::endl;
    std::cout << pt << std::endl;

    //generate randomness r_a
    std::vector<int64_t> random_rvec, plain_randomciphertext;
    random_rvec.reserve(12);//ringsize);
    for (size_t i = 0; i < 12; i++) {//ringsize; i++) { //generate a random array of shorts
        random_rvec.emplace_back(std::rand() % plaintextModulus);
		plain_randomciphertext.push_back((vectorOfInts[i]*random_rvec[i])%plaintextModulus);
    }

    //pack them into a packed plaintext (vector encryption)
    auto plainrandom_r = cc->MakePackedPlaintext(random_rvec);
    auto encryptedrandom_r = cc->Encrypt(FinalPubKey, plainrandom_r); // Encrypt

    //randomciphertext computed as E(m_ar_a) instead of E(m_a)E(r_a) to save encryption
    auto ptrandom = cc->MakePackedPlaintext(plain_randomciphertext);

    TIC(t);

    auto randomcipherText = cc->Encrypt(FinalPubKey, ptrandom);

    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
        std::cout << "Ciphertext generation time: " << elapsed_time << " ms\n";

    //send randomized ciphertext to other node
    mynode.sendSerialMsg(randomcipherText, Msgsent, "ciphertext", sendtoNode);

    //send encrypted randomness E(r_a)
    mynode.sendSerialMsg(encryptedrandom_r, Msgsent, "randomness ciphertext", sendtoNode);

    //recive randomized ciphertext from other node E(m_br_b)
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> clientCT(nullptr);
    mynode.getSerialMsgWait(clientCT, "ciphertext", getfromNode);

    //receive encrypted randomness from other side E(r_b)
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ClientRandomness(nullptr);
    mynode.getSerialMsgWait(ClientRandomness, "randomness ciphertext", getfromNode);

    //compute multiplication of the ciphertexts
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> resultCT (nullptr), temp1(nullptr), temp2(nullptr);

    //randomcipherText*ClientRandomness - ClientCT*encryptedrandom_r
    temp1 = cc->EvalMult(randomcipherText, ClientRandomness);
    temp2 = cc->EvalMult(clientCT, encryptedrandom_r);

    if(node_name == startNode) {
        resultCT = cc->EvalSub(temp1, temp2);
    } else {
	resultCT = cc->EvalSub(temp2, temp1);
    }

    //vector for all partially decrypted mult ciphertexts
    TIC(t);

    std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> partialCiphertextVec(2);

    //compute partially decrypted ciphertext
    //client id
    std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> ciphertextPartial;
    if (node_name == startNode) {
        ciphertextPartial = cc->MultipartyDecryptLead({resultCT}, keyPair.secretKey);
    } else {
        ciphertextPartial = cc->MultipartyDecryptMain({resultCT}, keyPair.secretKey);
    }

    mynode.sendSerialMsg(ciphertextPartial[0], Msgsent, "partialct", sendtoNode);

    //get partial ct from other node
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> otherClientPartialCT(nullptr);
    mynode.getSerialMsgWait(otherClientPartialCT, "partialct", getfromNode);

    if(node_name == startNode) {
	partialCiphertextVec[0] = ciphertextPartial[0];
	partialCiphertextVec[1] = otherClientPartialCT;
    } else {
	partialCiphertextVec[0] = otherClientPartialCT;
	partialCiphertextVec[1] = ciphertextPartial[0];
    }

    lbcrypto::Plaintext plaintextMultipartyNew;
    cc->MultipartyDecryptFusion(partialCiphertextVec, &plaintextMultipartyNew);

    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
        std::cout << "Distributed decryption time: " << elapsed_time << " ms\n";

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

    mynode.Stop(); //exception thrown if not stopped
    return 0;
}
