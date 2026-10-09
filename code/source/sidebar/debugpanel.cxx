/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "debugpanel.hxx"

#include "../uibridge.hxx"
#include <startuptarget.hxx>

#include <com/sun/star/code/core/Position.hpp>
#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/debug/DebugEventKind.hpp>
#include <com/sun/star/code/debug/DebugException.hpp>
#include <com/sun/star/code/debug/LaunchDescriptor.hpp>
#include <com/sun/star/code/debug/Variable.hpp>
#include <com/sun/star/code/debug/XDebugAdapter.hpp>
#include <com/sun/star/code/debug/XDebugConsole.hpp>
#include <com/sun/star/code/debug/XDebugManager.hpp>
#include <com/sun/star/code/debug/XDebugManagerListener.hpp>
#include <com/sun/star/code/debug/XLaunchable.hpp>
#include <com/sun/star/code/debug/XDebugSessionListener.hpp>
#include <com/sun/star/code/debug/theDebugManager.hpp>
#include <com/sun/star/code/frontend/XEditorHost.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectTarget.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/implbase.hxx>
#include <o3tl/safeint.hxx>
#include <osl/file.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/svapp.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Button.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/Entry.hxx>
#include <vcl/weld/Label.hxx>
#include <vcl/weld/Menu.hxx>
#include <vcl/weld/MessageDialog.hxx>
#include <vcl/weld/TextView.hxx>
#include <vcl/weld/Toolbar.hxx>
#include <vcl/weld/TreeView.hxx>

#include <algorithm>
#include <thread>
#include <utility>

using namespace css;
using namespace css::code::debug;

namespace code::sidebar
{
namespace
{
// text column indexes beyond the first (insert() takes that one); in the
// breakpoints tree they count after the toggle
constexpr int BKP_COL_CONDITION = 1;
constexpr int STACK_COL_PLACE = 1;
constexpr int VAR_COL_VALUE = 1;
constexpr int VAR_COL_TYPE = 2;

uno::Reference<uno::XComponentContext> Context() { return comphelper::getProcessComponentContext(); }

uno::Reference<css::code::frontend::XEditorHost> EditorHost()
{
    uno::Reference<css::code::frontend::XWorkbench> xWorkbench
        = css::code::core::theCode::get(Context())->getWorkbench();
    return xWorkbench.is() ? xWorkbench->getEditorHost()
                           : uno::Reference<css::code::frontend::XEditorHost>();
}

// "file.cxx:42" as people read places; lines are zero-based inside
OUString DisplayPlace(const OUString& rUrl, sal_Int32 nLine)
{
    OUString aName = rUrl;
    const sal_Int32 nSlash = aName.lastIndexOf('/');
    if (nSlash >= 0)
        aName = aName.copy(nSlash + 1);
    return aName + ":" + OUString::number(nLine + 1);
}

// The add-entry's little language: "file:42", ":42" or "42" (the open
// file), with an optional " if <expression>" after it.  The line comes out
// as people typed it, one-based.
bool ParseBreakpointEntry(const OUString& rText, OUString& rFile, sal_Int32& rLine,
                          OUString& rCondition)
{
    OUString aText = rText.trim();
    const sal_Int32 nIf = aText.indexOf(" if ");
    if (nIf >= 0)
    {
        rCondition = aText.copy(nIf + 4).trim();
        aText = aText.copy(0, nIf).trim();
    }
    const sal_Int32 nColon = aText.lastIndexOf(':');
    const OUString aLine = nColon >= 0 ? aText.copy(nColon + 1) : aText;
    rFile = nColon >= 0 ? aText.copy(0, nColon) : OUString();
    rLine = aLine.toInt32();
    return rLine > 0;
}

// The descriptor of the project's startup target, as Start without a request
// runs it: the active project (else the first open one), its startup target,
// the effective launch the target describes.  False when there is nothing to
// run.
bool ResolveStartupDescriptor(LaunchDescriptor& rDescriptor)
{
    try
    {
        auto xContext = Context();
        auto xManager = css::code::project::theProjectManager::get(xContext);
        uno::Reference<css::code::project::XProject> xProject = xManager->getActiveProject();
        if (!xProject.is())
        {
            const auto aProjects = xManager->getOpenProjects();
            if (aProjects.hasElements())
                xProject = aProjects[0];
        }
        uno::Reference<css::code::project::XProjectTarget> xTarget
            = ::code::startup::ResolveStartupTarget(xContext, xProject);
        uno::Reference<XLaunchable> xLaunchable(xTarget, uno::UNO_QUERY);
        if (!xLaunchable.is())
            return false;
        rDescriptor = xLaunchable->createLaunchDescriptor();
        return !rDescriptor.Program.isEmpty();
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot resolve the startup target: " << rException.Message);
        return false;
    }
}

// Hears the session on whatever thread brought the news and leaves the
// event for the main thread, where the panel reads the queue (UiNotifier
// runs its function there, coalescing bursts).
class SessionListener final : public cppu::WeakImplHelper<XDebugSessionListener>
{
public:
    SessionListener(std::shared_ptr<DebugPanel::EventQueue> pQueue,
                    std::shared_ptr<::code::UiNotifier> xNotifier)
        : m_pQueue(std::move(pQueue))
        , m_xNotifier(std::move(xNotifier))
    {
    }

    void SAL_CALL debugEvent(const DebugEvent& rEvent) override
    {
        {
            std::scoped_lock aGuard(m_pQueue->aMutex);
            m_pQueue->aEvents.push_back(rEvent);
        }
        m_xNotifier->Notify();
    }
    // the session went away; the panel hears EXITED separately
    void SAL_CALL disposing(const lang::EventObject&) override {}

private:
    std::shared_ptr<DebugPanel::EventQueue> m_pQueue;
    std::shared_ptr<::code::UiNotifier> m_xNotifier;
};

// Hears the debug manager's launch requests -- on whatever thread asked, or
// on the subscribing one for a request that was pending -- and leaves them
// for the main thread, as SessionListener does with the session's events.
class ManagerListener final : public cppu::WeakImplHelper<XDebugManagerListener>
{
public:
    ManagerListener(std::shared_ptr<DebugPanel::EventQueue> pQueue,
                    std::shared_ptr<::code::UiNotifier> xNotifier)
        : m_pQueue(std::move(pQueue))
        , m_xNotifier(std::move(xNotifier))
    {
    }

    void SAL_CALL launchRequested(const LaunchDescriptor& rDescriptor) override
    {
        {
            std::scoped_lock aGuard(m_pQueue->aMutex);
            m_pQueue->aLaunches.push_back(rDescriptor);
        }
        m_xNotifier->Notify();
    }
    void SAL_CALL disposing(const lang::EventObject&) override {}

private:
    std::shared_ptr<DebugPanel::EventQueue> m_pQueue;
    std::shared_ptr<::code::UiNotifier> m_xNotifier;
};
}

struct DebugPanel::Outcome
{
    std::shared_ptr<bool> pAlive;
    DebugPanel* pSelf = nullptr;
    OUString aName;
    bool bOk = true;
    OUString aMessage;
    OUString aDetail;
    // what the operation fetched, shown on the main thread
    std::function<void(DebugPanel&)> aApply;
};

DebugPanel::DebugPanel(weld::Widget* pParent)
    : PanelLayout(pParent, u"DebugPanel"_ustr, u"code/ui/debugpanel.ui"_ustr)
    , mxToolbar(m_xBuilder->weld_toolbar(u"toolbar"_ustr))
    , mxBreakpoints(m_xBuilder->weld_tree_view(u"breakpoints"_ustr))
    , mxBkpEntry(m_xBuilder->weld_entry(u"bkpentry"_ustr))
    , mxBkpAdd(m_xBuilder->weld_button(u"bkpadd"_ustr))
    , mxThreads(m_xBuilder->weld_combo_box(u"threads"_ustr))
    , mxStack(m_xBuilder->weld_tree_view(u"stack"_ustr))
    , mxVariables(m_xBuilder->weld_tree_view(u"variables"_ustr))
    , mxConsole(m_xBuilder->weld_text_view(u"console"_ustr))
    , mxConsoleEntry(m_xBuilder->weld_entry(u"consoleentry"_ustr))
    , mxStatus(m_xBuilder->weld_label(u"status"_ustr))
{
    // the toggle in front of every breakpoint row: enabled or not
    mxBreakpoints->enable_toggle_buttons();
    mxBreakpoints->set_toggle_button_type(weld::ColumnToggleType::Check);
    mxBreakpoints->connect_toggled(LINK(this, DebugPanel, BkpToggledHdl));
    mxBreakpoints->connect_item_activated(LINK(this, DebugPanel, BkpActivatedHdl));
    mxBreakpoints->connect_command(LINK(this, DebugPanel, BkpCommandHdl));
    mxBkpAdd->connect_clicked(LINK(this, DebugPanel, BkpAddHdl));
    mxBkpEntry->connect_activate(LINK(this, DebugPanel, BkpEntryActivateHdl));
    mxThreads->connect_changed(LINK(this, DebugPanel, ThreadChangedHdl));
    mxStack->connect_selection_changed(LINK(this, DebugPanel, FrameSelectedHdl));
    mxStack->connect_item_activated(LINK(this, DebugPanel, FrameActivatedHdl));
    mxVariables->connect_expanding(LINK(this, DebugPanel, VarExpandingHdl));
    mxToolbar->connect_clicked(LINK(this, DebugPanel, ToolbarClickHdl));
    mxConsoleEntry->connect_activate(LINK(this, DebugPanel, ConsoleActivateHdl));

    // the session's events arrive on its reader thread; the widgets are only
    // touched here, on the main thread, once per burst
    mpEvents = std::make_shared<EventQueue>();
    mxNotifier = std::make_shared<::code::UiNotifier>([this] { ProcessEvents(); });
    mxListener = new SessionListener(mpEvents, mxNotifier);
    mxManagerListener = new ManagerListener(mpEvents, mxNotifier);

    UpdateControls();

    // Last, with the widgets built: a request that is pending is delivered
    // from inside this call, on this thread.  It goes through the queue like
    // any other, so it is acted on from the main loop once the panel exists.
    try
    {
        mxManager = theDebugManager::get(Context());
        mxManager->addDebugManagerListener(mxManagerListener);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot listen to the debug manager: " << rException.Message);
        mxManager.clear();
    }
}

DebugPanel::~DebugPanel()
{
    *mpAlive = false; // an answer on its way finds nobody to tell
    mxNotifier->Detach();
    if (mxManager.is())
    {
        try
        {
            mxManager->removeDebugManagerListener(mxManagerListener);
        }
        catch (const uno::Exception&)
        {
        }
    }
    if (uno::Reference<XDebugSession> xSession = std::move(mxSession))
    {
        // The sidebar drops the panel when its deck closes, a running
        // session or not: the tool must not be left behind.  terminate()
        // can take a moment, so it happens on a thread of its own.
        auto xListener = mxListener;
        std::thread(
            [xSession, xListener]
            {
                try
                {
                    if (xListener.is())
                        xSession->removeListener(xListener);
                    xSession->terminate();
                }
                catch (const uno::Exception&)
                {
                }
            })
            .detach();
    }
}

// ---- the session's events --------------------------------------------------

void DebugPanel::ProcessEvents()
{
    std::deque<DebugEvent> aEvents;
    std::deque<LaunchDescriptor> aLaunches;
    {
        std::scoped_lock aGuard(mpEvents->aMutex);
        aEvents.swap(mpEvents->aEvents);
        aLaunches.swap(mpEvents->aLaunches);
    }
    for (const DebugEvent& rEvent : aEvents)
        HandleEvent(rEvent);
    // after the events: a session that ended in this burst is not "live" any more
    for (const LaunchDescriptor& rDescriptor : aLaunches)
        StartDebugging(rDescriptor);
}

// Called with whatever the queue held -- which may include events from
// before the launch call returned the session to the panel (the listener is
// on from inside the launch): the handlers that need the session check for
// it themselves, the rest (output, state) are safe without one.
void DebugPanel::HandleEvent(const DebugEvent& rEvent)
{
    meState = rEvent.State;
    switch (rEvent.Kind)
    {
        case DebugEventKind_STATE_CHANGED:
            if (meState == DebugSessionState_RUNNING)
            {
                // the frame and variable handles died when it ran on
                maFrames.clear();
                mnCurrentFrame = 0;
                mxStack->clear();
                ShowVariables({});
            }
            else if (meState == DebugSessionState_STOPPED)
            {
                mnStoppedThread = rEvent.ThreadId;
                RefreshStopped();
            }
            else if (meState == DebugSessionState_EXITED)
                OnExited();
            break;
        case DebugEventKind_OUTPUT:
            AppendConsole(rEvent.Text);
            break;
        case DebugEventKind_BREAKPOINT_CHANGED:
            RefreshBreakpoints();
            break;
        case DebugEventKind_THREADS_CHANGED:
        default:
            // the next stop reads the threads again anyway
            break;
    }
    UpdateControls();
}

void DebugPanel::OnExited()
{
    maThreads.clear();
    maFrames.clear();
    mnCurrentFrame = 0;
    mnStoppedThread = 0;
    mxThreads->clear();
    mxStack->clear();
    ShowVariables({});
    // the breakpoints stay in the panel, for the next session to take
    for (PanelBreakpoint& rBreakpoint : maBreakpoints)
    {
        rBreakpoint.nSessionId = 0;
        rBreakpoint.bVerified = false;
    }
    FillBreakpoints();
    if (uno::Reference<XDebugSession> xSession = std::move(mxSession))
    {
        // the tool must be ended whatever happened; nobody needs to wait
        auto xListener = mxListener;
        std::thread(
            [xSession, xListener]
            {
                try
                {
                    if (xListener.is())
                        xSession->removeListener(xListener);
                    xSession->terminate();
                }
                catch (const uno::Exception&)
                {
                }
            })
            .detach();
    }
}

// ---- running things off the main thread ------------------------------------

void DebugPanel::SetBusy(bool bBusy, const OUString& rMessage)
{
    mbBusy = bBusy;
    mxStatus->set_label(rMessage);
    UpdateControls();
}

// The operation takes as long as the tool does; it runs on a thread, and
// what it ends with is handled on the main one.
void DebugPanel::RunAsync(const OUString& rName, std::function<void(Outcome&)> aOperation)
{
    if (mbBusy)
        return;
    SetBusy(true, rName + "...");
    std::thread(
        [pAlive = mpAlive, pSelf = this, rName, aOperation = std::move(aOperation)]
        {
            auto* pOutcome = new Outcome;
            pOutcome->pAlive = pAlive;
            pOutcome->pSelf = pSelf;
            pOutcome->aName = rName;
            try
            {
                aOperation(*pOutcome);
            }
            catch (const DebugException& rException)
            {
                pOutcome->bOk = false;
                pOutcome->aMessage = rException.Message;
                pOutcome->aDetail = rException.Detail;
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
            Application::PostUserEvent(LINK(nullptr, DebugPanel, DoneHdl), pOutcome);
        })
        .detach();
}

IMPL_STATIC_LINK(DebugPanel, DoneHdl, void*, pData, void)
{
    std::unique_ptr<DebugPanel::Outcome> xOutcome(static_cast<DebugPanel::Outcome*>(pData));
    if (!*xOutcome->pAlive)
        return; // the panel is gone, and so is the place to tell
    DebugPanel& rSelf = *xOutcome->pSelf;
    rSelf.SetBusy(false, OUString());
    if (!xOutcome->bOk)
        rSelf.ReportFailure(xOutcome->aName, xOutcome->aMessage, xOutcome->aDetail);
    else if (xOutcome->aApply)
        xOutcome->aApply(rSelf);
    rSelf.UpdateControls();
    // a stop that arrived while something else was running
    if (rSelf.mbStopRefreshWanted)
    {
        rSelf.mbStopRefreshWanted = false;
        if (rSelf.HasSession() && rSelf.meState == DebugSessionState_STOPPED)
            rSelf.RefreshStopped();
    }
}

void DebugPanel::ReportFailure(const OUString& rWhat, const OUString& rMessage,
                               const OUString& rDetail)
{
    std::unique_ptr<weld::MessageDialog> xBox(
        Application::CreateMessageDialog(mxBreakpoints.get(), VclMessageType::Warning,
                                         VclButtonsType::Ok, rWhat + " failed: " + rMessage));
    if (!rDetail.trim().isEmpty() && rDetail.trim() != rMessage.trim())
        xBox->set_secondary_text(rDetail.trim());
    xBox->run();
}

// ---- the toolbar -----------------------------------------------------------

void DebugPanel::UpdateControls()
{
    const bool bSession = HasSession();
    const bool bStarting = bSession && meState == DebugSessionState_STARTING;
    const bool bRunning = bSession && meState == DebugSessionState_RUNNING;
    const bool bStopped = bSession && meState == DebugSessionState_STOPPED;

    mxToolbar->set_item_sensitive(u"start"_ustr, !mbBusy && (!bSession || bStarting || bStopped));
    mxToolbar->set_item_sensitive(u"pause"_ustr, !mbBusy && bRunning);
    for (const OUString& rStep : { u"stepover"_ustr, u"stepinto"_ustr, u"stepout"_ustr })
        mxToolbar->set_item_sensitive(rStep, !mbBusy && bStopped);
    mxToolbar->set_item_sensitive(u"stop"_ustr, !mbBusy && bSession);
    const OUString aStart = bSession ? u"Continue"_ustr : u"Start Debugging"_ustr;
    mxToolbar->set_item_label(u"start"_ustr, aStart);
    mxToolbar->set_item_tooltip_text(u"start"_ustr, aStart);

    mxBreakpoints->set_sensitive(!mbBusy);
    mxBkpEntry->set_sensitive(!mbBusy);
    mxBkpAdd->set_sensitive(!mbBusy);
    mxThreads->set_sensitive(!mbBusy && bStopped);
    mxStack->set_sensitive(!mbBusy && bStopped);
    mxVariables->set_sensitive(!mbBusy && bStopped);
    mxConsoleEntry->set_sensitive(!mbBusy && bSession);
}

IMPL_LINK(DebugPanel, ToolbarClickHdl, const OUString&, rIdent, void)
{
    if (rIdent == "start")
        StartOrContinue();
    else if (rIdent == "pause")
        RunSessionCommand(u"Pause"_ustr,
                          [](const uno::Reference<XDebugSession>& xSession, sal_Int32)
                          { xSession->pause(0); }); // the whole program
    else if (rIdent == "stepover")
        RunSessionCommand(u"Step Over"_ustr,
                          [](const uno::Reference<XDebugSession>& xSession, sal_Int32 nThread)
                          { xSession->stepOver(nThread); });
    else if (rIdent == "stepinto")
        RunSessionCommand(u"Step Into"_ustr,
                          [](const uno::Reference<XDebugSession>& xSession, sal_Int32 nThread)
                          { xSession->stepInto(nThread); });
    else if (rIdent == "stepout")
        RunSessionCommand(u"Step Out"_ustr,
                          [](const uno::Reference<XDebugSession>& xSession, sal_Int32 nThread)
                          { xSession->stepOut(nThread); });
    else if (rIdent == "stop")
        RunSessionCommand(u"Stop Debugging"_ustr,
                          [](const uno::Reference<XDebugSession>& xSession, sal_Int32)
                          { xSession->terminate(); });
}

void DebugPanel::RunSessionCommand(
    const OUString& rName,
    std::function<void(const css::uno::Reference<css::code::debug::XDebugSession>&, sal_Int32)>
        aCall)
{
    if (!HasSession())
        return;
    auto xSession = mxSession;
    const sal_Int32 nThread = SelectedThread();
    RunAsync(rName, [xSession, nThread, aCall = std::move(aCall)](Outcome&)
             { aCall(xSession, nThread); });
}

void DebugPanel::StartOrContinue()
{
    if (!HasSession())
    {
        // the startup target is the panel's own to resolve here; building it
        // first is the toolbar flow's business, not this button's
        LaunchDescriptor aDescriptor;
        if (!ResolveStartupDescriptor(aDescriptor))
        {
            ShowMessage(VclMessageType::Warning, VclButtonsType::Ok,
                        u"No startup target to run."_ustr);
            return;
        }
        StartDebugging(aDescriptor);
        return;
    }
    RunSessionCommand(u"Continue"_ustr,
                      [](const uno::Reference<XDebugSession>& xSession, sal_Int32)
                      { xSession->resume(0); });
}

bool DebugPanel::ShowMessage(VclMessageType eType, VclButtonsType eButtons, const OUString& rText)
{
    std::unique_ptr<weld::MessageDialog> xBox(
        Application::CreateMessageDialog(mxBreakpoints.get(), eType, eButtons, rText));
    return xBox->run() == RET_YES;
}

// The descriptor is authoritative: nothing here remembers it.  A running
// session is replaced only when the user agrees.
void DebugPanel::StartDebugging(const LaunchDescriptor& rDescriptor)
{
    if (rDescriptor.Program.isEmpty())
    {
        ShowMessage(VclMessageType::Warning, VclButtonsType::Ok,
                    u"No startup target to run."_ustr);
        return;
    }
    if (mbBusy)
    {
        mxStatus->set_label(u"Busy; start again in a moment."_ustr);
        return;
    }

    uno::Reference<XDebugSession> xOld;
    if (HasSession())
    {
        if (!ShowMessage(VclMessageType::Question, VclButtonsType::YesNo,
                         u"Stop the running session and start again?"_ustr))
            return;
        // the dialog ran the main loop: the session may have ended meanwhile
        // (OnExited took it), and the panel may be busy now
        if (mbBusy)
            return;
        xOld = mxSession;
        // the panel shows no session from here on; the old one is ended by
        // the worker, below
        mxSession.clear();
        OnExited();
        mxSession.clear();
        meState = DebugSessionState_EXITED;
    }

    // breakpoints set before the launch are applied right after it, while
    // the session is still STARTING -- the backend keeps them per session
    const std::vector<PanelBreakpoint> aRequests = maBreakpoints;
    auto xListener = mxListener;
    auto xContext = Context();
    auto pEvents = mpEvents;
    const LaunchDescriptor aDescriptor = rDescriptor;

    RunAsync(
        u"Start Debugging"_ustr,
        [xContext, xListener, aDescriptor, aRequests, xOld, pEvents](Outcome& rOutcome)
        {
            if (xOld.is())
            {
                try
                {
                    xOld->removeListener(xListener);
                    xOld->terminate();
                }
                catch (const uno::Exception&)
                {
                    // ended already, or never mind: the new one starts anyway
                }
                // what the old session said before it went must not end the new one
                std::scoped_lock aGuard(pEvents->aMutex);
                pEvents->aEvents.clear();
            }
            auto xManager = theDebugManager::get(xContext);
            uno::Reference<XDebugAdapter> xAdapter = xManager->findAdapter(aDescriptor);
            if (!xAdapter.is())
                throw uno::RuntimeException("no debugger can run " + aDescriptor.Program
                                            + " (is the tool installed, and the program there?)");
            uno::Reference<XDebugSession> xSession = xAdapter->launch(aDescriptor);
            // from here the events flow into the panel's queue
            xSession->addListener(xListener);

            std::vector<std::pair<sal_Int32, Breakpoint>> aApplied; // serial -> as taken
            OUStringBuffer aTrouble;
            for (const PanelBreakpoint& rRequest : aRequests)
            {
                Breakpoint aAsk;
                aAsk.Url = rRequest.aUrl;
                aAsk.Line = rRequest.nLine;
                aAsk.Condition = rRequest.aCondition;
                aAsk.Enabled = rRequest.bEnabled;
                try
                {
                    aApplied.emplace_back(rRequest.nSerial, xSession->addBreakpoint(aAsk));
                }
                catch (const uno::Exception& rException)
                {
                    aTrouble.append("cannot set " + DisplayPlace(rRequest.aUrl, rRequest.nLine)
                                    + ": " + rException.Message + "\n");
                }
            }
            // the first resume starts the program
            xSession->resume(0);

            const OUString aMessages = aTrouble.makeStringAndClear();
            rOutcome.aApply = [xSession, aApplied, aMessages](DebugPanel& rSelf)
            {
                rSelf.mxSession = xSession;
                rSelf.meState = xSession->getState();
                for (const auto& rPair : aApplied)
                    if (PanelBreakpoint* pBreakpoint = rSelf.BreakpointOf(rPair.first))
                    {
                        pBreakpoint->nSessionId = rPair.second.Id;
                        pBreakpoint->nLine = rPair.second.Line;
                        pBreakpoint->bVerified = rPair.second.Verified;
                        if (!rPair.second.Url.isEmpty())
                            pBreakpoint->aUrl = rPair.second.Url;
                    }
                rSelf.FillBreakpoints();
                if (!aMessages.isEmpty())
                    rSelf.AppendConsole(aMessages);
                // a stop that happened before the panel held the session
                if (rSelf.meState == DebugSessionState_STOPPED)
                    rSelf.RefreshStopped();
            };
        });
}

// ---- looking around while stopped ------------------------------------------

sal_Int32 DebugPanel::SelectedThread() const
{
    const int nActive = mxThreads->get_active();
    if (nActive >= 0 && o3tl::make_unsigned(nActive) < maThreads.size())
        return maThreads[nActive].Id;
    return mnStoppedThread;
}

void DebugPanel::RefreshStopped()
{
    if (!HasSession())
        return;
    if (mbBusy)
    {
        mbStopRefreshWanted = true; // what runs now may show an old stop
        return;
    }
    auto xSession = mxSession;
    const sal_Int32 nPreferred = mnStoppedThread;
    RunAsync(
        u"Refresh"_ustr,
        [xSession, nPreferred](Outcome& rOutcome)
        {
            const auto aThreadsSeq = xSession->getThreads();
            std::vector<DebugThread> aThreads(aThreadsSeq.begin(), aThreadsSeq.end());
            // the thread that stopped, when it is known; else any stopped one
            sal_Int32 nThread = 0;
            for (const DebugThread& rThread : aThreads)
                if (rThread.Id == nPreferred && rThread.Stopped)
                    nThread = rThread.Id;
            if (nThread == 0)
                for (const DebugThread& rThread : aThreads)
                    if (rThread.Stopped)
                    {
                        nThread = rThread.Id;
                        break;
                    }
            std::vector<StackFrame> aFrames;
            std::vector<Variable> aVariables;
            if (nThread > 0)
            {
                const auto aFramesSeq = xSession->getStackFrames(nThread);
                aFrames.assign(aFramesSeq.begin(), aFramesSeq.end());
                if (!aFrames.empty())
                {
                    const auto aVariablesSeq = xSession->getVariables(aFrames[0].Id, 0);
                    aVariables.assign(aVariablesSeq.begin(), aVariablesSeq.end());
                }
            }
            rOutcome.aApply = [aThreads, nThread, aFrames, aVariables](DebugPanel& rSelf)
            {
                if (rSelf.meState != DebugSessionState_STOPPED)
                    return; // it ran on while this was read
                rSelf.ShowThreads(aThreads, nThread);
                rSelf.ShowFrames(aFrames);
                rSelf.mnCurrentFrame = aFrames.empty() ? 0 : aFrames[0].Id;
                rSelf.ShowVariables(aVariables);
            };
        });
}

void DebugPanel::RefreshFrames(sal_Int32 nThreadId)
{
    if (!HasSession() || nThreadId <= 0)
        return;
    auto xSession = mxSession;
    RunAsync(u"Refresh"_ustr,
             [xSession, nThreadId](Outcome& rOutcome)
             {
                 const auto aFramesSeq = xSession->getStackFrames(nThreadId);
                 std::vector<StackFrame> aFrames(aFramesSeq.begin(), aFramesSeq.end());
                 std::vector<Variable> aVariables;
                 if (!aFrames.empty())
                 {
                     const auto aVariablesSeq = xSession->getVariables(aFrames[0].Id, 0);
                     aVariables.assign(aVariablesSeq.begin(), aVariablesSeq.end());
                 }
                 rOutcome.aApply = [aFrames, aVariables](DebugPanel& rSelf)
                 {
                     if (rSelf.meState != DebugSessionState_STOPPED)
                         return;
                     rSelf.ShowFrames(aFrames);
                     rSelf.mnCurrentFrame = aFrames.empty() ? 0 : aFrames[0].Id;
                     rSelf.ShowVariables(aVariables);
                 };
             });
}

void DebugPanel::RefreshVariables(sal_Int32 nFrameId)
{
    if (!HasSession() || nFrameId == 0)
        return;
    auto xSession = mxSession;
    RunAsync(u"Refresh"_ustr,
             [xSession, nFrameId](Outcome& rOutcome)
             {
                 const auto aVariablesSeq = xSession->getVariables(nFrameId, 0);
                 std::vector<Variable> aVariables(aVariablesSeq.begin(), aVariablesSeq.end());
                 rOutcome.aApply = [aVariables](DebugPanel& rSelf)
                 {
                     if (rSelf.meState != DebugSessionState_STOPPED)
                         return;
                     rSelf.ShowVariables(aVariables);
                 };
             });
}

void DebugPanel::ShowThreads(const std::vector<DebugThread>& rThreads, sal_Int32 nShownThread)
{
    mbFilling = true;
    maThreads = rThreads;
    mxThreads->clear();
    int nActive = -1;
    for (size_t i = 0; i < rThreads.size(); ++i)
    {
        mxThreads->append_text(OUString::number(rThreads[i].Id) + ": " + rThreads[i].Name);
        if (rThreads[i].Id == nShownThread)
            nActive = static_cast<int>(i);
    }
    if (nActive >= 0)
        mxThreads->set_active(nActive);
    mbFilling = false;
}

void DebugPanel::ShowFrames(const std::vector<StackFrame>& rFrames)
{
    mbFilling = true;
    maFrames = rFrames;
    mxStack->freeze();
    mxStack->clear();
    for (const StackFrame& rFrame : rFrames)
    {
        const OUString aId = OUString::number(rFrame.Id);
        OUString aFunction = rFrame.FunctionName;
        if (!rFrame.ModuleName.isEmpty())
            aFunction += " [" + rFrame.ModuleName + "]";
        std::unique_ptr<weld::TreeIter> xRow = mxStack->make_iterator();
        mxStack->insert(nullptr, -1, &aFunction, &aId, nullptr, nullptr, false, xRow.get());
        if (!rFrame.Url.isEmpty())
            mxStack->set_text(*xRow, DisplayPlace(rFrame.Url, rFrame.Line), STACK_COL_PLACE);
    }
    mxStack->thaw();
    if (!rFrames.empty())
        mxStack->select(0); // the innermost frame: where it stopped
    mbFilling = false;
}

void DebugPanel::ShowVariables(const std::vector<Variable>& rVariables)
{
    maVarRefs.clear();
    maVarLoaded.clear();
    maVarPending.clear();
    mxVariables->freeze();
    mxVariables->clear();
    mxVariables->thaw();
    if (!rVariables.empty())
        InsertVariableRows(nullptr, rVariables);
}

void DebugPanel::InsertVariableRows(const weld::TreeIter* pParent,
                                    const std::vector<Variable>& rVariables)
{
    mxVariables->freeze();
    for (const Variable& rVariable : rVariables)
    {
        const OUString aId = "v:" + OUString::number(++mnNextVarSerial);
        maVarRefs[aId] = rVariable.Reference;
        std::unique_ptr<weld::TreeIter> xRow = mxVariables->make_iterator();
        // children on demand: the expander shows, and the children are
        // read from the session only when it is clicked
        mxVariables->insert(pParent, -1, &rVariable.Name, &aId, nullptr, nullptr,
                            rVariable.HasChildren && rVariable.Reference != 0, xRow.get());
        mxVariables->set_text(*xRow, rVariable.Value, VAR_COL_VALUE);
        mxVariables->set_text(*xRow, rVariable.TypeName, VAR_COL_TYPE);
    }
    mxVariables->thaw();
}

IMPL_LINK(DebugPanel, VarExpandingHdl, const weld::TreeIter&, rIter, bool)
{
    const OUString aId = mxVariables->get_id(rIter);
    if (maVarLoaded.count(aId))
        return true; // filled before; just open
    auto it = maVarRefs.find(aId);
    if (it == maVarRefs.end() || it->second == 0 || !HasSession()
        || meState != DebugSessionState_STOPPED || mbBusy)
        return false;
    maVarPending[aId] = mxVariables->make_iterator(&rIter);
    auto xSession = mxSession;
    const sal_Int32 nReference = it->second;
    RunAsync(u"Expand"_ustr,
             [xSession, nReference, aId](Outcome& rOutcome)
             {
                 const auto aChildrenSeq = xSession->getVariables(0, nReference);
                 std::vector<Variable> aChildren(aChildrenSeq.begin(), aChildrenSeq.end());
                 rOutcome.aApply = [aId, aChildren](DebugPanel& rSelf)
                 {
                     auto itPending = rSelf.maVarPending.find(aId);
                     if (itPending == rSelf.maVarPending.end())
                         return; // the tree was rebuilt meanwhile
                     std::unique_ptr<weld::TreeIter> xParent = std::move(itPending->second);
                     rSelf.maVarPending.erase(itPending);
                     rSelf.maVarLoaded.insert(aId);
                     rSelf.InsertVariableRows(xParent.get(), aChildren);
                     rSelf.mxVariables->expand_row(*xParent);
                 };
             });
    return false; // opened once the children have arrived
}

IMPL_LINK_NOARG(DebugPanel, ThreadChangedHdl, weld::ComboBox&, void)
{
    if (mbFilling)
        return;
    RefreshFrames(SelectedThread());
}

IMPL_LINK_NOARG(DebugPanel, FrameSelectedHdl, weld::ItemView&, void)
{
    if (mbFilling)
        return;
    const int nRow = mxStack->get_selected_index();
    if (nRow < 0 || o3tl::make_unsigned(nRow) >= maFrames.size())
        return;
    const sal_Int32 nFrame = maFrames[nRow].Id;
    if (nFrame == mnCurrentFrame)
        return;
    mnCurrentFrame = nFrame;
    RefreshVariables(nFrame);
}

IMPL_LINK(DebugPanel, FrameActivatedHdl, const weld::TreeIter&, rIter, bool)
{
    const sal_Int32 nFrame = mxStack->get_id(rIter).toInt32();
    auto it = std::find_if(maFrames.begin(), maFrames.end(),
                           [&](const StackFrame& r) { return r.Id == nFrame; });
    if (it == maFrames.end())
        return false;
    Reveal(it->Url, it->Line);
    return true;
}

// ---- breakpoints -----------------------------------------------------------

DebugPanel::PanelBreakpoint* DebugPanel::BreakpointOf(sal_Int32 nSerial)
{
    auto it = std::find_if(maBreakpoints.begin(), maBreakpoints.end(),
                           [&](const PanelBreakpoint& r) { return r.nSerial == nSerial; });
    return it == maBreakpoints.end() ? nullptr : &*it;
}

void DebugPanel::FillBreakpoints()
{
    mbFilling = true;
    mxBreakpoints->freeze();
    mxBreakpoints->clear();
    for (const PanelBreakpoint& rBreakpoint : maBreakpoints)
    {
        const OUString aId = "b:" + OUString::number(rBreakpoint.nSerial);
        OUString aPlace = DisplayPlace(rBreakpoint.aUrl, rBreakpoint.nLine);
        // a place the running code does not have (yet) -- say so
        if (rBreakpoint.nSessionId != 0 && !rBreakpoint.bVerified)
            aPlace += " (pending)";
        std::unique_ptr<weld::TreeIter> xRow = mxBreakpoints->make_iterator();
        mxBreakpoints->insert(nullptr, -1, &aPlace, &aId, nullptr, nullptr, false, xRow.get());
        mxBreakpoints->set_toggle(*xRow, rBreakpoint.bEnabled ? TRISTATE_TRUE : TRISTATE_FALSE);
        if (!rBreakpoint.aCondition.isEmpty())
            mxBreakpoints->set_text(*xRow, rBreakpoint.aCondition, BKP_COL_CONDITION);
    }
    mxBreakpoints->thaw();
    mbFilling = false;
}

void DebugPanel::AddBreakpointFromEntry()
{
    OUString aFile, aCondition;
    sal_Int32 nLine = 0; // one-based, as typed
    if (!ParseBreakpointEntry(mxBkpEntry->get_text(), aFile, nLine, aCondition))
    {
        mxStatus->set_label(u"Give a place as file:line, or a line in the open file."_ustr);
        return;
    }
    if (aFile.isEmpty())
    {
        // the open document; the editor tells no caret over UNO, so only
        // the file comes from it, the line from the entry
        try
        {
            if (auto xHost = EditorHost())
                aFile = xHost->getCurrentDocumentUrl();
        }
        catch (const uno::Exception&)
        {
        }
        if (aFile.isEmpty())
        {
            mxStatus->set_label(u"No file is open; name one as file:line."_ustr);
            return;
        }
    }
    else if (aFile.startsWith("/"))
    {
        // an absolute path becomes a URL, so that Go to Source works; a
        // bare name stays as typed and is the debugger's to resolve
        OUString aUrl;
        if (osl::FileBase::getFileURLFromSystemPath(aFile, aUrl) == osl::FileBase::E_None)
            aFile = aUrl;
    }

    PanelBreakpoint aBreakpoint;
    aBreakpoint.nSerial = ++mnNextBreakpointSerial;
    aBreakpoint.aUrl = aFile;
    aBreakpoint.nLine = nLine - 1; // the debug API counts from zero
    aBreakpoint.aCondition = aCondition;
    maBreakpoints.push_back(aBreakpoint);
    mxBkpEntry->set_text(OUString());
    mxStatus->set_label(OUString());
    FillBreakpoints();

    if (!HasSession())
        return; // kept here, applied right after the next launch
    auto xSession = mxSession;
    const sal_Int32 nSerial = aBreakpoint.nSerial;
    Breakpoint aAsk;
    aAsk.Url = aBreakpoint.aUrl;
    aAsk.Line = aBreakpoint.nLine;
    aAsk.Condition = aBreakpoint.aCondition;
    aAsk.Enabled = true;
    RunAsync(u"Set Breakpoint"_ustr,
             [xSession, aAsk, nSerial](Outcome& rOutcome)
             {
                 const Breakpoint aTaken = xSession->addBreakpoint(aAsk);
                 rOutcome.aApply = [aTaken, nSerial](DebugPanel& rSelf)
                 {
                     if (PanelBreakpoint* pBreakpoint = rSelf.BreakpointOf(nSerial))
                     {
                         pBreakpoint->nSessionId = aTaken.Id;
                         pBreakpoint->nLine = aTaken.Line;
                         pBreakpoint->bVerified = aTaken.Verified;
                         if (!aTaken.Url.isEmpty())
                             pBreakpoint->aUrl = aTaken.Url;
                         rSelf.FillBreakpoints();
                     }
                 };
             });
}

void DebugPanel::RemoveBreakpoint(sal_Int32 nSerial)
{
    PanelBreakpoint* pBreakpoint = BreakpointOf(nSerial);
    if (!pBreakpoint)
        return;
    const sal_Int32 nSessionId = pBreakpoint->nSessionId;
    auto erase = [nSerial](DebugPanel& rSelf)
    {
        std::erase_if(rSelf.maBreakpoints,
                      [&](const PanelBreakpoint& r) { return r.nSerial == nSerial; });
        rSelf.FillBreakpoints();
    };
    if (!HasSession() || nSessionId == 0)
    {
        erase(*this);
        return;
    }
    auto xSession = mxSession;
    RunAsync(u"Remove Breakpoint"_ustr,
             [xSession, nSessionId, erase](Outcome& rOutcome)
             {
                 xSession->removeBreakpoint(nSessionId);
                 rOutcome.aApply = erase;
             });
}

void DebugPanel::RemoveAllBreakpoints()
{
    if (maBreakpoints.empty())
        return;
    auto clear = [](DebugPanel& rSelf)
    {
        rSelf.maBreakpoints.clear();
        rSelf.FillBreakpoints();
    };
    if (!HasSession())
    {
        clear(*this);
        return;
    }
    std::vector<sal_Int32> aIds;
    for (const PanelBreakpoint& rBreakpoint : maBreakpoints)
        if (rBreakpoint.nSessionId != 0)
            aIds.push_back(rBreakpoint.nSessionId);
    auto xSession = mxSession;
    RunAsync(u"Remove All Breakpoints"_ustr,
             [xSession, aIds, clear](Outcome& rOutcome)
             {
                 for (sal_Int32 nId : aIds)
                     xSession->removeBreakpoint(nId);
                 rOutcome.aApply = clear;
             });
}

void DebugPanel::ToggleBreakpoint(sal_Int32 nSerial)
{
    PanelBreakpoint* pBreakpoint = BreakpointOf(nSerial);
    if (!pBreakpoint)
        return;
    const bool bEnabled = !pBreakpoint->bEnabled;
    if (!HasSession() || pBreakpoint->nSessionId == 0)
    {
        pBreakpoint->bEnabled = bEnabled;
        FillBreakpoints();
        return;
    }
    // the session API has no enable call: the breakpoint is set anew
    auto xSession = mxSession;
    const sal_Int32 nSessionId = pBreakpoint->nSessionId;
    Breakpoint aAsk;
    aAsk.Url = pBreakpoint->aUrl;
    aAsk.Line = pBreakpoint->nLine;
    aAsk.Condition = pBreakpoint->aCondition;
    aAsk.Enabled = bEnabled;
    RunAsync(u"Toggle Breakpoint"_ustr,
             [xSession, nSessionId, aAsk, nSerial](Outcome& rOutcome)
             {
                 xSession->removeBreakpoint(nSessionId);
                 const Breakpoint aTaken = xSession->addBreakpoint(aAsk);
                 rOutcome.aApply = [aTaken, nSerial](DebugPanel& rSelf)
                 {
                     if (PanelBreakpoint* pRow = rSelf.BreakpointOf(nSerial))
                     {
                         pRow->nSessionId = aTaken.Id;
                         pRow->bEnabled = aTaken.Enabled;
                         pRow->bVerified = aTaken.Verified;
                         rSelf.FillBreakpoints();
                     }
                 };
             });
}

// The session's own word on the breakpoints: moved lines, resolved places,
// ones made or dropped behind the panel's back (a console command, say).
void DebugPanel::RefreshBreakpoints()
{
    if (!HasSession() || mbBusy)
        return;
    auto xSession = mxSession;
    RunAsync(
        u"Refresh"_ustr,
        [xSession](Outcome& rOutcome)
        {
            const auto aSeq = xSession->getBreakpoints();
            std::vector<Breakpoint> aTheirs(aSeq.begin(), aSeq.end());
            rOutcome.aApply = [aTheirs](DebugPanel& rSelf)
            {
                // ours updated or dropped by theirs, theirs without ours added
                std::erase_if(rSelf.maBreakpoints,
                              [&](const PanelBreakpoint& rMine)
                              {
                                  return rMine.nSessionId != 0
                                         && std::none_of(aTheirs.begin(), aTheirs.end(),
                                                         [&](const Breakpoint& rTheir)
                                                         { return rTheir.Id == rMine.nSessionId; });
                              });
                for (const Breakpoint& rTheir : aTheirs)
                {
                    auto it = std::find_if(rSelf.maBreakpoints.begin(), rSelf.maBreakpoints.end(),
                                           [&](const PanelBreakpoint& r)
                                           { return r.nSessionId == rTheir.Id; });
                    if (it == rSelf.maBreakpoints.end())
                    {
                        PanelBreakpoint aNew;
                        aNew.nSerial = ++rSelf.mnNextBreakpointSerial;
                        aNew.nSessionId = rTheir.Id;
                        it = rSelf.maBreakpoints.insert(rSelf.maBreakpoints.end(), aNew);
                    }
                    it->nLine = rTheir.Line;
                    it->aCondition = rTheir.Condition;
                    it->bEnabled = rTheir.Enabled;
                    it->bVerified = rTheir.Verified;
                    if (!rTheir.Url.isEmpty())
                        it->aUrl = rTheir.Url;
                }
                rSelf.FillBreakpoints();
            };
        });
}

IMPL_LINK_NOARG(DebugPanel, BkpAddHdl, weld::Button&, void) { AddBreakpointFromEntry(); }

IMPL_LINK_NOARG(DebugPanel, BkpEntryActivateHdl, weld::Entry&, bool)
{
    AddBreakpointFromEntry();
    return true;
}

IMPL_LINK(DebugPanel, BkpToggledHdl, const weld::TreeView::iter_col&, rIterCol, void)
{
    if (mbFilling)
        return;
    const OUString aId = mxBreakpoints->get_id(rIterCol.first);
    if (aId.startsWith("b:"))
        ToggleBreakpoint(aId.copy(2).toInt32());
}

IMPL_LINK(DebugPanel, BkpActivatedHdl, const weld::TreeIter&, rIter, bool)
{
    const OUString aId = mxBreakpoints->get_id(rIter);
    if (!aId.startsWith("b:"))
        return false;
    if (const PanelBreakpoint* pBreakpoint = BreakpointOf(aId.copy(2).toInt32()))
        Reveal(pBreakpoint->aUrl, pBreakpoint->nLine);
    return true;
}

IMPL_LINK(DebugPanel, BkpCommandHdl, const CommandEvent&, rEvent, bool)
{
    if (rEvent.GetCommand() != CommandEventId::ContextMenu || mbBusy)
        return false;
    std::unique_ptr<weld::TreeIter> xRow = rEvent.IsMouseEvent()
        ? mxBreakpoints->get_dest_row_at_pos(rEvent.GetMousePosPixel(), false, false)
        : mxBreakpoints->get_cursor();
    if (!xRow)
        return false;
    const OUString aId = mxBreakpoints->get_id(*xRow);
    if (!aId.startsWith("b:"))
        return false;
    const sal_Int32 nSerial = aId.copy(2).toInt32();
    mxBreakpoints->select(*xRow);
    const tools::Rectangle aArea = rEvent.IsMouseEvent()
                                       ? tools::Rectangle(rEvent.GetMousePosPixel(), Size(1, 1))
                                       : mxBreakpoints->get_row_area(*xRow);

    // the menu lives in the panel's own .ui, as solutionpanel's does
    std::unique_ptr<weld::Builder> xBuilder(
        Application::CreateBuilder(mxBreakpoints.get(), u"code/ui/debugpanel.ui"_ustr));
    std::unique_ptr<weld::Menu> xMenu = xBuilder->weld_menu(u"breakpointmenu"_ustr);
    const OUString aCommand = xMenu->popup_at_rect(mxBreakpoints.get(), aArea);
    if (aCommand == "open")
    {
        if (const PanelBreakpoint* pBreakpoint = BreakpointOf(nSerial))
            Reveal(pBreakpoint->aUrl, pBreakpoint->nLine);
    }
    else if (aCommand == "remove")
        RemoveBreakpoint(nSerial);
    else if (aCommand == "removeall")
        RemoveAllBreakpoints();
    return true;
}

// ---- the console -----------------------------------------------------------

void DebugPanel::AppendConsole(const OUString& rText)
{
    if (rText.isEmpty())
        return;
    // insert at the end and follow it, as the Output panel's log does
    mxConsole->set_position(mnConsoleLength);
    mxConsole->replace_selection(rText);
    mnConsoleLength += rText.getLength();
    mxConsole->select_region(mnConsoleLength, mnConsoleLength);
}

void DebugPanel::SendConsoleInput()
{
    const OUString aText = mxConsoleEntry->get_text().trim();
    if (aText.isEmpty() || !HasSession() || mbBusy)
        return;
    auto xSession = mxSession;
    if (aText.startsWith("="))
    {
        // "=expression": evaluated in the selected frame
        const OUString aExpression = aText.copy(1).trim();
        const sal_Int32 nFrame = mnCurrentFrame;
        if (meState != DebugSessionState_STOPPED || nFrame == 0 || aExpression.isEmpty())
        {
            mxStatus->set_label(u"Stop the program first to evaluate an expression."_ustr);
            return;
        }
        mxConsoleEntry->set_text(OUString());
        AppendConsole("= " + aExpression + "\n");
        RunAsync(u"Evaluate"_ustr,
                 [xSession, nFrame, aExpression](Outcome& rOutcome)
                 {
                     const OUString aValue = xSession->evaluate(nFrame, aExpression);
                     rOutcome.aApply = [aValue](DebugPanel& rSelf)
                     { rSelf.AppendConsole(aValue + "\n"); };
                 });
        return;
    }
    // anything else is the tool's own command line -- a capability only
    // some sessions have
    uno::Reference<XDebugConsole> xConsole(xSession, uno::UNO_QUERY);
    if (!xConsole.is())
    {
        mxStatus->set_label(u"This debugger has no command line of its own."_ustr);
        return;
    }
    mxConsoleEntry->set_text(OUString());
    AppendConsole("> " + aText + "\n");
    RunAsync(u"Console"_ustr,
             [xConsole, aText](Outcome& rOutcome)
             {
                 const OUString aOutput = xConsole->executeCommand(aText);
                 rOutcome.aApply = [aOutput](DebugPanel& rSelf) { rSelf.AppendConsole(aOutput); };
             });
}

IMPL_LINK_NOARG(DebugPanel, ConsoleActivateHdl, weld::Entry&, bool)
{
    SendConsoleInput();
    return true;
}

// ---- opening places in the editor ------------------------------------------

void DebugPanel::Reveal(const OUString& rUrl, sal_Int32 nLine)
{
    if (rUrl.isEmpty() || !rUrl.startsWith("file:"))
        return; // a place the editor cannot open (a bare name gdb resolves)
    try
    {
        if (auto xHost = EditorHost())
        {
            // the debug API and core::Position both count lines from zero:
            // no translation here
            css::code::core::Position aPosition;
            aPosition.Line = nLine;
            aPosition.Column = 0;
            xHost->revealPosition(rUrl, aPosition);
        }
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot open " << rUrl << ": " << rException.Message);
    }
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
