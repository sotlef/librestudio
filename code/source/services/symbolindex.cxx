/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// theSymbolIndex: the symbols of the open projects and documents.
//
// When a project opens, a background job reads each of its files a provider
// understands and hands the declarations it finds to the store.  A document
// that is edited is analysed again after a short pause, from the text in the
// editor (not the file), and its shard in the store is replaced; a document
// that is closed falls back to what is on disk.  A provider is called on
// threads of its own, never on the UI thread; a lookup (findDefinition) is the
// one exception, it analyses the file it is asked about on the caller's thread.

#include "backgroundjob.hxx"
#include "common.hxx"
#include "symbolstore.hxx"

#include <com/sun/star/code/core/theJobRegistry.hpp>
#include <com/sun/star/code/document/DocumentEvent.hpp>
#include <com/sun/star/code/document/XDocument.hpp>
#include <com/sun/star/code/document/XDocumentListener.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/index/IndexEvent.hpp>
#include <com/sun/star/code/index/XCompileCommandProvider.hpp>
#include <com/sun/star/code/index/XSymbolIndex.hpp>
#include <com/sun/star/code/index/XSymbolIndexListener.hpp>
#include <com/sun/star/code/index/XSymbolProvider.hpp>
#include <com/sun/star/code/index/theCompileCommandProvider.hpp>
#include <com/sun/star/code/project/ItemKind.hpp>
#include <com/sun/star/code/project/ProjectEvent.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectFiles.hpp>
#include <com/sun/star/code/project/XProjectItem.hpp>
#include <com/sun/star/code/project/XProjectListener.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <rtl/character.hxx>
#include <rtl/ref.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <chrono>
#include <map>
#include <set>
#include <thread>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::index;

namespace codeservices
{
namespace
{
// How long a document must be left alone before it is analysed: typing a word
// is one analysis, not one per key.
constexpr std::chrono::milliseconds Pause(250);
// How many files a scan analyses before it tells the listeners of them.
constexpr sal_Int32 ReportEvery = 25;

class SymbolIndex final
    : public comphelper::WeakComponentImplHelper<XSymbolIndex, document::XDocumentListener,
                                                 project::XProjectListener, lang::XServiceInfo>
{
public:
    explicit SymbolIndex(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
        , m_xStore(CreateSymbolStore())
    {
    }

    // Subscribes to the documents and projects; not in the constructor, which
    // cannot hand out `this`.
    void Attach();

    // XSymbolIndex
    void SAL_CALL registerProvider(const uno::Reference<XSymbolProvider>& xProvider) override;
    void SAL_CALL unregisterProvider(const OUString& rName) override;
    uno::Sequence<uno::Reference<XSymbolProvider>> SAL_CALL getProviders() override;
    uno::Reference<XSymbolStore> SAL_CALL getStore() override;
    void SAL_CALL setStore(const uno::Reference<XSymbolStore>& xStore) override;
    IndexState SAL_CALL getState() override;
    uno::Sequence<Symbol> SAL_CALL getDocumentSymbols(const OUString& rUrl) override;
    uno::Sequence<Symbol> SAL_CALL findSymbols(const OUString& rQuery, sal_Int32 nLimit) override;
    uno::Sequence<Symbol> SAL_CALL findDefinition(const OUString& rUrl,
                                                  const css::code::core::Position& rPosition) override;
    void SAL_CALL addIndexListener(const uno::Reference<XSymbolIndexListener>& xListener) override;
    void SAL_CALL
    removeIndexListener(const uno::Reference<XSymbolIndexListener>& xListener) override;

    // XDocumentListener
    void SAL_CALL documentChanged(const document::DocumentEvent& rEvent) override;
    // XProjectListener
    void SAL_CALL projectChanged(const project::ProjectEvent& rEvent) override;
    void SAL_CALL disposing(const lang::EventObject&) override {}

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.SymbolIndex"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;

    uno::Reference<XSymbolProvider> ProviderFor(const OUString& rUrl);
    uno::Sequence<OUString> ArgumentsFor(const OUString& rUrl);
    // Analyses one file and replaces its shard; false if it is not text or no
    // provider knows it (and then its shard is dropped).
    bool IndexFile(const OUString& rUrl);
    // Starts the pause for a document; whatever was pending for it is outdated.
    void Schedule(const OUString& rUrl);
    void Analyse(const OUString& rUrl, sal_uInt64 nTicket);
    void ScanProject(const OUString& rProjectUrl);
    void DropProject(const OUString& rProjectUrl);
    void ScanAllProjects();
    void Fire(IndexState eState, sal_Int32 nDone, sal_Int32 nTotal,
              const std::vector<OUString>& rUrls);
    uno::Reference<XSymbolStore> Store();

    uno::Reference<uno::XComponentContext> m_xContext;
    uno::Reference<XSymbolStore> m_xStore;
    std::vector<uno::Reference<XSymbolProvider>> m_aProviders;
    // The ticket of the newest request per document; an older one finds out it
    // was overtaken when its pause is over.
    std::map<OUString, sal_uInt64> m_aTickets;
    sal_uInt64 m_nLastTicket = 0;
    // the files of each open project, as of the last scan
    std::map<OUString, std::set<OUString>> m_aProjectFiles;
    sal_Int32 m_nScans = 0; // running scans: INDEXING while there is one
    comphelper::OInterfaceContainerHelper4<XSymbolIndexListener> m_aListeners;
};

void SymbolIndex::Attach()
{
    document::theDocumentManager::get(m_xContext)
        ->addDocumentListener(uno::Reference<document::XDocumentListener>(this));
    project::theProjectManager::get(m_xContext)
        ->addProjectListener(uno::Reference<project::XProjectListener>(this));
    // what was opened before we came into existence
    for (const auto& xDocument : document::theDocumentManager::get(m_xContext)->getOpenDocuments())
        Schedule(xDocument->getUrl());
    ScanAllProjects();
}

void SymbolIndex::ScanAllProjects()
{
    for (const auto& xProject : project::theProjectManager::get(m_xContext)->getOpenProjects())
        ScanProject(xProject->getUrl());
}

uno::Reference<XSymbolStore> SymbolIndex::Store()
{
    std::unique_lock aGuard(m_aMutex);
    return m_xStore;
}

uno::Reference<XSymbolProvider> SymbolIndex::ProviderFor(const OUString& rUrl)
{
    const OUString aMime = GetMimeType(rUrl);
    std::unique_lock aGuard(m_aMutex);
    for (const auto& xCandidate : m_aProviders)
    {
        const uno::Sequence<OUString> aTypes = xCandidate->getSupportedMimeTypes();
        if (std::find(aTypes.begin(), aTypes.end(), aMime) != aTypes.end())
            return xCandidate;
    }
    return {};
}

uno::Sequence<OUString> SymbolIndex::ArgumentsFor(const OUString& rUrl)
{
    try
    {
        return theCompileCommandProvider::get(m_xContext)->getCompileArguments(rUrl);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.services", "compile arguments of " << rUrl << ": " << rException.Message);
        return {};
    }
}

bool SymbolIndex::IndexFile(const OUString& rUrl)
{
    uno::Reference<XSymbolProvider> xProvider = ProviderFor(rUrl);
    OUString aText;
    if (!xProvider.is() || !ReadText(m_xContext, rUrl, aText))
    {
        Store()->removeDocument(rUrl);
        return false;
    }
    sal_Int64 nVersion = 0;
    try
    {
        uno::Reference<document::XTextDocument> xDocument(
            document::theDocumentManager::get(m_xContext)->getDocument(rUrl), uno::UNO_QUERY);
        if (xDocument.is())
            nVersion = xDocument->getVersion();
    }
    catch (const uno::Exception&)
    {
    }
    try
    {
        Store()->replaceDocument(rUrl, nVersion,
                                 xProvider->extractSymbols(rUrl, aText, ArgumentsFor(rUrl)));
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.services", "symbols of " << rUrl << " failed: " << rException.Message);
        return false;
    }
    return true;
}

// the word (letters, digits, '_') the position is in or at the end of
OUString IdentifierAt(const OUString& rText, const css::code::core::Position& rPosition)
{
    sal_Int32 nStart = 0;
    for (sal_Int32 nLine = 0; nLine < rPosition.Line; ++nLine)
    {
        nStart = rText.indexOf('\n', nStart);
        if (nStart < 0)
            return {};
        ++nStart;
    }
    sal_Int32 nEnd = rText.indexOf('\n', nStart);
    if (nEnd < 0)
        nEnd = rText.getLength();
    auto isWord = [&](sal_Int32 n)
    { return n >= nStart && n < nEnd && (rtl::isAsciiAlphanumeric(rText[n]) || rText[n] == '_'); };
    sal_Int32 nAt = nStart + rPosition.Column;
    if (!isWord(nAt) && isWord(nAt - 1))
        --nAt;
    if (!isWord(nAt))
        return {};
    sal_Int32 nFrom = nAt;
    while (isWord(nFrom - 1))
        --nFrom;
    sal_Int32 nTo = nAt;
    while (isWord(nTo + 1))
        ++nTo;
    return rText.copy(nFrom, nTo - nFrom + 1);
}

void SymbolIndex::Schedule(const OUString& rUrl)
{
    sal_uInt64 nTicket;
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        nTicket = ++m_nLastTicket;
        m_aTickets[rUrl] = nTicket;
    }
    // The thread keeps the index alive while it waits.
    std::thread(
        [xSelf = rtl::Reference<SymbolIndex>(this), rUrl, nTicket]
        {
            std::this_thread::sleep_for(Pause);
            xSelf->Analyse(rUrl, nTicket);
        })
        .detach();
}

void SymbolIndex::Analyse(const OUString& rUrl, sal_uInt64 nTicket)
{
    try
    {
        {
            std::unique_lock aGuard(m_aMutex);
            auto it = m_aTickets.find(rUrl);
            if (m_bDisposed || it == m_aTickets.end() || it->second != nTicket)
                return; // overtaken
            m_aTickets.erase(it);
        }
        IndexFile(rUrl);
        Fire(IndexState_IDLE, 0, 0, { rUrl });
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.services", "indexing " << rUrl << " failed: " << rException.Message);
    }
}

// The files of a project that something can be said about: leaves of kind FILE.
void CollectFiles(const uno::Reference<project::XProjectItem>& xRoot, std::vector<OUString>& rUrls)
{
    std::vector<uno::Reference<project::XProjectItem>> aStack;
    if (xRoot.is())
        aStack.push_back(xRoot);
    while (!aStack.empty())
    {
        uno::Reference<project::XProjectItem> xItem = aStack.back();
        aStack.pop_back();
        const auto aChildren = xItem->getChildren();
        for (const auto& xChild : aChildren)
            aStack.push_back(xChild);
        if (xItem->getKind() == project::ItemKind_FILE && !xItem->getUrl().isEmpty()
            && !aChildren.hasElements())
            rUrls.push_back(xItem->getUrl());
    }
}

void SymbolIndex::ScanProject(const OUString& rProjectUrl)
{
    {
        std::unique_lock aGuard(m_aMutex);
        if (m_bDisposed)
            return;
        ++m_nScans;
    }
    Fire(IndexState_INDEXING, 0, 0, {});

    rtl::Reference<BackgroundJob> xJob = BackgroundJob::Start(
        "code.index.scan:" + rProjectUrl, u"Indexing symbols"_ustr,
        [xSelf = rtl::Reference<SymbolIndex>(this), rProjectUrl](BackgroundJob& rJob)
        {
            std::vector<OUString> aFiles;
            try
            {
                uno::Reference<project::XProjectFiles> xFiles(
                    project::theProjectManager::get(xSelf->m_xContext)->getProject(rProjectUrl),
                    uno::UNO_QUERY);
                if (xFiles.is())
                    CollectFiles(xFiles->getRootItem(), aFiles);
            }
            catch (const uno::Exception& rException)
            {
                SAL_WARN("code.services", "files of " << rProjectUrl << ": " << rException.Message);
            }
            std::erase_if(aFiles, [&](const OUString& rUrl) { return !xSelf->ProviderFor(rUrl).is(); });

            // what the project no longer lists is dropped, unless it is open in an editor
            std::set<OUString> aNow(aFiles.begin(), aFiles.end());
            std::vector<OUString> aGone;
            {
                std::unique_lock aGuard(xSelf->m_aMutex);
                for (const OUString& rUrl : xSelf->m_aProjectFiles[rProjectUrl])
                    if (!aNow.contains(rUrl))
                        aGone.push_back(rUrl);
                xSelf->m_aProjectFiles[rProjectUrl] = aNow;
            }
            for (const OUString& rUrl : aGone)
                if (!document::theDocumentManager::get(xSelf->m_xContext)->getDocument(rUrl).is())
                    xSelf->Store()->removeDocument(rUrl);

            const sal_Int32 nTotal = aFiles.size();
            sal_Int32 nDone = 0;
            std::vector<OUString> aReport;
            for (const OUString& rUrl : aFiles)
            {
                {
                    std::unique_lock aGuard(xSelf->m_aMutex);
                    if (xSelf->m_bDisposed || rJob.IsCancelled())
                        break;
                }
                // a file already known is only analysed again when it changes
                if (xSelf->Store()->getDocumentVersion(rUrl) < 0)
                    xSelf->IndexFile(rUrl);
                aReport.push_back(rUrl);
                ++nDone;
                rJob.SetProgress(nTotal ? static_cast<sal_Int16>(nDone * 100 / nTotal) : 100);
                rJob.SetMessage(OUString::number(nDone) + " / " + OUString::number(nTotal));
                if (aReport.size() >= static_cast<size_t>(ReportEvery))
                {
                    xSelf->Fire(IndexState_INDEXING, nDone, nTotal, aReport);
                    aReport.clear();
                }
            }
            bool bIdle;
            {
                std::unique_lock aGuard(xSelf->m_aMutex);
                bIdle = --xSelf->m_nScans == 0;
            }
            xSelf->Fire(bIdle ? IndexState_IDLE : IndexState_INDEXING, nDone, nTotal, aReport);
            return !rJob.IsCancelled();
        });
    try
    {
        core::theJobRegistry::get(m_xContext)->registerJob(xJob);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.services", "cannot register the indexing job: " << rException.Message);
    }
}

void SymbolIndex::DropProject(const OUString& rProjectUrl)
{
    std::set<OUString> aFiles;
    {
        std::unique_lock aGuard(m_aMutex);
        auto it = m_aProjectFiles.find(rProjectUrl);
        if (it == m_aProjectFiles.end())
            return;
        aFiles = std::move(it->second);
        m_aProjectFiles.erase(it);
    }
    std::vector<OUString> aUrls(aFiles.begin(), aFiles.end());
    for (const OUString& rUrl : aUrls)
    {
        // a document still open keeps its symbols; the rest goes with the project
        uno::Reference<document::XDocument> xDocument
            = document::theDocumentManager::get(m_xContext)->getDocument(rUrl);
        if (!xDocument.is())
            Store()->removeDocument(rUrl);
    }
    Fire(IndexState_IDLE, 0, 0, aUrls);
}

void SymbolIndex::Fire(IndexState eState, sal_Int32 nDone, sal_Int32 nTotal,
                       const std::vector<OUString>& rUrls)
{
    IndexEvent aEvent;
    aEvent.Source = getXWeak();
    aEvent.State = eState;
    aEvent.Done = nDone;
    aEvent.Total = nTotal;
    aEvent.Urls = comphelper::containerToSequence(rUrls);
    std::unique_lock aGuard(m_aMutex);
    if (m_bDisposed)
        return;
    m_aListeners.forEach(aGuard, [&](const uno::Reference<XSymbolIndexListener>& xListener)
                         { xListener->indexChanged(aEvent); });
}

void SymbolIndex::documentChanged(const document::DocumentEvent& rEvent)
{
    switch (rEvent.Kind)
    {
        case document::DocumentEventKind_SAVED: // the text did not change
            break;
        default: // opened, modified, reloaded: the editor's text; closed: what is on disk
            Schedule(rEvent.Url);
    }
}

void SymbolIndex::projectChanged(const project::ProjectEvent& rEvent)
{
    switch (rEvent.Kind)
    {
        case project::ProjectEventKind_OPENED:
        case project::ProjectEventKind_ITEMS_CHANGED:
        case project::ProjectEventKind_CONFIGURATION_CHANGED:
            ScanProject(rEvent.ProjectUrl);
            break;
        case project::ProjectEventKind_CLOSED:
            DropProject(rEvent.ProjectUrl);
            break;
        default:
            break;
    }
}

void SymbolIndex::registerProvider(const uno::Reference<XSymbolProvider>& xProvider)
{
    if (!xProvider.is())
        throw lang::IllegalArgumentException(u"null provider"_ustr, getXWeak(), 0);
    const OUString aName = xProvider->getName();
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        std::erase_if(m_aProviders,
                      [&](const uno::Reference<XSymbolProvider>& x) { return x->getName() == aName; });
        m_aProviders.push_back(xProvider);
    }
    // what is open may be something the newcomer knows
    for (const auto& xDocument : document::theDocumentManager::get(m_xContext)->getOpenDocuments())
        Schedule(xDocument->getUrl());
    ScanAllProjects();
}

void SymbolIndex::unregisterProvider(const OUString& rName)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase_if(m_aProviders,
                  [&](const uno::Reference<XSymbolProvider>& x) { return x->getName() == rName; });
}

uno::Sequence<uno::Reference<XSymbolProvider>> SymbolIndex::getProviders()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aProviders);
}

uno::Reference<XSymbolStore> SymbolIndex::getStore() { return Store(); }

void SymbolIndex::setStore(const uno::Reference<XSymbolStore>& xStore)
{
    if (!xStore.is())
        throw lang::IllegalArgumentException(u"null store"_ustr, getXWeak(), 0);
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        m_xStore = xStore;
        m_aProjectFiles.clear(); // the new store knows nothing yet
    }
    for (const auto& xDocument : document::theDocumentManager::get(m_xContext)->getOpenDocuments())
        Schedule(xDocument->getUrl());
    ScanAllProjects();
}

IndexState SymbolIndex::getState()
{
    std::unique_lock aGuard(m_aMutex);
    return m_nScans > 0 ? IndexState_INDEXING : IndexState_IDLE;
}

uno::Sequence<Symbol> SymbolIndex::getDocumentSymbols(const OUString& rUrl)
{
    return Store()->getDocumentSymbols(rUrl);
}

uno::Sequence<Symbol> SymbolIndex::findSymbols(const OUString& rQuery, sal_Int32 nLimit)
{
    return Store()->findByName(rQuery, nLimit);
}

uno::Sequence<Symbol> SymbolIndex::findDefinition(const OUString& rUrl,
                                                   const css::code::core::Position& rPosition)
{
    uno::Reference<XSymbolProvider> xProvider = ProviderFor(rUrl);
    OUString aText;
    if (!xProvider.is() || !ReadText(m_xContext, rUrl, aText))
        return {};

    Symbol aTarget;
    bool bResolved = false;
    try
    {
        const uno::Any aAny
            = xProvider->resolveSymbolAt(rUrl, aText, ArgumentsFor(rUrl), rPosition);
        bResolved = (aAny >>= aTarget);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code.services", "resolving in " << rUrl << " failed: " << rException.Message);
    }
    if (!bResolved)
    {
        // The provider could not tell (an include it could not find, a macro): the
        // definitions that have the name under the caret are the best guess left.
        const OUString aName = IdentifierAt(aText, rPosition);
        if (aName.isEmpty())
            return {};
        std::vector<Symbol> aSame;
        for (const Symbol& rSymbol : Store()->findByName(aName, 0))
            if (rSymbol.Name == aName && rSymbol.IsDefinition)
                aSame.push_back(rSymbol);
        return comphelper::containerToSequence(aSame);
    }

    uno::Sequence<Symbol> aFound = Store()->findById(aTarget.Id);
    if (aFound.hasElements())
    {
        // the definitions come first; if there are some, the declarations are noise
        std::vector<Symbol> aDefinitions;
        for (const Symbol& rSymbol : aFound)
            if (rSymbol.IsDefinition)
                aDefinitions.push_back(rSymbol);
        return aDefinitions.empty() ? aFound : comphelper::containerToSequence(aDefinitions);
    }
    // not stored (a file outside the project, or not yet scanned): what the
    // provider itself says, if it says where
    if (!aTarget.Url.isEmpty())
        return { aTarget };
    return {};
}

void SymbolIndex::addIndexListener(const uno::Reference<XSymbolIndexListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void SymbolIndex::removeIndexListener(const uno::Reference<XSymbolIndexListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

void SymbolIndex::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    m_aProviders.clear();
    m_aTickets.clear();
    m_aProjectFiles.clear();
    comphelper::WeakComponentImplHelperBase::disposing(rGuard);
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_SymbolIndex_get_implementation(uno::XComponentContext* pContext,
                                                       uno::Sequence<uno::Any> const&)
{
    rtl::Reference<codeservices::SymbolIndex> xIndex(new codeservices::SymbolIndex(pContext));
    xIndex->Attach();
    return cppu::acquire(xIndex.get());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
