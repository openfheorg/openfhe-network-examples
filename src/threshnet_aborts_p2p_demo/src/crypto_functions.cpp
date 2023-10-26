// @file path_measure_crypto_functions.cpp// @author TPOC: info@DualityTech.com
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
// This file defines all the methods and variables of the node object.
/*
Methods:
> InitializeCC
> 
*/

#include "crypto_functions.h"

//OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements

lbcrypto::PublicKey<lbcrypto::DCRTPoly> JointPublickeyGen(NodeImpl& mynode, std::string& NodeName, std::string& startNode, std::string& endNode, std::vector<std::string>& nodes_path, message_format& Msgsent, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc) {

    std::string dsendtoNode = nodes_path[0];
    std::string dgetfromNode = nodes_path[1];
    std::string usendtoNode = nodes_path[2];
    std::string ugetfromNode = nodes_path[3];

    lbcrypto::KeyPair<lbcrypto::DCRTPoly> keyPair (nullptr);
	if (NodeName == startNode) {
		keyPair = cc->KeyGen();
	} else {
		//get the public key of the previous client
		std::cout << "\n Wait for previous client's public key" << std::endl;
        
		//get prev client's public key
		lbcrypto::PublicKey<lbcrypto::DCRTPoly> prevPubKey(nullptr);
		mynode.getSerialMsgWait(prevPubKey, "PublicKey", dgetfromNode);

		//compute public key from
		keyPair = cc->MultipartyKeyGen(prevPubKey);
	}

    mynode.set_keyPair(keyPair);
    
    if (NodeName != endNode) {
        mynode.sendSerialMsg(keyPair.publicKey, Msgsent, "PublicKey", dsendtoNode);
    }

    lbcrypto::PublicKey<lbcrypto::DCRTPoly> FinalPubKey(nullptr);
    if (NodeName == endNode) {
		FinalPubKey = keyPair.publicKey;
        mynode.broadcastSerialMsg(keyPair.publicKey, Msgsent, "PublicKey");
	} else {
        std::cout << "\n Wait for final public key" << std::endl;
		mynode.getSerialMsgWait(FinalPubKey, "PublicKey", endNode);
	}

	mynode.set_FinalPubKey(FinalPubKey);

    return FinalPubKey;
}

lbcrypto::EvalKey<lbcrypto::DCRTPoly> JointEvalMultkeyGen(NodeImpl& mynode, std::string& NodeName, std::string& startNode, std::string& endNode, std::vector<std::string>& nodes_path, message_format& Msgsent, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc) {
    
    std::string dsendtoNode = nodes_path[0];
    std::string dgetfromNode = nodes_path[1];
    std::string usendtoNode = nodes_path[2];
    std::string ugetfromNode = nodes_path[3];

    TimeVar t;
    
    if (NodeName == startNode) {

        TIC(t);
		// Generate evalmult key part for A
		auto evalMultKey = cc->KeySwitchGen(mynode.get_keyPair().secretKey, mynode.get_keyPair().secretKey);	
	
        auto elapsed_time = TOC_MS(t);
    
        if (debug_flag_val)
            std::cout << "Eval Mult key generation compute multkey time: " << elapsed_time << " ms\n";

        TIC(t);
		//send evalMultkey to the next node
		mynode.sendSerialMsg(evalMultKey, Msgsent, "evalmultkey", dsendtoNode);

        elapsed_time = TOC_MS(t);
    
        if (debug_flag_val)
            std::cout << "Eval Mult key generation sending multkey to next node time: " << elapsed_time << " ms\n";

	} else {
		//get previous eval multkey
		std::cout << "\n Wait for previous client's evalmult key" << std::endl;

        lbcrypto::EvalKey<lbcrypto::DCRTPoly> PrevevalMultKey(nullptr);

        TIC(t);
        mynode.getSerialMsgWait(PrevevalMultKey,"evalmultkey", dgetfromNode);

        auto elapsed_time = TOC_MS(t);
        if (debug_flag_val)
            std::cout << "Eval Mult key generation getting prev multkey time: " << elapsed_time << " ms\n";

        TIC(t);
		//compute evalmultkey for this client
		auto evalMultKey = cc->MultiKeySwitchGen(mynode.get_keyPair().secretKey, mynode.get_keyPair().secretKey, PrevevalMultKey);

		//compute evalmultAB/evalmultABC (adding up with prev evalmultkey)
		auto evalMultPrevandCurrent = cc->MultiAddEvalKeys(PrevevalMultKey, evalMultKey, mynode.get_keyPair().publicKey->GetKeyTag());

        elapsed_time = TOC_MS(t);
        if (debug_flag_val)
            std::cout << "Eval Mult key generation compute multkey time: " << elapsed_time << " ms\n";

		if (NodeName == endNode) {
			auto evalMultskAll = cc->MultiMultEvalKey(mynode.get_keyPair().secretKey, evalMultPrevandCurrent,
											mynode.get_FinalPubKey()->GetKeyTag());


            TIC(t);
			//std::ostringstream evmmKey;
			mynode.broadcastSerialMsg(evalMultPrevandCurrent, Msgsent, "evalmultkey");

            auto elapsed_time = TOC_MS(t);
    
            if (debug_flag_val)
                std::cout << "Eval Mult key generation broadcasting multkeyall time: " << elapsed_time << " ms\n";
            
            TIC(t);
            mynode.sendSerialMsg(evalMultskAll, Msgsent, "evalmixmultkey", usendtoNode);
            elapsed_time = TOC_MS(t);
    
            if (debug_flag_val)
                std::cout << "Eval Mult key generation sending mixmultkey to prev node time: " << elapsed_time << " ms\n";

		} else {
            TIC(t);
			mynode.sendSerialMsg(evalMultPrevandCurrent, Msgsent, "evalmultkey", dsendtoNode);
            auto elapsed_time = TOC_MS(t);
    
            if (debug_flag_val)
                std::cout << "Eval Mult key generation sending multkey to next node time: " << elapsed_time << " ms\n";
		}
        

	}

	//joint evalmult key generation in the direction Node2 -> Node1
	if (NodeName != endNode) {

		//request and receive the evalmultAll key and evalmult computed from all parties
		lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultAll(nullptr), PrevevalMixMultKey (nullptr);

		std::cout << "\n Wait for evalmultall key" << std::endl;

        TIC(t);

        mynode.getSerialMsgWait(evalMultAll, "evalmultkey", endNode);

        auto elapsed_time = TOC_MS(t);
    
        if (debug_flag_val)
            std::cout << "Eval Mult key generation getting evalmultall time: " << elapsed_time << " ms\n";

		std::cerr << "\n Wait for PrevevalMixMultKey" << std::endl;

        TIC(t);
		mynode.getSerialMsgWait(PrevevalMixMultKey, "evalmixmultkey", ugetfromNode);

        elapsed_time = TOC_MS(t);
    
        if (debug_flag_val)
            std::cout << "Eval Mult key generation getting evalmixmult from next node time: " << elapsed_time << " ms\n";

        TIC(t);

		//Add evalmultkeys to compute final evalmultkey
		auto evalMultKeyskAll = cc->MultiMultEvalKey(mynode.get_keyPair().secretKey, evalMultAll,
										mynode.get_FinalPubKey()->GetKeyTag());
		
		auto evalMultAddskAll = cc->MultiAddEvalMultKeys(evalMultKeyskAll, PrevevalMixMultKey,
												mynode.get_FinalPubKey()->GetKeyTag());


        elapsed_time = TOC_MS(t);
    
        if (debug_flag_val)
            std::cout << "Eval Mult key generation compute current evalmultall time: " << elapsed_time << " ms\n";

		if (NodeName == startNode) {
            TIC(t);
			mynode.broadcastSerialMsg(evalMultAddskAll, Msgsent, "evalmixmultkey");
            auto elapsed_time = TOC_MS(t);
    
            if (debug_flag_val)
                std::cout << "Eval Mult key generation broadcasting final evalmultfinal time: " << elapsed_time << " ms\n";
            mynode.set_evalMultFinal(evalMultAddskAll);
			return evalMultAddskAll;
		} else {
            TIC(t);
			mynode.sendSerialMsg(evalMultAddskAll, Msgsent, "evalmixmultkey", usendtoNode);
            auto elapsed_time = TOC_MS(t);
    
            if (debug_flag_val)
                std::cout << "Eval Mult key generation sending evalmixmult to prev node time: " << elapsed_time << " ms\n";
		}
	}

    TIC(t);
    lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultFinal(nullptr);
	//get the final evaluation mult key if client id is not 1
	if (NodeName != startNode) {
		mynode.getSerialMsgWait(evalMultFinal, "evalmixmultkey", startNode);
	}
    
    auto elapsed_time = TOC_MS(t);
    
    if (debug_flag_val)
        std::cout << "Eval Mult key generation getting finalevalmult time: " << elapsed_time << " ms\n";
        
    mynode.set_evalMultFinal(evalMultFinal);
    return evalMultFinal;
}


std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> JointEvalSumkeyGen(NodeImpl& mynode, std::string& NodeName, std::string& startNode, std::string& endNode, std::vector<std::string>& nodes_path, message_format& Msgsent, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc) {
    
    std::string dsendtoNode = nodes_path[0];
    std::string dgetfromNode = nodes_path[1];
    std::string usendtoNode = nodes_path[2];
    std::string ugetfromNode = nodes_path[3];

    std::ostringstream esKey0, esKey1;
	std::string serializedevalSumFinalKey;
    //generate evalSumKey
    if (NodeName == startNode) {
        // Generate evalmult key part for A
        cc->EvalSumKeyGen(mynode.get_keyPair().secretKey);
        auto evalSumKeys = std::make_shared<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>>(cc->GetEvalSumKeyMap(mynode.get_keyPair().secretKey->GetKeyTag()));	
    
        //send evalSumkeys to server with round number (same as client number)
        mynode.sendSerialMsg(evalSumKeys, Msgsent, "evalSumKeys0", dsendtoNode);
        mynode.sendSerialMsg(evalSumKeys, Msgsent, "evalSumKeys1", dsendtoNode);

    } else {

        //get previous eval multkey
        std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> PrevevalSumKey0, PrevevalSumKey1;
        mynode.getSerialMsgWait(PrevevalSumKey0,"evalSumKeys0", dgetfromNode);
        mynode.getSerialMsgWait(PrevevalSumKey1,"evalSumKeys1", dgetfromNode);
        
        //compute evalmultkey for this client
        auto evalSumKeys0 = cc->MultiEvalSumKeyGen(mynode.get_keyPair().secretKey, PrevevalSumKey0, mynode.get_keyPair().publicKey->GetKeyTag());

        //compute evalSumKeys1 (adding up with prev evalsumkey)
        auto evalSumKeys1 = cc->MultiAddEvalSumKeys(PrevevalSumKey1, evalSumKeys0, mynode.get_keyPair().publicKey->GetKeyTag());

      
        //send evalSumkeys to server with round number (same as client number)       
        if (NodeName == endNode) {
            mynode.broadcastSerialMsg(evalSumKeys1, Msgsent, "evalSumKeys1");
			mynode.set_evalSumFinal(evalSumKeys1);
            return evalSumKeys1;
        } else {
			mynode.sendSerialMsg(evalSumKeys0, Msgsent, "evalSumKeys0", dsendtoNode);
            mynode.sendSerialMsg(evalSumKeys1, Msgsent, "evalSumKeys1", dsendtoNode);
		}

    } 

    std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> evalSumFinal;
    //get the final evalsums key from the last client
    if (NodeName != endNode) {
        std::cout << "\n Wait for seconds as evalsumfinal key" << std::endl;
		mynode.getSerialMsgWait(evalSumFinal,"evalSumKeys1", endNode);
    }

	mynode.set_evalSumFinal(evalSumFinal);
    return evalSumFinal;
}

lbcrypto::PrivateKey<lbcrypto::DCRTPoly> RecoverDroppedSecret(NodeImpl& mynode, usint node_id, std::vector<uint32_t>& node_nack, message_format& Msgsent, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc, std::unordered_map<uint32_t, lbcrypto::DCRTPoly>& recoveryShares, usint num_of_nodes, usint threshold) {
    //receive the threshold t number of secret shares of party i
    usint ctr = 1;
    while(ctr < threshold) {
        for(usint j = 1; j<=num_of_nodes; j++) {
            if ((j != node_id) && (std::find(node_nack.begin(), node_nack.end(), j) == node_nack.end())) { 
                std::string nodeGet = "Node" + std::to_string(j);
                    
                lbcrypto::DCRTPoly secretshareRecover;
                mynode.getSerialMsgWait(secretshareRecover, "secretsharerecovery", nodeGet);
                recoveryShares[j] = secretshareRecover;
                ctr++;
            }
        }
    }
        
    //recover the party i's secret key 
    lbcrypto::PrivateKey<lbcrypto::DCRTPoly> recovered_sk = std::make_shared<lbcrypto::PrivateKeyImpl<lbcrypto::DCRTPoly>>(cc);
    cc->RecoverSharedKey(recovered_sk, recoveryShares, num_of_nodes, threshold, "shamir"); //share_type - shamir or additive (should be the same used in ShareKeys in thresh_aborts_client.cpp)
    return recovered_sk;
}

std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> SendRecoveredPartialCT(NodeImpl& mynode, usint node_id, std::vector<uint32_t>& node_nack, message_format& Msgsent, usint num_of_nodes, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>& resultCT, lbcrypto::PrivateKey<lbcrypto::DCRTPoly>& recovered_sk) {
    //compute partial decryption share with recovered secret key
    auto ciphertextPartial1 = cc->MultipartyDecryptMain({resultCT}, recovered_sk);

    //send AbortsciphertextPartial to other clients via the server
    for (usint j = 1; j <= num_of_nodes; j++) {
        if ((node_id != j) && (std::find(node_nack.begin(), node_nack.end(), j)==node_nack.end())) {
            std::string nodeSend = "Node" + std::to_string(j);
            mynode.sendSerialMsg(ciphertextPartial1, Msgsent, "abortspartialciphertext", nodeSend, 5, 10);
        }
    }
    return ciphertextPartial1;
}
