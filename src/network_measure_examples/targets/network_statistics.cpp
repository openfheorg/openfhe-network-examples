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
#include "path_measure_crypto_functions.h"
#include "register_functions.h"

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

    std::string dsendtoNode, dgetfromNode, usendtoNode, ugetfromNode, startNode, endNode, controllerNode;

    controllerNode = "Controller";
    //cryptocontext generated by the controller node
    lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc(nullptr);

    OPENFHE_DEBUG("Cryptocontext Generation");
    // get CryptoContext from the lead node.
    //DBC added: this works without this but it is standard practice when deserializing CC
    lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();

    mynode.getSerialMsgWait(cc, "cryptocontext", controllerNode);
    //DBC added: this works without this but it is standard practice when deserializing CC
    // It is possible that the keys are carried over in the cryptocontext
    // serialization so clearing the keys is important
    /////////////////////////////////////////////////////////////////
    cc->ClearEvalMultKeys();
    cc->ClearEvalAutomorphismKeys();

    //plaintext modulus and ring dimension
    //unsigned int plaintextModulus = cc->GetCryptoParameters()->GetPlaintextModulus();
    unsigned int ringsize = cc->GetCryptoParameters()->GetElementParams()->GetCyclotomicOrder()/2;

    //compute number of nodes in the computation
    std::vector<int64_t> vectorOfInts;
    vectorOfInts.reserve(ringsize/2);

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

    startNode = "Node1";
    endNode = "Node" +std::to_string(num_of_nodes);

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

    //possibly wait for clients to start public key gen phase
    Debug_pause(PAUSE_FLAG, "public key wait", params);

    TimeVar t;
    TIC(t);

    OPENFHE_DEBUG("Joint Public key Generation");

    auto FinalPubKey = JointPublickeyGen(mynode, node_name, startNode, endNode, nodes_path, Msgsent, cc);

    auto elapsed_time = TOC_MS(t);

    if (TEST_MODE)
    	std::cout << "Joint Public Key generation time: " << elapsed_time << " ms\n";

    if (node_name == endNode) {
        mynode.sendSerialMsg(FinalPubKey, Msgsent, "finalpublickey", controllerNode);
    }

    //Eval mult key generation multiple rounds
    TIC(t);

    OPENFHE_DEBUG("EvalMult key Generation");

    //joint evalmult key generation does a round trip communication from Node1->Node2->Node1

    // joint evalmult key generation in the direction Node1 -> Node2
    auto evalMultFinal = JointEvalMultkeyGen(mynode, node_name, startNode, endNode, nodes_path, Msgsent, cc);

    elapsed_time = TOC_MS(t);

    cc->InsertEvalMultKey({evalMultFinal});

    if (node_name == startNode) {
        mynode.sendSerialMsg(evalMultFinal, Msgsent, "evalmultfinal", controllerNode);
    }

    if (TEST_MODE)
    	std::cout << "Eval Mult key generation time: " << elapsed_time << " ms\n";
    //####################################################################################
    //Eval sum key generation multiple rounds
    TIC(t);

    auto evalSumFinal = JointEvalSumkeyGen(mynode, node_name, startNode, endNode, nodes_path, Msgsent, cc);

    elapsed_time = TOC_MS(t);

    cc->InsertEvalSumKey({evalSumFinal});

    if (node_name == endNode) {
        mynode.sendSerialMsg(evalSumFinal, Msgsent, "evalsumfinal", controllerNode);
    }

    if (TEST_MODE)
    	std::cout << "Eval Sum key generation time: " << elapsed_time << " ms\n";

    //get the ciphertext from previous node or the initial ciphertext from the controller
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> prevCT(nullptr), FinalCT(nullptr);
    if (node_name != startNode) {
        mynode.getSerialMsgWait(prevCT, "ciphertext", dgetfromNode);
    } else {
	mynode.getSerialMsgWait(prevCT, "zero ciphertext", controllerNode);
    }
    initialize_map(ringsize);

    //generate data and accumulate with the ciphertext from previous node in the path
    std::cout << "Input data: " << std::endl;
    for (usint i = 0; i < 3; i++) {
	std::string vector_name = "v"+std::to_string(i+1);
        std::cout << vectorOfInts[i] << " ";
        write(prevCT, cc, vector_name, vectorOfInts[i], node_id-1);
    }
    std::cout << std::endl;

    add(prevCT, cc, "n", 1.0, 0);    //increment the counter

    //send ciphertext to other node
    if (node_name == endNode) {
	mynode.sendSerialMsg(prevCT, Msgsent, "ciphertext", usendtoNode);
	FinalCT = prevCT;
    } else {
	mynode.sendSerialMsg(prevCT, Msgsent, "ciphertext", dsendtoNode);
    }

    if (node_name != endNode) {
	if (node_name != startNode) {
		mynode.getSerialMsgWait(FinalCT, "ciphertext", ugetfromNode);
		mynode.sendSerialMsg(FinalCT, Msgsent, "ciphertext", usendtoNode);
	} else {
		mynode.getSerialMsgWait(FinalCT, "ciphertext", ugetfromNode);
	}

    }
    //compute partially decrypted ciphertext
    //client id
    std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> ciphertextPartial;
    if (node_name == startNode) {
        ciphertextPartial = cc->MultipartyDecryptLead({FinalCT}, mynode.get_keyPair().secretKey);
    } else {
        ciphertextPartial = cc->MultipartyDecryptMain({FinalCT}, mynode.get_keyPair().secretKey);
    }

    mynode.sendSerialMsg(ciphertextPartial[0], Msgsent, "partialct", controllerNode);
    std::cout << "Mynode " << node_name << " done" << std::endl;
    std::cout << "\n";

    mynode.Stop(); //exception thrown if not stopped

    return 0;
}
