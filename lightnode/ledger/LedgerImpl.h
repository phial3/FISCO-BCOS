#pragma once

#include <bcos-concepts/Basic.h>
#include <bcos-concepts/Block.h>
#include <bcos-concepts/Receipt.h>
#include <bcos-concepts/Transaction.h>
#include <bcos-concepts/ledger/Ledger.h>
#include <bcos-concepts/storage/Storage.h>
#include <bcos-crypto/hasher/Hasher.h>
#include <bcos-framework/ledger/LedgerTypeDef.h>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <ranges>
#include <stdexcept>
#include <tuple>

#ifndef LEDGER_LOG
#define LEDGER_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("LEDGER")
#endif

namespace bcos::ledger
{

template <bcos::crypto::hasher::Hasher Hasher, bcos::concepts::storage::Storage Storage,
    bcos::concepts::block::Block Block>
class LedgerImpl : public bcos::concepts::ledger::LedgerBase<LedgerImpl<Hasher, Storage, Block>>
{
public:
    LedgerImpl(Storage storage) : m_storage{std::move(storage)} {}

    template <bcos::concepts::ledger::DataFlag... Flags>
    auto impl_getBlock(bcos::concepts::block::BlockNumber auto blockNumber)
    {
        Block block;
        auto blockNumberStr = boost::lexical_cast<std::string>(blockNumber);
        (getBlockData<Flags>(blockNumberStr, block), ...);
        return block;
    }

    void impl_setBlock(Storage& storage, bcos::concepts::block::Block auto block)
    {
        if (block.blockHeader.data.blockNumber == 0 && !std::empty(block.transactions))
            return;

        auto blockNumberStr = boost::lexical_cast<std::string>(block.blockHeader.data.blockNumber);

        // number 2 entry
        bcos::storage::Entry numberEntry;
        numberEntry.importFields({blockNumberStr});
        storage.setRow(SYS_CURRENT_STATE, SYS_KEY_CURRENT_NUMBER, std::move(numberEntry));

        // number 2 hash
        bcos::storage::Entry hashEntry;
        hashEntry.importFields({block.blockHeader.dataHash});
        storage.setRow(SYS_NUMBER_2_HASH, blockNumberStr, std::move(hashEntry));

        // hash 2 number
        bcos::storage::Entry hash2NumberEntry;
        hash2NumberEntry.importFields({blockNumberStr});
        storage.setRow(SYS_HASH_2_NUMBER,
            std::string_view{block.blockHeader.dataHash.data(), block.blockHeader.dataHash.size()},
            std::move(hash2NumberEntry));

        // number 2 header
        bcos::storage::Entry number2HeaderEntry;
        number2HeaderEntry.importFields({bcos::concepts::serialize::encode(block.blockHeader)});
        storage.setRow(SYS_NUMBER_2_BLOCK_HEADER, blockNumberStr, std::move(number2HeaderEntry));

        // number 2 nonce
        std::remove_cvref_t<decltype(block)> blockNonceList;
        blockNonceList.nonceList = std::move(block.nonceList);
        bcos::storage::Entry number2NonceEntry;
        number2NonceEntry.importFields({bcos::concepts::serialize::encode(blockNonceList)});
        storage.setRow(SYS_BLOCK_NUMBER_2_NONCES, blockNumberStr, std::move(number2NonceEntry));

        // number 2 transactions
        std::remove_cvref_t<decltype(block)> transactionsBlock;
        transactionsBlock.transactionsMetaData = std::move(block.transactionsMetaData);
        if (std::empty(transactionsBlock.transactionsMetaData) && !std::empty(block.transactions))
        {
            transactionsBlock.transactionsMetaData.resize(block.transactions.size());
#pragma omp parallel for
            for (auto i = 0u; i < block.transactions.size(); ++i)
            {
                transactionsBlock.transactionsMetaData[i].hash =
                    bcos::concepts::hash::calculate<Hasher>(block.transactions[i]);
                transactionsBlock.transactionsMetaData[i].to =
                    std::move(block.transactions[i].data.to);
            }
        }
        bcos::storage::Entry number2TransactionHashesEntry;
        number2TransactionHashesEntry.importFields(
            {bcos::concepts::serialize::encode(transactionsBlock)});
        storage.setRow(SYS_NUMBER_2_TXS, blockNumberStr, std::move(number2TransactionHashesEntry));

        // hash 2 receipts
        size_t totalTransactionCount = 0;
        size_t failedTransactionCount = 0;
#pragma omp parallel for
        for (auto i = 0u; i < block.receipts.size(); ++i)
        {
            auto& hash = transactionsBlock.transactionsMetaData[i].hash;
            auto& receipt = block.receipts[i];
            if (receipt.data.status != 0)
            {
#pragma omp atomic
                ++failedTransactionCount;
            }
#pragma omp atomic
            ++totalTransactionCount;

            bcos::storage::Entry receiptEntry;
            receiptEntry.importFields({bcos::concepts::serialize::encode(receipt)});
#pragma omp critical
            storage.setRow(SYS_HASH_2_RECEIPT, std::string_view{hash.data(), hash.size()},
                std::move(receiptEntry));
        }

        LEDGER_LOG(DEBUG) << LOG_DESC("Calculate tx counts in block")
                          << LOG_KV("number", blockNumberStr)
                          << LOG_KV("totalCount", totalTransactionCount)
                          << LOG_KV("failedCount", failedTransactionCount);

        auto transactionCount = impl_getTotalTransactionCount();
        transactionCount.total += totalTransactionCount;
        transactionCount.failed += failedTransactionCount;

        bcos::storage::Entry totalEntry;
        totalEntry.importFields({boost::lexical_cast<std::string>(transactionCount.total)});
        storage.setRow(SYS_CURRENT_STATE, SYS_KEY_TOTAL_TRANSACTION_COUNT, std::move(totalEntry));

        if (transactionCount.failed > 0)
        {
            bcos::storage::Entry failedEntry;
            failedEntry.importFields({boost::lexical_cast<std::string>(transactionCount.failed)});
            storage.setRow(
                SYS_CURRENT_STATE, SYS_KEY_TOTAL_FAILED_TRANSACTION, std::move(failedEntry));
        }

        LEDGER_LOG(INFO) << LOG_DESC("setBlock")
                         << LOG_KV("number", block.blockHeader.data.blockNumber)
                         << LOG_KV("totalTxs", transactionCount.total)
                         << LOG_KV("failedTxs", transactionCount.failed)
                         << LOG_KV("incTxs", totalTransactionCount)
                         << LOG_KV("incFailedTxs", failedTransactionCount);
    }

    template <concepts::ledger::DataFlag Flag>
    auto impl_getTransactionsOrReceipts(std::ranges::range auto const& hashes)
    {
        if constexpr (!std::is_same_v<Flag, concepts::ledger::TRANSACTIONS> &&
                      !std::is_same_v<Flag, concepts::ledger::RECEIPTS>)
        {
            static_assert(!sizeof(hashes), "Unspported data flag!");
        }

        using OutputItemType =
            std::conditional_t<std::is_same_v<Flag, concepts::ledger::TRANSACTIONS>,
                std::ranges::range_value_t<decltype(Block::transactions)>,
                std::ranges::range_value_t<decltype(Block::receipts)>>;
        constexpr auto tableName = std::is_same_v<Flag, concepts::ledger::TRANSACTIONS> ?
                                       SYS_HASH_2_TX :
                                       SYS_HASH_2_RECEIPT;
        auto entries = m_storage.getRows(std::string_view{tableName}, hashes);
        std::vector<OutputItemType> outputs(std::size(entries));

#pragma omp parallel for
        for (auto i = 0u; i < std::size(entries); ++i)
        {
            if (!entries[i]) [[unlikely]]
                BOOST_THROW_EXCEPTION(std::runtime_error{"Get transaction not found"});
            auto field = entries[i]->getField(0);
            bcos::concepts::serialize::decode(outputs[i], field);
        }
        return outputs;
    }

    struct TransactionCount
    {
        int64_t total;
        int64_t failed;
        int64_t blockNumber;
    };
    TransactionCount impl_getTotalTransactionCount()
    {
        LEDGER_LOG(INFO) << "GetTotalTransactionCount request";
        constexpr static auto keys = std::to_array({SYS_KEY_TOTAL_TRANSACTION_COUNT,
            SYS_KEY_TOTAL_FAILED_TRANSACTION, SYS_KEY_CURRENT_NUMBER});

        TransactionCount transactionCount;
        auto entries = m_storage.getRows(SYS_CURRENT_STATE, keys);
        for (auto i = 0u; i < std::size(entries); ++i)
        {
            auto& entry = entries[i];

            int64_t value = 0;
            if (!entry) [[unlikely]]
            {
                LEDGER_LOG(WARNING)
                    << "GetTotalTransactionCount error" << LOG_KV("index", i) << " empty";
            }
            else
            {
                value = boost::lexical_cast<int64_t>(entry->getField(0));
            }

            switch (i)
            {
            case 0:
                transactionCount.total = value;
                break;
            case 1:
                transactionCount.failed = value;
                break;
            case 2:
                transactionCount.blockNumber = value;
                break;
            default:
                BOOST_THROW_EXCEPTION(std::runtime_error{"Unexpected getRows index"});
                break;
            }
        }

        return transactionCount;
    }

    template <bool isTransaction>
    void impl_setTransactionOrReceiptBuffers(
        std::ranges::range auto const& hashes, std::ranges::range auto buffers)
    {
        auto count = std::size(buffers);
        if (count != std::size(hashes))
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{"No match count"});
        }
        constexpr auto tableName = isTransaction ? SYS_HASH_2_TX : SYS_HASH_2_RECEIPT;

        for (auto i = 0u; i < count; ++i)
        {
            bcos::storage::Entry entry;
            entry.importFields({std::move(buffers[i])});

            auto&& hash = hashes[i];
            m_storage.setRow(
                tableName, std::string_view(std::data(hash), std::size(hash)), std::move(entry));
        }
    }

private:
    template <bcos::concepts::ledger::DataFlag Flag>
    void getBlockData(std::string_view key, Block& block)
    {
        if constexpr (std::is_same_v<Flag, bcos::concepts::ledger::HEADER>)
        {
            auto entry = m_storage.getRow(SYS_NUMBER_2_BLOCK_HEADER, key);
            if (!entry) [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(std::runtime_error{"GetBlock not found!"});
            }

            auto field = entry->getField(0);
            bcos::concepts::serialize::decode(block.blockHeader, field);
        }
        else if constexpr (std::is_same_v<Flag, concepts::ledger::TRANSACTIONS> ||
                           std::is_same_v<Flag, concepts::ledger::RECEIPTS>)
        {
            if (std::empty(block.transactionsMetaData))
            {
                auto entry = m_storage.getRow(SYS_NUMBER_2_TXS, key);
                if (!entry) [[unlikely]]
                    BOOST_THROW_EXCEPTION(std::runtime_error{"GetBlock not found!"});

                auto field = entry->getField(0);
                Block metadataBlock;
                bcos::concepts::serialize::decode(metadataBlock, field);
                block.transactionsMetaData = std::move(metadataBlock.transactionsMetaData);
            }

            auto hashesRange =
                block.transactionsMetaData |
                std::views::transform(
                    [](typename decltype(block.transactionsMetaData)::value_type const& metaData) {
                        return std::string_view{metaData.hash.data(), metaData.hash.size()};
                    });

            auto outputs = getTransactionsOrReceipts<Flag>(std::move(hashesRange));
            if constexpr (std::is_same_v<Flag, concepts::ledger::TRANSACTIONS>)
            {
                block.transactions = std::move(outputs);
            }
            else
            {
                block.receipts = std::move(outputs);
            }
        }
        else if constexpr (std::is_same_v<Flag, concepts::ledger::NONCES>)
        {}
        else if constexpr (std::is_same_v<Flag, concepts::ledger::ALL>)
        {
            getBlockData<concepts::ledger::HEADER>(key, block);
            getBlockData<concepts::ledger::TRANSACTIONS>(key, block);
            getBlockData<concepts::ledger::RECEIPTS>(key, block);
            getBlockData<concepts::ledger::NONCES>(key, block);
        }
        else
        {
            static_assert(!sizeof(Block), "Wrong input flag!");
        }
    }

    Storage m_storage;
};

}  // namespace bcos::ledger