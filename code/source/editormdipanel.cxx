/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "editormdipanel.hxx"

#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Container.hxx>
#include <vcl/weld/MDI.hxx>
#include <vcl/weld/TextView.hxx>
#include <vcl/weld/Widget.hxx>

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

    OpenDemoDocument(u"main.cpp"_ustr,
        {
            u"#include <iostream>"_ustr,
            u""_ustr,
            u"int main()"_ustr,
            u"{"_ustr,
            u"    std::cout << \"Hello, LibreStudio Code!\" << std::endl;"_ustr,
            u"    return 0;"_ustr,
            u"}"_ustr,
        });
    OpenDemoDocument(u"utils.h"_ustr,
        {
            u"#pragma once"_ustr,
            u""_ustr,
            u"int add(int a, int b);"_ustr,
        });
    OpenDemoDocument(u"README.md"_ustr,
        { u"LibreStudio Code -- work in progress. Nothing here is real yet."_ustr });
}

EditorMDIPanel::~EditorMDIPanel()
{
    disposeOnce();
}

void EditorMDIPanel::dispose()
{
    maDocumentContents.clear();
    mxMDI.reset();
    InterimItemWindow::dispose();
}

void EditorMDIPanel::OpenDemoDocument(const OUString& rName, std::vector<OUString> aLines)
{
    const int nDocId = mxMDI->insert_document(-1, rName);
    weld::Container* pPage = mxMDI->get_document_page(nDocId);
    if (!pPage)
        return;

    DocumentContent aContent;
    aContent.mxBuilder = Application::CreateBuilder(pPage, u"code/ui/editordocumentcontent.ui"_ustr);
    aContent.mxRoot = aContent.mxBuilder->weld_widget(u"EditorDocumentContent"_ustr);
    aContent.mxView = aContent.mxBuilder->weld_text_view(u"content"_ustr);

    OUString sText;
    for (const OUString& rLine : aLines)
        sText += rLine + "\n";
    aContent.mxView->set_text(sText);

    maDocumentContents[nDocId] = std::move(aContent);
}

IMPL_LINK(EditorMDIPanel, CloseRequestHdl, int, nDocId, void)
{
    // No unsaved-changes confirmation yet -- see the plan's scope note;
    // this is where one would hook in once there's something to save.
    maDocumentContents.erase(nDocId);
    mxMDI->close_document(nDocId);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
