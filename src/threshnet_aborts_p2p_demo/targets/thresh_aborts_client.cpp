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
#include "node.h"
#include "crypto_functions.h"
#include <string.h>
#include <iostream>
#include <sstream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::milliseconds
#include <cmath>

constexpr unsigned ABORTS_SLEEP_TIME_MSECS = 10;
constexpr unsigned TIME_OUT = 1000;

int main(int argc, char** argv) {
    OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements

	CommParams params;
	
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


    uint32_t recovering_node_id = 1;
    std::string nodeRecover = "Node" + std::to_string(recovering_node_id);
	std::string arguments = params.ExtraArgument.c_str();

    std::string compute_operation;
    bool aborts = false;
    if (arguments.find("-") == std::string::npos) {
        compute_operation = arguments;
    } else {
        auto arguments_split = SplitString(arguments, "-");

        compute_operation = arguments_split[0];
        aborts = arguments_split[1] == "abort"? 1: 0;
    }
    
	std::string startNode = "Node1";

    //parse input file and load data 
	std::vector<int64_t> vectorOfInts;
    //vectorOfInts.reserve(ringsize);

    std::string inputfile = file2String(params.ApplicationFile + "_" + params.NodeName);
	std::istringstream fin (inputfile);
	usint num_of_nodes = 0, node_id = 0;
    int64_t data;

    usint ctr = 0;

	//load vectorofInts from input file along with number of parties and id of the node
    while (fin >> data)
    {
		if (ctr == 0) {
            num_of_nodes = data;
		} else if (ctr == 1) {
			node_id = data;
		} else {
            vectorOfInts.push_back(data);
		}
		ctr++;
    }

    std::string nodename_from_inputfile = "Node"+std::to_string(node_id);

	if ((num_of_nodes == 0) || (node_id == 0) || (node_name != nodename_from_inputfile)) {
		std::cerr << "Wrong input file" << std::endl;
		exit(EXIT_FAILURE);
	}

    std::string endNode = "Node" +std::to_string(num_of_nodes);
    std::string dsendtoNode, dgetfromNode, usendtoNode, ugetfromNode;

    std::cout << "node id " << node_id << std::endl;
    //todo: choose sendToNode and getfromNode from NetworkMap directly
    if (node_id == 1) {
        dsendtoNode = "Node" + std::to_string(node_id + 1);
        ugetfromNode = "Node" + std::to_string(node_id + 1);
    } else if ((node_id > 1) && (node_id < num_of_nodes)) {
        dsendtoNode = "Node" + std::to_string(node_id + 1);
		usendtoNode = "Node" + std::to_string(node_id - 1);
		dgetfromNode = "Node" + std::to_string(node_id - 1);
        ugetfromNode = "Node" + std::to_string(node_id + 1);
    } else {
        usendtoNode = "Node" + std::to_string(node_id - 1);
        dgetfromNode = "Node" + std::to_string(node_id - 1);
    }

    std::vector<std::string> nodes_path(4);
    
	nodes_path[0] = dsendtoNode;
    nodes_path[1] = dgetfromNode;
    nodes_path[2] = usendtoNode;
    nodes_path[3] = ugetfromNode;

    //establish connection first
    Msgsent.Data = "synchronize";
    Msgsent.msgType = "synchronize msg";
    mynode.broadcastMsg(Msgsent);
    for (uint32_t i = 1; i <= num_of_nodes; i++)
    {
        if(i != node_id) {
            std::string nodeGet = "Node" + std::to_string(i);
            auto reply = mynode.getMsgWait(nodeGet, "synchronize msg");
        }
    }


    //cryptocontext generation.
    //For now, the cryptocontext is generated by node 1 and sent to node 2. This will be changed later to be
    //generated by a key server when a client-server communication is added to the peer to peer framework.
	lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc(nullptr);

	OPENFHE_DEBUG("Cryptocontext Generation");

    TimeVar t;
    
    if (node_name == startNode) {
        TIC(t);
        int plaintextModulus = 65537;
		double sigma = 3.2;
		lbcrypto::SecurityLevel securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;
		
        usint batchSize = 16;
		usint multDepth = 4;
        usint digitSize = 30;
		usint dcrtBits = 60;
		
		lbcrypto::CCParams<lbcrypto::CryptoContextBFVRNS> parameters;		

        parameters.SetPlaintextModulus(plaintextModulus);
		parameters.SetSecurityLevel(securityLevel);
		parameters.SetStandardDeviation(sigma);
		parameters.SetSecretKeyDist(lbcrypto::UNIFORM_TERNARY);
		parameters.SetMultiplicativeDepth(multDepth);
		parameters.SetBatchSize(batchSize);
        parameters.SetDigitSize(digitSize);
		parameters.SetScalingModSize(dcrtBits);
        parameters.SetMultiplicationTechnique(lbcrypto::HPS);

		cc = GenCryptoContext(parameters);

		cc->Enable(lbcrypto::PKE);
		cc->Enable(lbcrypto::LEVELEDSHE);
        cc->Enable(lbcrypto::ADVANCEDSHE);
		cc->Enable(lbcrypto::MULTIPARTY);

        auto elapsed_time = TOC_MS(t);

        if (TEST_MODE)
	        std::cout << "Cryptocontext generation time: " << elapsed_time << " ms\n";

        TIC(t);
        mynode.broadcastSerialMsg(cc, Msgsent, "cryptocontext");

        elapsed_time = TOC_MS(t);

        if (TEST_MODE)
	        std::cout << "Cryptocontext broadcasting time: " << elapsed_time << " ms\n";

    } else {
		// get CryptoContext from the lead node.
        lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();
        
        TIC(t);
        
		mynode.getSerialMsgWait(cc, "cryptocontext", startNode);
	
        auto elapsed_time = TOC_MS(t);

        if (TEST_MODE)
	        std::cout << "Cryptocontext receiving time: " << elapsed_time << " ms\n";
    }

    cc->ClearEvalMultKeys();
	cc->ClearEvalAutomorphismKeys();

    unsigned int ringsize = cc->GetCryptoParameters()->GetElementParams()->GetCyclotomicOrder()/2;
    auto elemParams = cc->GetCryptoParameters()->GetElementParams();

    std::cout << "ring dimension " << ringsize << std::endl;
    std::cout << "log2 q = " << log2(cc->GetCryptoParameters() ->GetElementParams() ->GetModulus().ConvertToDouble()) << std::endl;

    TIC(t);
    
    OPENFHE_DEBUG("Joint Public key Generation");
    
	auto FinalPubKey = JointPublickeyGen(mynode, node_name, startNode, endNode, nodes_path, Msgsent, cc);

    auto elapsed_time = TOC_MS(t);

    if (TEST_MODE)
    	std::cout << "Joint Public Key generation time: " << elapsed_time << " ms\n";

    //Secret sharing secret key for aborts protocol
	TIC(t);

    usint threshold = floor(num_of_nodes/2) + 1;
    auto keyPair = mynode.get_keyPair();
    //share the secret key to other parties
	auto secretshares = cc->ShareKeys(keyPair.secretKey, num_of_nodes, threshold, node_id, "shamir");
	
    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
	    std::cout << "Generate secret shares for Aborts protocol time: " << elapsed_time << " ms\n";

    TIC(t);
    std::unordered_map<uint32_t, lbcrypto::DCRTPoly> recdSecretShares;
    //send every node share to the node
    for (usint i = 1; i <= num_of_nodes; i++) {
        if (i != node_id) {
            lbcrypto::DCRTPoly recdShare;
            std::string nodeSend = "Node" + std::to_string(i);
            mynode.sendSerialMsg(secretshares[i], Msgsent, "secretshare", nodeSend);
            mynode.getSerialMsgWait(recdShare, "secretshare", nodeSend);
            recdSecretShares[i] = recdShare;
        }
    }

    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
	    std::cout << "Srialize and Send secret shares for Aborts protocol time: " << elapsed_time << " ms\n";

    if (compute_operation == "multiply") {
        //Eval mult key generation multiple rounds
        

        OPENFHE_DEBUG("EvalMult key Generation");

        //joint evalmult key generation does a round trip communication from Node1->Node2->Node1
        // joint evalmult key generation in the direction Node1 -> Node2
        TIC(t);
        
        auto evalMultFinal = JointEvalMultkeyGen(mynode, node_name, startNode, endNode, nodes_path, Msgsent, cc);

        elapsed_time = TOC_MS(t);

        cc->InsertEvalMultKey({evalMultFinal});
        

        if (TEST_MODE)
            std::cout << "Eval Mult key generation time: " << elapsed_time << " ms\n";
    }
    //####################################################################################
    if (compute_operation == "vectorsum") {
        //Eval sum key generation multiple rounds
        TIC(t);

        auto evalSumFinal = JointEvalSumkeyGen(mynode, node_name, startNode, endNode, nodes_path, Msgsent, cc);
        
        elapsed_time = TOC_MS(t);

        cc->InsertEvalSumKey({evalSumFinal});

        if (TEST_MODE)
            std::cout << "Eval Sum key generation time: " << elapsed_time << " ms\n";
    }
    //####################################################################################

    //####################################################################################

	// generate cipher text
    //pack them into a packed plaintext (vector encryption)
    lbcrypto::Plaintext pt = cc->MakePackedPlaintext(vectorOfInts);

    std::cout << "\n Original Plaintext: \n" << std::endl;
    std::cout << pt << std::endl;

    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> cipherText1 = cc->Encrypt(FinalPubKey, pt);  // Encrypt
    TIC(t);
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> cipherText = cc->Encrypt(FinalPubKey, pt);  // Encrypt

	elapsed_time = TOC_MS(t);

    if (TEST_MODE)
	    std::cout << "Ciphertext generation time: " << elapsed_time << " ms\n";

    //send the ciphertext to other nodes
    mynode.broadcastSerialMsg(cipherText, Msgsent, "ciphertext");
	std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> CipherTexts(num_of_nodes);

    
	for (uint32_t i = 1; i <= num_of_nodes;i++) {
        if (node_id != i) {
			lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ClientCT(nullptr);
            std::string nodeGet = "Node" + std::to_string(i);
            mynode.getSerialMsgWait(ClientCT,"ciphertext", nodeGet);
			CipherTexts[i-1] = ClientCT;
		} else {
		    CipherTexts[i-1] = cipherText;
		}
	}

	//compute multiplication of the ciphertexts
	std::cout << "Number of ciphertexts: " << CipherTexts.size() << std::endl;
	lbcrypto::Ciphertext<lbcrypto::DCRTPoly> resultCT (nullptr), temp(nullptr);

    if (aborts == 1) {
	  std::cout << "This client is Aborting the protocol and exiting." <<std::endl;
	  exit(EXIT_FAILURE);
	}

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
    
    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
	    std::cout << "Compute time: " << elapsed_time << " ms\n";
        
    //vector for all partially decrypted mult ciphertexts
	std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> partialCiphertextVec(num_of_nodes);

    TIC(t);
	//compute partially decrypted ciphertext
	std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> ciphertextPartial;
	if (node_id == 1) {
        ciphertextPartial = cc->MultipartyDecryptLead({resultCT}, keyPair.secretKey);

	} else {
        ciphertextPartial = cc->MultipartyDecryptMain({resultCT}, keyPair.secretKey);
	}

    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
        std::cout << "Distributed decryption partial ct time: " << elapsed_time << " ms\n";
    
    TIC(t);
	mynode.broadcastSerialMsg(ciphertextPartial, Msgsent, "partialciphertext", 5, 10);

    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
        std::cout << "Distributed decryption broadcast partial ct time: " << elapsed_time << " ms\n";

    std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> ciphertextPartial1;
    std::vector<uint32_t> node_nack;

    TIC(t);
    //receive partial ciphertexts from all alive nodes and track dropped nodes
    for (uint32_t i = 1; i <= num_of_nodes;i++) {
        if (node_id != i) {
            std::string nodeGet = "Node" + std::to_string(i);

            auto replyMsg = mynode.getMsgWait(nodeGet, "partialciphertext", ABORTS_SLEEP_TIME_MSECS, TIME_OUT);

            if (!replyMsg.acknowledgement) {
                std::cout << "Party " << i << " unavailable for sending partial decryption share, initiating aborts protocol" << std::endl;
                std::cout << "recovering client id " << recovering_node_id << std::endl;
                node_nack.push_back(i);
            } else {
                auto serialpartialCT = replyMsg.Data;
                if (!serialpartialCT.size())
		        OPENFHE_THROW(lbcrypto::openfhe_error, "Received empty serialized object partial ct");
	
	            std::istringstream is(serialpartialCT);
	            lbcrypto::Serial::Deserialize(ciphertextPartial1, is, GlobalSerializationType);
                partialCiphertextVec[i-1] = ciphertextPartial1[0];
            }
        } else {
            partialCiphertextVec[i-1] = ciphertextPartial[0];
        }
    }
    
    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
        std::cout << "Distributed decryption receive other partial cts time: " << elapsed_time << " ms\n";

    TIC(t);
    //check threshold - number of alive nodes
    std::cout << "num of nodes dropped out: " << node_nack.size() << std::endl;
    if ((node_nack.size() > 0) && (node_nack.size() >= threshold))
        OPENFHE_THROW(lbcrypto::config_error, "majority number of nodes dropped out, not enough nodes to recover");

    //get partial CTs of dropped nodes
    if (node_nack.size()) {
        std::unordered_map<uint32_t, lbcrypto::DCRTPoly> recoveryShares;
        for (uint32_t i = 0; i < node_nack.size();i++) {
            if (node_id == recovering_node_id) {
                recoveryShares[recovering_node_id] = recdSecretShares[node_nack[i]];
                auto recoveredSecret  = RecoverDroppedSecret(mynode, node_id, node_nack,Msgsent, cc, recoveryShares, num_of_nodes, threshold);
                ciphertextPartial1 = SendRecoveredPartialCT(mynode, node_id, node_nack, Msgsent, num_of_nodes, cc, resultCT, recoveredSecret);
            
            } else {

                nodeRecover = "Node" + std::to_string(recovering_node_id);
                mynode.sendSerialMsg(recdSecretShares[node_nack[i]], Msgsent, "secretsharerecovery", nodeRecover);
                mynode.getSerialMsgWait(ciphertextPartial1, "abortspartialciphertext", nodeRecover);
            }
            partialCiphertextVec[node_nack[i]-1] = ciphertextPartial1[0];
        }
    }

    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
        std::cout << "Distributed decryption recover abort partial ct time: " << elapsed_time << " ms\n";

    TIC(t);
	lbcrypto::Plaintext plaintextMultipartyNew;
    cc->MultipartyDecryptFusion(partialCiphertextVec, &plaintextMultipartyNew);

    elapsed_time = TOC_MS(t);

    if (TEST_MODE)
        std::cout << "Distributed decryption decryptfusion time: " << elapsed_time << " ms\n";

    std::cout << "\n Resulting Fused " << compute_operation << " Plaintext: \n" << std::endl;

	plaintextMultipartyNew->SetLength(24);
    std::cout << plaintextMultipartyNew << std::endl;
    std::cout << "\n";

    mynode.Stop(); //exception thrown if not stopped
	return 0;
}
