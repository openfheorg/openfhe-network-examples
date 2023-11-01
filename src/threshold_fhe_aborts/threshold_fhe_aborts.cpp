// @file  threshold-fhe.cpp - Examples of threshold FHE for BGVrns, BFVrns, and
// CKKS
// @author TPOC: contact@palisade-crypto.org
//
// @copyright Copyright (c) 2020, Duality Technologies Inc.
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

#include "gen-cryptocontext.h"
#include "cryptocontext-ser.h"

#include "key/key.h"
#include "key/key-ser.h"

#include "scheme/bfvrns/bfvrns-ser.h"
#include "scheme/bfvrns/cryptocontext-bfvrns.h"

#include "utils/serial.h"
#include "utils/exception.h"

using namespace std;
using namespace lbcrypto;

void RunBFVrns();

int main(int argc, char *argv[]) {

  std::cout << "\n=================RUNNING FOR BFVrns====================="
            << std::endl;

  RunBFVrns();

  return 0;
}

void RunBFVrns() {
  TimeVar t;
  
  TIC(t);
  
  int plaintextModulus = 65537;
  double sigma = 3.2;
  lbcrypto::SecurityLevel securityLevel = lbcrypto::SecurityLevel::HEStd_128_classic;

  usint batchSize = 1024;
  usint multDepth = 2;
  usint digitSize = 30;
  usint dcrtBits = 60;

  lbcrypto::CCParams<lbcrypto::CryptoContextBFVRNS> parameters;		

  parameters.SetPlaintextModulus(plaintextModulus);
  parameters.SetSecurityLevel(securityLevel);
  parameters.SetStandardDeviation(sigma);
  parameters.SetSecretKeyDist(UNIFORM_TERNARY);
  parameters.SetMultiplicativeDepth(multDepth);
  parameters.SetBatchSize(batchSize);
  parameters.SetDigitSize(digitSize);
  parameters.SetScalingModSize(dcrtBits);

  lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc = GenCryptoContext(parameters);

  cc->Enable(PKE);
  cc->Enable(LEVELEDSHE);
  cc->Enable(ADVANCEDSHE);
  cc->Enable(MULTIPARTY);
  
  auto elapsed_seconds = TOC_MS(t);

  std::cout << "Cryptocontext generation time: " << elapsed_seconds << " ms\n";
  ////////////////////////////////////////////////////////////
  // Set-up of parameters
  ////////////////////////////////////////////////////////////

  // Output the generated parameters
  std::cout << "p = " << cc->GetCryptoParameters()->GetPlaintextModulus()
            << std::endl;
  std::cout
      << "n = "
      << cc->GetCryptoParameters()->GetElementParams()->GetCyclotomicOrder() / 2
      << std::endl;
  std::cout << "log2 q = "
            << log2(cc->GetCryptoParameters()
                        ->GetElementParams()
                        ->GetModulus()
                        .ConvertToDouble())
            << std::endl;

  // Initialize Public Key Containers for two parties A and B
  KeyPair<DCRTPoly> kp1;
  KeyPair<DCRTPoly> kp2;
  KeyPair<DCRTPoly> kp3;
  KeyPair<DCRTPoly> kpMultiparty;

  ////////////////////////////////////////////////////////////
  // Perform Key Generation Operation
  ////////////////////////////////////////////////////////////

  std::cout << "Running public key generation (used for source data)..." << std::endl;
  std::cout << "Round 1 (party A) started." << std::endl;
  
  TIC(t);
  kp1 = cc->KeyGen();

  kp2 = cc->MultipartyKeyGen(kp1.publicKey);
  
  kp3 = cc->MultipartyKeyGen(kp2.publicKey);

  elapsed_seconds = TOC_MS(t);

  std::cout << "Joint Public Key generation time: " << elapsed_seconds << " ms\n";

  std::cout << "Round 3 of publickey generation completed." << std::endl;

  TIC(t);

  usint N = 3;
  std::string share_type = "shamir"; // two types - shamir and additive
  //usint tadd = 2;
  usint tshamir = 2;
  usint thresh = tshamir;

  auto kp1smap = cc->ShareKeys(kp1.secretKey, N, thresh, 1, share_type);
  
  auto kp2smap = cc->ShareKeys(kp2.secretKey, N, thresh, 2, share_type);
  
  auto kp3smap = cc->ShareKeys(kp3.secretKey, N, thresh, 3, share_type);

  elapsed_seconds = TOC_MS(t);

  std::cout << "Aborts secret shares generation time: " << elapsed_seconds << " ms\n";
  //-------------------------------------------------
  
  TIC(t);

  // Generate evalmult key part for A
  auto evalMultKey = cc->KeySwitchGen(kp1.secretKey, kp1.secretKey);
  
  auto evalMultKey2 =
      cc->MultiKeySwitchGen(kp2.secretKey, kp2.secretKey, evalMultKey);

  auto evalMultAB = cc->MultiAddEvalKeys(evalMultKey, evalMultKey2,
                                         kp2.publicKey->GetKeyTag());

  auto evalMultKey3 =
      cc->MultiKeySwitchGen(kp3.secretKey, kp3.secretKey, evalMultAB);//evalMultKey);

  auto evalMultABC = cc->MultiAddEvalKeys(evalMultAB, evalMultKey3,
                                         kp3.publicKey->GetKeyTag());


  auto evalMultCABC = cc->MultiMultEvalKey(kp3.secretKey, evalMultABC, 
                                          kp3.publicKey->GetKeyTag());
  
  auto evalMultBABC = cc->MultiMultEvalKey(kp2.secretKey, evalMultABC,
                                          kp3.publicKey->GetKeyTag());

  auto evalMultBCABC = cc->MultiAddEvalMultKeys(evalMultCABC, evalMultBABC,
                                          evalMultCABC->GetKeyTag());

  auto evalMultAABC = cc->MultiMultEvalKey(kp1.secretKey, evalMultABC,
                                          kp3.publicKey->GetKeyTag());

  auto evalMultFinal = cc->MultiAddEvalMultKeys(evalMultAABC, evalMultBCABC,
                                                evalMultAABC->GetKeyTag());

  cc->InsertEvalMultKey({evalMultFinal});

  elapsed_seconds = TOC_MS(t);

  std::cout << "Joint evalmult Key generation time: " << elapsed_seconds << " ms\n";
  //---------------------------------------------------
  std::cout << "Running evalsum key generation (used for source data)..." << std::endl;
  std::cout << "Round 1 (party A) started." << std::endl;
  
  TIC(t);
  // Generate evalsum key part for A
  cc->EvalSumKeyGen(kp1.secretKey);
  auto evalSumKeys = std::make_shared<std::map<usint, EvalKey<DCRTPoly>>>(
      cc->GetEvalSumKeyMap(kp1.secretKey->GetKeyTag()));

  auto evalSumKeysB = cc->MultiEvalSumKeyGen(kp2.secretKey, evalSumKeys,
                                             kp2.publicKey->GetKeyTag());

  auto evalSumKeysAB = cc->MultiAddEvalSumKeys(evalSumKeys, evalSumKeysB,
                                                 kp2.publicKey->GetKeyTag());
  

  auto evalSumKeysC = cc->MultiEvalSumKeyGen(kp3.secretKey, evalSumKeysB,
                                             kp3.publicKey->GetKeyTag());

  auto evalSumKeysJoin = cc->MultiAddEvalSumKeys(evalSumKeysC, evalSumKeysAB,
                                                 kp3.publicKey->GetKeyTag());
  
  cc->InsertEvalSumKey(evalSumKeysJoin);

  elapsed_seconds = TOC_MS(t);

  std::cout << "Joint evalsum Key generation time: " << elapsed_seconds << " ms\n";
  ////////////////////////////////////////////////////////////
  // Encode source data
  ////////////////////////////////////////////////////////////
  std::vector<int64_t> vectorOfInts1 = {1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1, 0};
  std::vector<int64_t> vectorOfInts2 = {1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0};
  std::vector<int64_t> vectorOfInts3 = {2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 0};

  Plaintext plaintext1 = cc->MakePackedPlaintext(vectorOfInts1);
  Plaintext plaintext2 = cc->MakePackedPlaintext(vectorOfInts2);
  Plaintext plaintext3 = cc->MakePackedPlaintext(vectorOfInts3);

  ////////////////////////////////////////////////////////////
  // Encryption
  ////////////////////////////////////////////////////////////

  Ciphertext<DCRTPoly> ciphertext1;
  Ciphertext<DCRTPoly> ciphertext2;
  Ciphertext<DCRTPoly> ciphertext3;
  
  TIC(t);
  ciphertext1 = cc->Encrypt(kp3.publicKey, plaintext1);
  
  elapsed_seconds = TOC_MS(t);

  std::cout << "Encryption generation time: " << elapsed_seconds << " ms\n";
  
  ciphertext2 = cc->Encrypt(kp3.publicKey, plaintext2);
  ciphertext3 = cc->Encrypt(kp3.publicKey, plaintext3);
  
  ////////////////////////////////////////////////////////////
  // Homomorphic Operations
  ////////////////////////////////////////////////////////////

  Ciphertext<DCRTPoly> ciphertextAdd12;
  Ciphertext<DCRTPoly> ciphertextAdd123;

  ciphertextAdd12 = cc->EvalAdd(ciphertext1, ciphertext2);
  ciphertextAdd123 = cc->EvalAdd(ciphertextAdd12, ciphertext3);

  auto ciphertextMult = cc->EvalMult(ciphertext1, ciphertext3);

  auto ciphertextEvalSum = cc->EvalSum(ciphertext3, batchSize);


  ////////////////////////////////////////////////////////////
  // Decryption after Accumulation Operation on Encrypted Data with Multiparty
  ////////////////////////////////////////////////////////////

  Plaintext plaintextAddNew1;
  Plaintext plaintextAddNew2;
  Plaintext plaintextAddNew3;

  DCRTPoly partialPlaintext1;
  DCRTPoly partialPlaintext2;
  DCRTPoly partialPlaintext3;

  Plaintext plaintextMultipartyNew;

  auto cryptoParams =
      kp1.secretKey->GetCryptoParameters();
  const shared_ptr<typename DCRTPoly::Params> elementParams =
      cryptoParams->GetElementParams();


  TIC(t);
  //Aborts - recovering kp1.secret key from the shares assuming party A dropped out (need a protocol to identify this)
  PrivateKey<DCRTPoly> kp1_recovered_sk = std::make_shared<PrivateKeyImpl<DCRTPoly>>(cc);

  cc->RecoverSharedKey(kp1_recovered_sk, kp1smap, N, thresh, share_type);

  elapsed_seconds = TOC_MS(t);

  std::cout << "Aborts shares recovery generation time: " << elapsed_seconds << " ms\n";
  // Distributed decryption

  TIC(t);
  // partial decryption by party A
  auto ciphertextPartial1 =
      cc->MultipartyDecryptLead({ciphertextAdd123}, kp1_recovered_sk);


  // partial decryption by party B
  auto ciphertextPartial2 =
      cc->MultipartyDecryptMain({ciphertextAdd123}, kp2.secretKey);

  // partial decryption by party C
  auto ciphertextPartial3 =
      cc->MultipartyDecryptMain({ciphertextAdd123}, kp3.secretKey);

  vector<Ciphertext<DCRTPoly>> partialCiphertextVec;
  partialCiphertextVec.push_back(ciphertextPartial1[0]);
  partialCiphertextVec.push_back(ciphertextPartial2[0]);
  partialCiphertextVec.push_back(ciphertextPartial3[0]);

  // Two partial decryptions are combined
  cc->MultipartyDecryptFusion(partialCiphertextVec, &plaintextMultipartyNew);

  elapsed_seconds = TOC_MS(t);

  std::cout << "Distributed decryption time with aborts: " << elapsed_seconds << " ms\n";

  cout << "\n Original Plaintext: \n" << endl;
  cout << plaintext1 << endl;
  cout << plaintext2 << endl;
  cout << plaintext3 << endl;

  plaintextMultipartyNew->SetLength(plaintext1->GetLength());

  cout << "\n Resulting Fused Plaintext: \n" << endl;
  cout << plaintextMultipartyNew << endl;
  cout << "\n";

  Plaintext plaintextMultipartyMult;

  TIC(t);
  ciphertextPartial1 =
      cc->MultipartyDecryptLead({ciphertextMult}, kp1.secretKey);

  ciphertextPartial2 =
      cc->MultipartyDecryptMain({ciphertextMult}, kp2.secretKey);

  ciphertextPartial3 =
      cc->MultipartyDecryptMain({ciphertextMult}, kp3.secretKey);

  vector<Ciphertext<DCRTPoly>> partialCiphertextVecMult;
  partialCiphertextVecMult.push_back(ciphertextPartial1[0]);
  partialCiphertextVecMult.push_back(ciphertextPartial2[0]);
  partialCiphertextVecMult.push_back(ciphertextPartial3[0]);

  cc->MultipartyDecryptFusion(partialCiphertextVecMult,
                              &plaintextMultipartyMult);

  elapsed_seconds = TOC_MS(t);

  std::cout << "Distributed decryption time without aborts: " << elapsed_seconds << " ms\n";

  plaintextMultipartyMult->SetLength(plaintext1->GetLength());

  cout << "\n Resulting Fused Plaintext after Multiplication of plaintexts 1 "
          "and 3: \n"
       << endl;
  cout << plaintextMultipartyMult << endl;

  cout << "\n";

  Plaintext plaintextMultipartyEvalSum;

  ciphertextPartial1 =
      cc->MultipartyDecryptLead({ciphertextEvalSum}, kp1_recovered_sk);

  ciphertextPartial2 =
      cc->MultipartyDecryptMain({ciphertextEvalSum}, kp2.secretKey);

  ciphertextPartial3 =
      cc->MultipartyDecryptMain({ciphertextEvalSum}, kp3.secretKey);

  vector<Ciphertext<DCRTPoly>> partialCiphertextVecEvalSum;
  partialCiphertextVecEvalSum.push_back(ciphertextPartial1[0]);
  partialCiphertextVecEvalSum.push_back(ciphertextPartial2[0]);
  partialCiphertextVecEvalSum.push_back(ciphertextPartial3[0]);

  cc->MultipartyDecryptFusion(partialCiphertextVecEvalSum,
                              &plaintextMultipartyEvalSum);

  plaintextMultipartyEvalSum->SetLength(plaintext1->GetLength());

  cout << "\n Fused result after summation of ciphertext 3: \n" << endl;
  cout << plaintextMultipartyEvalSum << endl;

}
