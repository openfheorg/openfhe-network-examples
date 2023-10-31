// @file pre.cpp - Example of Proxy Re-Encryption.
// @author TPOC: contact@palisade-crypto.org
//
// @copyright Copyright (c) 2019, New Jersey Institute of Technology (NJIT)
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
//
// @section DESCRIPTION
// Demo software for multiparty proxy reencryption operations for three different security modes -
// IND_CPA_SECURE_PARAMS, BOUNDED_HRA_SECURE_PARAMS, PROVABLE_HRA_SECURE_PARAMS.
// Default parameters set for each security mode is chosen using the multihop_params.py script 
// to allow upto a minimum of 10 hops.

#include <chrono>
#include <fstream>
#include <iostream> 
#include <iterator>
#include <getopt.h>
#include "openfhe.h"
#include "scheme/bgvrns/cryptocontext-bgvrns.h"
#include "gen-cryptocontext.h"
#include "utils/serial.h"
#include "key/key-ser.h"
#include "cryptocontext-ser.h"
#include "scheme/bgvrns/bgvrns-ser.h"

using namespace std;
using namespace lbcrypto;

int num_of_hops=1; //number of hops
usint security_model = 0; //0 - CPA secure PRE, 1 - fixed 20 bits noise, 2 - provable secure HRA noise flooding with BV switching, 3 - provable secure HRA noise flooding with Hybrid switching
const auto GlobalSerializationType = lbcrypto::SerType::BINARY;
//int run_demo_pre(int argc, char *argv[]);

void usage() {
  std::cout << "-m security model (0 CPA secure PRE, 1 Fixed 20 bits noise, 2 Provable secure HRA BV switching, 3 Provable secure HRA Hybrid switching)"
            << "-d number of hops"
            << std::endl;
}

int main(int argc, char *argv[]) {
  // Generate parameters.
  double diff, reencdiff, rekeydiff;
  
  char opt(0);
  static struct option long_options[] =
  {
      {"Security model",       required_argument, NULL, 'm'},
      {"Number of hops",       required_argument, NULL, 'd'},
      {"help",                 no_argument,       NULL, 'h'},
      {NULL, 0, NULL, 0}
  };

  const char* optstring = "m:d:h";
  while ((opt = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
      std::cerr << "opt1: " << opt << "; optarg: " << optarg << std::endl;
      switch (opt) {
      case 'm':
          security_model = atoi(optarg);
          break;
      case 'd':
          num_of_hops = atoi(optarg);
          break;
      case 'h':
          usage();
      default:
          return false;
      }
  }
  
  TimeVar t;
  //Default set to IND-CPA parameters
  //The parameters ring_dimension, relinWindow, qmodulus are chosen for a plaintext modulus of 2 to allow upto 10 hops 
  //without breaking decryption satisfying theoretical constraint inequalities using the python script multihop_params.py
  //changing the plaintext modulus would require running the python script to update these other parameters for correctness.
  lbcrypto::CCParams<lbcrypto::CryptoContextBGVRNS> parameters;

  int plaintextModulus = 2;
  uint32_t multDepth = 0;
  double sigma = 3.2;		
  SecurityLevel securityLevel = HEStd_128_classic;
  usint ringDimension = 1024;
  usint digitSize = 3;
  usint dcrtbits = 0;

  usint qmodulus = 27;
  usint firstqmod = 27;
  parameters.SetPREMode(INDCPA);

  //0 - CPA secure PRE, 1 - fixed 20 bits noise, 2 - provable secure HRA noise flooding with BV switching, 3 - provable secure HRA noise flooding with Hybrid switching
  if (security_model == 0) {
      plaintextModulus = 2;
      multDepth = 0;
      sigma = 3.2;		
      securityLevel = HEStd_128_classic;
      ringDimension = 1024;
      digitSize = 3;
      dcrtbits = 0;

      qmodulus = 27;
      firstqmod = 27;
      security_model = 0;
      parameters.SetPREMode(INDCPA);
      parameters.SetKeySwitchTechnique(BV);
  } else if (security_model == 1) {
      plaintextModulus = 2;
      multDepth = 0;
      sigma = 3.2;		
      securityLevel = HEStd_128_classic;
      ringDimension = 2048;
      digitSize = 18;
      dcrtbits = 0;

      qmodulus = 54;
      firstqmod = 54;
      security_model = 1; 
      parameters.SetPREMode(FIXED_NOISE_HRA);
      parameters.SetKeySwitchTechnique(BV);
  } else if (security_model == 2) {
      plaintextModulus = 2;
      ringDimension = 16384;
      digitSize = 1;
      dcrtbits = 30;

      qmodulus = 438;
      firstqmod = 60;
      security_model = 2;
      parameters.SetPREMode(NOISE_FLOODING_HRA);
      parameters.SetKeySwitchTechnique(BV);
  } else if (security_model == 3) {
      plaintextModulus = 2;
      ringDimension = 16384;
      digitSize = 0;
      dcrtbits = 30;

      qmodulus = 438;
      firstqmod = 60;
      uint32_t dnum = 3;
      parameters.SetPREMode(NOISE_FLOODING_HRA);
      parameters.SetKeySwitchTechnique(HYBRID);
      parameters.SetNumLargeDigits(dnum);
    }

  parameters.SetMultiplicativeDepth(multDepth);
  parameters.SetPlaintextModulus(plaintextModulus);
	parameters.SetSecurityLevel(securityLevel);
	parameters.SetStandardDeviation(sigma);
  parameters.SetSecretKeyDist(UNIFORM_TERNARY);
  parameters.SetRingDim(ringDimension);
  parameters.SetFirstModSize(firstqmod);
  parameters.SetScalingModSize(dcrtbits);
  parameters.SetDigitSize(digitSize);
  parameters.SetScalingTechnique(FIXEDMANUAL);
  parameters.SetMultiHopModSize(qmodulus);
  parameters.SetStatisticalSecurity(48);
  parameters.SetNumAdversarialQueries(262144); //2^18

  lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cryptoContext = GenCryptoContext(parameters);
        
	// Enable features that you wish to use
  cryptoContext->Enable(PKE);
  cryptoContext->Enable(KEYSWITCH);
  cryptoContext->Enable(LEVELEDSHE);
  cryptoContext->Enable(PRE);


  std::cout << "p = "
            << cryptoContext->GetCryptoParameters()->GetPlaintextModulus()
            << std::endl;
  std::cout << "n = "
            << cryptoContext->GetCryptoParameters()
                       ->GetElementParams()
                       ->GetCyclotomicOrder() /
                   2
            << std::endl;
  std::cout << "log2 q = "
            << log2(cryptoContext->GetCryptoParameters()
                        ->GetElementParams()
                        ->GetModulus()
                        .ConvertToDouble())
            << std::endl;
  std::cout << "r = " << cryptoContext->GetCryptoParameters()->GetDigitSize()
            << std::endl;

  ////////////////////////////////////////////////////////////
  // Perform Key Generation Operation
  ////////////////////////////////////////////////////////////

  // Initialize Key Pair Containers
  KeyPair<DCRTPoly> keyPair1;

  std::cout << "\nRunning key generation (used for source data)..."
            << std::endl;

  TIC(t);
  keyPair1 = cryptoContext->KeyGen();

  auto keygendiff = TOC_US(t);

  std::ostringstream osp;
	lbcrypto::Serial::Serialize(keyPair1.publicKey, osp, GlobalSerializationType);
    std::cout << "Public key size " << osp.str().size() << std::endl;

  for (auto ix = 0U; ix < 10U; ix++){
   TIC(t);

   keyPair1 = cryptoContext->KeyGen();

   diff = TOC_US(t);

   cout << ix <<"th keygen time: " 
       << "\t" << diff << " microseconds" << endl;

  }

  if (!keyPair1.good()) {
    std::cout << "Key generation failed!" << std::endl;
    exit(1);
  }

  ////////////////////////////////////////////////////////////
  // Encode source data
  ////////////////////////////////////////////////////////////
  vector<int64_t> vectorOfInts;
  unsigned int nShort=0;
  int ringsize=0;
  ringsize = cryptoContext->GetRingDimension();
  nShort = ringsize;


  for (size_t i = 0; i < nShort; i++){ //generate a random array of shorts
		  if(plaintextModulus==2){
          vectorOfInts.push_back(std::rand() % plaintextModulus);
      }
      else{
          vectorOfInts.push_back((std::rand() % plaintextModulus) - (std::floor(plaintextModulus/2)-1));
      }
		}

  Plaintext plaintext = cryptoContext->MakeCoefPackedPlaintext(vectorOfInts);
  ////////////////////////////////////////////////////////////
  // Encryption
  ////////////////////////////////////////////////////////////

  TIC(t);
  auto ciphertext1 = cryptoContext->Encrypt(keyPair1.publicKey, plaintext);

  auto encdiff = TOC_US(t);

  std::ostringstream osc;
	lbcrypto::Serial::Serialize(ciphertext1, osc, GlobalSerializationType);
  std::cout << "ciphertext size " << osc.str().size() << std::endl;

  for (auto ix = 0U; ix < 10U; ix++){
   TIC(t);

   auto ciphertext1 = cryptoContext->Encrypt(keyPair1.publicKey, plaintext);

   diff = TOC_US(t);
   cout << ix <<"th encryption time: " 
       << "\t" << diff << " microseconds" << endl;

  }
  ////////////////////////////////////////////////////////////
  // Decryption of Ciphertext
  ////////////////////////////////////////////////////////////

  Plaintext plaintextDec1;

  TIC(t);

  cryptoContext->Decrypt(keyPair1.secretKey, ciphertext1, &plaintextDec1);

  auto decbfdiff = TOC_US(t);
  for (auto ix = 0U; ix < 10U; ix++){
   TIC(t);

   cryptoContext->Decrypt(keyPair1.secretKey, ciphertext1, &plaintextDec1);

   diff = TOC_US(t);
   cout << ix <<"th decryption time: " 
       << "\t" << diff << " microseconds" << endl;

  }

  plaintextDec1->SetLength(plaintext->GetLength());

  Ciphertext<DCRTPoly> reEncryptedCT1, reEncryptedCT;
  Plaintext plaintextDec;

  //multiple hop
  vector<KeyPair<DCRTPoly>> keyPairs;
  vector<Ciphertext<DCRTPoly>> reEncryptedCTs;

  keyPairs.push_back(keyPair1);
  reEncryptedCTs.push_back(ciphertext1);
  
  for(int i=0;i<num_of_hops;i++){
    std::cout << "hop" << i << std::endl;
    auto keyPair = cryptoContext->KeyGen();
    keyPairs.push_back(keyPair);

    lbcrypto::EvalKey<DCRTPoly> reencryptionKey;
    TIC(t);
    reencryptionKey =
        cryptoContext->ReKeyGen(keyPairs[i].secretKey, keyPairs[i+1].publicKey);

    rekeydiff = TOC_US(t);

    //measure the size of reencryption key
    std::ostringstream os;
		lbcrypto::Serial::Serialize(reencryptionKey, os, GlobalSerializationType);
    std::cout << "Re-encryption key size " << os.str().size() << std::endl;

    if (security_model == 0) {
      std::cout << "CPA secure PRE" << std::endl;
      TIC(t);
      reEncryptedCT = cryptoContext->ReEncrypt(reEncryptedCTs[i], reencryptionKey, keyPairs[i].publicKey); //IND-CPA secure
      reencdiff = TOC_US(t);
    }
    else if (security_model == 1) {
      std::cout << "Fixed noise (20 bits) practically secure PRE" << std::endl;
      TIC(t);
      reEncryptedCT = cryptoContext->ReEncrypt(reEncryptedCTs[i], reencryptionKey, keyPairs[i].publicKey); //fixed bits noise HRA secure
      reencdiff = TOC_US(t);
    } else if ((security_model == 2) || (security_model == 3)) {
      std::cout << "Provable HRA secure PRE";
	  if (security_model == 2) {
		std::cout << " BV Key switching";
	  } else {
		std::cout << " Hybrid Key switching";
	  }
	  std::cout << std::endl;
      TIC(t);
      reEncryptedCT1 = cryptoContext->ReEncrypt(reEncryptedCTs[i], reencryptionKey, keyPairs[i].publicKey); //HRA secure noiseflooding
      reEncryptedCT = cryptoContext->ModReduce(reEncryptedCT1); //mod reduction for noise flooding
      reencdiff = TOC_US(t);
    } else {
      std::cerr << "Not a valid security mode" << std::endl;
      exit(EXIT_FAILURE);
    }
    
    std::ostringstream osr;
	  lbcrypto::Serial::Serialize(reEncryptedCT, osr, GlobalSerializationType);
    std::cout << "Reencrypted ciphertext size " << osr.str().size() << std::endl;


    cout << "Key generation time, Encryption time, Decryption before time, ReKey generation time, Re-Encryption generation time" << endl;
    cout << keygendiff << ", " << encdiff << ", " << decbfdiff << ", " << rekeydiff << ", " << reencdiff << endl;

    reEncryptedCTs.push_back(reEncryptedCT);
  }

  int kp_size_vec, ct_size_vec;
  kp_size_vec = keyPairs.size();
  ct_size_vec = reEncryptedCTs.size();
  TIC(t);
  cryptoContext->Decrypt(keyPairs[kp_size_vec-1].secretKey, reEncryptedCTs[ct_size_vec-1], &plaintextDec);  

  auto decaftdiff = TOC_US(t);

  cout << "Decryption generation after re-encryption time" << endl;
  cout << decaftdiff << endl;

  for (auto ix = 0U; ix < 11U; ix++){
   TIC(t);
   cryptoContext->Decrypt(keyPairs[kp_size_vec-1].secretKey, reEncryptedCTs[ct_size_vec-1], &plaintextDec);

   diff = TOC_US(t);
   cout << ix <<"th Decryption after reenc time: " 
       << "\t" << diff << " microseconds" << endl;

  }

  cout << "\n";
  //##########################################################

  vector<int64_t> unpackedPT, unpackedDecPT;
  unpackedPT = plaintextDec1->GetCoefPackedValue();
  unpackedDecPT = plaintextDec->GetCoefPackedValue();
  for (unsigned int j = 0; j < unpackedPT.size(); j++) {
		  if (unpackedPT[j] != unpackedDecPT[j]) {
      std::cout << "Decryption failure" << std::endl;
			std::cout << j << ", " << unpackedPT[j] << ", "
					  << unpackedDecPT[j] << std::endl;
		  }
		}

  std::cout << "Execution Completed." << std::endl;

  return 0;
}
