// Copyright (c) 2018 The CommerceBlock Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//An implementation of ECIES AES256CBC Encryption

#include "ecies.h"
#include "random.h"
#include "utilstrencodings.h"
#include "crypto/sha1.h"
#include "crypto/sha512.h"
#include "crypto/hmac_sha256.h"
#include <sstream>
#include <iostream>


//Keys are randomly selected.
CECIES::CECIES(){
}

void CECIES::check(const CKey& privKey, const CPubKey& pubKey){
  _bOK=false;
  if(!privKey.GetPubKey().IsFullyValid()) return;
  if(!pubKey.IsFullyValid()) return;
  _bOK=true;
}

CECIES::~CECIES(){
}


bool CECIES::CheckMagic(const uCharVec& encryptedMessage) const{
	uCharVec magic(encryptedMessage.begin(), encryptedMessage.begin() + _magic.size());
	return (magic == _magic);
}

std::string CECIES::Encode(const uCharVec& vch){
	return EncodeBase64(&vch[0], vch.size());
}

bool CECIES::Decode(const std::string strIn, uCharVec& decoded){
	bool bInvalid;
	decoded=DecodeBase64(strIn.c_str(), &bInvalid);
	return !bInvalid;
}
	
//Encryption: generate ephmeral private key, and include it's public key in the header.
//Generate a dhared secret using the ephemeral private key and the recipient's public key.
bool CECIES::Encrypt(uCharVec& em, 
 	const uCharVec& m, const CPubKey& pubKey){
	CKey ephemeralKey;	
	ephemeralKey.MakeNewKey(true);
	return Encrypt(em, m, pubKey, ephemeralKey);
}

//Encryption: use the specified privKey as the "ephemeral" private key (the same 
//privKey should only be used for ecnryption once).
//Generate a shared secret (ecdh_key) using the ephemeral private key and the 
//recipient's public key. The encryption key is the first 256 bytes of 
//sha512(ecdh_key), and the MAC key is the remaining 256 bytes.
//The initialization vector is the first BLOCKSIZE bytes of the SHA1 of 
//SHA512(ecdh_key)
bool CECIES::Encrypt(uCharVec& em, 
 	const uCharVec& m, const CPubKey& pubKey, const CKey& privKey){

	//Initialize the encryptor
	check(privKey, pubKey);
	if(!_bOK) return false;

	unsigned char k[KEYSIZE];

	GetK(pubKey, privKey, k, _k_mac_encrypt);

	unsigned char iv[BLOCKSIZE];
	GetStrongRandBytes(iv,BLOCKSIZE);

	AES256CBCEncrypt encryptor(k, iv, true);
		
	int size=m.size();
	uCharVec ciphertext(size+BLOCKSIZE);

	int paddedSize=encryptor.Encrypt(m.data(), size, ciphertext.data());
	ciphertext.resize(paddedSize);
	//Payload: _magic + pubkey + iv + ciphertext

	uCharVec msg(_magic.begin(), _magic.end());
	CPubKey ephemeralPub = privKey.GetPubKey();
	msg.insert(msg.end(), ephemeralPub.begin(), ephemeralPub.end());
	msg.insert(msg.end(), iv, &iv[BLOCKSIZE]);
	msg.insert(msg.end(), ciphertext.begin(), ciphertext.end());
	
	//Generate MAC
	unsigned char mac[MACSIZE];
	GetMAC(_k_mac_encrypt, msg, mac);

	//Message: payload + MAC
	msg.insert(msg.end(),std::begin(mac), std::end(mac));
	//Base64 encode
	std::string strEncoded=Encode(msg);
	em=uCharVec(strEncoded.begin(), strEncoded.end());
	return true;
}

void CECIES::GetMAC(const CPubKey& pubKey, const CKey& privKey, 
					const uCharVec& msg, 
					unsigned char mac[MACSIZE]){
	unsigned char k_mac[KEYSIZE];
	unsigned char k_enc[KEYSIZE];

	GetK(pubKey, privKey, k_enc, k_mac);
	GetMAC(k_mac, msg, mac);
}

void CECIES::GetK(const CPubKey& pubKey, const CKey& privKey, 
					unsigned char k_enc[KEYSIZE],
					unsigned char k_mac[KEYSIZE]){

	uint256 ecdh_key = privKey.ECDH(pubKey);
	
	//sha512 hash of the elliptic curve diffie-hellman key to 
	//produce an encryption key and a MAC key
	unsigned char arrKey[CSHA512::OUTPUT_SIZE];
	CSHA512().Write(ecdh_key.begin(), ecdh_key.size()).Finalize(arrKey);

	memcpy(k_enc, arrKey, KEYSIZE);
	memcpy(k_mac, arrKey+KEYSIZE, KEYSIZE);
}

void CECIES::GetMAC(const unsigned char key[KEYSIZE], 
					const uCharVec& msg, 
					unsigned char mac[MACSIZE]){
	CHMAC_SHA256(&key[0], KEYSIZE)
		.Write(msg.data(), msg.size())
		.Finalize(mac);
}

bool CECIES::Decrypt(uCharVec& m, 
				const uCharVec& em, 
				const CKey& privKey){
	CPubKey pubKey;
	return Decrypt(m, em, privKey, pubKey);
}

bool CECIES::Decrypt(uCharVec& m, 
				const uCharVec& em, 
				const CKey& privKey, 
				const CPubKey& pubKey){

	std::string sem(em.begin(), em.end());
	uCharVec decoded;
	if(!Decode(sem, decoded)) return false;
	if(!CheckMagic(decoded)) return false;
	uCharVec ciphertext;

	//Initialize decryptor
	auto it1=decoded.begin()+_magic.size();
	auto it2=it1+33;
	// ecdh shared secret from ephemeral public key (in message header) and my private key
	CPubKey ephemeralPub;
	if(pubKey == CPubKey()){
		ephemeralPub.Set(it1, it2);
	} else {
		ephemeralPub=pubKey;
	}
	check(privKey, ephemeralPub);
	if(!_bOK) return false;
	uint256 ecdh_key = privKey.ECDH(ephemeralPub);
	
	it1=it2;
	it2=it1+BLOCKSIZE;
	uCharVec iv(it1, it2);

	it1=it2;
	it2=decoded.end()-MACSIZE;

	//sha512 hash of the elliptic curve diffie-hellman key to produce an encryption key and a MAC key
	unsigned char arrKey[CSHA512::OUTPUT_SIZE];
	CSHA512().Write(ecdh_key.begin(), ecdh_key.size()).Finalize(arrKey);

	unsigned char k[KEYSIZE];
	memcpy(k, &arrKey[0], sizeof(k));
	memcpy(_k_mac_decrypt, &arrKey[0]+sizeof(k), sizeof(_k_mac_decrypt));

	//Check the message authentication code (MAC)
	uCharVec MAC_written(it2, decoded.end());
	//Generate MAC
	uCharVec payload(decoded.begin(), it2);
	unsigned char mac[MACSIZE];
	CHMAC_SHA256(&_k_mac_decrypt[0], sizeof(_k_mac_decrypt))
		.Write(&payload[0], payload.size())
		.Finalize(mac);
	uCharVec MAC_calculated(&mac[0], &mac[0]+sizeof(mac));
	if(MAC_written != MAC_calculated) return false;
	
	AES256CBCDecrypt decryptor(k, iv.data(), true);

	ciphertext=uCharVec(it1, it2);

	int paddedSize = ciphertext.size();
	m.resize(paddedSize);
	int size=decryptor.Decrypt(ciphertext.data(), paddedSize, m.data());
	//Remove the padding.
	m.resize(size);
    return true;
}

bool CECIES::Encrypt(std::string& em, 
 	const std::string& m, const CPubKey& pubKey){
	uCharVec vem;
	uCharVec vm(m.begin(), m.end());
	bool bResult=Encrypt(vem, vm, pubKey);
	if(bResult) em=std::string(vem.begin(), vem.end());
    return bResult;
}

bool CECIES::Decrypt(std::string& m, 
 	const std::string& em, const CKey& privKey){
	uCharVec vem(em.begin(), em.end());
	uCharVec vm;
	bool bResult=Decrypt(vm, vem, privKey);
	if (bResult) m=std::string(vm.begin(), vm.end());
    return bResult;
}
