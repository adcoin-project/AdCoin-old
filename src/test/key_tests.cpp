// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "base58.h"
#include "script/script.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "test/test_bitcoin.h"

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

static const std::string strSecret1     ("6vPUFainWmNG8LePSwVWbo5HvCW1PCX4tXjw1VzcSPKnfE5EoQS");
static const std::string strSecret2     ("6v7oQe3zTTgF2XUsmjaJn4DXNrTMBJRrSyiXmPYkEgCM1dZ7s8q");
static const std::string strSecret1C    ("T8eBHMqRtNpqmPi25VWqeaMwfom2gyqiASgVPCyHR3XvXeVePoV9");
static const std::string strSecret2C    ("T7T1qLYCbnkCujY36Sv1TsCH5THJ4TvhFfLJnvX7qN15KJGmRJuV");
static const CBitcoinAddress addr1 ("AZUGd3ZQDJC6bKPLzARiLX4tz5Gvbv6nes");
static const CBitcoinAddress addr2 ("AQJKPYwS7CBqcfzMjXNwMnxn989dcLQbpo");
static const CBitcoinAddress addr1C("AJzftUANCqiqzvAE3rMPeZiNduxppZAJ54");
static const CBitcoinAddress addr2C("AaVbdy8XcvNn1GPN4sA1UdMByAjeeX7uBs");


static const std::string strAddressBad("Lbi6bpMhSwp2CXkivEeUK9wzyQEFzHDfSr");


#ifdef KEY_TESTS_DUMPINFO
void dumpKeyInfo(uint256 privkey)
{
    CKey key;
    key.resize(32);
    memcpy(&secret[0], &privkey, 32);
    std::vector<unsigned char> sec;
    sec.resize(32);
    memcpy(&sec[0], &secret[0], 32);
    printf("  * secret (hex): %s\n", HexStr(sec).c_str());

    for (int nCompressed=0; nCompressed<2; nCompressed++)
    {
        bool fCompressed = nCompressed == 1;
        printf("  * %s:\n", fCompressed ? "compressed" : "uncompressed");
        CBitcoinSecret bsecret;
        bsecret.SetSecret(secret, fCompressed);
        printf("    * secret (base58): %s\n", bsecret.ToString().c_str());
        CKey key;
        key.SetSecret(secret, fCompressed);
        std::vector<unsigned char> vchPubKey = key.GetPubKey();
        printf("    * pubkey (hex): %s\n", HexStr(vchPubKey).c_str());
        printf("    * address (base58): %s\n", CBitcoinAddress(vchPubKey).ToString().c_str());
    }
}
#endif


BOOST_FIXTURE_TEST_SUITE(key_tests, BasicTestingSetup)

std::string string_to_hex(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}


BOOST_AUTO_TEST_CASE(key_test1)
{
    CBitcoinSecret bsecret1, bsecret2, bsecret1C, bsecret2C, baddress1;
    BOOST_CHECK( bsecret1.SetString (strSecret1));
    BOOST_CHECK( bsecret2.SetString (strSecret2));
    BOOST_CHECK( bsecret1C.SetString(strSecret1C));
    BOOST_CHECK( bsecret2C.SetString(strSecret2C));
    BOOST_CHECK(!baddress1.SetString(strAddressBad));

    CKey key1  = bsecret1.GetKey();
    BOOST_CHECK(key1.IsCompressed() == false);
    CKey key2  = bsecret2.GetKey();
    BOOST_CHECK(key2.IsCompressed() == false);
    CKey key1C = bsecret1C.GetKey();
    BOOST_CHECK(key1C.IsCompressed() == true);
    CKey key2C = bsecret2C.GetKey();
    BOOST_CHECK(key2C.IsCompressed() == true);

    CPubKey pubkey1  = key1. GetPubKey();
    CPubKey pubkey2  = key2. GetPubKey();
    CPubKey pubkey1C = key1C.GetPubKey();
    CPubKey pubkey2C = key2C.GetPubKey();

    BOOST_CHECK(key1.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key1C.VerifyPubKey(pubkey1));
    BOOST_CHECK(key1C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey1C));
    BOOST_CHECK(key2.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey2));
    BOOST_CHECK(key2C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(addr1.Get()  == CTxDestination(pubkey1.GetID()));
    BOOST_CHECK(addr2.Get()  == CTxDestination(pubkey2.GetID()));
    BOOST_CHECK(addr1C.Get() == CTxDestination(pubkey1C.GetID()));
    BOOST_CHECK(addr2C.Get() == CTxDestination(pubkey2C.GetID()));

    for (int n=0; n<16; n++)
    {
        std::string strMsg = strprintf("Very secret message %i: 11", n);
        uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

        // normal signatures

        std::vector<unsigned char> sign1, sign2, sign1C, sign2C;

        BOOST_CHECK(key1.Sign (hashMsg, sign1));
        BOOST_CHECK(key2.Sign (hashMsg, sign2));
        BOOST_CHECK(key1C.Sign(hashMsg, sign1C));
        BOOST_CHECK(key2C.Sign(hashMsg, sign2C));

        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2C));

        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2C));

        // compact signatures (with key recovery)

        std::vector<unsigned char> csign1, csign2, csign1C, csign2C;

        BOOST_CHECK(key1.SignCompact (hashMsg, csign1));
        BOOST_CHECK(key2.SignCompact (hashMsg, csign2));
        BOOST_CHECK(key1C.SignCompact(hashMsg, csign1C));
        BOOST_CHECK(key2C.SignCompact(hashMsg, csign2C));

        CPubKey rkey1, rkey2, rkey1C, rkey2C;

        BOOST_CHECK(rkey1.RecoverCompact (hashMsg, csign1));
        BOOST_CHECK(rkey2.RecoverCompact (hashMsg, csign2));
        BOOST_CHECK(rkey1C.RecoverCompact(hashMsg, csign1C));
        BOOST_CHECK(rkey2C.RecoverCompact(hashMsg, csign2C));

        BOOST_CHECK(rkey1  == pubkey1);
        BOOST_CHECK(rkey2  == pubkey2);
        BOOST_CHECK(rkey1C == pubkey1C);
        BOOST_CHECK(rkey2C == pubkey2C);
    }

    // test deterministic signing
    std::vector<unsigned char> detsig, detsigc;
    std::string strMsg = "Very deterministic message";
    uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());
    BOOST_CHECK(key1.Sign(hashMsg, detsig));
    BOOST_CHECK(key1C.Sign(hashMsg, detsigc));
    // std::string sigStr( detsig.begin(), detsig.end() );
    // std::cout << "Sig: " << string_to_hex(sigStr) << std::endl;
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("3045022100828FFBE6C8448CDC1BB9D951F9973F355B0AD95825DD8576432BA169E49B679702202D4517A17257D171021621224A1978353E5B0E32C1F9433F73068BCD3D0C4689"));
    BOOST_CHECK(key2.Sign(hashMsg, detsig));
    BOOST_CHECK(key2C.Sign(hashMsg, detsigc));
    // std::string sigStr2( detsig.begin(), detsig.end() );
    // std::cout << "Sig2: " << string_to_hex(sigStr2) << std::endl;
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("3045022100EC77C5F541509E943A37AB82614BD161C9887F8E294DC4B304AEE997E047823C02206CD207B4D50E515BEF28FC75CCDB46CC1A639B3210BFD9C3B92876A836A3A0B1"));
    BOOST_CHECK(key1.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key1C.SignCompact(hashMsg, detsigc));
    // std::string sigStr3( detsig.begin(), detsig.end() );
    // std::string sigStr3C( detsigc.begin(), detsigc.end() );
    // std::cout << "Sig3: " << string_to_hex(sigStr3) << std::endl;
    // std::cout << "Sig3C: " << string_to_hex(sigStr3C) << std::endl;
    BOOST_CHECK(detsig == ParseHex("1C828FFBE6C8448CDC1BB9D951F9973F355B0AD95825DD8576432BA169E49B67972D4517A17257D171021621224A1978353E5B0E32C1F9433F73068BCD3D0C4689"));
    BOOST_CHECK(detsigc == ParseHex("20828FFBE6C8448CDC1BB9D951F9973F355B0AD95825DD8576432BA169E49B67972D4517A17257D171021621224A1978353E5B0E32C1F9433F73068BCD3D0C4689"));
    BOOST_CHECK(key2.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key2C.SignCompact(hashMsg, detsigc));
    std::string sigStr4( detsig.begin(), detsig.end() );
    // std::string sigStr4C( detsigc.begin(), detsigc.end() );
    // std::cout << "Sig4: " << string_to_hex(sigStr4) << std::endl;
    // std::cout << "Sig4C: " << string_to_hex(sigStr4C) << std::endl;
    BOOST_CHECK(detsig == ParseHex("1CEC77C5F541509E943A37AB82614BD161C9887F8E294DC4B304AEE997E047823C6CD207B4D50E515BEF28FC75CCDB46CC1A639B3210BFD9C3B92876A836A3A0B1"));
    BOOST_CHECK(detsigc == ParseHex("20EC77C5F541509E943A37AB82614BD161C9887F8E294DC4B304AEE997E047823C6CD207B4D50E515BEF28FC75CCDB46CC1A639B3210BFD9C3B92876A836A3A0B1"));
}

BOOST_AUTO_TEST_SUITE_END()
