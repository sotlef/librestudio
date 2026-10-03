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
/** The toolbar's configuration box (.uno:CodeConfiguration): a combo box of the
    active project's configurations -- Debug, Release, ... -- showing the
    current one.  Choosing another makes it current (XProjectManager::
    setCurrentConfiguration).  It is a custom item window rather than a slot
    with a value: what it lists is the project's, not a fixed set.
*/
class ConfigurationControl final : public SfxToolBoxControl
{
public:
    SFX_DECL_TOOLBOX_CONTROL();

    ConfigurationControl(sal_uInt16 nSlotId, ToolBoxItemId nId, ToolBox& rToolBox);
    virtual ~ConfigurationControl() override;

    virtual void StateChangedAtToolBoxControl(sal_uInt16 nSID, SfxItemState eState,
                                              const SfxPoolItem* pState) override;
    virtual VclPtr<InterimItemWindow> CreateItemWindow(vcl::Window* pParent) override;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
