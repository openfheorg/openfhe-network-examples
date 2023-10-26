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

#include <iostream>

#include "utils/exception.h"
#include "utils/debug.h"
#include "node.h"


lbcrypto::PublicKey<lbcrypto::DCRTPoly> JointPublickeyGen(NodeImpl& mynode, std::string& NodeName, std::string& startNode, std::string& endNode, std::vector<std::string>& nodes_path, message_format& Msgsent, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc);

lbcrypto::EvalKey<lbcrypto::DCRTPoly> JointEvalMultkeyGen(NodeImpl& mynode, std::string& NodeName, std::string& startNode, std::string& endNode, std::vector<std::string>& nodes_path, message_format& Msgsent, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc);

std::shared_ptr<std::map<usint, lbcrypto::EvalKey<lbcrypto::DCRTPoly>>> JointEvalSumkeyGen(NodeImpl& mynode, std::string& NodeName, std::string& startNode, std::string& endNode, std::vector<std::string>& nodes_path, message_format& Msgsent, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc);

lbcrypto::PrivateKey<lbcrypto::DCRTPoly> RecoverDroppedSecret(NodeImpl& mynode, usint node_id, std::vector<uint32_t>& node_nack, message_format& Msgsent, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc, std::unordered_map<uint32_t, lbcrypto::DCRTPoly>& recoveryShares, usint num_of_nodes, usint threshold);

std::vector<lbcrypto::Ciphertext<lbcrypto::DCRTPoly>> SendRecoveredPartialCT(NodeImpl& mynode, usint node_id, std::vector<uint32_t>& node_nack, message_format& Msgsent, usint num_of_nodes, lbcrypto::CryptoContext<lbcrypto::DCRTPoly>& cc, lbcrypto::Ciphertext<lbcrypto::DCRTPoly>& resultCT, lbcrypto::PrivateKey<lbcrypto::DCRTPoly>& recovered_sk);
                                    