// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "coins.h"
#include "consensus/consensus.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "validation.h"
#include "miner.h"
#include "policy/policy.h"
#include "pubkey.h"
#include "script/standard.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include <inttypes.h>
#include <iostream>
#include <fstream>
#include <stdarg.h>  // For va_start, etc.

#include "test/test_bitcoin.h"

#include <memory>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(miner_tests, TestingSetup)

static CFeeRate blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);

// ADCOIN TODO: this test is failing, it's because we don't have the right nonces (practically mining)
static
struct {
    unsigned char extranonce;
    unsigned int nonce;
} blockinfo[] = {
  {4, 74284},
  {2, 991522},
  {1, 0},
  {1, 0},
  {2, 0},
  {2, 0},
  {1, 0},
  {2, 0},
  {2,   0},
  {1, 0},
  {1, 0},
  {2, 0},
  {2, 0},
  {1, 0},
  {2, 0},
  {2, 362961},
  {1, 0},
  {2, 119538},
  {1, 0},
  {1, 0},
  {3, 0},
  {2, 410745},
  {2, 184270},
  {1, 200498},
  {2, 0x0170ee44},
  {1, 0x6e12f4aa},
  {2, 0x43f4f4db},
  {2, 3228},
  {2, 0xb5a50f10},
  {2, 0xb3902841},
  {2, 0xd198647e},
  {2, 65520},
  {1, 0x633a9a1c},
  {2, 0x9a722ed8},
  {2, 423641},
  {1, 0xd65022a1},
  {2, 758796},
  {1, 430871},
  {2, 0xfb7c80b7},
  {1, 883316},
  {1, 0xe34017a0},
  {3, 352019},
  {2, 0xba5c40bf},
  {5, 489380},
  {1, 0xa9ab516a},
  {5, 437695},
  {1, 0x37277cb3},
  {1, 643555},
  {1, 226530},
  {2, 842645},
  {1, 123282},
  {1, 0x75336f63},
  {1, 357734},
  {1, 0xd6531aec},
  {5, 833863},
  {5, 0x9d6fac0d},
  {1, 125114},
  {1, 89594},
  {6, 762447},
  {2, 492342},
  {2, 511295},
  {1, 0xad49ab71},
  {1, 126055},
  {1, 0x15acb65d},
  {2, 1632897},
  {2, 47333},
  {1, 410169},
  {1, 328615},
  {1, 382512},
  {5, 734829},
  {5, 0x73901915},
  {1, 731769},
  {1, 0x6d6b0a1d},
  {2, 0x888ba681},
  {2, 451114},
  {1, 0xb7fcaa55},
  {2, 118648},
  {1, 0x5aa13484},
  {2, 0x5bf4c2f3},
  {2, 0x94d401dd},
  {1, 777565},
  {1, 841590},
  {1, 499231},
  {5, 0x85ba6dbd},
  {1, 0xfd9b2000},
  {1, 63882},
  {1, 52907},
  {1, 227423},
  {1, 0x70de86d9},
  {1, 653918},
  {1, 0x49e92832},
  {2, 0x6926dbb9},
  {0, 0x64452497},
  {1, 493895},
  {2, 337007},
  {2, 321840},
  {2, 919595},
  {1, 38282},
  {1, 0xc0842a09},
  {1, 0xdfed39c5},
  {1, 0x3144223e},
  {1, 0xb3d12f84},
  {1, 0x7366aab7},
  {5, 0x6240691b},
  {2, 0xd3529b57},
  {1, 0xf4cae3b1},
  {1, 0x5b1df222},
  {1, 306101},
  {2, 0xbbccedc6},
  {2, 523521},
};

CBlockIndex CreateBlockIndex(int nHeight)
{
    CBlockIndex index;
    index.nHeight = nHeight;
    index.pprev = chainActive.Tip();
    return index;
}

bool TestSequenceLocks(const CTransaction &tx, int flags)
{
    LOCK(mempool.cs);
    return CheckSequenceLocks(tx, flags);
}

// Test suite for ancestor feerate transaction selection.
// Implemented as an additional function, rather than a separate test case,
// to allow reusing the blockchain created in CreateNewBlock_validity.
// Note that this test assumes blockprioritysize is 0.
void TestPackageSelection(const CChainParams& chainparams, CScript scriptPubKey, std::vector<CTransactionRef>& txFirst)
{
    // Test the ancestor feerate transaction selection.
    TestMemPoolEntryHelper entry;

    // Test that a medium fee transaction will be selected after a higher fee
    // rate package with a low fee rate parent.
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 5000000000LL - 1000;
    // This tx has a low fee: 1000 satoshis
    uint256 hashParentTx = tx.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a medium fee: 10000 satoshis
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = 5000000000LL - 10000;
    uint256 hashMediumFeeTx = tx.GetHash();
    mempool.addUnchecked(hashMediumFeeTx, entry.Fee(10000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a high fee, but depends on the first transaction
    tx.vin[0].prevout.hash = hashParentTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000; // 50k satoshi fee
    uint256 hashHighFeeTx = tx.GetHash();
    mempool.addUnchecked(hashHighFeeTx, entry.Fee(50000).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));

    std::unique_ptr<CBlockTemplate> pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[1]->GetHash() == hashParentTx);
    BOOST_CHECK(pblocktemplate->block.vtx[2]->GetHash() == hashHighFeeTx);
    BOOST_CHECK(pblocktemplate->block.vtx[3]->GetHash() == hashMediumFeeTx);

    // Test that a package below the block min tx fee doesn't get included
    tx.vin[0].prevout.hash = hashHighFeeTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000; // 0 fee
    uint256 hashFreeTx = tx.GetHash();
    mempool.addUnchecked(hashFreeTx, entry.Fee(0).FromTx(tx));
    size_t freeTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);

    // Calculate a fee on child transaction that will put the package just
    // below the block min tx fee (assuming 1 child tx of the same size).
    CAmount feeToUse = blockMinFeeRate.GetFee(2*freeTxSize) - 1;

    tx.vin[0].prevout.hash = hashFreeTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000 - feeToUse;
    uint256 hashLowFeeTx = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx, entry.Fee(feeToUse).FromTx(tx));
    pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    // Verify that the free tx and the low fee tx didn't get selected
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeTx);
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashLowFeeTx);
    }

    // Test that packages above the min relay fee do get included, even if one
    // of the transactions is below the min relay fee
    // Remove the low fee transaction and replace with a higher fee transaction
    mempool.removeRecursive(tx);
    tx.vout[0].nValue -= 2; // Now we should be just over the min relay fee
    hashLowFeeTx = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx, entry.Fee(feeToUse+2).FromTx(tx));
    pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[4]->GetHash() == hashFreeTx);
    BOOST_CHECK(pblocktemplate->block.vtx[5]->GetHash() == hashLowFeeTx);

    // Test that transaction selection properly updates ancestor fee
    // calculations as ancestor transactions get included in a block.
    // Add a 0-fee transaction that has 2 outputs.
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vout.resize(2);
    tx.vout[0].nValue = 5000000000LL - 100000000;
    tx.vout[1].nValue = 100000000; // 1BTC output
    uint256 hashFreeTx2 = tx.GetHash();
    mempool.addUnchecked(hashFreeTx2, entry.Fee(0).SpendsCoinbase(true).FromTx(tx));

    // This tx can't be mined by itself
    tx.vin[0].prevout.hash = hashFreeTx2;
    tx.vout.resize(1);
    feeToUse = blockMinFeeRate.GetFee(freeTxSize);
    tx.vout[0].nValue = 5000000000LL - 100000000 - feeToUse;
    uint256 hashLowFeeTx2 = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx2, entry.Fee(feeToUse).SpendsCoinbase(false).FromTx(tx));
    pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);

    // Verify that this tx isn't selected.
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeTx2);
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashLowFeeTx2);
    }

    // This tx will be mineable, and should cause hashLowFeeTx2 to be selected
    // as well.
    tx.vin[0].prevout.n = 1;
    tx.vout[0].nValue = 100000000 - 10000; // 10k satoshi fee
    mempool.addUnchecked(tx.GetHash(), entry.Fee(10000).FromTx(tx));
    pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[8]->GetHash() == hashLowFeeTx2);
}

std::string string_format(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(CreateNewBlock_validity)
{
    // // Note that by default, these tests run with size accounting enabled.
    // const CChainParams& chainparams = Params(CBaseChainParams::MAIN);
    // CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    // std::unique_ptr<CBlockTemplate> pblocktemplate;
    // CMutableTransaction tx,tx2;
    // CScript script;
    // uint256 hash;
    // TestMemPoolEntryHelper entry;
    // entry.nFee = 11;
    // entry.dPriority = 111.0;
    // entry.nHeight = 11;
    //
    // LOCK(cs_main);
    // fCheckpointsEnabled = false;
    //
    // // Simple block creation, nothing special yet:
    // BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    //
    // std::ofstream myfile;
    // myfile.open ("logs_miner_test.txt");
    // myfile << string_format("test: %d", 1);
    // myfile.close();
    //
    //
    // // We can't make transactions until we have inputs
    // // Therefore, load 100 blocks :)
    // int baseheight = 0;
    // std::vector<CTransactionRef> txFirst;
    // for (unsigned int i = 0; i < sizeof(blockinfo)/sizeof(*blockinfo); ++i)
    // {
    //     CBlock *pblock = &pblocktemplate->block; // pointer for convenience
    //     pblock->nVersion = 1;
    //     pblock->nTime = chainActive.Tip()->GetMedianTimePast()+1;
    //     CMutableTransaction txCoinbase(*pblock->vtx[0]);
    //     txCoinbase.nVersion = 1;
    //     txCoinbase.vin[0].scriptSig = CScript();
    //     txCoinbase.vin[0].scriptSig.push_back(blockinfo[i].extranonce);
    //     txCoinbase.vin[0].scriptSig.push_back(chainActive.Height());
    //     txCoinbase.vout.resize(1); // Ignore the (optional) segwit commitment added by CreateNewBlock (as the hardcoded nonces don't account for this)
    //     txCoinbase.vout[0].scriptPubKey = CScript();
    //     pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    //     if (txFirst.size() == 0)
    //         baseheight = chainActive.Height();
    //     if (txFirst.size() < 4)
    //         txFirst.push_back(pblock->vtx[0]);
    //     pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
    //     pblock->nNonce = blockinfo[i].nonce;
    //     // uint32_t maxTries = 1000000;
    //     // if(!CheckProofOfWork(pblock->GetPoWHash(), pblock->nBits, chainparams.GetConsensus())) {
    //     //   pblock->nNonce = 1000000;
    //     //   while (!CheckProofOfWork(pblock->GetPoWHash(), pblock->nBits, chainparams.GetConsensus())) {
    //     //     ++pblock->nNonce;
    //     //     if(--maxTries == 0) {
    //     //       break;
    //     //     }
    //     //   }
    //     // }
    //
    //     // std::ofstream myfile;
    //     // myfile.open ("logs_miner_test.txt", std::ofstream::app);
    //     // myfile << string_format("i: %d Maxtries left: %" PRIu32 ", final nonce: %" PRIu32 "\n", i, maxTries, pblock->nNonce);
    //     // myfile.close();
    //     // printf("i: %d Maxtries left: %" PRIu32 ", final nonce: %" PRIu32 "\n", i, maxTries, pblock->nNonce);
    //
    //     std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
    //     BOOST_CHECK(ProcessNewBlock(chainparams, shared_pblock, true, NULL));
    //     pblock->hashPrevBlock = pblock->GetHash();
    // }
    //
    // // Just to make sure we can still make simple blocks
    // BOOST_CHECK(false); // so we can see logs
    // BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    //
    // const CAmount BLOCKSUBSIDY = 25*COIN;
    // const CAmount LOWFEE = CENT;
    // const CAmount HIGHFEE = COIN;
    // const CAmount HIGHERFEE = 4*COIN;
    //
    // // block sigops > limit: 1000 CHECKMULTISIG + 1
    // tx.vin.resize(1);
    // // NOTE: OP_NOP is used to force 20 SigOps for the CHECKMULTISIG
    // tx.vin[0].scriptSig = CScript() << OP_0 << OP_0 << OP_0 << OP_NOP << OP_CHECKMULTISIG << OP_1;
    // tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    // tx.vin[0].prevout.n = 0;
    // tx.vout.resize(1);
    // tx.vout[0].nValue = BLOCKSUBSIDY;
    // for (unsigned int i = 0; i < 1001; ++i)
    // {
    //     tx.vout[0].nValue -= LOWFEE;
    //     hash = tx.GetHash();
    //     bool spendsCoinbase = (i == 0) ? true : false; // only first tx spends coinbase
    //     // If we don't set the # of sig ops in the CTxMemPoolEntry, template creation fails
    //     mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
    //     tx.vin[0].prevout.hash = hash;
    // }
    // BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    // mempool.clear();
    //
    // tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    // tx.vout[0].nValue = BLOCKSUBSIDY;
    // for (unsigned int i = 0; i < 1001; ++i)
    // {
    //     tx.vout[0].nValue -= LOWFEE;
    //     hash = tx.GetHash();
    //     bool spendsCoinbase = (i == 0) ? true : false; // only first tx spends coinbase
    //     // If we do set the # of sig ops in the CTxMemPoolEntry, template creation passes
    //     mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).SigOpsCost(80).FromTx(tx));
    //     tx.vin[0].prevout.hash = hash;
    // }
    // BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    // mempool.clear();
    //
    // // block size > limit
    // tx.vin[0].scriptSig = CScript();
    // // 18 * (520char + DROP) + OP_1 = 9433 bytes
    // std::vector<unsigned char> vchData(520);
    // for (unsigned int i = 0; i < 18; ++i)
    //     tx.vin[0].scriptSig << vchData << OP_DROP;
    // tx.vin[0].scriptSig << OP_1;
    // tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    // tx.vout[0].nValue = BLOCKSUBSIDY;
    // for (unsigned int i = 0; i < 128; ++i)
    // {
    //     tx.vout[0].nValue -= LOWFEE;
    //     hash = tx.GetHash();
    //     bool spendsCoinbase = (i == 0) ? true : false; // only first tx spends coinbase
    //     mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
    //     tx.vin[0].prevout.hash = hash;
    // }
    // BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    // mempool.clear();
    //
    // // orphan in mempool, template creation fails
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).FromTx(tx));
    // BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    // mempool.clear();
    //
    // // child with higher priority than parent
    // tx.vin[0].scriptSig = CScript() << OP_1;
    // tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    // tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    // tx.vin[0].prevout.hash = hash;
    // tx.vin.resize(2);
    // tx.vin[1].scriptSig = CScript() << OP_1;
    // tx.vin[1].prevout.hash = txFirst[0]->GetHash();
    // tx.vin[1].prevout.n = 0;
    // tx.vout[0].nValue = tx.vout[0].nValue+BLOCKSUBSIDY-HIGHERFEE; //First txn output + fresh coinbase - new txn fee
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Fee(HIGHERFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    // BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    // mempool.clear();
    //
    // // coinbase in mempool, template creation fails
    // tx.vin.resize(1);
    // tx.vin[0].prevout.SetNull();
    // tx.vin[0].scriptSig = CScript() << OP_0 << OP_1;
    // tx.vout[0].nValue = 0;
    // hash = tx.GetHash();
    // // give it a fee so it'll get mined
    // mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    // BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    // mempool.clear();
    //
    // // invalid (pre-p2sh) txn in mempool, template creation fails
    // tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    // tx.vin[0].prevout.n = 0;
    // tx.vin[0].scriptSig = CScript() << OP_1;
    // tx.vout[0].nValue = BLOCKSUBSIDY-LOWFEE;
    // script = CScript() << OP_0;
    // tx.vout[0].scriptPubKey = GetScriptForDestination(CScriptID(script));
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    // tx.vin[0].prevout.hash = hash;
    // tx.vin[0].scriptSig = CScript() << std::vector<unsigned char>(script.begin(), script.end());
    // tx.vout[0].nValue -= LOWFEE;
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    // BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    // mempool.clear();
    //
    // // double spend txn pair in mempool, template creation fails
    // tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    // tx.vin[0].scriptSig = CScript() << OP_1;
    // tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    // tx.vout[0].scriptPubKey = CScript() << OP_1;
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    // tx.vout[0].scriptPubKey = CScript() << OP_2;
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    // BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    // mempool.clear();
    //
    // // subsidy changing
    // int nHeight = chainActive.Height();
    // // Create an actual 209999-long block chain (without valid blocks).
    // while (chainActive.Tip()->nHeight < 839999) {
    //     CBlockIndex* prev = chainActive.Tip();
    //     CBlockIndex* next = new CBlockIndex();
    //     next->phashBlock = new uint256(GetRandHash());
    //     pcoinsTip->SetBestBlock(next->GetBlockHash());
    //     next->pprev = prev;
    //     next->nHeight = prev->nHeight + 1;
    //     next->BuildSkip();
    //     chainActive.SetTip(next);
    // }
    // BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    // // Extend to a 210000-long block chain.
    // while (chainActive.Tip()->nHeight < 840000) {
    //     CBlockIndex* prev = chainActive.Tip();
    //     CBlockIndex* next = new CBlockIndex();
    //     next->phashBlock = new uint256(GetRandHash());
    //     pcoinsTip->SetBestBlock(next->GetBlockHash());
    //     next->pprev = prev;
    //     next->nHeight = prev->nHeight + 1;
    //     next->BuildSkip();
    //     chainActive.SetTip(next);
    // }
    // BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    // // Delete the dummy blocks again.
    // while (chainActive.Tip()->nHeight > nHeight) {
    //     CBlockIndex* del = chainActive.Tip();
    //     chainActive.SetTip(del->pprev);
    //     pcoinsTip->SetBestBlock(del->pprev->GetBlockHash());
    //     delete del->phashBlock;
    //     delete del;
    // }
    //
    // // non-final txs in mempool
    // SetMockTime(chainActive.Tip()->GetMedianTimePast()+1);
    // int flags = LOCKTIME_VERIFY_SEQUENCE|LOCKTIME_MEDIAN_TIME_PAST;
    // // height map
    // std::vector<int> prevheights;
    //
    // // relative height locked
    // tx.nVersion = 2;
    // tx.vin.resize(1);
    // prevheights.resize(1);
    // tx.vin[0].prevout.hash = txFirst[0]->GetHash(); // only 1 transaction
    // tx.vin[0].prevout.n = 0;
    // tx.vin[0].scriptSig = CScript() << OP_1;
    // tx.vin[0].nSequence = chainActive.Tip()->nHeight + 1; // txFirst[0] is the 2nd block
    // prevheights[0] = baseheight + 1;
    // tx.vout.resize(1);
    // tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    // tx.vout[0].scriptPubKey = CScript() << OP_1;
    // tx.nLockTime = 0;
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    // BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    // BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    // BOOST_CHECK(SequenceLocks(tx, flags, &prevheights, CreateBlockIndex(chainActive.Tip()->nHeight + 2))); // Sequence locks pass on 2nd block
    //
    // // relative time locked
    // tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    // tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | (((chainActive.Tip()->GetMedianTimePast()+1-chainActive[1]->GetMedianTimePast()) >> CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) + 1); // txFirst[1] is the 3rd block
    // prevheights[0] = baseheight + 2;
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    // BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    // BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    //
    // for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
    //     chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime += 512; //Trick the MedianTimePast
    // BOOST_CHECK(SequenceLocks(tx, flags, &prevheights, CreateBlockIndex(chainActive.Tip()->nHeight + 1))); // Sequence locks pass 512 seconds later
    // for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
    //     chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime -= 512; //undo tricked MTP
    //
    // // absolute height locked
    // tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    // tx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL - 1;
    // prevheights[0] = baseheight + 3;
    // tx.nLockTime = chainActive.Tip()->nHeight + 1;
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    // BOOST_CHECK(!CheckFinalTx(tx, flags)); // Locktime fails
    // BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    // BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 2, chainActive.Tip()->GetMedianTimePast())); // Locktime passes on 2nd block
    //
    // // absolute time locked
    // tx.vin[0].prevout.hash = txFirst[3]->GetHash();
    // tx.nLockTime = chainActive.Tip()->GetMedianTimePast();
    // prevheights.resize(1);
    // prevheights[0] = baseheight + 4;
    // hash = tx.GetHash();
    // mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    // BOOST_CHECK(!CheckFinalTx(tx, flags)); // Locktime fails
    // BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    // BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 2, chainActive.Tip()->GetMedianTimePast() + 1)); // Locktime passes 1 second later
    //
    // // mempool-dependent transactions (not added)
    // tx.vin[0].prevout.hash = hash;
    // prevheights[0] = chainActive.Tip()->nHeight + 1;
    // tx.nLockTime = 0;
    // tx.vin[0].nSequence = 0;
    // BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    // BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    // tx.vin[0].nSequence = 1;
    // BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    // tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG;
    // BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    // tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | 1;
    // BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    //
    // BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    //
    // // None of the of the absolute height/time locked tx should have made
    // // it into the template because we still check IsFinalTx in CreateNewBlock,
    // // but relative locked txs will if inconsistently added to mempool.
    // // For now these will still generate a valid template until BIP68 soft fork
    // BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3);
    // // However if we advance height by 1 and time by 512, all of them should be mined
    // for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
    //     chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime += 512; //Trick the MedianTimePast
    // chainActive.Tip()->nHeight++;
    // SetMockTime(chainActive.Tip()->GetMedianTimePast() + 1);
    //
    // BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    // BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 5);
    //
    // chainActive.Tip()->nHeight--;
    // SetMockTime(0);
    // mempool.clear();
    //
    // TestPackageSelection(chainparams, scriptPubKey, txFirst);
    //
    // fCheckpointsEnabled = true;
}

BOOST_AUTO_TEST_SUITE_END()
