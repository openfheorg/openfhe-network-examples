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
#include "gen-cryptocontext.h"
#include "utils/serial.h"
#include "key/key-ser.h"
#include "cryptocontext-ser.h"
#include "scheme/bgvrns/bgvrns-ser.h"

using namespace std;
using namespace lbcrypto;

constexpr auto GlobalSerializationType = SerType::BINARY;
constexpr PlaintextModulus plaintextModulus = 2;

uint32_t num_of_hops    = 5;  // number of hops
uint32_t security_model = 0;  // 0 - CPA secure PRE
                              // 1 - fixed 20 bits noise
                              // 2 - provable secure HRA noise flooding with BV switching
                              // 3 - provable secure HRA noise flooding with Hybrid switching

void usage() {
    std::cout << "-m security model (0 CPA secure PRE, 1 Fixed 20 bits noise, 2 Provable secure HRA BV switching, 3 Provable secure HRA Hybrid switching)"
              << "-d number of hops"
              << std::endl;
}

int main(int argc, char *argv[]) {
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
            return 0;
        }
    }

    // Generate parameters
    CCParams<CryptoContextBGVRNS> parameters;
    uint32_t ringDimension;
    uint32_t digitSize;

    if (security_model == 0) {
        ringDimension = 2048;
        digitSize = 3;
        parameters.SetPREMode(INDCPA);
        parameters.SetKeySwitchTechnique(BV);
        parameters.SetFirstModSize(27);
    } else if (security_model == 1) {
        ringDimension = 4096;
        digitSize = 16;
        parameters.SetPREMode(FIXED_NOISE_HRA);
        parameters.SetKeySwitchTechnique(BV);
        parameters.SetFirstModSize(54);
    } else if (security_model == 2) {
        ringDimension = 32768;
        digitSize = 10;
        parameters.SetPREMode(NOISE_FLOODING_HRA);
        parameters.SetKeySwitchTechnique(BV);
        parameters.SetPRENumHops(num_of_hops);
        parameters.SetStatisticalSecurity(40);
        parameters.SetNumAdversarialQueries(1 << 20);
    } else if (security_model == 3) {
        ringDimension = 32768;
        digitSize = 0;
        parameters.SetPREMode(NOISE_FLOODING_HRA);
        parameters.SetKeySwitchTechnique(HYBRID);
        parameters.SetPRENumHops(num_of_hops);
        parameters.SetStatisticalSecurity(40);
        parameters.SetNumAdversarialQueries(1 << 20);
    } else {
        std::cerr << "Not a valid security mode" << std::endl;
        exit(EXIT_FAILURE);
    }

    parameters.SetMultiplicativeDepth(0);
    parameters.SetPlaintextModulus(plaintextModulus);
    parameters.SetRingDim(ringDimension);
    parameters.SetDigitSize(digitSize);
    parameters.SetScalingTechnique(FIXEDMANUAL);
    parameters.SetSecurityLevel(HEStd_128_quantum);

    auto cc = GenCryptoContext(parameters);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(PRE);

    std::cout << "p = " << cc->GetCryptoParameters()->GetPlaintextModulus() << std::endl;
    std::cout << "n = " << cc->GetCryptoParameters()->GetElementParams()->GetCyclotomicOrder() / 2 << std::endl;
    std::cout << "log2 q = " << log2(cc->GetCryptoParameters()->GetElementParams()->GetModulus().ConvertToDouble()) << std::endl;
    std::cout << "r = " << cc->GetCryptoParameters()->GetDigitSize() << std::endl;

    ////////////////////////////////////////////////////////////
    // Perform Key Generation Operation
    ////////////////////////////////////////////////////////////

    // Initialize Key Pair Containers
    std::cout << "\nRunning key generation (used for source data)..." << std::endl;

    TimeVar t;
    TIC(t);
    auto keyPair1 = cc->KeyGen();
    auto keygendiff = TOC_US(t);

    std::ostringstream osp;
    Serial::Serialize(keyPair1.publicKey, osp, GlobalSerializationType);
    std::cout << "Public key size " << osp.str().size() << std::endl;

    for (uint32_t ix = 0; ix < num_of_hops; ++ix) {
        TIC(t);
        keyPair1 = cc->KeyGen();
        cout << ix << "th keygen time: " << "\t" << TOC_US(t) << " microseconds" << endl;
        if (!keyPair1.good()) {
            std::cerr << "Key generation failed!" << std::endl;
            exit(1);
        }
    }


    ////////////////////////////////////////////////////////////
    // Encode source data
    ////////////////////////////////////////////////////////////

    std::vector<int64_t> vectorOfInts(ringDimension);
    for (auto& v : vectorOfInts)
        v = (std::rand() % plaintextModulus);
    auto plaintext = cc->MakeCoefPackedPlaintext(vectorOfInts);

    ////////////////////////////////////////////////////////////
    // Encryption
    ////////////////////////////////////////////////////////////

    TIC(t);
    auto ciphertext1 = cc->Encrypt(keyPair1.publicKey, plaintext);
    auto encdiff = TOC_US(t);

    std::ostringstream osc;
    Serial::Serialize(ciphertext1, osc, GlobalSerializationType);
    std::cout << "ciphertext size " << osc.str().size() << std::endl;

    for (uint32_t ix = 0; ix < num_of_hops; ++ix) {
        TIC(t);
        auto ciphertext1 = cc->Encrypt(keyPair1.publicKey, plaintext);
        cout << ix << "th encryption time: " << "\t" << TOC_US(t) << " microseconds" << endl;
    }

    ////////////////////////////////////////////////////////////
    // Decryption of Ciphertext
    ////////////////////////////////////////////////////////////

    Plaintext plaintextDec1;

    TIC(t);
    cc->Decrypt(keyPair1.secretKey, ciphertext1, &plaintextDec1);
    auto decbfdiff = TOC_US(t);

    for (uint32_t ix = 0; ix < num_of_hops; ++ix) {
        TIC(t);
        cc->Decrypt(keyPair1.secretKey, ciphertext1, &plaintextDec1);
        cout << ix << "th decryption time: " << "\t" << TOC_US(t) << " microseconds" << endl;
    }

    plaintextDec1->SetLength(plaintext->GetLength());

    Ciphertext<DCRTPoly> reEncryptedCT1, reEncryptedCT;
    Plaintext plaintextDec;

    // multiple hop
    vector<KeyPair<DCRTPoly>> keyPairs{keyPair1};
    vector<Ciphertext<DCRTPoly>> reEncryptedCTs{ciphertext1};

    for (uint32_t i = 0; i < num_of_hops; ++i) {
        std::cout << "hop" << i << std::endl;
        keyPairs.push_back(cc->KeyGen());

        TIC(t);
        auto reencryptionKey = cc->ReKeyGen(keyPairs[i].secretKey, keyPairs[i + 1].publicKey);
        auto rekeydiff = TOC_US(t);

        // measure the size of reencryption key
        std::ostringstream os;
        Serial::Serialize(reencryptionKey, os, GlobalSerializationType);
        std::cout << "Re-encryption key size " << os.str().size() << std::endl;


        TIC(t);
        switch (security_model) {
            case 0:
                std::cout << "CPA secure PRE" << std::endl;
                reEncryptedCT = cc->ReEncrypt(reEncryptedCTs[i], reencryptionKey); //IND-CPA secure
                break;
            case 1:
                std::cout << "Fixed noise (20 bits) practically secure PRE" << std::endl;
                reEncryptedCT = cc->ReEncrypt(reEncryptedCTs[i], reencryptionKey, keyPairs[i].publicKey);
                break;
            case 2:
                std::cout << "Provable HRA secure PRE with noise flooding with BV switching" << std::endl;
                reEncryptedCT = cc->ReEncrypt(reEncryptedCTs[i], reencryptionKey, keyPairs[i].publicKey);
                if (i < num_of_hops - 1)
                    reEncryptedCT = cc->ModReduce(reEncryptedCT);  // mod reduction for noise flooding
                break;
            case 3:
                std::cout << "Provable HRA secure PRE with noise flooding with Hybrid switching" << std::endl;
                reEncryptedCT = cc->ReEncrypt(reEncryptedCTs[i], reencryptionKey, keyPairs[i].publicKey);
                if (i < num_of_hops - 1)
                    reEncryptedCT = cc->ModReduce(reEncryptedCT);  // mod reduction for noise flooding
                break;
            default:
                std::cerr << "Not a valid security mode" << std::endl;
                exit(EXIT_FAILURE);

        }
        auto reencdiff = TOC_US(t);

        std::ostringstream osr;
        Serial::Serialize(reEncryptedCT, osr, GlobalSerializationType);
        std::cout << "Reencrypted ciphertext size " << osr.str().size() << std::endl;

        cout << "Key generation time, Encryption time, Decryption before time, ReKey generation time, Re-Encryption generation time" << endl;
        cout << keygendiff << ", " << encdiff << ", " << decbfdiff << ", " << rekeydiff << ", " << reencdiff << endl;

        reEncryptedCTs.push_back(reEncryptedCT);
    }

    TIC(t);
    cc->Decrypt(keyPairs.back().secretKey, reEncryptedCTs.back(), &plaintextDec);
    auto decaftdiff = TOC_US(t);

    cout << "Decryption generation after re-encryption time" << endl;
    cout << decaftdiff << endl;

    for (uint32_t ix = 0; ix < num_of_hops; ++ix) {
        TIC(t);
        cc->Decrypt(keyPairs.back().secretKey, reEncryptedCTs.back(), &plaintextDec);
        cout << ix <<"th Decryption after reenc time: " << "\t" << TOC_US(t) << " microseconds" << endl;
    }

    cout << "\n";
    //##########################################################

    // verification
    auto& unpackedPT    = plaintextDec1->GetCoefPackedValue();
    auto& unpackedDecPT = plaintextDec->GetCoefPackedValue();
    for (size_t j = 0; j < unpackedPT.size(); ++j) {
        if (unpackedPT[j] != unpackedDecPT[j]) {
            std::cout << "Decryption failure" << std::endl;
            std::cout << j << ", " << unpackedPT[j] << ", " << unpackedDecPT[j] << std::endl;
        }
    }
    std::cout << "Execution Completed." << std::endl;

    return 0;
}
