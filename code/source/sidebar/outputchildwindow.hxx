/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/childwin.hxx>

namespace code::sidebar
{

/** Outer container of the Output dock, registered under SID_CODE_OUTPUT
    (code/inc/codeids.hxx) -- the bottom-only-dockable counterpart to
    sfx2::sidebar::SidebarChildWindow.
*/
class OutputChildWindow final : public SfxChildWindow
{
public:
    OutputChildWindow(vcl::Window* pParent, sal_uInt16 nId, SfxBindings& rBindings,
                       SfxChildWinInfo& rInfo);

    SFX_DECL_CHILDWINDOW_WITHID(OutputChildWindow);
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
