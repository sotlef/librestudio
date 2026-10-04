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
#include <com/sun/star/code/vcs/git/CommitInfo.hpp>
#include <com/sun/star/code/vcs/git/FileChange.hpp>
#include <com/sun/star/code/vcs/git/StashEntry.hpp>
#include <com/sun/star/code/vcs/git/XGitWorkingCopy.hpp>
#include <sfx2/sidebar/PanelLayout.hxx>
#include <vcl/weld/TreeView.hxx>

#include <functional>
#include <map>
#include <memory>
#include <vector>

class CommandEvent;
namespace weld
{
class Button;
class CheckButton;
class Label;
class TextWidget;
class TextView;
class Toggleable;
class TreeIter;
}

namespace code
{
class UiNotifier;
}

namespace code::sidebar
{
/** Git Changes: the files of the working copy that differ from the last
    commit, with what is needed to deal with them -- stage and unstage by a
    check mark, a message and a button to commit, a menu to stash, to roll back,
    to add what git does not track yet.  It knows only XGitWorkingCopy: what it
    shows and does is that interface's, and the Git it stands on is the
    repository service's.

    Everything git is asked is asked off the main thread, as it can take long;
    the answers are shown on it.
*/
class GitChangesPanel final : public PanelLayout
{
public:
    explicit GitChangesPanel(weld::Widget* pParent);
    virtual ~GitChangesPanel() override;

    // what an operation that ran on a thread of its own comes back with
    struct Outcome;

private:
    enum class Group
    {
        Staged,
        Changes,
        Untracked,
        Stashes
    };

    // Looks again, off the main thread; the answer is shown by Show().
    void Refresh();
    void Show(const Outcome& rOutcome);
    // what the rows are made of, from what was read
    void Fill();
    void UpdateButtons();
    void SetBusy(bool bBusy, const OUString& rMessage);
    // aAfter runs on the main thread when the operation went well
    void RunAsync(const OUString& rName, std::function<void()> aOperation, bool bClearMessage,
                  std::function<void()> aAfter = {});
    // lists what a push sends and pushes if the user agrees
    void ConfirmPush(const std::vector<css::code::vcs::git::CommitInfo>& rCommits);
    void ReportFailure(const OUString& rWhat, const OUString& rMessage, const OUString& rOutput);
    bool Confirm(const OUString& rQuestion);

    // the rows' paths (or stash indexes), by what the id of a row says
    bool ParseRow(const OUString& rId, Group& reGroup, OUString& rPath) const;
    std::vector<OUString> PathsOf(Group eGroup) const;
    void StagePaths(const std::vector<OUString>& rPaths);
    void UnstagePaths(const std::vector<OUString>& rPaths);
    void RollbackPaths(const std::vector<OUString>& rPaths, bool bUntracked);
    void Commit(bool bAmend, bool bPush);
    void Stash(bool bIncludeUntracked);
    void OpenFile(const css::code::vcs::git::FileChange& rChange);
    const css::code::vcs::git::FileChange* ChangeOf(const OUString& rPath) const;

    DECL_LINK(ToggledHdl, const weld::TreeView::iter_col&, void);
    DECL_LINK(ActivatedHdl, const weld::TreeIter&, bool);
    DECL_LINK(CommandHdl, const CommandEvent&, bool);
    DECL_LINK(RefreshHdl, weld::Button&, void);
    DECL_LINK(MoreHdl, weld::Button&, void);
    DECL_LINK(CommitHdl, weld::Button&, void);
    DECL_LINK(CommitPushHdl, weld::Button&, void);
    DECL_LINK(AmendHdl, weld::Toggleable&, void);
    DECL_LINK(MessageChangedHdl, weld::TextWidget&, void);
    DECL_STATIC_LINK(GitChangesPanel, DoneHdl, void*, void);

    std::unique_ptr<weld::Label> mxBranch;
    std::unique_ptr<weld::Button> mxRefresh;
    std::unique_ptr<weld::Button> mxMore;
    std::unique_ptr<weld::TreeView> mxTree;
    std::unique_ptr<weld::TextView> mxMessage;
    std::unique_ptr<weld::CheckButton> mxAmend;
    std::unique_ptr<weld::Button> mxCommit;
    std::unique_ptr<weld::Button> mxCommitPush;
    std::unique_ptr<weld::Label> mxStatus;

    css::uno::Reference<css::code::vcs::XRepository> mxRepository;
    css::uno::Reference<css::code::vcs::git::XGitWorkingCopy> mxWorking;
    std::vector<css::code::vcs::git::FileChange> maChanges;
    std::vector<css::code::vcs::git::StashEntry> maStashes;
    // the groups the user closed: the rebuilt tree keeps them so
    std::map<OUString, bool> maExpanded;

    // told of the repositories and of the saved files; a burst is one refresh
    std::shared_ptr<::code::UiNotifier> mxNotifier;
    css::uno::Reference<css::uno::XInterface> mxVcsListener;
    css::uno::Reference<css::uno::XInterface> mxDocumentListener;

    // tells an answer that comes late that the panel is gone
    std::shared_ptr<bool> mpAlive = std::make_shared<bool>(true);
    bool mbLoading = false;
    bool mbReloadWanted = false;
    bool mbBusy = false;
    bool mbFilling = false;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
