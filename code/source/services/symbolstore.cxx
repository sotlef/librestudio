/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The symbols kept in memory, one shard per file.  Everything the index knows
// about a file is replaced together, so that a re-analysis never leaves half of
// the old answer behind; a store on disk would keep the same shape.

#include "symbolstore.hxx"

#include <comphelper/sequence.hxx>

#include <algorithm>
#include <map>
#include <mutex>
#include <set>

using namespace css;
using namespace css::code::index;

namespace codeservices
{
namespace
{
struct Shard
{
    sal_Int64 nVersion = -1;
    std::vector<Symbol> aSymbols;
};

class SymbolStore final : public cppu::WeakImplHelper<XSymbolStore>
{
public:
    void SAL_CALL replaceDocument(const OUString& rUrl, sal_Int64 nVersion,
                                  const uno::Sequence<Symbol>& rSymbols) override
    {
        std::scoped_lock aGuard(m_aMutex);
        Forget(rUrl);
        Shard& rShard = m_aShards[rUrl];
        rShard.nVersion = nVersion;
        rShard.aSymbols.assign(rSymbols.begin(), rSymbols.end());
        for (const Symbol& rSymbol : rShard.aSymbols)
            m_aUrlsById[rSymbol.Id].insert(rUrl);
    }

    void SAL_CALL removeDocument(const OUString& rUrl) override
    {
        std::scoped_lock aGuard(m_aMutex);
        Forget(rUrl);
        m_aShards.erase(rUrl);
    }

    sal_Int64 SAL_CALL getDocumentVersion(const OUString& rUrl) override
    {
        std::scoped_lock aGuard(m_aMutex);
        auto it = m_aShards.find(rUrl);
        return it == m_aShards.end() ? -1 : it->second.nVersion;
    }

    uno::Sequence<OUString> SAL_CALL getDocumentUrls() override
    {
        std::scoped_lock aGuard(m_aMutex);
        std::vector<OUString> aUrls;
        for (const auto& rEntry : m_aShards)
            aUrls.push_back(rEntry.first);
        return comphelper::containerToSequence(aUrls);
    }

    uno::Sequence<Symbol> SAL_CALL getDocumentSymbols(const OUString& rUrl) override
    {
        std::scoped_lock aGuard(m_aMutex);
        auto it = m_aShards.find(rUrl);
        if (it == m_aShards.end())
            return {};
        return comphelper::containerToSequence(it->second.aSymbols);
    }

    uno::Sequence<Symbol> SAL_CALL findByName(const OUString& rQuery, sal_Int32 nLimit) override;
    uno::Sequence<Symbol> SAL_CALL findById(const OUString& rId) override;

private:
    // drops the Id entries of a file's current shard
    void Forget(const OUString& rUrl)
    {
        auto it = m_aShards.find(rUrl);
        if (it == m_aShards.end())
            return;
        for (const Symbol& rSymbol : it->second.aSymbols)
        {
            auto itId = m_aUrlsById.find(rSymbol.Id);
            if (itId == m_aUrlsById.end())
                continue;
            itId->second.erase(rUrl);
            if (itId->second.empty())
                m_aUrlsById.erase(itId);
        }
    }

    std::mutex m_aMutex;
    std::map<OUString, Shard> m_aShards;
    std::map<OUString, std::set<OUString>> m_aUrlsById;
};

uno::Sequence<Symbol> SymbolStore::findByName(const OUString& rQuery, sal_Int32 nLimit)
{
    const OUString aQuery = rQuery.toAsciiLowerCase();
    struct Hit
    {
        int nRank; // 0 = exact, 1 = prefix, 2 = elsewhere in the name
        const Symbol* pSymbol;
    };
    std::vector<Hit> aHits;
    {
        std::scoped_lock aGuard(m_aMutex);
        for (const auto& rEntry : m_aShards)
            for (const Symbol& rSymbol : rEntry.second.aSymbols)
            {
                const OUString aName = rSymbol.Name.toAsciiLowerCase();
                const sal_Int32 nAt = aName.indexOf(aQuery);
                if (nAt < 0)
                    continue;
                const int nRank = aName.getLength() == aQuery.getLength() ? 0 : nAt == 0 ? 1 : 2;
                aHits.push_back({ nRank, &rSymbol });
            }
        // the pointers stay valid only under the lock: sort and copy before it is released
        std::stable_sort(aHits.begin(), aHits.end(),
                         [](const Hit& rLeft, const Hit& rRight)
                         {
                             if (rLeft.nRank != rRight.nRank)
                                 return rLeft.nRank < rRight.nRank;
                             return rLeft.pSymbol->Name.getLength() < rRight.pSymbol->Name.getLength();
                         });
        if (nLimit > 0 && aHits.size() > static_cast<size_t>(nLimit))
            aHits.resize(nLimit);
        std::vector<Symbol> aResult;
        aResult.reserve(aHits.size());
        for (const Hit& rHit : aHits)
            aResult.push_back(*rHit.pSymbol);
        return comphelper::containerToSequence(aResult);
    }
}

uno::Sequence<Symbol> SymbolStore::findById(const OUString& rId)
{
    std::vector<Symbol> aResult;
    {
        std::scoped_lock aGuard(m_aMutex);
        auto itId = m_aUrlsById.find(rId);
        if (itId != m_aUrlsById.end())
            for (const OUString& rUrl : itId->second)
                for (const Symbol& rSymbol : m_aShards[rUrl].aSymbols)
                    if (rSymbol.Id == rId)
                        aResult.push_back(rSymbol);
    }
    // definitions first
    std::stable_partition(aResult.begin(), aResult.end(),
                          [](const Symbol& rSymbol) { return rSymbol.IsDefinition; });
    return comphelper::containerToSequence(aResult);
}
}

uno::Reference<XSymbolStore> CreateSymbolStore() { return new SymbolStore; }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
