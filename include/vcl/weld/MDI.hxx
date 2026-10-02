/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/dllapi.h>
#include <vcl/weld/Widget.hxx>

namespace weld
{
class Container;

/** A document-tab strip with a real open-document model behind it:
    per-document close and pin buttons, and a content area per document
    (get_document_page(), the same relationship weld::Notebook::get_page()
    has to its tab pages).

    Deliberately a sibling of weld::Notebook, not an extension of it:
    Notebook is the generic tab-page abstraction every dialog in the
    office suite uses (Options, Format > Paragraph, ...), where a close/pin
    button makes no sense. MDI exists for the narrower, document-specific
    case (currently only "Code"'s own editor area) and owns that model
    itself, the same way Notebook owns its own page list.

    Document identity is an opaque int assigned by insert_document() --
    position can change (close/insert), identity must not.

    Only implemented for the gtk3 and Qt backends; the generic/headless
    SalInstanceBuilder's own weld_mdi() returns nullptr (see TODO.md).
*/
class VCL_DLLPUBLIC MDI : virtual public Widget
{
protected:
    Link<int, void> m_aDocumentCloseRequestHdl;
    Link<int, void> m_aDocumentPinToggledHdl;
    Link<int, void> m_aCurrentDocumentChangedHdl;

    // The close button does not close the document itself -- it asks.
    // Nothing yet declines (no unsaved-changes prompt exists), but the
    // split exists so one can slot in later without touching this class
    // again: the handler decides whether to call close_document().
    void signal_document_close_request(int nDocId) { m_aDocumentCloseRequestHdl.Call(nDocId); }
    // Fired after the pin state has already flipped (toggling is handled
    // entirely inside the widget -- there is nothing for a listener to
    // veto or decide here, unlike close).
    void signal_document_pin_toggled(int nDocId) { m_aDocumentPinToggledHdl.Call(nDocId); }
    void signal_current_document_changed(int nDocId) { m_aCurrentDocumentChangedHdl.Call(nDocId); }

public:
    // Returns the new document's id. nPos of -1 appends.
    virtual int insert_document(int nPos, const OUString& rLabel, const OUString* pIconName = nullptr)
        = 0;
    virtual void close_document(int nDocId) = 0;

    virtual void set_document_label(int nDocId, const OUString& rLabel) = 0;
    // bDirty shows/hides a small "modified" marker on the tab; nothing
    // currently calls this with true (no live editing behind these tabs
    // yet), but the primitive is real.
    virtual void set_document_dirty(int nDocId, bool bDirty) = 0;
    virtual bool get_document_dirty(int nDocId) const = 0;
    virtual void set_document_pinned(int nDocId, bool bPinned) = 0;
    virtual bool get_document_pinned(int nDocId) const = 0;

    virtual int get_current_document() const = 0;
    virtual void set_current_document(int nDocId) = 0;
    virtual int get_document_count() const = 0;
    virtual int get_document_id(int nIndex) const = 0;

    /// The content area for one open document.
    virtual weld::Container* get_document_page(int nDocId) const = 0;

    void connect_document_close_request(const Link<int, void>& rLink)
    {
        m_aDocumentCloseRequestHdl = rLink;
    }
    void connect_document_pin_toggled(const Link<int, void>& rLink)
    {
        m_aDocumentPinToggledHdl = rLink;
    }
    void connect_current_document_changed(const Link<int, void>& rLink)
    {
        m_aCurrentDocumentChangedHdl = rLink;
    }
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
