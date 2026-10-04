/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gitbranchespopover.hxx"

#include <com/sun/star/code/vcs/VcsException.hpp>
#include <com/sun/star/code/vcs/XBranchBrowser.hpp>
#include <com/sun/star/code/vcs/git/XGitRepository.hpp>
#include <sal/log.hxx>
#include <vcl/event.hxx>
#include <vcl/keycodes.hxx>
#include <vcl/ptrstyle.hxx>
#include <vcl/settings.hxx>
#include <vcl/window.hxx>
#include <vcl/svapp.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Button.hxx>
#include <vcl/weld/CheckButton.hxx>
#include <vcl/weld/customweld.hxx>
#include <vcl/weld/Dialog.hxx>
#include <vcl/weld/Entry.hxx>
#include <vcl/weld/Label.hxx>
#include <vcl/weld/MessageDialog.hxx>
#include <vcl/weld/Popover.hxx>
#include <vcl/weld/TreeView.hxx>
#include <vcl/weld/Window.hxx>

#include <algorithm>
#include <thread>

using namespace css;
using namespace css::code::vcs;

namespace code
{
namespace
{
enum Column
{
    COL_NAME = 0,
    COL_INFO
};

// what an operation that ran on a thread of its own comes back with
struct Outcome
{
    std::shared_ptr<bool> pAlive;
    GitBranchesPopover* pSelf;
    OUString aName;
    bool bOk = true;
    bool bCloseWhenDone = false;
    OUString aMessage;
    OUString aOutput;
};

// "↑1 ↓2" as the list shows how far a branch is from what it follows
OUString Distance(const BranchInfo& rBranch)
{
    OUString aText;
    if (rBranch.Ahead > 0)
        aText += "↑" + OUString::number(rBranch.Ahead);
    if (rBranch.Behind > 0)
        aText += (aText.isEmpty() ? u""_ustr : u" "_ustr) + "↓" + OUString::number(rBranch.Behind);
    return aText;
}

// What the user made of the size of the list, kept for the next time it opens.
Size g_aListSize(340, 260);
constexpr Size MinListSize(260, 120);
constexpr Size MaxListSize(1400, 1000);

// Where the mouse is, in the coordinates of the main window, which does not
// move when the popup does: a drag measured inside the grip would chase the
// grip, which moves as the popup is resized.
bool PointerPosition(Point& rPos)
{
    vcl::Window* pWindow = Application::GetFirstTopLevelWindow();
    if (!pWindow)
        return false;
    rPos = pWindow->GetPointerState().maPos;
    return true;
}

// Which edges of the popup a grip moves.  The popup is resized as a window is:
// the edge that is dragged follows the mouse and the opposite one stays where
// it is, whichever side of the status bar the popup came up on.
struct Edges
{
    bool bLeft = false;
    bool bRight = false;
    bool bTop = false;
    bool bBottom = false;
};

class Grip final : public weld::CustomWidgetController
{
public:
    // the mouse's distance from where it took hold, as the popup's growth
    // sideways and up/down, in pixels
    Grip(PointerStyle ePointer, Edges aEdges, std::function<void()> aBegin,
         std::function<void(const Edges&, tools::Long, tools::Long)> aResize)
        : m_ePointer(ePointer)
        , m_aEdges(aEdges)
        , m_aBegin(std::move(aBegin))
        , m_aResize(std::move(aResize))
    {
    }

    virtual void SetDrawingArea(weld::DrawingArea* pDrawingArea) override
    {
        weld::CustomWidgetController::SetDrawingArea(pDrawingArea);
        pDrawingArea->set_cursor(m_ePointer);
    }

    // nothing to see: the border is only where the cursor changes
    virtual void Paint(vcl::RenderContext&, const tools::Rectangle&) override {}

    virtual bool MouseButtonDown(const MouseEvent& rEvent) override
    {
        if (!rEvent.IsLeft() || !PointerPosition(m_aPress))
            return false;
        m_bDragging = true;
        m_aBegin();
        GetDrawingArea()->grab_mouse();
        return true;
    }

    virtual bool MouseMove(const MouseEvent&) override
    {
        Point aPos;
        if (!m_bDragging || !PointerPosition(aPos))
            return false;
        m_aResize(m_aEdges, aPos.X() - m_aPress.X(), aPos.Y() - m_aPress.Y());
        return true;
    }

    virtual bool MouseButtonUp(const MouseEvent&) override
    {
        if (!m_bDragging)
            return false;
        m_bDragging = false;
        GetDrawingArea()->release_mouse();
        return true;
    }

private:
    PointerStyle m_ePointer;
    Edges m_aEdges;
    std::function<void()> m_aBegin;
    std::function<void(const Edges&, tools::Long, tools::Long)> m_aResize;
    Point m_aPress;
    bool m_bDragging = false;
};
}

GitBranchesPopover::GitBranchesPopover(weld::Window* pParent, const tools::Rectangle& rRect,
                                       uno::Reference<XRepository> xRepository)
    : m_pParent(pParent)
    , m_aRect(rRect)
    , m_xRepository(std::move(xRepository))
    , m_xBuilder(Application::CreateBuilder(pParent, u"code/ui/branchespopover.ui"_ustr))
    , m_xPopover(m_xBuilder->weld_popover(u"popover"_ustr))
    , m_xSearch(m_xBuilder->weld_entry(u"search"_ustr))
    , m_xTable(m_xBuilder->weld_tree_view(u"branches"_ustr))
    , m_xNewBranch(m_xBuilder->weld_button(u"newbranch"_ustr))
    , m_xFetch(m_xBuilder->weld_button(u"fetch"_ustr))
    , m_xPull(m_xBuilder->weld_button(u"pull"_ustr))
    , m_xPush(m_xBuilder->weld_button(u"push"_ustr))
    , m_xStatus(m_xBuilder->weld_label(u"status"_ustr))
{
    auto addGrip = [&](const OUString& rId, PointerStyle ePointer, Edges aEdges)
    {
        m_aGripControllers.push_back(std::make_unique<Grip>(
            ePointer, aEdges, [this] { BeginResize(); },
            [this](const Edges& rEdges, tools::Long nDX, tools::Long nDY)
            { Resize(rEdges.bLeft, rEdges.bRight, rEdges.bTop, rEdges.bBottom, nDX, nDY); }));
        m_aGrips.push_back(
            std::make_unique<weld::CustomWeld>(*m_xBuilder, rId, *m_aGripControllers.back()));
    };
    addGrip(u"wedge"_ustr, PointerStyle::WindowWSize, { true, false, false, false });
    addGrip(u"eedge"_ustr, PointerStyle::WindowESize, { false, true, false, false });
    addGrip(u"nedge"_ustr, PointerStyle::WindowNSize, { false, false, true, false });
    addGrip(u"sedge"_ustr, PointerStyle::WindowSSize, { false, false, false, true });
    addGrip(u"nwedge"_ustr, PointerStyle::WindowNWSize, { true, false, true, false });
    addGrip(u"needge"_ustr, PointerStyle::WindowNESize, { false, true, true, false });
    addGrip(u"swedge"_ustr, PointerStyle::WindowSWSize, { true, false, false, true });
    addGrip(u"seedge"_ustr, PointerStyle::WindowSESize, { false, true, false, true });
    m_xTable->set_size_request(g_aListSize.Width(), g_aListSize.Height());
    m_xPopover->connect_closed(LINK(this, GitBranchesPopover, ClosedHdl));
    m_xSearch->connect_changed(LINK(this, GitBranchesPopover, SearchHdl));
    m_xSearch->connect_activate(LINK(this, GitBranchesPopover, SearchActivateHdl));
    m_xSearch->connect_key_press(LINK(this, GitBranchesPopover, SearchKeyHdl));
    m_xTable->connect_item_activated(LINK(this, GitBranchesPopover, ActivatedHdl));
    m_xNewBranch->connect_clicked(LINK(this, GitBranchesPopover, NewBranchHdl));
    m_xFetch->connect_clicked(LINK(this, GitBranchesPopover, FetchHdl));
    m_xPull->connect_clicked(LINK(this, GitBranchesPopover, PullHdl));
    m_xPush->connect_clicked(LINK(this, GitBranchesPopover, PushHdl));
}

GitBranchesPopover::~GitBranchesPopover()
{
    *m_pAlive = false; // an operation still running finds nobody to tell
    if (m_bShown)
        m_xPopover->popdown();
}

void GitBranchesPopover::BeginResize()
{
    m_aStartSize = g_aListSize;
    m_bHasStartRect = m_xPopover->get_screen_rect(m_aStartRect);
}

// The edges that are dragged follow the mouse by (nDX, nDY) from where it took
// hold, the opposite ones stay where they were.  The size is the list's, kept
// between the starts; the popup is that and the same border around, so the
// change of the one is the change of the other.
void GitBranchesPopover::Resize(bool bLeft, bool bRight, bool bTop, bool bBottom, tools::Long nDX,
                                tools::Long nDY)
{
    const tools::Long nGrowX = bLeft ? -nDX : bRight ? nDX : 0;
    const tools::Long nGrowY = bTop ? -nDY : bBottom ? nDY : 0;
    g_aListSize = Size(std::clamp(m_aStartSize.Width() + nGrowX, MinListSize.Width(), MaxListSize.Width()),
                       std::clamp(m_aStartSize.Height() + nGrowY, MinListSize.Height(),
                                  MaxListSize.Height()));
    m_xTable->set_size_request(g_aListSize.Width(), g_aListSize.Height());
    m_xPopover->resize_to_request();

    // what the clamps allowed, not what was asked
    if (!m_bHasStartRect)
        return; // the backend places the popup itself; it grows where it wishes
    const tools::Long nWidth = m_aStartRect.GetWidth() + g_aListSize.Width() - m_aStartSize.Width();
    const tools::Long nHeight
        = m_aStartRect.GetHeight() + g_aListSize.Height() - m_aStartSize.Height();
    const tools::Long nX = bLeft ? m_aStartRect.Right() + 1 - nWidth : m_aStartRect.Left();
    const tools::Long nY = bTop ? m_aStartRect.Bottom() + 1 - nHeight : m_aStartRect.Top();
    m_xPopover->set_screen_position(Point(nX, nY));
}

void GitBranchesPopover::Show()
{
    Load();
    Fill();
    // weld::Placement only knows Under/End; both backends flip the popup to
    // the other side when there is no room, and below a status bar there is none
    m_xPopover->popup_at_rect(m_pParent, m_aRect);
    // the generic-vcl popover does not size itself without this
    m_xPopover->resize_to_request();
    m_bShown = true;
    m_xSearch->grab_focus();
}

void GitBranchesPopover::Close()
{
    if (m_bShown)
        m_xPopover->popdown();
}

// the branches, as the repository lists them now
void GitBranchesPopover::Load()
{
    m_aBranches.clear();
    try
    {
        uno::Reference<XBranchBrowser> xBrowser(m_xRepository, uno::UNO_QUERY);
        if (xBrowser.is())
        {
            const uno::Sequence<BranchInfo> aBranches = xBrowser->getBranches();
            m_aBranches.assign(aBranches.begin(), aBranches.end());
        }
        m_xStatus->set_label(OUString());
    }
    catch (const uno::Exception& rException)
    {
        m_xStatus->set_label(rException.Message);
    }
}

void GitBranchesPopover::Fill()
{
    const OUString aQuery = m_xSearch->get_text().trim().toAsciiLowerCase();
    auto matches = [&](const BranchInfo& rBranch)
    { return aQuery.isEmpty() || rBranch.Name.toAsciiLowerCase().indexOf(aQuery) >= 0; };

    m_aRows.clear();
    m_xTable->freeze();
    m_xTable->clear();
    for (const bool bRemote : { false, true })
    {
        bool bHeading = false;
        for (size_t i = 0; i < m_aBranches.size(); ++i)
        {
            const BranchInfo& rBranch = m_aBranches[i];
            if (rBranch.Remote.isEmpty() == bRemote || !matches(rBranch))
                continue;
            if (!bHeading)
            {
                bHeading = true;
                const int nHeading = m_xTable->n_children();
                m_xTable->append();
                m_xTable->set_text(nHeading, bRemote ? u"Remote"_ustr : u"Local"_ustr, COL_NAME);
                m_xTable->set_text_emphasis(nHeading, true, COL_NAME);
                m_aRows.push_back(-1);
            }
            const int nRow = m_xTable->n_children();
            m_xTable->append();
            m_xTable->set_text(nRow, rBranch.Name, COL_NAME);
            m_xTable->set_text_emphasis(nRow, rBranch.IsCurrent, COL_NAME);
            m_xTable->set_text(nRow, rBranch.IsCurrent ? u"✓ "_ustr + Distance(rBranch) : Distance(rBranch),
                               COL_INFO);
            m_aRows.push_back(static_cast<int>(i));
        }
    }
    m_xTable->thaw();
    for (size_t nRow = 0; nRow < m_aRows.size(); ++nRow)
        if (m_aRows[nRow] >= 0)
        {
            m_xTable->select(static_cast<int>(nRow));
            break;
        }
}

void GitBranchesPopover::SwitchTo(int nRow)
{
    if (m_bBusy || nRow < 0 || static_cast<size_t>(nRow) >= m_aRows.size() || m_aRows[nRow] < 0)
        return;
    const BranchInfo aBranch = m_aBranches[m_aRows[nRow]];
    if (aBranch.IsCurrent)
    {
        Close();
        return;
    }
    uno::Reference<git::XGitRepository> xGit(m_xRepository, uno::UNO_QUERY);
    if (!xGit.is())
        return;
    RunAsync("Switching to " + aBranch.Name, [xGit, aBranch] { xGit->checkout(aBranch.Name); }, true);
}

void GitBranchesPopover::SetBusy(bool bBusy, const OUString& rMessage)
{
    m_bBusy = bBusy;
    for (weld::Button* pButton :
         { m_xNewBranch.get(), m_xFetch.get(), m_xPull.get(), m_xPush.get() })
        pButton->set_sensitive(!bBusy);
    m_xStatus->set_label(rMessage);
}

// The operation takes as long as the network does: it runs on a thread, and
// what it ends with is handled on the main one.
void GitBranchesPopover::RunAsync(const OUString& rName, std::function<void()> aOperation,
                                  bool bCloseWhenDone)
{
    SetBusy(true, rName + "...");
    std::thread(
        [pAlive = m_pAlive, pSelf = this, rName, aOperation = std::move(aOperation), bCloseWhenDone]
        {
            auto* pOutcome = new Outcome;
            pOutcome->pAlive = pAlive;
            pOutcome->pSelf = pSelf;
            pOutcome->aName = rName;
            pOutcome->bCloseWhenDone = bCloseWhenDone;
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
            Application::PostUserEvent(LINK(nullptr, GitBranchesPopover, DoneHdl), pOutcome);
        })
        .detach();
}

void GitBranchesPopover::ReportFailure(const OUString& rWhat, const OUString& rMessage,
                                       const OUString& rOutput)
{
    // a dialog over an open popup would fight it for the focus
    Close();
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
        m_pParent, VclMessageType::Warning, VclButtonsType::Ok, rWhat + " failed: " + rMessage));
    if (!rOutput.trim().isEmpty() && rOutput.trim() != rMessage.trim())
        xBox->set_secondary_text(rOutput.trim());
    xBox->run();
}

IMPL_STATIC_LINK(GitBranchesPopover, DoneHdl, void*, pData, void)
{
    std::unique_ptr<Outcome> xOutcome(static_cast<Outcome*>(pData));
    if (!*xOutcome->pAlive)
        return; // the popup is gone, and so is the place to tell
    GitBranchesPopover& rSelf = *xOutcome->pSelf;
    rSelf.SetBusy(false, OUString());
    if (!xOutcome->bOk)
    {
        rSelf.ReportFailure(xOutcome->aName, xOutcome->aMessage, xOutcome->aOutput);
        rSelf.Load();
        rSelf.Fill();
        return;
    }
    if (xOutcome->bCloseWhenDone)
    {
        rSelf.Close();
        return;
    }
    // what was fetched or pulled may be new branches
    rSelf.Load();
    rSelf.Fill();
}

IMPL_LINK_NOARG(GitBranchesPopover, ClosedHdl, weld::Popover&, void) { m_bShown = false; }

IMPL_LINK_NOARG(GitBranchesPopover, SearchHdl, weld::TextWidget&, void) { Fill(); }

IMPL_LINK_NOARG(GitBranchesPopover, SearchActivateHdl, weld::Entry&, bool)
{
    SwitchTo(m_xTable->get_selected_index());
    return true;
}

// Up and Down move through the list while the typing stays in the field
IMPL_LINK(GitBranchesPopover, SearchKeyHdl, const KeyEvent&, rEvent, bool)
{
    const sal_uInt16 nKey = rEvent.GetKeyCode().GetCode();
    if (nKey != KEY_DOWN && nKey != KEY_UP)
        return false;
    const int nStep = nKey == KEY_DOWN ? 1 : -1;
    for (int nRow = m_xTable->get_selected_index() + nStep;
         nRow >= 0 && static_cast<size_t>(nRow) < m_aRows.size(); nRow += nStep)
        if (m_aRows[nRow] >= 0)
        {
            m_xTable->select(nRow);
            m_xTable->scroll_to_row(nRow);
            break;
        }
    return true;
}

IMPL_LINK(GitBranchesPopover, ActivatedHdl, const weld::TreeIter&, rIter, bool)
{
    SwitchTo(m_xTable->get_iter_index_in_parent(rIter));
    return true;
}

IMPL_LINK_NOARG(GitBranchesPopover, FetchHdl, weld::Button&, void)
{
    uno::Reference<git::XGitRepository> xGit(m_xRepository, uno::UNO_QUERY);
    if (xGit.is())
        RunAsync(u"Fetch"_ustr, [xGit] { xGit->fetch(); }, false);
}

IMPL_LINK_NOARG(GitBranchesPopover, PullHdl, weld::Button&, void)
{
    uno::Reference<git::XGitRepository> xGit(m_xRepository, uno::UNO_QUERY);
    if (xGit.is())
        RunAsync(u"Pull"_ustr, [xGit] { xGit->pull(); }, false);
}

IMPL_LINK_NOARG(GitBranchesPopover, PushHdl, weld::Button&, void)
{
    uno::Reference<git::XGitRepository> xGit(m_xRepository, uno::UNO_QUERY);
    if (xGit.is())
        RunAsync(u"Push"_ustr, [xGit] { xGit->push(); }, false);
}

IMPL_LINK_NOARG(GitBranchesPopover, NewBranchHdl, weld::Button&, void)
{
    uno::Reference<git::XGitRepository> xGit(m_xRepository, uno::UNO_QUERY);
    if (!xGit.is())
        return;
    // the popup goes first: the dialog is modal, and the two would fight over the focus
    Close();

    std::unique_ptr<weld::Builder> xBuilder(
        Application::CreateBuilder(m_pParent, u"code/ui/newbranchdialog.ui"_ustr));
    std::unique_ptr<weld::Dialog> xDialog(xBuilder->weld_dialog(u"NewBranchDialog"_ustr));
    std::unique_ptr<weld::Entry> xName(xBuilder->weld_entry(u"name"_ustr));
    std::unique_ptr<weld::Entry> xFrom(xBuilder->weld_entry(u"from"_ustr));
    std::unique_ptr<weld::CheckButton> xCheckout(xBuilder->weld_check_button(u"checkout"_ustr));
    xName->grab_focus();
    for (;;)
    {
        if (xDialog->run() != RET_OK)
            return;
        if (!xName->get_text().trim().isEmpty())
            break;
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
            xDialog.get(), VclMessageType::Warning, VclButtonsType::Ok,
            u"Enter a name for the new branch."_ustr));
        xBox->run();
    }
    const OUString aName = xName->get_text().trim();
    const OUString aFrom = xFrom->get_text().trim();
    const bool bCheckout = xCheckout->get_active();
    RunAsync("Creating " + aName,
             [xGit, aName, aFrom, bCheckout] { xGit->createBranch(aName, aFrom, bCheckout); },
             false);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
