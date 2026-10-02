/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/viewfac.hxx>
#include <sfx2/viewsh.hxx>
#include <vcl/vclptr.hxx>

#include <ifaceids.hxx>

class SfxItemSet;
class SfxRequest;

class EditorMDIPanel;

// The real SFX view shell: owns the remaining v1 content panel (now
// parented directly under the frame's own content window --
// GetViewFrame().GetWindow() -- the same way basctl::Shell parents its
// panels, rather than under a hand-built top-level WorkWindow of our own).
// Menu/toolbar/statusbar are no longer hand-coded here: framework's
// LayoutManager builds them from
// code/uiconfig/code/{menubar,toolbar,statusbar}/*.xml. Neither of the
// other two v1 panels is one of these fixed content panels anymore: Solution
// Explorer and Output are both real, dockable/collapsible sfx2::sidebar-style
// decks now (see code/source/sidebar/), registered as child windows below,
// not owned by this class at all.
namespace code
{

class Shell final : public SfxViewShell
{
private:
    VclPtr<EditorMDIPanel> mxEditorArea;

    void InitPanels();
    void LayoutPanels(const Point& rPos, const Size& rSize);

public:
    SFX_DECL_INTERFACE(CODE_INTERFACE_VIEWSH)
    SFX_DECL_VIEWFACTORY(Shell);

private:
    static void InitInterface_Impl();

public:
    Shell(SfxViewFrame& rFrame, SfxViewShell* pOldSh);
    virtual ~Shell() override;

    virtual void OuterResizePixel(const Point& rOfs, const Size& rSize) override;

    // Real dispatch targets for the .sdi's SID_NEWDOC/SID_SAVEDOC -- still
    // no-ops (same "decorative for now" spirit as v1's menu), but reached
    // through the real SfxDispatcher this time, not a hand-rolled switch.
    void Execute(SfxRequest& rReq);
    void GetState(SfxItemSet& rSet);
};

} // namespace code

// The .sdi compiler (svidl) cannot parse C++ namespaces.
typedef code::Shell code_Shell;

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
