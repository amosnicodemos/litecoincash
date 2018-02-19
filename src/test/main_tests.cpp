// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "validation.h"
#include "net.h"

#include "test/test_bitcoin.h"

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

static void TestBlockSubsidyHalvings(const Consensus::Params& consensusParams)
{
    int maxHalvings = 64;
    CAmount nInitialSubsidy = 50 * COIN * COIN_SCALE;

    CAmount nPreviousSubsidy = nInitialSubsidy * 2; // for height == 0
    BOOST_CHECK_EQUAL(nPreviousSubsidy, nInitialSubsidy * 2);
    for (int nHalvings = 0; nHalvings < maxHalvings; nHalvings++) {
        int nHeight = nHalvings * consensusParams.nSubsidyHalvingInterval;
        CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
        if(nHeight >= 6164768) {	// LitecoinCash: Money issue has finished by this point
			BOOST_CHECK(nSubsidy == 0);
		} else {
			BOOST_CHECK(nSubsidy <= nInitialSubsidy);
			BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
			nPreviousSubsidy = nSubsidy;
        }
    }
    BOOST_CHECK_EQUAL(GetBlockSubsidy(maxHalvings * consensusParams.nSubsidyHalvingInterval, consensusParams), 0);
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
{
	const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
	const Consensus::Params mainConsensusParams = chainParams->GetConsensus();	// Get Litecoin Cash extra params
    Consensus::Params consensusParams;
    consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
    consensusParams.lastScryptBlock = mainConsensusParams.lastScryptBlock;		// Set Litecoin Cash extra params
    consensusParams.slowStartBlocks = mainConsensusParams.slowStartBlocks;		// Set Litecoin Cash extra params
    TestBlockSubsidyHalvings(consensusParams);
}

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    TestBlockSubsidyHalvings(chainParams->GetConsensus()); // As in main
    TestBlockSubsidyHalvings(150); // As in regtest
    TestBlockSubsidyHalvings(1000); // Just another interval
}

BOOST_AUTO_TEST_CASE(block_subsidy_slowstart_test)	// LitecoinCash
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params consensusParams = chainParams->GetConsensus();

	int slowStart = consensusParams.lastScryptBlock + 1;
	int slowEnd = consensusParams.lastScryptBlock + consensusParams.slowStartBlocks + 3;

	CAmount nSum = 0;
    for (int nHeight = slowStart; nHeight < slowEnd; nHeight += 1) {
        CAmount nSubsidy = GetBlockSubsidy(nHeight, chainParams->GetConsensus());
        if(nHeight == slowStart)
			BOOST_CHECK(nSubsidy == 550000 * COIN * COIN_SCALE);

		if(nHeight >= slowEnd)
			BOOST_CHECK(nSubsidy == 25 * COIN * COIN_SCALE);	// First halvening happenned

        nSum += nSubsidy;

        BOOST_CHECK(MoneyRange(nSum));
	}
    BOOST_CHECK_EQUAL(nSum, 57506248750000ULL);
}

BOOST_AUTO_TEST_CASE(block_subsidy_money_limit)	// LitecoinCash
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params consensusParams = chainParams->GetConsensus();
	CAmount nSum = 0;
    for (int nHeight = 0; nHeight < 6215971; nHeight += 1) {	// Test a few blocks past money limit block
        CAmount nSubsidy = GetBlockSubsidy(nHeight, chainParams->GetConsensus());
        nSum += nSubsidy;
        BOOST_CHECK(MoneyRange(nSum));
	}
    BOOST_CHECK_EQUAL(nSum, 8399999998750000ULL);
}


bool ReturnFalse() { return false; }
bool ReturnTrue() { return true; }

BOOST_AUTO_TEST_CASE(test_combiner_all)
{
    boost::signals2::signal<bool (), CombinerAll> Test;
    BOOST_CHECK(Test());
    Test.connect(&ReturnFalse);
    BOOST_CHECK(!Test());
    Test.connect(&ReturnTrue);
    BOOST_CHECK(!Test());
    Test.disconnect(&ReturnFalse);
    BOOST_CHECK(Test());
    Test.disconnect(&ReturnTrue);
    BOOST_CHECK(Test());
}
BOOST_AUTO_TEST_SUITE_END()
