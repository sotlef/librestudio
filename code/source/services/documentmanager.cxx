/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common.hxx"

#include <com/sun/star/code/document/DocumentEvent.hpp>
#include <com/sun/star/code/document/XDocumentFactory.hpp>
#include <com/sun/star/code/document/XDocumentListener.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/detail/file.h>
#include <osl/file.hxx>
#include <rtl/ref.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/string.hxx>

#include <algorithm>
#include <vector>

using namespace css;
using namespace css::code::document;

namespace codeservices
{
namespace
{
class DocumentManager;

// The built-in document: plain text, UTF-8 on disk.  Used for every MIME type
// no registered XDocumentFactory claims.
class TextDocument final : public comphelper::WeakComponentImplHelper<XTextDocument>
{
public:
    TextDocument(rtl::Reference<DocumentManager> xOwner, OUString aUrl)
        : m_xOwner(std::move(xOwner))
        , m_aUrl(std::move(aUrl))
        , m_aMime(GetMimeType(m_aUrl))
    {
    }

    void load(); // throws io::IOException

    // XDocument
    OUString SAL_CALL getUrl() override { return m_aUrl; }
    OUString SAL_CALL getMimeType() override { return m_aMime; }
    DocumentState SAL_CALL getState() override
    {
        std::unique_lock aGuard(m_aMutex);
        return m_eState;
    }
    void SAL_CALL save() override;
    void SAL_CALL reload() override;

    // XTextDocument
    OUString SAL_CALL getText() override
    {
        std::unique_lock aGuard(m_aMutex);
        return m_aText;
    }
    void SAL_CALL setText(const OUString& rText) override;
    sal_Int32 SAL_CALL getVersion() override
    {
        std::unique_lock aGuard(m_aMutex);
        return m_nVersion;
    }
    sal_Int32 SAL_CALL getLineCount() override;
    OUString SAL_CALL getLine(sal_Int32 nIndex) override;
    void SAL_CALL applyEdit(const TextEdit& rEdit) override;

    void detach()
    {
        std::unique_lock aGuard(m_aMutex);
        m_xOwner.clear();
    }

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override
    {
        m_xOwner.clear();
        comphelper::WeakComponentImplHelperBase::disposing(rGuard);
    }
    void changed(DocumentEventKind eKind);
    // offset of the start of a line, or -1 beyond the last
    sal_Int32 lineStart(sal_Int32 nLine) const;
    sal_Int32 lineEnd(sal_Int32 nLine) const; // before the terminator

    rtl::Reference<DocumentManager> m_xOwner;
    const OUString m_aUrl;
    const OUString m_aMime;
    OUString m_aText;
    sal_Int32 m_nVersion = 0;
    DocumentState m_eState = DocumentState_CLEAN;
};

class DocumentManager final
    : public comphelper::WeakComponentImplHelper<XDocumentManager, lang::XServiceInfo>
{
public:
    // XDocumentManager
    uno::Reference<XDocument> SAL_CALL openDocument(const OUString& rUrl) override;
    sal_Bool SAL_CALL closeDocument(const OUString& rUrl, sal_Bool bForce) override;
    void SAL_CALL saveAll() override;
    uno::Sequence<uno::Reference<XDocument>> SAL_CALL getOpenDocuments() override;
    uno::Reference<XDocument> SAL_CALL getDocument(const OUString& rUrl) override;
    void SAL_CALL registerFactory(const uno::Reference<XDocumentFactory>& xFactory) override;
    void SAL_CALL unregisterFactory(const uno::Reference<XDocumentFactory>& xFactory) override;
    void SAL_CALL addDocumentListener(const uno::Reference<XDocumentListener>& xListener) override;
    void SAL_CALL
    removeDocumentListener(const uno::Reference<XDocumentListener>& xListener) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.DocumentManager"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override { return {}; }

    // called by the documents, without their lock held
    void notify(const uno::Reference<XDocument>& xDocument, DocumentEventKind eKind,
                DocumentState eState);

private:
    void disposing(std::unique_lock<std::mutex>& rGuard) override;
    void fire(std::unique_lock<std::mutex>& rGuard, const DocumentEvent& rEvent);

    std::vector<uno::Reference<XDocument>> m_aDocuments; // in opening order
    std::vector<uno::Reference<XDocumentFactory>> m_aFactories;
    comphelper::OInterfaceContainerHelper4<XDocumentListener> m_aListeners;
};

// ---- TextDocument ---------------------------------------------------------

void TextDocument::load()
{
    osl::File aFile(m_aUrl);
    if (aFile.open(osl_File_OpenFlag_Read) != osl::FileBase::E_None)
        throw io::IOException("cannot open " + m_aUrl, getXWeak());

    OStringBuffer aBytes;
    char aBuffer[16384];
    sal_uInt64 nRead = 0;
    osl::FileBase::RC eRC;
    while ((eRC = aFile.read(aBuffer, sizeof aBuffer, nRead)) == osl::FileBase::E_None && nRead)
        aBytes.append(aBuffer, static_cast<sal_Int32>(nRead));
    aFile.close();
    if (eRC != osl::FileBase::E_None)
        throw io::IOException("cannot read " + m_aUrl, getXWeak());

    const OString aUtf8 = aBytes.makeStringAndClear();
    std::unique_lock aGuard(m_aMutex);
    m_aText = OUString(aUtf8.getStr(), aUtf8.getLength(), RTL_TEXTENCODING_UTF8,
                       OSTRING_TO_OUSTRING_CVTFLAGS);
    ++m_nVersion;
    m_eState = DocumentState_CLEAN;
}

void TextDocument::save()
{
    OUString aText;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        aText = m_aText;
    }
    const OString aUtf8 = OUStringToOString(aText, RTL_TEXTENCODING_UTF8);

    osl::File aFile(m_aUrl);
    // Create alone means "must not exist yet" (O_EXCL); NoExcl lets it open the
    // file that is being saved, and Trunc drops the old content.
    if (aFile.open(osl_File_OpenFlag_Write | osl_File_OpenFlag_Create | osl_File_OpenFlag_NoExcl
                   | osl_File_OpenFlag_Trunc)
        != osl::FileBase::E_None)
        throw io::IOException("cannot open for writing " + m_aUrl, getXWeak());
    sal_uInt64 nWritten = 0;
    const osl::FileBase::RC eRC = aFile.write(aUtf8.getStr(), aUtf8.getLength(), nWritten);
    aFile.close();
    if (eRC != osl::FileBase::E_None || nWritten != static_cast<sal_uInt64>(aUtf8.getLength()))
        throw io::IOException("cannot write " + m_aUrl, getXWeak());

    {
        std::unique_lock aGuard(m_aMutex);
        m_eState = DocumentState_CLEAN;
    }
    changed(DocumentEventKind_SAVED);
}

void TextDocument::reload()
{
    load();
    changed(DocumentEventKind_RELOADED);
}

void TextDocument::changed(DocumentEventKind eKind)
{
    rtl::Reference<DocumentManager> xOwner;
    DocumentState eState;
    {
        std::unique_lock aGuard(m_aMutex);
        xOwner = m_xOwner;
        eState = m_eState;
    }
    if (xOwner.is())
        xOwner->notify(this, eKind, eState);
}

void TextDocument::setText(const OUString& rText)
{
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        if (m_aText == rText)
            return;
        m_aText = rText;
        ++m_nVersion;
        m_eState = DocumentState_MODIFIED;
    }
    changed(DocumentEventKind_MODIFIED);
}

sal_Int32 TextDocument::lineStart(sal_Int32 nLine) const
{
    sal_Int32 nPos = 0;
    for (; nLine > 0; --nLine)
    {
        const sal_Int32 nEol = m_aText.indexOf('\n', nPos);
        if (nEol < 0)
            return -1;
        nPos = nEol + 1;
    }
    return nPos;
}

sal_Int32 TextDocument::lineEnd(sal_Int32 nLine) const
{
    const sal_Int32 nStart = lineStart(nLine);
    sal_Int32 nEnd = m_aText.indexOf('\n', nStart);
    if (nEnd < 0)
        nEnd = m_aText.getLength();
    if (nEnd > nStart && m_aText[nEnd - 1] == '\r')
        --nEnd;
    return nEnd;
}

sal_Int32 TextDocument::getLineCount()
{
    std::unique_lock aGuard(m_aMutex);
    sal_Int32 nCount = 1;
    for (sal_Int32 n = m_aText.indexOf('\n'); n >= 0; n = m_aText.indexOf('\n', n + 1))
        ++nCount;
    return nCount;
}

OUString TextDocument::getLine(sal_Int32 nIndex)
{
    std::unique_lock aGuard(m_aMutex);
    const sal_Int32 nStart = nIndex < 0 ? -1 : lineStart(nIndex);
    if (nStart < 0)
        throw lang::IndexOutOfBoundsException(u"no such line"_ustr, getXWeak());
    return m_aText.copy(nStart, lineEnd(nIndex) - nStart);
}

void TextDocument::applyEdit(const TextEdit& rEdit)
{
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);

        // A position is valid up to the end of its line, never into the terminator.
        auto offsetOf = [&](const css::code::core::Position& rPos, sal_Int32& rOffset)
        {
            if (rPos.Line < 0 || rPos.Column < 0)
                return false;
            const sal_Int32 nStart = lineStart(rPos.Line);
            if (nStart < 0 || rPos.Column > lineEnd(rPos.Line) - nStart)
                return false;
            rOffset = nStart + rPos.Column;
            return true;
        };
        sal_Int32 nFrom = 0;
        sal_Int32 nTo = 0;
        if (!offsetOf(rEdit.Range.Start, nFrom) || !offsetOf(rEdit.Range.End, nTo) || nTo < nFrom)
            throw lang::IllegalArgumentException(u"range outside the text"_ustr, getXWeak(), 0);

        m_aText = m_aText.replaceAt(nFrom, nTo - nFrom, rEdit.NewText);
        ++m_nVersion;
        m_eState = DocumentState_MODIFIED;
    }
    changed(DocumentEventKind_MODIFIED);
}

// ---- DocumentManager ------------------------------------------------------

void DocumentManager::fire(std::unique_lock<std::mutex>& rGuard, const DocumentEvent& rEvent)
{
    m_aListeners.forEach(rGuard, [&](const uno::Reference<XDocumentListener>& xListener)
                         { xListener->documentChanged(rEvent); });
}

void DocumentManager::notify(const uno::Reference<XDocument>& xDocument, DocumentEventKind eKind,
                             DocumentState eState)
{
    DocumentEvent aEvent;
    aEvent.Source = xDocument;
    aEvent.Kind = eKind;
    aEvent.Url = xDocument->getUrl();
    aEvent.State = eState;
    std::unique_lock aGuard(m_aMutex);
    fire(aGuard, aEvent);
}

uno::Reference<XDocument> DocumentManager::getDocument(const OUString& rUrl)
{
    const OUString aUrl = NormalizeUrl(rUrl);
    std::unique_lock aGuard(m_aMutex);
    for (const auto& xDocument : m_aDocuments)
        if (xDocument->getUrl() == aUrl)
            return xDocument;
    return {};
}

uno::Reference<XDocument> DocumentManager::openDocument(const OUString& rUrl)
{
    const OUString aUrl = NormalizeUrl(rUrl);
    if (auto xOpen = getDocument(aUrl))
        return xOpen;

    // Loading is slow and may call out; do it without the lock.
    const OUString aMime = GetMimeType(aUrl);
    uno::Reference<XDocumentFactory> xFactory;
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        for (const auto& xCandidate : m_aFactories)
        {
            const uno::Sequence<OUString> aTypes = xCandidate->getSupportedMimeTypes();
            if (std::find(aTypes.begin(), aTypes.end(), aMime) != aTypes.end())
            {
                xFactory = xCandidate;
                break;
            }
        }
    }

    uno::Reference<XDocument> xDocument;
    if (xFactory.is())
        xDocument = xFactory->loadDocument(aUrl);
    else
    {
        rtl::Reference<TextDocument> xText = new TextDocument(this, aUrl);
        xText->load();
        xDocument = uno::Reference<XDocument>(xText.get());
    }

    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        // Someone may have opened it while we were loading: theirs wins.
        for (const auto& xExisting : m_aDocuments)
            if (xExisting->getUrl() == aUrl)
            {
                uno::Reference<XDocument> xWinner = xExisting;
                aGuard.unlock();
                xDocument->dispose();
                return xWinner;
            }
        m_aDocuments.push_back(xDocument);

        DocumentEvent aEvent;
        aEvent.Source = xDocument;
        aEvent.Kind = DocumentEventKind_OPENED;
        aEvent.Url = aUrl;
        aEvent.State = xDocument->getState();
        fire(aGuard, aEvent);
    }
    return xDocument;
}

sal_Bool DocumentManager::closeDocument(const OUString& rUrl, sal_Bool bForce)
{
    const OUString aUrl = NormalizeUrl(rUrl);
    uno::Reference<XDocument> xDocument;
    {
        std::unique_lock aGuard(m_aMutex);
        auto it = std::find_if(m_aDocuments.begin(), m_aDocuments.end(),
                               [&](const uno::Reference<XDocument>& x)
                               { return x->getUrl() == aUrl; });
        if (it == m_aDocuments.end())
            return true;
        // The point of Force: unsaved changes are never lost by accident.
        if (!bForce && (*it)->getState() == DocumentState_MODIFIED)
            return false;
        xDocument = *it;
        m_aDocuments.erase(it);

        DocumentEvent aEvent;
        aEvent.Source = xDocument;
        aEvent.Kind = DocumentEventKind_CLOSED;
        aEvent.Url = aUrl;
        aEvent.State = xDocument->getState();
        fire(aGuard, aEvent);
    }
    xDocument->dispose();
    return true;
}

void DocumentManager::saveAll()
{
    std::vector<uno::Reference<XDocument>> aDocuments;
    {
        std::unique_lock aGuard(m_aMutex);
        aDocuments = m_aDocuments;
    }
    for (const auto& xDocument : aDocuments)
        if (xDocument->getState() == DocumentState_MODIFIED)
            xDocument->save();
}

uno::Sequence<uno::Reference<XDocument>> DocumentManager::getOpenDocuments()
{
    std::unique_lock aGuard(m_aMutex);
    return comphelper::containerToSequence(m_aDocuments);
}

void DocumentManager::registerFactory(const uno::Reference<XDocumentFactory>& xFactory)
{
    if (!xFactory.is())
        throw lang::IllegalArgumentException(u"null factory"_ustr, getXWeak(), 0);
    std::unique_lock aGuard(m_aMutex);
    throwIfDisposed(aGuard);
    if (std::find(m_aFactories.begin(), m_aFactories.end(), xFactory) == m_aFactories.end())
        m_aFactories.push_back(xFactory);
}

void DocumentManager::unregisterFactory(const uno::Reference<XDocumentFactory>& xFactory)
{
    std::unique_lock aGuard(m_aMutex);
    std::erase(m_aFactories, xFactory);
}

void DocumentManager::addDocumentListener(const uno::Reference<XDocumentListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.addInterface(aGuard, xListener);
}

void DocumentManager::removeDocumentListener(const uno::Reference<XDocumentListener>& xListener)
{
    std::unique_lock aGuard(m_aMutex);
    m_aListeners.removeInterface(aGuard, xListener);
}

void DocumentManager::disposing(std::unique_lock<std::mutex>& rGuard)
{
    lang::EventObject aSource(getXWeak());
    m_aListeners.disposeAndClear(rGuard, aSource);
    // The documents hold a reference back to us; let go of it.
    for (const auto& xDocument : m_aDocuments)
        if (auto* pText = dynamic_cast<TextDocument*>(xDocument.get()))
            pText->detach();
    m_aDocuments.clear();
    m_aFactories.clear();
}
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_DocumentManager_get_implementation(uno::XComponentContext*,
                                                           uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::DocumentManager);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
