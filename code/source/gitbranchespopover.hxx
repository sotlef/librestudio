/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/vcs/BranchInfo.hpp>
#include <com/sun/star/code/vcs/XRepository.hpp>
#include <tools/gen.hxx>
#include <tools/link.hxx>
#include <vcl/event.hxx>
#include <vcl/weld/customweld.hxx>

#include <functional>
#include <memory>
#include <vector>

namespace weld
{
class Builder;
class Button;
class Entry;
class Label;
class Popover;
class TextWidget;
class TreeIter;
class TreeView;
class Window;
}

namespace code
{
/** The popup of the status bar's branch item for a Git repository, after
    CLion's: a search field, Fetch / Pull / Push, and the local and the remote
    branches; Enter or a double click switches to the one chosen.  What it asks
    of the repository is XBranchBrowser and XGitRepository, nothing else; the
    network operations run on a thread of their own, so the window stays alive.
*/
class GitBranchesPopover final
{
public:
    GitBranchesPopover(weld::Window* pParent, const tools::Rectangle& rRect,
                       css::uno::Reference<css::code::vcs::XRepository> xRepository);
    ~GitBranchesPopover();

    void Show();
    void Close();
    bool IsShown() const { return m_bShown; }

private:
    DECL_LINK(ClosedHdl, weld::Popover&, void);
    DECL_LINK(SearchHdl, weld::TextWidget&, void);
    DECL_LINK(SearchActivateHdl, weld::Entry&, bool);
    DECL_LINK(SearchKeyHdl, const KeyEvent&, bool);
    DECL_LINK(ActivatedHdl, const weld::TreeIter&, bool);
    DECL_LINK(FetchHdl, weld::Button&, void);
    DECL_LINK(PullHdl, weld::Button&, void);
    DECL_LINK(PushHdl, weld::Button&, void);
    DECL_LINK(NewBranchHdl, weld::Button&, void);
    DECL_STATIC_LINK(GitBranchesPopover, DoneHdl, void*, void);

    // a drag of some edges: they follow the mouse by (nDX, nDY) from where it
    // took hold, the opposite ones stay put
    void Resize(bool bLeft, bool bRight, bool bTop, bool bBottom, tools::Long nDX, tools::Long nDY);
    void BeginResize();
    void Load();
    void Fill();
    void SwitchTo(int nRow);
    // runs the operation off the main thread; the result is shown on it
    void RunAsync(const OUString& rName, std::function<void()> aOperation, bool bCloseWhenDone);
    void SetBusy(bool bBusy, const OUString& rMessage);
    void ReportFailure(const OUString& rWhat, const OUString& rMessage, const OUString& rOutput);

    weld::Window* m_pParent;
    tools::Rectangle m_aRect;
    css::uno::Reference<css::code::vcs::XRepository> m_xRepository;
    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Popover> m_xPopover;
    std::unique_ptr<weld::Entry> m_xSearch;
    std::unique_ptr<weld::TreeView> m_xTable;
    std::unique_ptr<weld::Button> m_xNewBranch;
    std::unique_ptr<weld::Button> m_xFetch;
    std::unique_ptr<weld::Button> m_xPull;
    std::unique_ptr<weld::Button> m_xPush;
    std::unique_ptr<weld::Label> m_xStatus;
    // what the popup is resized by: its corner and the edges that move
    std::vector<std::unique_ptr<weld::CustomWidgetController>> m_aGripControllers;
    std::vector<std::unique_ptr<weld::CustomWeld>> m_aGrips;
    // the sizes at the start of a drag
    Size m_aStartSize;
    tools::Rectangle m_aStartRect;
    bool m_bHasStartRect = false;

    std::vector<css::code::vcs::BranchInfo> m_aBranches;
    // what each row of the table is: an index into m_aBranches, -1 for a heading
    std::vector<int> m_aRows;
    // tells an operation that finishes late that the popup is gone
    std::shared_ptr<bool> m_pAlive = std::make_shared<bool>(true);
    bool m_bShown = false;
    bool m_bBusy = false;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
