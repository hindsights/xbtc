#include "Validator.hpp"
#include "CoinView.hpp"
#include "script/ScriptVM.hpp"
#include "script/Script.hpp"
#include "data/Block.hpp"
#include "data/Coin.hpp"
#include "util/Key.hpp"
#include "AppInfo.hpp"
#include "AppConfig.hpp"
#include "Consensus.hpp"
#include "Compatibility.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/log/log.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/data/big_number_io.hpp>
#include <xul/io/data_encoding.hpp>

#include <functional>
#include <unordered_map>


namespace xbtc {


static const std::string SCRIPT_EMPTY;

class TransactionSignatureChecker : public SignatureChecker
{
public:
    const Transaction* transaction;
    int index;
    Transaction temptx;

    explicit TransactionSignatureChecker(const Transaction* tx, int idx) : transaction(tx), index(idx)
    {
    }
    virtual bool check(const std::string& sig, const std::string& pubkey, const std::string& code)
    {
        if (sig.empty())
        {
            assert(false);
            return false;
        }
        if (pubkey.empty())
        {
            assert(false);
            return false;
        }
//        assert(sig.size() == 71);
        PublicKey key;
        if (!key.assign(pubkey))
        {
            assert(false);
            return false;
        }
        std::string tempsig = sig;
        int hashType = tempsig.back();
        assert(hashType == 1 || hashType == 0);
        if (hashType == 0)
        {
            XUL_APP_REL_WARN("TransactionSignatureChecker::check hashType is zero " << transaction->getHash() << " " << index
                << " " << xul::hex_encoding::upper_case().encode(sig)
                << " " << xul::hex_encoding::upper_case().encode(pubkey)
                << " " << xul::hex_encoding::upper_case().encode(code));
        }
        tempsig.pop_back();
        uint256 sighash = hashSignature(code, hashType);
        if (!key.verify(sighash, tempsig))
        {
//            assert(false);
            return false;
        }
        return true;
    }
    uint256 hashSignature(const std::string& code, int hashType)
    {
        assert(index >= 0 && index < transaction->inputs.size());
        temptx = *transaction;
        for (int i = 0; i < temptx.inputs.size(); ++i)
        {
            temptx.inputs[i].signatureScript = SCRIPT_EMPTY;
        }
        temptx.inputs[index].signatureScript = code;
        uint256 hash = Hasher256::hash_data(temptx, hashType);
        // std::string hashstr = xul::data_encoding::little_endian().encode(temptx, hashType);
        // XUL_APP_WARN("hashSignature " << xul::hex_encoding::lower_case().encode(hashstr) << " " << hash);
        return hash;
    }

};

SignatureChecker* createSignatureChecker(const Transaction* tx, int idx)
{
    return new TransactionSignatureChecker(tx, idx);
}


class ValidatorImpl : public xul::object_impl<Validator>
{
public:
    explicit ValidatorImpl(CoinView* coinView, const AppConfig* config) : m_coinView(coinView), m_config(config)
    {
        XUL_LOGGER_INIT("Validator");
        XUL_REL_EVENT("new");
    }
    ~ValidatorImpl()
    {
        XUL_REL_EVENT("delete");
    }
    virtual bool validateBlockHeader(const BlockHeader& header)
    {
        if (!checkProofOfWork(header))
            return false;
        return true;
    }
    virtual bool validateBlockIndex(const BlockIndex* block)
    {
        if (!validateBlockHeader(block->header))
            return false;
        return true;
    }
    virtual bool validateBlock(const Block* block, const BlockIndex* blockIndex)
    {
        return true;
    }
    virtual bool verifyTransactions(const Block* block, const BlockIndex* blockIndex)
    {
        if (!checkDuplicateTransaction(block, blockIndex))
            return false;
        if (!verifyTransactionInputs(block, blockIndex))
            return false;
        return true;
    }
private:
    bool checkDuplicateTransaction(const Block* block, const BlockIndex* blockIndex)
    {
        bool forbidDuplicateTransaction = Compatibility::forbidDuplicateTransaction(blockIndex);
        if (!forbidDuplicateTransaction)
            return true;
        for (const auto& tx : block->transactions)
        {
            for (int i = 0; i < tx.outputs.size(); ++i)
            {
                XUL_DEBUG("checkDuplicateTransaction " << blockIndex->height << " " << tx.getHash()
                          << " " << xul::hex_encoding::upper_case().encode(tx.outputs[i].scriptPublicKey));
                if (m_coinView->hasCoin(TransactionOutPoint(tx.getHash(), i)))
                {
                    XUL_ERROR("checkDuplicateTransaction invalid transaction " << tx.getHash() << " " << xul::make_tuple(blockIndex->height, i));
                    assert(false);
                    return false;
                }
            }
        }
        return true;
    }
    bool verifyTransactionInput(const Block* block, const BlockIndex* blockIndex, int txindex, int index)
    {
        const Transaction& tx = block->transactions[txindex];
        if (tx.isCoinBase())
            return true;
        if (blockIndex->height == 515)
        {
            XUL_EVENT("verifyTransactionInput checkpoint " << blockIndex->height);
        }
        const TransactionInput& txin = tx.inputs[index];
        XUL_DEBUG("verifyTransactionInput " << index << " " << blockIndex->height << " " << tx.getHash()
                  << " " << txin.previousOutput.hash << " " << txin.previousOutput.index);
        TransactionSignatureChecker checker(&tx, index);
        ScriptVM vm(checker);
        Coin* coin = m_coinView->fetchCoin(txin.previousOutput);
        const TransactionOutput* txout = nullptr;
        if (coin && coin->output.value > 0)
        {
            txout = &coin->output;
        }
        else
        {
            XUL_DEBUG("verifyTransactionInput invalid coin " << tx.getHash() << " " << txin.previousOutput.hash << " " << blockIndex->height);
//            assert(false);
            for (int i = 0; i < txindex; ++i)
            {
                const Transaction& prevtx = block->transactions[i];
                if (prevtx.getHash() == txin.previousOutput.hash)
                {
                    assert(txin.previousOutput.index >= 0 && txin.previousOutput.index < prevtx.outputs.size());
                    txout = &prevtx.outputs[txin.previousOutput.index];
                    break;
                }
            }
            for (int i = 0; i < block->transactions.size(); ++i)
            {
                if (i == txindex)
                    continue;
                const Transaction& prevtx = block->transactions[i];
                if (prevtx.getHash() == txin.previousOutput.hash)
                {
                    assert(txin.previousOutput.index >= 0 && txin.previousOutput.index < prevtx.outputs.size());
                    txout = &prevtx.outputs[txin.previousOutput.index];
                    break;
                }
            }
            if (txout == nullptr)
            {
                XUL_WARN("verifyTransactionInput no prevout " << tx.getHash() << " " << txin.previousOutput.hash << " " << txin.previousOutput.index << " " << blockIndex->height);
                assert(false);
                return false;
            }
        }
        if (!vm.eval(txin.signatureScript))
        {
            XUL_WARN("verifyTransactionInput failed to eval input script: " << tx.getHash() << " " << txin.previousOutput.hash
                << " " << xul::hex_encoding::lower_case().encode(txin.signatureScript) << " " << blockIndex->height);
            assert(false);
            return false;
        }
        if (!vm.eval(txout->scriptPublicKey))
        {
            XUL_WARN("verifyTransactionInput failed to eval output script: " << tx.getHash() << " " << txin.previousOutput.hash
                << " " << xul::hex_encoding::lower_case().encode(txout->scriptPublicKey) << " " << blockIndex->height);
            assert(false);
            return false;
        }
        bool ret;
        if (!vm.getBoolValue(-1, ret) || !ret)
        {
            XUL_WARN("verifyTransactionInput eval failed: " << tx.getHash() << " " << txin.previousOutput.hash
                     << " " << xul::hex_encoding::lower_case().encode(txin.signatureScript)
                     << " " << xul::hex_encoding::lower_case().encode(txout->scriptPublicKey) << " " << blockIndex->height);
            assert(false);
            return false;
        }
        return true;
    }
    bool checkProofOfWork(const BlockHeader& header)
    {
        uint256 target;
        if (!Consensus::decodeCompactNumber(target, header.bits))
        {
            XUL_WARN("checkProofOfWork invalid bits " << header.bits);
            return false;
        }
        if (header.hash > target)
        {
            XUL_WARN("checkProofOfWork invalid pow " << header.bits << header.hash << " " << target);
            return false;
        }
        return true;
    }
    bool verifyTransactionInputs(const Block* block, const BlockIndex* blockIndex)
    {
        for (int i = 0; i < block->transactions.size(); ++i)
        {
            for (int j = 0; j < block->transactions[i].inputs.size(); ++j)
            {
                if (!verifyTransactionInput(block, blockIndex, i, j))
                    return false;
            }
        }
        return true;
    }
private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<CoinView> m_coinView;
    boost::intrusive_ptr<const AppConfig> m_config;
};


Validator* createValidator(CoinView* coinView, const AppConfig* config)
{
    return new ValidatorImpl(coinView, config);
}


}
