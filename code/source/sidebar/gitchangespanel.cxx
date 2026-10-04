/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gitchangespanel.hxx"

#include "../uibridge.hxx"

#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/document/DocumentEvent.hpp>
#include <com/sun/star/code/document/XDocumentListener.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/frontend/XEditorHost.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <com/sun/star/code/vcs/RepositorySummary.hpp>
#include <com/sun/star/code/vcs/VcsException.hpp>
#include <com/sun/star/code/vcs/XRepositoryListener.hpp>
#include <com/sun/star/code/vcs/XVcsManager.hpp>
#include <com/sun/star/code/vcs/theVcsManager.hpp>
#include <com/sun/star/code/vcs/git/ChangeKind.hpp>
#include <com/sun/star/code/vcs/git/XGitRepository.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <sal/log.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/svapp.hxx>
#include <vcl/unohelp2.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Button.hxx>
#include <vcl/weld/CheckButton.hxx>
#include <vcl/weld/Dialog.hxx>
#include <vcl/weld/Label.hxx>
#include <vcl/weld/Menu.hxx>
#include <vcl/weld/MessageDialog.hxx>
#include <vcl/weld/TextView.hxx>
#include <vcl/weld/TreeView.hxx>

#include <algorithm>
#include <set>
#include <thread>

using namespace css;
using namespace css::code::vcs;

namespace code::sidebar
{
namespace
{
// The id of a group's row is "g:" and its number; of a file's, "f:", the number of its group, ":" and
// its path; of a stash's, "s:" and its index.
OUString GroupId(int nGroup) { return "g:" + OUString::number(nGroup); }

// text column indexes, after the toggle that comes before the first
constexpr int COL_TEXT = 0;
constexpr int COL_KIND = 1;

// the repository of the active project, else the first one open that has a
// working copy to look at
uno::Reference<XRepository> FindRepository()
{
    try
    {
        auto xContext = comphelper::getProcessComponentContext();
        uno::Reference<XVcsManager> xManager = theVcsManager::get(xContext);
        if (auto xProject = css::code::project::theProjectManager::get(xContext)->getActiveProject())
            if (auto xRepository = xManager->getRepository(xProject->getUrl()))
                return xRepository;
        const auto aRepositories = xManager->getRepositories();
        if (aRepositories.hasElements())
            return aRepositories[0];
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "Git Changes cannot tell the repository: " << rException.Message);
    }
    return {};
}

OUString KindLetter(git::ChangeKind eKind)
{
    switch (eKind)
    {
        case git::ChangeKind_ADDED:
            return u"A"_ustr;
        case git::ChangeKind_DELETED:
            return u"D"_ustr;
        case git::ChangeKind_RENAMED:
            return u"R"_ustr;
        case git::ChangeKind_COPIED:
            return u"C"_ustr;
        case git::ChangeKind_CONFLICTED:
            return u"!"_ustr;
        case git::ChangeKind_UNTRACKED:
            return u"?"_ustr;
        default:
            return u"M"_ustr;
    }
}

// Heard of the files saved or closed: what the working copy differs by
// changes then, and no repository event says so.  A change to the text is not
// news, the file is the same until it is saved.
class SavedListener final : public cppu::WeakImplHelper<css::code::document::XDocumentListener>
{
public:
    explicit SavedListener(std::shared_ptr<::code::UiNotifier> xNotifier)
        : m_xNotifier(std::move(xNotifier))
    {
    }
    void SAL_CALL documentChanged(const css::code::document::DocumentEvent& rEvent) override
    {
        switch (rEvent.Kind)
        {
            case css::code::document::DocumentEventKind_SAVED:
            case css::code::document::DocumentEventKind_CLOSED:
            case css::code::document::DocumentEventKind_RELOADED:
                m_xNotifier->Notify();
                break;
            default:
                break;
        }
    }
    void SAL_CALL disposing(const lang::EventObject&) override {}

private:
    std::shared_ptr<::code::UiNotifier> m_xNotifier;
};
}

struct GitChangesPanel::Outcome
{
    std::shared_ptr<bool> pAlive;
    GitChangesPanel* pSelf = nullptr;
    // a look (true) or an operation (false)
    bool bLook = false;
    OUString aName;
    bool bOk = true;
    bool bClearMessage = false;
    std::function<void()> aAfter;
    OUString aMessage;
    OUString aOutput;
    std::vector<git::FileChange> aChanges;
    std::vector<git::StashEntry> aStashes;
    uno::Reference<XRepository> xRepository;
};

GitChangesPanel::GitChangesPanel(weld::Widget* pParent)
    : PanelLayout(pParent, u"GitChangesPanel"_ustr, u"code/ui/gitchangespanel.ui"_ustr)
    , mxBranch(m_xBuilder->weld_label(u"branch"_ustr))
    , mxRefresh(m_xBuilder->weld_button(u"refresh"_ustr))
    , mxMore(m_xBuilder->weld_button(u"more"_ustr))
    , mxTree(m_xBuilder->weld_tree_view(u"tree"_ustr))
    , mxMessage(m_xBuilder->weld_text_view(u"message"_ustr))
    , mxAmend(m_xBuilder->weld_check_button(u"amend"_ustr))
    , mxCommit(m_xBuilder->weld_button(u"commit"_ustr))
    , mxCommitPush(m_xBuilder->weld_button(u"commitpush"_ustr))
    , mxStatus(m_xBuilder->weld_label(u"status"_ustr))
{
    // the toggle in front of every row: staged or not
    mxTree->enable_toggle_buttons();
    mxTree->set_toggle_button_type(weld::ColumnToggleType::Check);
    mxTree->connect_toggled(LINK(this, GitChangesPanel, ToggledHdl));
    mxTree->connect_item_activated(LINK(this, GitChangesPanel, ActivatedHdl));
    mxTree->connect_command(LINK(this, GitChangesPanel, CommandHdl));
    mxRefresh->connect_clicked(LINK(this, GitChangesPanel, RefreshHdl));
    mxMore->connect_clicked(LINK(this, GitChangesPanel, MoreHdl));
    mxCommit->connect_clicked(LINK(this, GitChangesPanel, CommitHdl));
    mxCommitPush->connect_clicked(LINK(this, GitChangesPanel, CommitPushHdl));
    mxAmend->connect_toggled(LINK(this, GitChangesPanel, AmendHdl));
    mxMessage->connect_changed(LINK(this, GitChangesPanel, MessageChangedHdl));

    mxNotifier = std::make_shared<::code::UiNotifier>([this] { Refresh(); });
    try
    {
        auto xContext = comphelper::getProcessComponentContext();
        auto xVcs = uno::Reference<XRepositoryListener>(new ::code::ServiceListener(mxNotifier));
        theVcsManager::get(xContext)->addVcsListener(xVcs);
        mxVcsListener = xVcs;
        auto xDocuments = uno::Reference<css::code::document::XDocumentListener>(new SavedListener(mxNotifier));
        css::code::document::theDocumentManager::get(xContext)->addDocumentListener(xDocuments);
        mxDocumentListener = xDocuments;
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "Git Changes cannot follow version control: " << rException.Message);
    }
    UpdateButtons();
    Refresh();
}

GitChangesPanel::~GitChangesPanel()
{
    *mpAlive = false; // an answer on its way finds nobody to tell
    mxNotifier->Detach();
    try
    {
        auto xContext = comphelper::getProcessComponentContext();
        if (mxVcsListener.is())
            theVcsManager::get(xContext)->removeVcsListener(
                uno::Reference<XRepositoryListener>(mxVcsListener, uno::UNO_QUERY));
        if (mxDocumentListener.is())
            css::code::document::theDocumentManager::get(xContext)->removeDocumentListener(
                uno::Reference<css::code::document::XDocumentListener>(mxDocumentListener,
                                                                       uno::UNO_QUERY));
    }
    catch (const uno::Exception&)
    {
    }
}

// ---- reading ---------------------------------------------------------------

void GitChangesPanel::Refresh()
{
    if (mbLoading)
    {
        mbReloadWanted = true; // what is read now may already be old
        return;
    }
    // the repository is looked for here, where a service call is quick; what is
    // asked of it takes long
    uno::Reference<XRepository> xRepository = FindRepository();
    uno::Reference<git::XGitWorkingCopy> xWorking(xRepository, uno::UNO_QUERY);
    if (!xWorking.is())
    {
        mxRepository.clear();
        mxWorking.clear();
        maChanges.clear();
        maStashes.clear();
        mxBranch->set_label(OUString());
        mxStatus->set_label(OUString());
        Fill();
        UpdateButtons();
        return;
    }
    mbLoading = true;
    std::thread(
        [pAlive = mpAlive, pSelf = this, xRepository, xWorking]
        {
            auto* pOutcome = new Outcome;
            pOutcome->pAlive = pAlive;
            pOutcome->pSelf = pSelf;
            pOutcome->bLook = true;
            pOutcome->xRepository = xRepository;
            try
            {
                const auto aChanges = xWorking->getChanges();
                pOutcome->aChanges.assign(aChanges.begin(), aChanges.end());
                const auto aStashes = xWorking->getStashes();
                pOutcome->aStashes.assign(aStashes.begin(), aStashes.end());
            }
            catch (const VcsException& rException)
            {
                pOutcome->bOk = false;
                pOutcome->aMessage = rException.Message;
                pOutcome->aOutput = rException.Output;
            }
            catch (const uno::Exception& rException)
            {
                pOutcome->bOk = false;
                pOutcome->aMessage = rException.Message;
            }
            Application::PostUserEvent(LINK(nullptr, GitChangesPanel, DoneHdl), pOutcome);
        })
        .detach();
}

void GitChangesPanel::Show(const Outcome& rOutcome)
{
    mbLoading = false;
    if (rOutcome.bOk)
    {
        mxRepository = rOutcome.xRepository;
        mxWorking.set(rOutcome.xRepository, uno::UNO_QUERY);
        maChanges = rOutcome.aChanges;
        maStashes = rOutcome.aStashes;
        try
        {
            const RepositorySummary aSummary = mxRepository->getSummary();
            OUString aBranch = aSummary.Label;
            if (aSummary.Ahead > 0)
                aBranch += " ↑" + OUString::number(aSummary.Ahead);
            if (aSummary.Behind > 0)
                aBranch += " ↓" + OUString::number(aSummary.Behind);
            mxBranch->set_label(aBranch);
        }
        catch (const uno::Exception&)
        {
        }
        if (!mbBusy)
            mxStatus->set_label(OUString::number(maChanges.size()) + (maChanges.size() == 1 ? u" changed file"_ustr : u" changed files"_ustr));
        Fill();
        UpdateButtons();
    }
    else
        mxStatus->set_label(rOutcome.aMessage);
    if (mbReloadWanted)
    {
        mbReloadWanted = false;
        Refresh();
    }
}

const git::FileChange* GitChangesPanel::ChangeOf(const OUString& rPath) const
{
    auto it = std::find_if(maChanges.begin(), maChanges.end(),
                           [&](const git::FileChange& r) { return r.Path == rPath; });
    return it == maChanges.end() ? nullptr : &*it;
}

// ---- the tree --------------------------------------------------------------

void GitChangesPanel::Fill()
{
    mbFilling = true;
    // what the user closed or opened stays so
    {
        std::unique_ptr<weld::TreeIter> xIter = mxTree->make_iterator();
        for (bool bRow = mxTree->get_iter_first(*xIter); bRow; bRow = mxTree->iter_next_sibling(*xIter))
            maExpanded[mxTree->get_id(*xIter)] = mxTree->get_row_expanded(*xIter);
    }
    mxTree->freeze();
    mxTree->clear();

    auto addGroup = [&](Group eGroup, const OUString& rTitle, size_t nCount,
                        const std::function<void(const weld::TreeIter&)>& rFill)
    {
        if (nCount == 0)
            return;
        const OUString aId = GroupId(static_cast<int>(eGroup));
        std::unique_ptr<weld::TreeIter> xGroup = mxTree->make_iterator();
        const OUString aTitle = rTitle + " (" + OUString::number(nCount) + ")";
        mxTree->insert(nullptr, -1, &aTitle, &aId, nullptr, nullptr, false, xGroup.get());
        mxTree->set_text_emphasis(*xGroup, true, COL_TEXT);
        // a group's check mark moves all of it at once; the stashes have none
        if (eGroup != Group::Stashes)
            mxTree->set_toggle(*xGroup, eGroup == Group::Staged ? TRISTATE_TRUE : TRISTATE_FALSE);
        rFill(*xGroup);
        auto it = maExpanded.find(aId);
        if (it == maExpanded.end() ? eGroup != Group::Stashes : it->second)
            mxTree->expand_row(*xGroup);
    };
    auto addFile = [&](const weld::TreeIter& rGroup, Group eGroup, const git::FileChange& rChange,
                       git::ChangeKind eKind)
    {
        const OUString aId = "f:" + OUString::number(static_cast<int>(eGroup)) + ":" + rChange.Path;
        std::unique_ptr<weld::TreeIter> xRow = mxTree->make_iterator();
        mxTree->insert(&rGroup, -1, &rChange.Path, &aId, nullptr, nullptr, false, xRow.get());
        mxTree->set_text(*xRow, KindLetter(eKind), COL_KIND);
        mxTree->set_toggle(*xRow, eGroup == Group::Staged ? TRISTATE_TRUE : TRISTATE_FALSE);
    };

    size_t nStaged = 0, nChanges = 0, nUntracked = 0;
    for (const auto& rChange : maChanges)
    {
        nStaged += rChange.HasStaged;
        if (rChange.HasUnstaged)
            (rChange.Unstaged == git::ChangeKind_UNTRACKED ? nUntracked : nChanges)++;
    }
    addGroup(Group::Staged, u"Staged Changes"_ustr, nStaged,
             [&](const weld::TreeIter& rGroup)
             {
                 for (const auto& rChange : maChanges)
                     if (rChange.HasStaged)
                         addFile(rGroup, Group::Staged, rChange, rChange.Staged);
             });
    addGroup(Group::Changes, u"Changes"_ustr, nChanges,
             [&](const weld::TreeIter& rGroup)
             {
                 for (const auto& rChange : maChanges)
                     if (rChange.HasUnstaged && rChange.Unstaged != git::ChangeKind_UNTRACKED)
                         addFile(rGroup, Group::Changes, rChange, rChange.Unstaged);
             });
    addGroup(Group::Untracked, u"Untracked Files"_ustr, nUntracked,
             [&](const weld::TreeIter& rGroup)
             {
                 for (const auto& rChange : maChanges)
                     if (rChange.HasUnstaged && rChange.Unstaged == git::ChangeKind_UNTRACKED)
                         addFile(rGroup, Group::Untracked, rChange, rChange.Unstaged);
             });
    addGroup(Group::Stashes, u"Stashes"_ustr, maStashes.size(),
             [&](const weld::TreeIter& rGroup)
             {
                 for (const auto& rStash : maStashes)
                 {
                     const OUString aId = "s:" + OUString::number(rStash.Index);
                     OUString aText = "stash@{" + OUString::number(rStash.Index) + "}: " + rStash.Message;
                     std::unique_ptr<weld::TreeIter> xRow = mxTree->make_iterator();
                     mxTree->insert(&rGroup, -1, &aText, &aId, nullptr, nullptr, false, xRow.get());
                     mxTree->set_text(*xRow, rStash.Branch, COL_KIND);
                 }
             });
    mxTree->thaw();
    mbFilling = false;
}

bool GitChangesPanel::ParseRow(const OUString& rId, Group& reGroup, OUString& rPath) const
{
    if (!rId.startsWith("f:"))
        return false;
    const sal_Int32 nColon = rId.indexOf(':', 2);
    if (nColon < 0)
        return false;
    reGroup = static_cast<Group>(rId.copy(2, nColon - 2).toInt32());
    rPath = rId.copy(nColon + 1);
    return true;
}

std::vector<OUString> GitChangesPanel::PathsOf(Group eGroup) const
{
    std::vector<OUString> aPaths;
    for (const auto& rChange : maChanges)
    {
        const bool bUntracked = rChange.HasUnstaged && rChange.Unstaged == git::ChangeKind_UNTRACKED;
        const bool bIn = eGroup == Group::Staged      ? rChange.HasStaged
                         : eGroup == Group::Untracked ? bUntracked
                                                      : rChange.HasUnstaged && !bUntracked;
        if (bIn)
            aPaths.push_back(rChange.Path);
    }
    return aPaths;
}

// ---- doing -----------------------------------------------------------------

void GitChangesPanel::UpdateButtons()
{
    const bool bRepository = mxWorking.is();
    size_t nStaged = 0;
    for (const auto& rChange : maChanges)
        nStaged += rChange.HasStaged;
    const bool bMessage = !mxMessage->get_text().trim().isEmpty();
    const bool bAmend = mxAmend->get_active();
    // an amend takes the changes into the last commit, with a message of its own or without
    mxCommit->set_label(bAmend ? u"_Amend"_ustr : u"_Commit"_ustr);
    mxCommitPush->set_label(bAmend ? u"Amend and _Push..."_ustr : u"Commit and _Push..."_ustr);
    const bool bCanCommit = bRepository && !mbBusy && (bAmend ? (bMessage || nStaged > 0) : (bMessage && nStaged > 0));
    mxCommit->set_sensitive(bCanCommit);
    mxCommitPush->set_sensitive(bCanCommit);
    mxAmend->set_sensitive(bRepository && !mbBusy);
    mxMore->set_sensitive(bRepository && !mbBusy);
    mxRefresh->set_sensitive(bRepository);
}

void GitChangesPanel::SetBusy(bool bBusy, const OUString& rMessage)
{
    mbBusy = bBusy;
    mxTree->set_sensitive(!bBusy);
    mxStatus->set_label(rMessage);
    UpdateButtons();
}

// The operation takes as long as git does; it runs on a thread, and what it
// ends with is handled on the main one.
void GitChangesPanel::RunAsync(const OUString& rName, std::function<void()> aOperation,
                               bool bClearMessage, std::function<void()> aAfter)
{
    if (mbBusy)
        return;
    SetBusy(true, rName + "...");
    std::thread(
        [pAlive = mpAlive, pSelf = this, rName, aOperation = std::move(aOperation), bClearMessage,
         aAfter = std::move(aAfter)]
        {
            auto* pOutcome = new Outcome;
            pOutcome->pAlive = pAlive;
            pOutcome->pSelf = pSelf;
            pOutcome->aName = rName;
            pOutcome->bClearMessage = bClearMessage;
            pOutcome->aAfter = aAfter;
            try
            {
                aOperation();
            }
            catch (const VcsException& rException)
            {
                pOutcome->bOk = false;
                pOutcome->aMessage = rException.Message;
                pOutcome->aOutput = rException.Output;
            }
            catch (const uno::Exception& rException)
            {
                pOutcome->bOk = false;
                pOutcome->aMessage = rException.Message;
            }
            catch (const std::exception&)
            {
                pOutcome->bOk = false;
                pOutcome->aMessage = u"internal error"_ustr;
            }
            Application::PostUserEvent(LINK(nullptr, GitChangesPanel, DoneHdl), pOutcome);
        })
        .detach();
}

IMPL_STATIC_LINK(GitChangesPanel, DoneHdl, void*, pData, void)
{
    std::unique_ptr<GitChangesPanel::Outcome> xOutcome(static_cast<GitChangesPanel::Outcome*>(pData));
    if (!*xOutcome->pAlive)
        return; // the panel is gone, and so is the place to tell
    GitChangesPanel& rSelf = *xOutcome->pSelf;
    if (xOutcome->bLook)
    {
        rSelf.Show(*xOutcome);
        return;
    }
    rSelf.SetBusy(false, OUString());
    if (xOutcome->bOk)
    {
        if (xOutcome->bClearMessage)
        {
            rSelf.mxMessage->set_text(OUString());
            rSelf.mxAmend->set_active(false);
        }
    }
    else
        rSelf.ReportFailure(xOutcome->aName, xOutcome->aMessage, xOutcome->aOutput);
    rSelf.Refresh();
    if (xOutcome->bOk && xOutcome->aAfter)
        xOutcome->aAfter();
}

void GitChangesPanel::ReportFailure(const OUString& rWhat, const OUString& rMessage,
                                    const OUString& rOutput)
{
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
        mxTree.get(), VclMessageType::Warning, VclButtonsType::Ok, rWhat + " failed: " + rMessage));
    if (!rOutput.trim().isEmpty() && rOutput.trim() != rMessage.trim())
        xBox->set_secondary_text(rOutput.trim());
    xBox->run();
}

bool GitChangesPanel::Confirm(const OUString& rQuestion)
{
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
        mxTree.get(), VclMessageType::Question, VclButtonsType::YesNo, rQuestion));
    xBox->set_default_response(RET_NO);
    return xBox->run() == RET_YES;
}

void GitChangesPanel::StagePaths(const std::vector<OUString>& rPaths)
{
    if (rPaths.empty() || !mxWorking.is())
        return;
    auto xWorking = mxWorking;
    const auto aPaths = comphelper::containerToSequence(rPaths);
    RunAsync(u"Stage"_ustr, [xWorking, aPaths] { xWorking->stage(aPaths); }, false);
}

void GitChangesPanel::UnstagePaths(const std::vector<OUString>& rPaths)
{
    if (rPaths.empty() || !mxWorking.is())
        return;
    auto xWorking = mxWorking;
    const auto aPaths = comphelper::containerToSequence(rPaths);
    RunAsync(u"Unstage"_ustr, [xWorking, aPaths] { xWorking->unstage(aPaths); }, false);
}

void GitChangesPanel::RollbackPaths(const std::vector<OUString>& rPaths, bool bUntracked)
{
    if (rPaths.empty() || !mxWorking.is())
        return;
    const OUString aWhat = rPaths.size() == 1 ? rPaths[0] : OUString::number(rPaths.size()) + " files";
    const OUString aQuestion = bUntracked ? OUString("Delete " + aWhat + "? This cannot be undone.")
                                          : OUString("Discard the changes to " + aWhat
                                                     + "? This cannot be undone.");
    if (!Confirm(aQuestion))
        return;
    auto xWorking = mxWorking;
    const auto aPaths = comphelper::containerToSequence(rPaths);
    RunAsync(u"Rollback"_ustr, [xWorking, aPaths] { xWorking->rollback(aPaths); }, false);
}

void GitChangesPanel::Commit(bool bAmend, bool bPush)
{
    if (!mxWorking.is())
        return;
    const OUString aMessage = mxMessage->get_text();
    if (aMessage.trim().isEmpty() && !bAmend)
        return;
    auto xWorking = mxWorking;
    uno::Reference<git::XGitRepository> xGit(mxRepository, uno::UNO_QUERY);
    if (bPush && !xGit.is())
        return;
    // The commit is made first and for good; what a push would send is read
    // once it is, and the user decides then whether it goes.
    auto pOutgoing = std::make_shared<std::vector<git::CommitInfo>>();
    RunAsync(bAmend ? u"Amend"_ustr : u"Commit"_ustr,
             [xWorking, xGit, aMessage, bAmend, bPush, pOutgoing]
             {
                 xWorking->commit(aMessage, false, bAmend);
                 if (!bPush)
                     return;
                 try
                 {
                     const auto aCommits = xGit->getOutgoingCommits();
                     pOutgoing->assign(aCommits.begin(), aCommits.end());
                 }
                 catch (const uno::Exception&)
                 {
                     // the dialog then has nothing to list, and does not come up
                 }
             },
             true,
             bPush ? std::function<void()>([this, pOutgoing] { ConfirmPush(*pOutgoing); })
                   : std::function<void()>());
}

void GitChangesPanel::ConfirmPush(const std::vector<git::CommitInfo>& rCommits)
{
    uno::Reference<git::XGitRepository> xGit(mxRepository, uno::UNO_QUERY);
    if (rCommits.empty() || !xGit.is())
        return;
    std::unique_ptr<weld::Builder> xBuilder(
        Application::CreateBuilder(mxTree.get(), u"code/ui/pushdialog.ui"_ustr));
    std::unique_ptr<weld::Dialog> xDialog(xBuilder->weld_dialog(u"PushDialog"_ustr));
    std::unique_ptr<weld::Label> xSummary(xBuilder->weld_label(u"summary"_ustr));
    std::unique_ptr<weld::TreeView> xList(xBuilder->weld_tree_view(u"commits"_ustr));
    OUString aBranch = mxBranch->get_label();
    xSummary->set_label(OUString::number(rCommits.size())
                        + (rCommits.size() == 1 ? u" commit will be pushed"_ustr
                                                : u" commits will be pushed"_ustr)
                        + (aBranch.isEmpty() ? OUString() : " from " + aBranch));
    xList->freeze();
    for (const git::CommitInfo& rCommit : rCommits)
    {
        const int nRow = xList->n_children();
        xList->append();
        xList->set_text(nRow, rCommit.Hash, 0);
        xList->set_text(nRow, rCommit.Subject, 1);
        xList->set_text(nRow, rCommit.Author, 2);
    }
    xList->thaw();
    if (xDialog->run() != RET_OK)
        return; // the commit stays, and so does what it did not send
    RunAsync(u"Push"_ustr, [xGit] { xGit->push(); }, false);
}

void GitChangesPanel::Stash(bool bIncludeUntracked)
{
    if (!mxWorking.is())
        return;
    auto xWorking = mxWorking;
    RunAsync(u"Stash"_ustr, [xWorking, bIncludeUntracked] { xWorking->stash(OUString(), bIncludeUntracked); },
             false);
}

void GitChangesPanel::OpenFile(const git::FileChange& rChange)
{
    // a deleted file has nothing to open
    if (rChange.HasUnstaged && rChange.Unstaged == git::ChangeKind_DELETED)
        return;
    try
    {
        uno::Reference<css::code::frontend::XWorkbench> xWorkbench
            = css::code::core::theCode::get(comphelper::getProcessComponentContext())->getWorkbench();
        if (xWorkbench.is() && xWorkbench->getEditorHost().is())
            xWorkbench->getEditorHost()->openDocument(rChange.Url);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot open " << rChange.Url << ": " << rException.Message);
    }
}

// ---- handlers --------------------------------------------------------------

IMPL_LINK(GitChangesPanel, ToggledHdl, const weld::TreeView::iter_col&, rIterCol, void)
{
    if (mbFilling)
        return;
    const OUString aId = mxTree->get_id(rIterCol.first);
    Group eGroup;
    OUString aPath;
    // a group's mark moves every file of it, a file's only the file
    std::vector<OUString> aPaths;
    if (aId.startsWith("g:"))
    {
        eGroup = static_cast<Group>(aId.copy(2).toInt32());
        aPaths = PathsOf(eGroup);
    }
    else if (ParseRow(aId, eGroup, aPath))
        aPaths.push_back(aPath);
    else
        return;
    // the mark has been flipped by the click; the tree is redrawn once git has answered
    if (eGroup == Group::Staged)
        UnstagePaths(aPaths);
    else
        StagePaths(aPaths);
}

IMPL_LINK(GitChangesPanel, ActivatedHdl, const weld::TreeIter&, rIter, bool)
{
    Group eGroup;
    OUString aPath;
    if (!ParseRow(mxTree->get_id(rIter), eGroup, aPath))
        return false; // a group or a stash: let the tree open or close it
    if (const git::FileChange* pChange = ChangeOf(aPath))
        OpenFile(*pChange);
    return true;
}

IMPL_LINK(GitChangesPanel, CommandHdl, const CommandEvent&, rEvent, bool)
{
    if (rEvent.GetCommand() != CommandEventId::ContextMenu || mbBusy)
        return false;
    std::unique_ptr<weld::TreeIter> xRow = rEvent.IsMouseEvent()
        ? mxTree->get_dest_row_at_pos(rEvent.GetMousePosPixel(), false, false)
        : mxTree->get_cursor();
    if (!xRow)
        return false;
    const OUString aId = mxTree->get_id(*xRow);
    mxTree->select(*xRow);
    const tools::Rectangle aArea = rEvent.IsMouseEvent()
                                       ? tools::Rectangle(rEvent.GetMousePosPixel(), Size(1, 1))
                                       : mxTree->get_row_area(*xRow);

    std::unique_ptr<weld::Builder> xBuilder(
        Application::CreateBuilder(mxTree.get(), u"code/ui/gitchangesmenus.ui"_ustr));
    if (aId.startsWith("s:"))
    {
        const sal_Int32 nIndex = aId.copy(2).toInt32();
        std::unique_ptr<weld::Menu> xMenu = xBuilder->weld_menu(u"stashmenu"_ustr);
        const OUString aCommand = xMenu->popup_at_rect(mxTree.get(), aArea);
        auto xWorking = mxWorking;
        if (aCommand == "pop")
            RunAsync(u"Pop stash"_ustr, [xWorking, nIndex] { xWorking->stashPop(nIndex); }, false);
        else if (aCommand == "apply")
            RunAsync(u"Apply stash"_ustr, [xWorking, nIndex] { xWorking->stashApply(nIndex); }, false);
        else if (aCommand == "drop" && Confirm(u"Drop this stash? This cannot be undone."_ustr))
            RunAsync(u"Drop stash"_ustr, [xWorking, nIndex] { xWorking->stashDrop(nIndex); }, false);
        return true;
    }

    Group eGroup;
    OUString aPath;
    if (!ParseRow(aId, eGroup, aPath))
        return false;
    const git::FileChange* pChange = ChangeOf(aPath);
    if (!pChange)
        return false;
    const git::FileChange aChange = *pChange;

    std::unique_ptr<weld::Menu> xMenu = xBuilder->weld_menu(u"filemenu"_ustr);
    // only what the file can be asked for: a staged one is unstaged, one that is
    // not is staged, and what git does not know is added
    if (eGroup == Group::Staged)
    {
        xMenu->remove(u"stage"_ustr);
        xMenu->remove(u"add"_ustr);
    }
    else
    {
        xMenu->remove(u"unstage"_ustr);
        if (eGroup == Group::Untracked)
            xMenu->remove(u"stage"_ustr);
        else
            xMenu->remove(u"add"_ustr);
    }
    if (aChange.HasUnstaged && aChange.Unstaged == git::ChangeKind_DELETED)
        xMenu->set_sensitive(u"open"_ustr, false);

    const OUString aCommand = xMenu->popup_at_rect(mxTree.get(), aArea);
    if (aCommand == "open")
        OpenFile(aChange);
    else if (aCommand == "stage" || aCommand == "add")
        StagePaths({ aPath });
    else if (aCommand == "unstage")
        UnstagePaths({ aPath });
    else if (aCommand == "rollback")
        RollbackPaths({ aPath }, eGroup == Group::Untracked);
    else if (aCommand == "copypath")
        vcl::unohelper::TextDataObject::CopyStringTo(aPath, mxTree->get_clipboard());
    return true;
}

IMPL_LINK_NOARG(GitChangesPanel, RefreshHdl, weld::Button&, void)
{
    if (mxRepository.is())
    {
        // the repository reads its own state again as well, so the branch is current too
        try
        {
            mxRepository->refresh();
        }
        catch (const uno::Exception&)
        {
        }
    }
    Refresh();
}

IMPL_LINK_NOARG(GitChangesPanel, MoreHdl, weld::Button&, void)
{
    std::unique_ptr<weld::Builder> xBuilder(
        Application::CreateBuilder(mxMore.get(), u"code/ui/gitchangesmenus.ui"_ustr));
    std::unique_ptr<weld::Menu> xMenu = xBuilder->weld_menu(u"moremenu"_ustr);
    const OUString aCommand
        = xMenu->popup_at_rect(mxMore.get(), tools::Rectangle(Point(0, 0), mxMore->get_preferred_size()));
    if (aCommand == "stashall")
        Stash(false);
    else if (aCommand == "stashuntracked")
        Stash(true);
}

IMPL_LINK_NOARG(GitChangesPanel, CommitHdl, weld::Button&, void) { Commit(mxAmend->get_active(), false); }

IMPL_LINK_NOARG(GitChangesPanel, CommitPushHdl, weld::Button&, void)
{
    Commit(mxAmend->get_active(), true);
}

IMPL_LINK_NOARG(GitChangesPanel, AmendHdl, weld::Toggleable&, void) { UpdateButtons(); }

IMPL_LINK_NOARG(GitChangesPanel, MessageChangedHdl, weld::TextWidget&, void) { UpdateButtons(); }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
