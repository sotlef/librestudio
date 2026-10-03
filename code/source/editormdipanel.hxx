/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "workbench.hxx"

#include <com/sun/star/code/document/XDocumentListener.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>

#include <vcl/InterimItemWindow.hxx>

#include <map>
#include <memory>
#include <vector>

namespace code { class UiNotifier; }
namespace weld { class Builder; }
namespace weld { class TextWidget; }
namespace weld { class MDI; }
namespace weld { class TextView; }
namespace weld { class Widget; }

// The editor area's MDI widget host: a real weld::MDI (include/vcl/weld/MDI.hxx,
// implemented for gtk3/Qt in vcl/unx/gtk3/gtkinst.cxx and vcl/qt5/QtInstanceMDI.cxx)
// replacing both the old svtools::TabBar-based tab strip and this session's
// own, rejected hand-composed EditorTabBar (weld::Button/weld::Box
// composition) -- weld::MDI owns the open-document model (close/dirty/
// pinned) and the per-document content area itself; this class is a thin
// consumer: open the starting demo documents, give each its content
// widget, and react to close requests.
class EditorMDIPanel final : public InterimItemWindow, public code::EditorSurface
{
public:
    explicit EditorMDIPanel(vcl::Window* pParent);
    virtual ~EditorMDIPanel() override;
    virtual void dispose() override;

    // code::EditorSurface
    virtual void ShowDocument(const OUString& rUrl,
                              const css::code::core::Position* pPosition) override;
    virtual bool SaveCurrentDocument() override;

private:
    // weld::MDI itself only owns the tab strip and a bare native container
    // per document (the same relationship weld::Notebook::get_page() has
    // to its tab pages) -- the actual content widget, and the builder that
    // constructed it, are this class' own to keep alive.
    struct DocumentContent
    {
        std::unique_ptr<weld::Builder> mxBuilder;
        // The .ui's own root widget ("EditorDocumentContent", a
        // GtkScrolledWindow) -- looking it up is what actually attaches it
        // into the MDI page (Builder::weld_X() parents whatever it looks
        // up if that object is still parentless; "content", the nested
        // GtkTextView below it in the same .ui, is never parentless to
        // begin with, so asking for that alone leaves the whole subtree
        // unattached). Kept alive alongside mxView, never otherwise used.
        std::unique_ptr<weld::Widget> mxRoot;
        std::unique_ptr<weld::TextView> mxView;
        // empty for the demo documents, which belong to no file
        OUString msUrl;
    };

    static css::uno::Reference<css::code::document::XTextDocument> DocumentOf(const OUString& rUrl);
    void SyncWithDocuments();

    DECL_LINK(CloseRequestHdl, int, void);
    DECL_LINK(TextChangedHdl, weld::TextWidget&, void);

    std::unique_ptr<weld::MDI> mxMDI;
    std::map<int, DocumentContent> maDocumentContents;
    // set while the view is being filled from the document, so that filling it
    // is not mistaken for typing
    bool mbSyncing = false;

    std::shared_ptr<::code::UiNotifier> mxNotifier;
    css::uno::Reference<css::code::document::XDocumentListener> mxListener;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
