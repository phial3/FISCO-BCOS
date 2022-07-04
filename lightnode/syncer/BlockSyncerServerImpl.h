#pragma once

#include <bcos-concepts/Block.h>
#include <bcos-concepts/ledger/Ledger.h>
#include <bcos-concepts/sync/SyncBlockMessages.h>

namespace bcos::sync
{

template <bcos::concepts::ledger::Ledger LedgerType, bcos::concepts::sync::RequestBlock RequestType,
    bcos::concepts::sync::ResponseBlock ResponseType>
class BlockSyncerServer
{
public:
    ResponseType onRequestBlock(RequestType const& request)
    {
        auto currentStatus = m_ledger.getTotalTransactionCount();
        auto blockNumber = currentStatus;

        ResponseType response;
        response.blockHeight = blockNumber;
        if (request.beginBlockNumber > blockNumber)
        {
            return response;
        }

        for (auto currentBlockNumber : std::ranges::iota_view{
                 request.beginBlockNumber, std::min(blockNumber, request.endBlockNumber)})
        {
            response.blocks.emplace_back(
                m_ledger.template getBlock<concepts::ledger::HEADER>(currentBlockNumber));
        }

        return response;
    }

private:
    LedgerType m_ledger;
};
}  // namespace bcos::sync