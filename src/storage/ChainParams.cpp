#include "ChainParams.hpp"
#include "data/Block.hpp"
#include "script/Script.hpp"
#include "data/MerkleTree.hpp"
#include "flags.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/io/data_encoding.hpp>
#include <xul/log/log.hpp>
#include <xul/data/big_number_io.hpp>

namespace xbtc {


static Block* doCreateGenesisBlock(const char* timestampstr, const std::string& genesisOutputScript, uint32_t timestamp, uint32_t nonce, uint32_t bits, int32_t version, const int64_t& genesisReward)
{
    Transaction txNew;
    txNew.version = 1;
    txNew.inputs.resize(1);
    txNew.outputs.resize(1);
    txNew.inputs[0].signatureScript = xul::data_encoding::little_endian().encode(ScriptIntegerWriter(486604799), ScriptNumberWriter(4), ScriptStringWriter(timestampstr));
    txNew.outputs[0].value = genesisReward;
    txNew.outputs[0].scriptPublicKey = genesisOutputScript;
    txNew.computeHash();

    Block* genesis = createBlock();
    BlockHeader& header = genesis->header;
    header.timestamp = timestamp;
    header.bits = bits;
    header.nonce = nonce;
    header.version = version;
    genesis->transactions.push_back(txNew);
    header.merkleRootHash = MerkleTree::build(genesis);
    header.computeHash();
    XUL_APP_DEBUG("genesis block " << header.merkleRootHash);
    return genesis;
}

static Block* createGenesisBlock(uint32_t timestamp, uint32_t nonce, uint32_t bits, int32_t version, const int64_t& genesisReward)
{
    const char* timestampstr = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    std::string s;
    ScriptUtils::parseHex(s, "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f");
    std::string genesisOutputScript = xul::data_encoding::little_endian().encode(ScriptStringWriter(s), static_cast<uint8_t>(OP_CHECKSIG));
    return doCreateGenesisBlock(timestampstr, genesisOutputScript, timestamp, nonce, bits, version, genesisReward);
}

static ChainParams* createChainParams()
{
    return xul::create_object<ChainParams>();
}

ChainParams* createMainChainParams()
{
    ChainParams* params = createChainParams();
    params->genesisBlock = createGenesisBlock(1231006505, 2083236893, 0x1d00ffff, 1, 50 * COIN);
    params->protocolMagic = 0xD9B4BEF9;
    params->defaultPort = 8333;
    params->dnsSeeds.emplace_back("seed.bitcoin.sipa.be"); // Pieter Wuille, only supports x1, x5, x9, and xd
    params->dnsSeeds.emplace_back("dnsseed.bluematt.me"); // Matt Corallo, only supports x9
    params->dnsSeeds.emplace_back("dnsseed.bitcoin.dashjr.org"); // Luke Dashjr
    params->dnsSeeds.emplace_back("seed.bitcoinstats.com"); // Christian Decker, supports x1 - xf
    params->dnsSeeds.emplace_back("seed.bitcoin.jonasschnelli.ch"); // Jonas Schnelli, only supports x1, x5, x9, and xd
    params->dnsSeeds.emplace_back("seed.btc.petertodd.org"); // Peter Todd, only supports x1, x5, x9, and xd
    return params;
}


ChainParams* createTestNetChainParams()
{
    ChainParams* params = createChainParams();
    params->genesisBlock = createGenesisBlock(1296688602, 414098458, 0x1d00ffff, 1, 50 * COIN);
    params->protocolMagic = 0x0709110B;
    params->defaultPort = 18333;
    params->dnsSeeds.emplace_back("testnet-seed.bitcoin.jonasschnelli.ch");
    params->dnsSeeds.emplace_back("seed.tbtc.petertodd.org");
    params->dnsSeeds.emplace_back("seed.testnet.bitcoin.sprovoost.nl");
    params->dnsSeeds.emplace_back("testnet-seed.bluematt.me"); // Just a static list of stable node(s), only supports x9
    return params;
}


}
