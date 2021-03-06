// Copyright (c) 2017-2017 Blockstream
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/ethpegin_valid.json.h>
#include <test/data/ethpegin_invalid1.json.h>
#include <test/data/ethpegin_invalid2.json.h>
#include <test/data/ethpegin_invalid3.json.h>
#include <test/data/ethpegin_invalid4.json.h>
#include <test/data/ethpegin_invalid5.json.h>
#include <test/data/ethpegin_invalid6.json.h>

#include "clientversion.h"
#include "chainparams.h"
#include "checkqueue.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "validation.h" // For CheckTransaction
#include "policy/policy.h"
#include "script/script.h"
#include "script/script_error.h"
#include "utilstrencodings.h"
#include "validation.h"
#include "streams.h"
#include "test/test_bitcoin.h"
#include "util.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

UniValue read_json_obj(const std::string& jsondata)
{
    UniValue v;
    if (!v.read(jsondata)) {
        BOOST_ERROR("Parse error.");
        return UniValue(UniValue::VOBJ);
    }
    return v.get_obj();
}

std::vector<std::vector<unsigned char> > witness_stack = {
    ParseHex("00ca9a3b00000000"),
    ParseHex("baedb90b5e95077d7b067c6fbba6bcd6bd935b80012f3f8626561b67a347c0a5"),
    ParseHex("a38fcbb10d8cec9ae6346a90d018a14567f5d5406ab810c0f8ae76f84067e5d4"),
    ParseHex("00141eef6361cd1507a303834285d1521d6baf1b19ae"),
    ParseHex("0200000001b399292c8100b8a1b66eb23896f799c1712390d560af0f70e81acd2d17a3b06e0000000049483045022100c3c749623486ea57ea93dfaf78d85590d78c7590a25768fe80f0ea4d6047419002202a0a00a90392b86c53c0fdda908c4591ba28040c16c25734c23b7df3c8b70acd01feffffff0228196bee000000001976a914470dd41542ee1a1bd75f1a838878648c8d65622488ac00ca9a3b0000000017a914cb60b1d7f76ba12b45a116c482c165a74c5d7e388765000000"),
    ParseHex("000000205e3913a320cd2e3a2efa141e47419f54cb9e82320cf8dbc812fc19b9a1b2413a57f5e9fb4fa22de191454a241387f5d10cc794ee0fbf72ae2841baf3129a4eab8133025affff7f20000000000200000002f9d0be670007d38fceece999cb6144658a99c307ccc37f6d8f69129ed0f4545ff321df9790633bc33c67239c4174df8142ee616ee6a2e2788fe4820fe70e9bce0105")
};

std::vector<std::vector<unsigned char> > witness_stack_eth = {
    ParseHex("00409452a3030000"),
    ParseHex("baedb90b5e95077d7b067c6fbba6bcd6bd935b80012f3f8626561b67a347c0a5"),
    ParseHex("a38fcbb10d8cec9ae6346a90d018a14567f5d5406ab810c0f8ae76f84067e5d4"),
    ParseHex("0397466f2b32bc3bb76d4741ae51cd1d8578b48d3f1e68da206d47321aec267ce7"),
    ParseHex("99ef0fb08c36a2e77e0810de32181b63e8250fbf9a398f9bf9e53444cbf6cdab"),
};

std::vector<unsigned char> pegin_transaction = ParseHex("020000000101f321df9790633bc33c67239c4174df8142ee616ee6a2e2788fe4820fe70e9bce0100004000ffffffff0201baedb90b5e95077d7b067c6fbba6bcd6bd935b80012f3f8626561b67a347c0a501000000003b9ab2e0001976a914809326f7628dc976fbe63806479a1b8dfcc8c4b988ac01baedb90b5e95077d7b067c6fbba6bcd6bd935b80012f3f8626561b67a347c0a5010000000000001720000000000000000002483045022100ae17064745d80650a6a5cbcbe15c8c45ba498d1c6f45a7c0f5f32d871b463fc60220799f2836471702c21f7cfe124651727b530ad41f7af4dc213c65f5030a2f6fc4012103a9d3c6c7c161a565a76113632fe13330cf2c0207ba79a76d1154cdc3cb94d940060800ca9a3b0000000020baedb90b5e95077d7b067c6fbba6bcd6bd935b80012f3f8626561b67a347c0a520a38fcbb10d8cec9ae6346a90d018a14567f5d5406ab810c0f8ae76f84067e5d41600141eef6361cd1507a303834285d1521d6baf1b19aebe0200000001b399292c8100b8a1b66eb23896f799c1712390d560af0f70e81acd2d17a3b06e0000000049483045022100c3c749623486ea57ea93dfaf78d85590d78c7590a25768fe80f0ea4d6047419002202a0a00a90392b86c53c0fdda908c4591ba28040c16c25734c23b7df3c8b70acd01feffffff0228196bee000000001976a914470dd41542ee1a1bd75f1a838878648c8d65622488ac00ca9a3b0000000017a914cb60b1d7f76ba12b45a116c482c165a74c5d7e38876500000097000000205e3913a320cd2e3a2efa141e47419f54cb9e82320cf8dbc812fc19b9a1b2413a57f5e9fb4fa22de191454a241387f5d10cc794ee0fbf72ae2841baf3129a4eab8133025affff7f20000000000200000002f9d0be670007d38fceece999cb6144658a99c307ccc37f6d8f69129ed0f4545ff321df9790633bc33c67239c4174df8142ee616ee6a2e2788fe4820fe70e9bce010500000000");

std::vector<unsigned char> pegin_transaction_eth = ParseHex("02000000010199ef0fb08c36a2e77e0810de32181b63e8250fbf9a398f9bf9e53444cbf6cdab0000004000ffffffff0201baedb90b5e95077d7b067c6fbba6bcd6bd935b80012f3f8626561b67a347c0a501000003a3529429e4001976a9148faac4306b889b29d7e4c617aa9309ee7555be0088ac01baedb90b5e95077d7b067c6fbba6bcd6bd935b80012f3f8626561b67a347c0a501000000000000161c000000000000000002483045022100fe8045be76c686f59935ff3f0c21ec858a6cf7704a9061386f3b9878b841155602206b29560506ecb9c8dc98f08408476aa663fbe9853851bc5c92c29b006c065a64012102dc8f44c0199b1c1364f8b899e964ff59c7458024c6eacebae23d24d506919a1f050800409452a303000020baedb90b5e95077d7b067c6fbba6bcd6bd935b80012f3f8626561b67a347c0a520a38fcbb10d8cec9ae6346a90d018a14567f5d5406ab810c0f8ae76f84067e5d4210397466f2b32bc3bb76d4741ae51cd1d8578b48d3f1e68da206d47321aec267ce72099ef0fb08c36a2e77e0810de32181b63e8250fbf9a398f9bf9e53444cbf6cdab00000000");

COutPoint prevout(uint256S("ce9b0ee70f82e48f78e2a2e66e61ee4281df74419c23673cc33b639097df21f3"), 1);
COutPoint prevout_eth(uint256S("abcdf6cb4434e5f99b8f399abf0f25e8631b1832de10087ee7a2368cb00fef99"), 1);

// Needed for easier parent PoW check, and setting fedpegscript
struct RegtestingSetup : public TestingSetup {
        RegtestingSetup() : TestingSetup(CBaseChainParams::REGTEST, "512103dff4923d778550cc13ce0d887d737553b4b58f4e8e886507fc39f5e447b2186451ae") {}
};

BOOST_FIXTURE_TEST_SUITE(pegin_witness_tests, RegtestingSetup)

BOOST_AUTO_TEST_CASE(witness_valid)
{

    CScriptWitness witness;
    witness.stack = witness_stack;

    BOOST_CHECK(IsValidPeginWitness(witness, prevout));

    // Missing byte on each field to make claim ill-formatted
    // This will break deserialization and other data-matching checks
    for (unsigned int i = 0; i < witness.stack.size(); i++) {
        witness.stack[i].pop_back();
        BOOST_CHECK(!IsValidPeginWitness(witness, prevout));
        witness.stack = witness_stack;
        BOOST_CHECK(IsValidPeginWitness(witness, prevout));
    }

    // Test mismatched but valid nOut to proof
    COutPoint fake_prevout = prevout;
    fake_prevout.n = 0;
    BOOST_CHECK(!IsValidPeginWitness(witness, fake_prevout));

    // Test mistmatched but valid txid
    fake_prevout = prevout;
    fake_prevout.hash = uint256S("2f103ee04a5649eecb932b4da4ca9977f53a12bbe04d9d1eb5ccc0f4a06334");
    BOOST_CHECK(!IsValidPeginWitness(witness, fake_prevout));

    // Ensure that all witness stack sizes are handled
    BOOST_CHECK(IsValidPeginWitness(witness, prevout));
    for (unsigned int i = 0; i < witness.stack.size(); i++) {
        witness.stack.pop_back();
        BOOST_CHECK(!IsValidPeginWitness(witness, prevout));
    }
    witness.stack = witness_stack;

    // Extra element causes failure
    witness.stack.push_back(witness.stack.back());
    BOOST_CHECK(!IsValidPeginWitness(witness, prevout));
    witness.stack = witness_stack;

    // Check validation of peg-in transaction's inputs and balance
    CDataStream ssTx(pegin_transaction, SER_NETWORK, PROTOCOL_VERSION);
    CTransactionRef txRef;
    ssTx >> txRef;
    CTransaction tx(*txRef);

    // Only one(valid) input witness should exist, and should match
    BOOST_CHECK(tx.wit.vtxinwit.size() == 1);
    BOOST_CHECK(tx.wit.vtxinwit[0].m_pegin_witness.stack == witness_stack);
    BOOST_CHECK(tx.vin[0].m_is_pegin);
    // Check that serialization doesn't cause issuance to become non-null
    BOOST_CHECK(tx.vin[0].assetIssuance.IsNull());
    BOOST_CHECK(IsValidPeginWitness(tx.wit.vtxinwit[0].m_pegin_witness, prevout));

    std::set<std::pair<uint256, COutPoint> > setPeginsSpent;
    CValidationState state;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    // Note: commented out as CheckTxInputs is now using IsValidEthPeginWitness
    // BOOST_CHECK(Consensus::CheckTxInputs(tx, state, coins, 0, setPeginsSpent, nullptr, false, true));
    // BOOST_CHECK(setPeginsSpent.size() == 1);
    setPeginsSpent.clear();

    // Strip pegin_witness
    CMutableTransaction mtxn(tx);
    mtxn.wit.vtxinwit[0].m_pegin_witness.SetNull();
    CTransaction tx2(mtxn);
    BOOST_CHECK(!Consensus::CheckTxInputs(tx2, state, coins, 0, setPeginsSpent, nullptr, false, true));
    BOOST_CHECK(setPeginsSpent.empty());

    // Invalidate peg-in (and spending) authorization by pegin marker.
    // This only checks for peg-in authorization, with the only input marked
    // as m_is_pegin
    CMutableTransaction mtxn2(tx);
    mtxn2.vin[0].m_is_pegin = false;
    CTransaction tx3(mtxn2);
    BOOST_CHECK(!Consensus::CheckTxInputs(tx3, state, coins, 0, setPeginsSpent, nullptr, false, true));
    BOOST_CHECK(setPeginsSpent.empty());
}

BOOST_AUTO_TEST_CASE(eth_witness_valid)
{

    CScriptWitness witness;
    witness.stack = witness_stack_eth;

    string strFailReason;
    BOOST_CHECK(IsValidEthPeginWitness(witness, prevout_eth, strFailReason));

    // Missing byte on each field to make claim ill-formatted
    // This will break deserialization and other data-matching checks
    for (unsigned int i = 0; i < witness.stack.size(); i++) {
        witness.stack[i].pop_back();
        // Does not affect claim script (i=3) as it is not currently used for tweaking
        BOOST_CHECK(!IsValidEthPeginWitness(witness, prevout_eth, strFailReason) || i == 3);
        witness.stack = witness_stack_eth;
        BOOST_CHECK(IsValidEthPeginWitness(witness, prevout_eth, strFailReason));
    }

    // Test mistmatched but valid txid
    COutPoint fake_prevout = prevout_eth;
    fake_prevout = prevout_eth;
    fake_prevout.hash = uint256S("2f103ee04a5649eecb932b4da4ca9977f53a12bbe04d9d1eb5ccc0f4a06334");
    BOOST_CHECK(!IsValidEthPeginWitness(witness, fake_prevout, strFailReason));

    // Ensure that all witness stack sizes are handled
    BOOST_CHECK(IsValidEthPeginWitness(witness, prevout_eth, strFailReason));
    for (unsigned int i = 0; i < witness.stack.size(); i++) {
        witness.stack.pop_back();
        BOOST_CHECK(!IsValidEthPeginWitness(witness, prevout_eth, strFailReason));
    }
    witness.stack = witness_stack_eth;

    // Extra element causes failure
    witness.stack.push_back(witness.stack.back());
    BOOST_CHECK(!IsValidEthPeginWitness(witness, prevout_eth, strFailReason));
    witness.stack = witness_stack_eth;

    // Check validation of peg-in transaction's inputs and balance
    CDataStream ssTx(pegin_transaction_eth, SER_NETWORK, PROTOCOL_VERSION);
    CTransactionRef txRef;
    ssTx >> txRef;
    CTransaction tx(*txRef);

    // Only one(valid) input witness should exist, and should match
    BOOST_CHECK(tx.wit.vtxinwit.size() == 1);
    BOOST_CHECK(tx.wit.vtxinwit[0].m_pegin_witness.stack == witness_stack_eth);
    BOOST_CHECK(tx.vin[0].m_is_pegin);
    // Check that serialization doesn't cause issuance to become non-null
    BOOST_CHECK(tx.vin[0].assetIssuance.IsNull());
    BOOST_CHECK(IsValidEthPeginWitness(tx.wit.vtxinwit[0].m_pegin_witness, prevout_eth, strFailReason));

    std::set<std::pair<uint256, COutPoint> > setPeginsSpent;
    CValidationState state;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    BOOST_CHECK(Consensus::CheckTxInputs(tx, state, coins, 0, setPeginsSpent, nullptr, false, true));
    BOOST_CHECK(setPeginsSpent.size() == 1);
    setPeginsSpent.clear();

    // Strip pegin_witness
    CMutableTransaction mtxn(tx);
    mtxn.wit.vtxinwit[0].m_pegin_witness.SetNull();
    CTransaction tx2(mtxn);
    BOOST_CHECK(!Consensus::CheckTxInputs(tx2, state, coins, 0, setPeginsSpent, nullptr, false, true));
    BOOST_CHECK(setPeginsSpent.empty());

    // Invalidate peg-in (and spending) authorization by pegin marker.
    // This only checks for peg-in authorization, with the only input marked
    // as m_is_pegin
    CMutableTransaction mtxn2(tx);
    mtxn2.vin[0].m_is_pegin = false;
    CTransaction tx3(mtxn2);
    BOOST_CHECK(!Consensus::CheckTxInputs(tx3, state, coins, 0, setPeginsSpent, nullptr, false, true));
    BOOST_CHECK(setPeginsSpent.empty());


    // TODO Test mixed pegin/non-pegin input case
    // TODO Test spending authorization in conjunction with valid witness program in pegin auth
}

BOOST_AUTO_TEST_CASE(eth_pegin_valid)
{
    UniValue test_valid = read_json_obj(std::string(json_tests::ethpegin_valid,
        json_tests::ethpegin_valid + sizeof(json_tests::ethpegin_valid)));

    std::vector<unsigned char> keyBytes = ParseHex("3ecb44df2159c26e0f995712d4f39b6f6e499b40749b1cf1246c37f9516cb6a4");
    CKey key;
    key.Set(keyBytes.begin(), keyBytes.end(), false);
    CPubKey pubkey = key.GetPubKey();

    std::string strFailReason;
    CAmount amount = 3999900000000;
    BOOST_CHECK(IsValidEthPegin(test_valid, amount, pubkey, strFailReason));
    BOOST_CHECK_EQUAL("", strFailReason);

    UniValue test_invalid1 = read_json_obj(std::string(json_tests::ethpegin_invalid1,
        json_tests::ethpegin_invalid1 + sizeof(json_tests::ethpegin_invalid1)));
    BOOST_CHECK(!IsValidEthPegin(test_invalid1, amount, pubkey, strFailReason));
    BOOST_CHECK_EQUAL("Unexpected ERC-20 transfer topic hash", strFailReason);

    UniValue test_invalid2 = read_json_obj(std::string(json_tests::ethpegin_invalid2,
        json_tests::ethpegin_invalid2 + sizeof(json_tests::ethpegin_invalid2)));
    BOOST_CHECK(!IsValidEthPegin(test_invalid2, amount, pubkey, strFailReason));
    BOOST_CHECK_EQUAL("Invalid CBT ERC-20 contract provided", strFailReason);

    UniValue test_invalid3 = read_json_obj(std::string(json_tests::ethpegin_invalid3,
        json_tests::ethpegin_invalid3 + sizeof(json_tests::ethpegin_invalid3)));
    BOOST_CHECK(!IsValidEthPegin(test_invalid3, amount, pubkey, strFailReason));
    BOOST_CHECK_EQUAL("Invalid fegpeg destination address", strFailReason);

    UniValue test_invalid4 = read_json_obj(std::string(json_tests::ethpegin_invalid4,
        json_tests::ethpegin_invalid4 + sizeof(json_tests::ethpegin_invalid4)));
    BOOST_CHECK(!IsValidEthPegin(test_invalid4, 1, pubkey, strFailReason));
    BOOST_CHECK_EQUAL("Pegin amount and ERC-20 transaction amount don't match", strFailReason);

    UniValue test_invalid5 = read_json_obj(std::string(json_tests::ethpegin_invalid5,
        json_tests::ethpegin_invalid5 + sizeof(json_tests::ethpegin_invalid5)));
    BOOST_CHECK(!IsValidEthPegin(test_invalid5, amount, pubkey, strFailReason));
    BOOST_CHECK_EQUAL("From address not matching claim_pubkey", strFailReason);

    UniValue test_invalid6 = read_json_obj(std::string(json_tests::ethpegin_invalid6,
    json_tests::ethpegin_invalid6 + sizeof(json_tests::ethpegin_invalid6)));
    BOOST_CHECK(!IsValidEthPegin(test_invalid6, amount, pubkey, strFailReason));
    BOOST_CHECK_EQUAL("Invalid eth transaction", strFailReason);
}

BOOST_AUTO_TEST_SUITE_END()
