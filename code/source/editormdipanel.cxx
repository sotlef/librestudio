/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "editormdipanel.hxx"

#include "uibridge.hxx"

#include <com/sun/star/code/document/DocumentState.hpp>
#include <com/sun/star/code/document/XDocument.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/code/frontend/SaveChoice.hpp>
#include <com/sun/star/code/frontend/XDialogs.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenController.hpp>
#include <com/sun/star/code/semantic/theSemanticTokenController.hpp>
#include <comphelper/processfactory.hxx>
#include <rtl/uri.hxx>
#include <sal/log.hxx>

#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Container.hxx>
#include <vcl/weld/MDI.hxx>
#include <vcl/weld/Widget.hxx>

#include <algorithm>

using namespace css;

namespace
{
// the last path segment of a file URL, decoded: the tab's label
OUString DisplayNameOf(const OUString& rUrl)
{
    return rtl::Uri::decode(rUrl.copy(rUrl.lastIndexOf('/') + 1), rtl_UriDecodeWithCharset,
                            RTL_TEXTENCODING_UTF8);
}
}

EditorMDIPanel::EditorMDIPanel(vcl::Window* pParent)
    : InterimItemWindow(pParent, u"code/ui/editormdi.ui"_ustr, u"EditorMDI"_ustr)
{
    mxMDI = m_xBuilder->weld_mdi(u"EditorMDI"_ustr);
    // Only gtk3/Qt have a real weld::MDI implementation (see TODO.md);
    // "Code" only ever runs under those backends in this environment, but
    // guard the one call site defensively rather than assume it always
    // succeeds.
    if (!mxMDI)
        return;

    mxMDI->connect_document_close_request(LINK(this, EditorMDIPanel, CloseRequestHdl));
    if (::code::Workbench* pWorkbench = ::code::Workbench::Instance())
        pWorkbench->SetEditorSurface(this);

    // Tabs follow the document manager: a document's modified state shows on its
    // tab, and a document closed by someone else loses its tab.  Events come
    // from any thread; the tabs are only touched on this one.
    mxNotifier = std::make_shared<::code::UiNotifier>([this] { SyncWithDocuments(); });
    mxListener = new ::code::ServiceListener(mxNotifier);
    try
    {
        css::code::document::theDocumentManager::get(comphelper::getProcessComponentContext())
            ->addDocumentListener(mxListener);
        // new colours for the text arrive on their own
        css::code::semantic::theSemanticTokenController::get(comphelper::getProcessComponentContext())
            ->addSemanticTokenListener(
                uno::Reference<css::code::semantic::XSemanticTokenListener>(mxListener, uno::UNO_QUERY));
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "the editor cannot follow the documents: " << rException.Message);
    }
}

EditorMDIPanel::~EditorMDIPanel()
{
    disposeOnce();
}

void EditorMDIPanel::dispose()
{
    if (::code::Workbench* pWorkbench = ::code::Workbench::Instance())
        pWorkbench->SetEditorSurface(nullptr);
    if (mxNotifier)
        mxNotifier->Detach();
    try
    {
        if (mxListener.is())
            css::code::document::theDocumentManager::get(comphelper::getProcessComponentContext())
                ->removeDocumentListener(mxListener);
        if (mxListener.is())
            css::code::semantic::theSemanticTokenController::get(comphelper::getProcessComponentContext())
                ->removeSemanticTokenListener(uno::Reference<css::code::semantic::XSemanticTokenListener>(
                    mxListener, uno::UNO_QUERY));
    }
    catch (const uno::Exception&)
    {
    }
    maDocumentContents.clear();
    mxMDI.reset();
    InterimItemWindow::dispose();
}

void EditorMDIPanel::SettingsChanged()
{
    for (auto& rEntry : maDocumentContents)
        if (rEntry.second.mxView)
            rEntry.second.mxView->SettingsChanged();
}

void EditorMDIPanel::ShowDocument(const OUString& rUrl, const css::code::core::Position* pPosition)
{
    if (!mxMDI)
        return;

    int nDocId = -1;
    for (const auto& [nId, rContent] : maDocumentContents)
        if (rContent.msUrl == rUrl)
            nDocId = nId;

    if (nDocId < 0)
    {
        // Through the document manager, so that the services know what is
        // open; the tab only displays what the manager read.
        uno::Reference<css::code::document::XTextDocument> xDocument;
        try
        {
            xDocument.set(css::code::document::theDocumentManager::get(
                              comphelper::getProcessComponentContext())
                              ->openDocument(rUrl),
                          uno::UNO_QUERY);
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code", "cannot open " << rUrl << ": " << rException.Message);
            return;
        }
        if (!xDocument.is())
            return;

        nDocId = mxMDI->insert_document(-1, DisplayNameOf(rUrl));
        weld::Container* pPage = mxMDI->get_document_page(nDocId);
        if (!pPage)
            return;

        DocumentContent aContent;
        aContent.mxBuilder
            = Application::CreateBuilder(pPage, u"code/ui/editordocumentcontent.ui"_ustr);
        aContent.mxView = std::make_unique<::code::CodeEditView>(*aContent.mxBuilder, xDocument);
        aContent.msUrl = rUrl;
        maDocumentContents[nDocId] = std::move(aContent);
        mxMDI->set_document_dirty(nDocId, xDocument->getState() == css::code::document::DocumentState_MODIFIED);
    }

    mxMDI->set_current_document(nDocId);

    if (pPosition)
        maDocumentContents[nDocId].mxView->RevealPosition(*pPosition);
}

bool EditorMDIPanel::SaveCurrentDocument()
{
    if (!mxMDI)
        return false;
    const auto it = maDocumentContents.find(mxMDI->get_current_document());
    if (it == maDocumentContents.end())
        return false;
    // an unmodified document has nothing to write; saving is still a success
    if (auto xDocument = DocumentOf(it->second.msUrl))
        if (xDocument->getState() == css::code::document::DocumentState_MODIFIED)
            xDocument->save();
    return true;
}

OUString EditorMDIPanel::CurrentDocumentUrl()
{
    if (!mxMDI)
        return OUString();
    const auto it = maDocumentContents.find(mxMDI->get_current_document());
    return it == maDocumentContents.end() ? OUString() : it->second.msUrl;
}

uno::Reference<css::code::document::XTextDocument> EditorMDIPanel::DocumentOf(const OUString& rUrl)
{
    try
    {
        return uno::Reference<css::code::document::XTextDocument>(
            css::code::document::theDocumentManager::get(comphelper::getProcessComponentContext())
                ->getDocument(rUrl),
            uno::UNO_QUERY);
    }
    catch (const uno::Exception&)
    {
        return {};
    }
}

// The tabs' view of the documents: dirty marks, tabs of closed documents, and
// the text of documents read again from disk.
void EditorMDIPanel::SyncWithDocuments()
{
    if (!mxMDI)
        return;
    std::vector<int> aGone;
    for (auto& [nId, rContent] : maDocumentContents)
    {
        auto xDocument = DocumentOf(rContent.msUrl);
        if (!xDocument.is())
        {
            aGone.push_back(nId);
            continue;
        }
        mxMDI->set_document_dirty(nId, xDocument->getState() == css::code::document::DocumentState_MODIFIED);

        // Reloaded or edited elsewhere: show what the document says now.
        rContent.mxView->Refresh();
    }
    for (int nId : aGone)
    {
        maDocumentContents.erase(nId);
        mxMDI->close_document(nId);
    }
}

IMPL_LINK(EditorMDIPanel, CloseRequestHdl, int, nDocId, void)
{
    const auto it = maDocumentContents.find(nDocId);
    if (it == maDocumentContents.end())
    {
        mxMDI->close_document(nDocId);
        return;
    }
    const OUString aUrl = it->second.msUrl;

    bool bForce = false;
    if (auto xDocument = DocumentOf(aUrl))
    {
        try
        {
            if (xDocument->getState() == css::code::document::DocumentState_MODIFIED)
            {
                // The question is the shell's to ask: it is a window.
                uno::Reference<css::code::frontend::XDialogs> xDialogs
                    = ::code::Workbench::Instance() ? ::code::Workbench::Instance()->getDialogs()
                                                    : uno::Reference<css::code::frontend::XDialogs>();
                switch (xDialogs.is() ? xDialogs->askSaveChanges(xDocument->getUrl().copy(xDocument->getUrl().lastIndexOf('/') + 1))
                                      : css::code::frontend::SaveChoice_DISCARD)
                {
                    case css::code::frontend::SaveChoice_CANCEL:
                        return; // the tab stays
                    case css::code::frontend::SaveChoice_SAVE:
                        xDocument->save();
                        break;
                    default: // DISCARD
                        break;
                }
                bForce = true;
            }
        }
        catch (const uno::Exception& rException)
        {
            // A failed save must not lose the text: keep the tab.
            SAL_WARN("code", "cannot save " << aUrl << ": " << rException.Message);
            if (::code::Workbench* pWorkbench = ::code::Workbench::Instance())
                pWorkbench->showMessage(u"Cannot save: "_ustr + rException.Message, 8000);
            return;
        }
    }

    maDocumentContents.erase(nDocId);
    mxMDI->close_document(nDocId);
    try
    {
        css::code::document::theDocumentManager::get(comphelper::getProcessComponentContext())
            ->closeDocument(aUrl, bForce);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot close " << aUrl << ": " << rException.Message);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
