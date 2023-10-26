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

#include "path_measure_crypto_functions.h"

//OPENFHE_DEBUG_FLAG(debug_flag_val);  // set to true to turn on DEBUG() statements

//todo: this might need to take more parameters in the future depending on the cryptocontext generation requirement.
/*void InitializeandSendCC(std::string NodeName, int) {
    int plaintextModulus = 65537;

    double sigma = 3.2;
    lbcrypto::SecurityLevel securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;

    lbcrypto::EncodingParams encodingParams(new lbcrypto::EncodingParamsImpl(plaintextModulus));

    usint batchSize = 1024;
    encodingParams->SetBatchSize(batchSize);

    cc = lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::genCryptoContextBFVrns(
            encodingParams, securityLevel, sigma, 0, 4, 0, OPTIMIZED, 2, 30, 60);

    uint32_t m = cc->GetCyclotomicOrder();
    lbcrypto::PackedEncoding::SetParams(m, encodingParams);

    // enable features that you wish to use
    cc->Enable(ENCRYPTION);
    cc->Enable(SHE);
    cc->Enable(MULTIPARTY);

}*/

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
    if (NodeName == endNode) {
        mynode.sendSerialMsg(keyPair.publicKey, Msgsent, "PublicKey", usendtoNode);
	} else {
		mynode.sendSerialMsg(keyPair.publicKey, Msgsent, "PublicKey", dsendtoNode);
	}
	
	//request and receive final public key from the final client
	lbcrypto::PublicKey<lbcrypto::DCRTPoly> FinalPubKey(nullptr);
	if (NodeName != endNode) {
		std::cout << "\n Wait for final public key" << std::endl;
		if (NodeName != startNode) {
            mynode.getSerialMsgWait(FinalPubKey, "PublicKey", ugetfromNode);
			std::cout << "\n after getting final public key" << std::endl;
			mynode.sendSerialMsg(FinalPubKey, Msgsent, "PublicKey", usendtoNode);
		} else {
			mynode.getSerialMsgWait(FinalPubKey, "PublicKey", ugetfromNode);
		}
		
	} else {
		FinalPubKey = keyPair.publicKey;
	}

	mynode.set_FinalPubKey(FinalPubKey);

    return FinalPubKey;
}

lbcrypto::EvalKey<lbcrypto::DCRTPoly> JointEvalMultkeyGen(NodeImpl& mynode, std::string& NodeName, std::string& startNode, std::string& endNode, std::vector<std::string>& nodes_path, message_format& Msgsent, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc) {
    
    std::string dsendtoNode = nodes_path[0];
    std::string dgetfromNode = nodes_path[1];
    std::string usendtoNode = nodes_path[2];
    std::string ugetfromNode = nodes_path[3];

    if (NodeName == startNode) {
		// Generate evalmult key part for A
		auto evalMultKey = cc->KeySwitchGen(mynode.get_keyPair().secretKey, mynode.get_keyPair().secretKey);	
	
		//send evalMultkey to server with round number (same as client number)
		mynode.sendSerialMsg(evalMultKey, Msgsent, "evalmultkey", dsendtoNode);

	} else {
		//get previous eval multkey
		std::cout << "\n Wait for previous client's evalmult key" << std::endl;

        lbcrypto::EvalKey<lbcrypto::DCRTPoly> PrevevalMultKey(nullptr);
        mynode.getSerialMsgWait(PrevevalMultKey,"evalmultkey", dgetfromNode);

		//compute evalmultkey for this client
		auto evalMultKey = cc->MultiKeySwitchGen(mynode.get_keyPair().secretKey, mynode.get_keyPair().secretKey, PrevevalMultKey);

		//compute evalmultAB/evalmultABC (adding up with prev evalmultkey)
		auto evalMultPrevandCurrent = cc->MultiAddEvalKeys(PrevevalMultKey, evalMultKey, mynode.get_keyPair().publicKey->GetKeyTag());

		if (NodeName == endNode) {
			mynode.sendSerialMsg(evalMultPrevandCurrent, Msgsent, "evalmultkey", usendtoNode);
			auto evalMultskAll = cc->MultiMultEvalKey(mynode.get_keyPair().secretKey, evalMultPrevandCurrent,
											mynode.get_FinalPubKey()->GetKeyTag());


			//std::ostringstream evmmKey;
			mynode.sendSerialMsg(evalMultskAll, Msgsent, "evalmixmultkey", usendtoNode);
		} else {
			mynode.sendSerialMsg(evalMultPrevandCurrent, Msgsent, "evalmultkey", dsendtoNode);
		}

	}
	
	//joint evalmult key generation in the direction Node2 -> Node1
	if (NodeName != endNode) {

		//request and receive the evalmultAll key and evalmult computed from all parties
		lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultAll(nullptr), PrevevalMixMultKey (nullptr);

		std::cout << "\n Wait for evalmultall key" << std::endl;
        mynode.getSerialMsgWait(evalMultAll, "evalmultkey", ugetfromNode);
        
		//send evalmultall key to upstream party (in the threshnet example, this is sent to every party by the final party)
		if (NodeName != startNode) {
            mynode.sendSerialMsg(evalMultAll, Msgsent, "evalmultkey", usendtoNode);
		}

		std::cerr << "\n Wait for PrevevalMixMultKey" << std::endl;
		mynode.getSerialMsgWait(PrevevalMixMultKey, "evalmixmultkey", ugetfromNode);

		//Add evalmultkeys to compute final evalmultkey
		auto evalMultKeyskAll = cc->MultiMultEvalKey(mynode.get_keyPair().secretKey, evalMultAll,
										mynode.get_FinalPubKey()->GetKeyTag());
		
		auto evalMultAddskAll = cc->MultiAddEvalMultKeys(evalMultKeyskAll, PrevevalMixMultKey,
												mynode.get_FinalPubKey()->GetKeyTag());


		if (NodeName == startNode) {
			mynode.sendSerialMsg(evalMultAddskAll, Msgsent, "evalmixmultkey", dsendtoNode);
            mynode.set_evalMultFinal(evalMultAddskAll);
			return evalMultAddskAll;
		} else {
			mynode.sendSerialMsg(evalMultAddskAll, Msgsent, "evalmixmultkey", usendtoNode);
		}
	}

    lbcrypto::EvalKey<lbcrypto::DCRTPoly> evalMultFinal(nullptr);
	//get the final evaluation mult key if client id is not 1
	if (NodeName != startNode) {
		std::cout << "\n Wait for evalmultfinal key" << std::endl;
		if (NodeName != endNode) {
			mynode.getSerialMsgWait(evalMultFinal, "evalmixmultkey", dgetfromNode);
			mynode.sendSerialMsg(evalMultFinal, Msgsent, "evalmixmultkey", dsendtoNode);
		} else {
            mynode.getSerialMsgWait(evalMultFinal, "evalmixmultkey", dgetfromNode);
		}
	}

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
            mynode.sendSerialMsg(evalSumKeys1, Msgsent, "evalSumKeys1", usendtoNode);
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

		if (NodeName != startNode) {
			mynode.getSerialMsgWait(evalSumFinal,"evalSumKeys1", ugetfromNode);
			mynode.sendSerialMsg(evalSumFinal, Msgsent, "evalSumKeys1", usendtoNode);
		} else {
			mynode.getSerialMsgWait(evalSumFinal,"evalSumKeys1", ugetfromNode);
		}
    }

	mynode.set_evalSumFinal(evalSumFinal);
    return evalSumFinal;
}