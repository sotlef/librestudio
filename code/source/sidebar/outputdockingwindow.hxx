/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/dockwin.hxx>

#include <memory>

namespace code::sidebar { class OutputController; }

namespace code::sidebar
{

/** The "Code" app's own, bottom-only-dockable counterpart to
    sfx2::sidebar::SidebarDockingWindow -- owns an OutputController the
    same way SidebarDockingWindow owns a SidebarController. Modelled on
    starmath's own SmCmdBoxWindow (starmath/source/view.cxx), the proven
    precedent for a non-Sidebar SfxDockingWindow restricted to TOP/BOTTOM
    docking via CheckAlignment().
*/
class OutputDockingWindow final : public SfxDockingWindow
{
public:
    OutputDockingWindow(SfxBindings& rBindings, SfxChildWindow& rChildWindow, vcl::Window* pParent);
    virtual ~OutputDockingWindow() override;
    virtual void dispose() override;

    /// brings the deck (a tab of the dock) to the front
    void ShowDeck(std::u16string_view rsDeckId);

private:
    virtual SfxChildAlignment CheckAlignment(SfxChildAlignment eCurrentAlignment,
                                              SfxChildAlignment eRequestedAlignment) override;
    virtual void Resize() override;

    std::unique_ptr<OutputController> mpOutputController;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
