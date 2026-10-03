/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/stbitem.hxx>
#include <tools/link.hxx>

#include <memory>

namespace weld { class Builder; class Popover; }

namespace code
{

/** The status bar's leftmost item (.uno:CodeBackgroundTasks,
    statusbar.xml): Visual Studio's background-tasks button -- an
    owner-drawn speech-bubble icon that pops a small "No background tasks
    running." popover when clicked. There is no real background-task
    machinery yet, so the popover text is the honest, constant truth.
*/
class TasksStatusBarControl final : public SfxStatusBarControl
{
public:
    SFX_DECL_STATUSBAR_CONTROL();

    TasksStatusBarControl(sal_uInt16 nSlotId, sal_uInt16 nId, StatusBar& rStb);
    virtual ~TasksStatusBarControl() override;

    virtual void Paint(const UserDrawEvent& rEvt) override;
    virtual void Click() override;

private:
    DECL_LINK(PopoverClosedHdl, weld::Popover&, void);

    // Built lazily on the first click, then kept: popup_at_rect() is not
    // modal, so something must own the popover while it is up anyway.
    std::unique_ptr<weld::Builder> mxBuilder;
    std::unique_ptr<weld::Popover> mxPopover;
    bool mbShown = false;
};

} // namespace code

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
