/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/vcs/XRepository.hpp>
#include <com/sun/star/code/vcs/XRepositoryListener.hpp>
#include <sfx2/stbitem.hxx>
#include <tools/link.hxx>

#include <memory>

namespace code { class UiNotifier; }

namespace code
{
class GitBranchesPopover;

/// The repository the active project is in; empty if it is in none.
css::uno::Reference<css::code::vcs::XRepository> CurrentRepository();

/// What the status bar's version control item says: the label of
/// CurrentRepository() with how far it is from its upstream; empty if there
/// is none (and then the item is hidden).
OUString VcsStatusText();

/** The status bar's right end (.uno:CodeVcsBranch, statusbar.xml): the branch
    of the working copy, whatever the system.  It knows nothing of Git: what it
    shows is the repository's summary, and the popup a click opens is looked up
    by the repository's kind.
*/
class VcsStatusBarControl final : public SfxStatusBarControl
{
public:
    SFX_DECL_STATUSBAR_CONTROL();

    VcsStatusBarControl(sal_uInt16 nSlotId, sal_uInt16 nId, StatusBar& rStb);
    virtual ~VcsStatusBarControl() override;

    virtual void StateChangedAtStatusBarControl(sal_uInt16 nSID, SfxItemState eState,
                                                const SfxPoolItem* pState) override;
    virtual void Paint(const UserDrawEvent& rEvt) override;
    virtual void Click() override;

private:
    // shows what the repository says now (or hides the item if there is none)
    void Update();

    // what the item says, without the room kept for the icon
    OUString maText;

    // Told of the repositories coming and going: they are found on threads of
    // theVcsManager, usually after this item was created, and no slot is
    // invalidated by that.
    std::shared_ptr<UiNotifier> mxNotifier;
    css::uno::Reference<css::code::vcs::XRepositoryListener> mxListener;
    // built at each click: it is of one repository, and nothing of it is worth keeping
    std::unique_ptr<GitBranchesPopover> mxPopover;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
