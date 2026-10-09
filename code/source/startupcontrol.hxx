/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/tbxctrl.hxx>

namespace code
{
/** The toolbar's startup item (.uno:CodeStartup): a Start button, a combo box
    of the launchable targets of the project (the one Start runs is selected),
    and an arrow with "Start Without Debugging".  Choosing a target stores it
    as the project's startup target (XProjectManager::setStartupTarget).  The
    whole item is hidden while no target can be launched.
*/
class StartupControl final : public SfxToolBoxControl
{
public:
    SFX_DECL_TOOLBOX_CONTROL();

    StartupControl(sal_uInt16 nSlotId, ToolBoxItemId nId, ToolBox& rToolBox);
    virtual ~StartupControl() override;

    virtual void StateChangedAtToolBoxControl(sal_uInt16 nSID, SfxItemState eState,
                                              const SfxPoolItem* pState) override;
    virtual VclPtr<InterimItemWindow> CreateItemWindow(vcl::Window* pParent) override;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
