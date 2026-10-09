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
class VCL_DLLPUBLIC Notebook : virtual public Widget
{
protected:
    Link<const OUString&, bool> m_aLeavePageHdl;
    Link<const OUString&, void> m_aEnterPageHdl;
    Link<const OUString&, void> m_aTabCloseHdl;

public:
    virtual int get_current_page() const = 0;
    virtual int get_page_index(const OUString& rIdent) const = 0;
    virtual OUString get_page_ident(int nPage) const = 0;
    virtual OUString get_current_page_ident() const = 0;
    virtual void set_current_page(int nPage) = 0;
    virtual void set_current_page(const OUString& rIdent) = 0;
    virtual void remove_page(const OUString& rIdent) = 0;
    virtual void insert_page(const OUString& rIdent, const OUString& rLabel, int nPos,
                             const OUString* pIconName = nullptr)
        = 0;
    void append_page(const OUString& rIdent, const OUString& rLabel,
                     const OUString* pIconName = nullptr)
    {
        insert_page(rIdent, rLabel, -1, pIconName);
    }
    virtual void set_tab_label_text(const OUString& rIdent, const OUString& rLabel) = 0;
    virtual OUString get_tab_label_text(const OUString& rIdent) const = 0;
    virtual void set_show_tabs(bool bShow) = 0;
    virtual int get_n_pages() const = 0;
    virtual weld::Container* get_page(const OUString& rIdent) const = 0;

    /// an icon (by theme icon name, "cmd/..." style) in front of the tab's label;
    /// where the backend has no tab icons this does nothing
    virtual void set_tab_image(const OUString& /*rIdent*/, const OUString& /*rIconName*/) {}
    /// tabs at the bottom edge instead of the top; only where the backend can
    virtual void set_tabs_at_bottom(bool /*bBottom*/) {}
    /// a close button on each tab; clicking it calls the handler with the page ident
    virtual void set_tabs_closable(bool /*bClosable*/) {}
    void connect_tab_close(const Link<const OUString&, void>& rLink) { m_aTabCloseHdl = rLink; }

    void connect_leave_page(const Link<const OUString&, bool>& rLink) { m_aLeavePageHdl = rLink; }
    void connect_enter_page(const Link<const OUString&, void>& rLink) { m_aEnterPageHdl = rLink; }
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
