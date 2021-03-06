// Copyright (c) 2018 The CommerceBlock Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//An implementation of ECIES AES256CBC Encryption

#ifndef OCEAN_ECIES_H
#define OCEAN_ECIES_H

#include "key.h"
#include "crypto/aes.h"

typedef std::vector<unsigned char> uCharVec;

class CECIES{
public:
	CECIES();
	virtual ~CECIES();
	
  static const int KEYSIZE = AES256_KEYSIZE;
  static const int BLOCKSIZE = AES_BLOCKSIZE;
  static const int MACSIZE = CSHA256::OUTPUT_SIZE;

	/**
    * Encrypt/decrypt a message string.
    */

	bool Encrypt(uCharVec& em, 
  const uCharVec& m, const CPubKey& pubKey);
  //Use a defined priv key instead of an ephemeral one.
  bool Encrypt(uCharVec& em, 
  const uCharVec& m, const CPubKey& pubKey, const CKey& privKey);
  bool Decrypt(uCharVec& m, 
  const uCharVec& em, const CKey& privKey, const CPubKey& pubKey);
  bool Decrypt(uCharVec& m, 
  const uCharVec& em, const CKey& privKey);
  bool Encrypt(std::string& em, 
  const std::string& m, const CPubKey& pubKey);
  bool Decrypt(std::string& m, 
  const std::string& em, const CKey& privKey);

	bool OK(){return _bOK;}

  static void GetMAC(const unsigned char key[AES256_KEYSIZE], 
          const uCharVec& msg, 
          unsigned char mac[CSHA256::OUTPUT_SIZE]);

  static void GetMAC(const CPubKey& pubKey, const CKey& privKey, 
          const uCharVec& msg, 
          unsigned char mac[CSHA256::OUTPUT_SIZE]);

  static void GetK(const CPubKey& pubKey, const CKey& privKey,
          unsigned char k_enc[AES256_KEYSIZE],
          unsigned char k_mac[AES256_KEYSIZE]);

private:

	unsigned char _k_mac_encrypt[AES256_KEYSIZE];
	unsigned char _k_mac_decrypt[AES256_KEYSIZE];


	bool CheckMagic(const uCharVec& encryptedMessage) const;

	bool _bOK = false;

	void check(const CKey& privKey, const CPubKey& pubKey);

  virtual std::string Encode(const uCharVec& vch);

  virtual bool Decode(const std::string strIn, uCharVec& decoded);

	//Use the electrum wallet default "magic" string
	const uCharVec _magic{'B','I','E','1'};
};

#endif //#ifndef OCEAN_ECIES_H
