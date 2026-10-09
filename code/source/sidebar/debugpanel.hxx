/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/debug/Breakpoint.hpp>
#include <com/sun/star/code/debug/DebugEvent.hpp>
#include <com/sun/star/code/debug/DebugSessionState.hpp>
#include <com/sun/star/code/debug/DebugThread.hpp>
#include <com/sun/star/code/debug/LaunchDescriptor.hpp>
#include <com/sun/star/code/debug/XDebugManager.hpp>
#include <com/sun/star/code/debug/XDebugManagerListener.hpp>
#include <com/sun/star/code/debug/StackFrame.hpp>
#include <com/sun/star/code/debug/Variable.hpp>
#include <com/sun/star/code/debug/XDebugSession.hpp>
#include <com/sun/star/code/debug/XDebugSessionListener.hpp>
#include <sfx2/sidebar/PanelLayout.hxx>
#include <vcl/weld/TreeView.hxx>

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

class CommandEvent;
enum class VclMessageType;
enum class VclButtonsType;
namespace weld
{
class Button;
class ComboBox;
class Entry;
class Label;
class TextView;
class Toolbar;
class TreeIter;
}

namespace code
{
class UiNotifier;
}

namespace code::sidebar
{
/** Debug: launching a program under a debugger of theDebugManager and
    looking at it -- breakpoints, the stopped thread's stack, the selected
    frame's variables, a console to the tool itself.  It knows only the
    com.sun.star.code.debug interfaces: what it shows and does is
    XDebugSession's, and the gdb underneath is the adapter's business.

    Everything the session is asked is asked off the main thread, as the
    tool can take long; the answers, and the session's own events, are
    shown on it.
*/
class DebugPanel final : public PanelLayout
{
public:
    explicit DebugPanel(weld::Widget* pParent);
    virtual ~DebugPanel() override;

    // what an operation that ran on a thread of its own comes back with
    struct Outcome;
    // the session's events, written by the listener on the reader's thread
    // and read on the main one
    struct EventQueue
    {
        std::mutex aMutex;
        std::deque<css::code::debug::DebugEvent> aEvents;
        // launch requests of the debug manager, to act on the main thread
        std::deque<css::code::debug::LaunchDescriptor> aLaunches;
    };

private:
    // a breakpoint as the panel keeps it: it exists here first, and in the
    // session only while one runs (nSessionId 0 otherwise)
    struct PanelBreakpoint
    {
        sal_Int32 nSerial = 0; // the row id, "b:" and this
        sal_Int32 nSessionId = 0;
        OUString aUrl; // a file URL, or whatever place the user typed
        sal_Int32 nLine = 0; // zero-based, as the debug API counts
        OUString aCondition;
        bool bEnabled = true;
        bool bVerified = false;
    };

    bool HasSession() const { return mxSession.is(); }
    void ProcessEvents();
    void HandleEvent(const css::code::debug::DebugEvent& rEvent);
    void OnExited();
    void UpdateControls();
    void SetBusy(bool bBusy, const OUString& rMessage);
    // aOperation runs on a thread of its own; what it put into the outcome's
    // apply-function runs on the main thread when it went well
    void RunAsync(const OUString& rName, std::function<void(Outcome&)> aOperation);
    void ReportFailure(const OUString& rWhat, const OUString& rMessage, const OUString& rDetail);

    void StartOrContinue();
    // Starts the program of rDescriptor under a debugger, asking first when a
    // session is live.  Main thread.
    void StartDebugging(const css::code::debug::LaunchDescriptor& rDescriptor);
    // true for the Yes/OK answer
    bool ShowMessage(VclMessageType eType, VclButtonsType eButtons, const OUString& rText);
    // rCall runs off the main thread, with the session and the thread the
    // user looks at
    void RunSessionCommand(
        const OUString& rName,
        std::function<void(const css::uno::Reference<css::code::debug::XDebugSession>&, sal_Int32)>
            rCall);
    void RefreshStopped();
    void RefreshFrames(sal_Int32 nThreadId);
    void RefreshVariables(sal_Int32 nFrameId);
    void RefreshBreakpoints();
    void ShowThreads(const std::vector<css::code::debug::DebugThread>& rThreads,
                     sal_Int32 nShownThread);
    void ShowFrames(const std::vector<css::code::debug::StackFrame>& rFrames);
    void ShowVariables(const std::vector<css::code::debug::Variable>& rVariables);
    void InsertVariableRows(const weld::TreeIter* pParent,
                            const std::vector<css::code::debug::Variable>& rVariables);
    void FillBreakpoints();
    void AddBreakpointFromEntry();
    void RemoveBreakpoint(sal_Int32 nSerial);
    void RemoveAllBreakpoints();
    void ToggleBreakpoint(sal_Int32 nSerial);
    void SendConsoleInput();
    void AppendConsole(const OUString& rText);
    void Reveal(const OUString& rUrl, sal_Int32 nLine);
    sal_Int32 SelectedThread() const;
    PanelBreakpoint* BreakpointOf(sal_Int32 nSerial);

    DECL_LINK(ToolbarClickHdl, const OUString&, void);
    DECL_LINK(BkpAddHdl, weld::Button&, void);
    DECL_LINK(BkpEntryActivateHdl, weld::Entry&, bool);
    DECL_LINK(BkpToggledHdl, const weld::TreeView::iter_col&, void);
    DECL_LINK(BkpActivatedHdl, const weld::TreeIter&, bool);
    DECL_LINK(BkpCommandHdl, const CommandEvent&, bool);
    DECL_LINK(ThreadChangedHdl, weld::ComboBox&, void);
    DECL_LINK(FrameSelectedHdl, weld::ItemView&, void);
    DECL_LINK(FrameActivatedHdl, const weld::TreeIter&, bool);
    DECL_LINK(VarExpandingHdl, const weld::TreeIter&, bool);
    DECL_LINK(ConsoleActivateHdl, weld::Entry&, bool);
    DECL_STATIC_LINK(DebugPanel, DoneHdl, void*, void);

    std::unique_ptr<weld::Toolbar> mxToolbar;
    std::unique_ptr<weld::TreeView> mxBreakpoints;
    std::unique_ptr<weld::Entry> mxBkpEntry;
    std::unique_ptr<weld::Button> mxBkpAdd;
    std::unique_ptr<weld::ComboBox> mxThreads;
    std::unique_ptr<weld::TreeView> mxStack;
    std::unique_ptr<weld::TreeView> mxVariables;
    std::unique_ptr<weld::TextView> mxConsole;
    std::unique_ptr<weld::Entry> mxConsoleEntry;
    std::unique_ptr<weld::Label> mxStatus;

    css::uno::Reference<css::code::debug::XDebugSession> mxSession;
    css::uno::Reference<css::code::debug::XDebugSessionListener> mxListener;
    // the manager's launch requests; mxManager is empty when subscribing failed
    css::uno::Reference<css::code::debug::XDebugManager> mxManager;
    css::uno::Reference<css::code::debug::XDebugManagerListener> mxManagerListener;
    css::code::debug::DebugSessionState meState
        = css::code::debug::DebugSessionState_EXITED; // meaningful only with a session

    std::vector<PanelBreakpoint> maBreakpoints;
    sal_Int32 mnNextBreakpointSerial = 0;
    std::vector<css::code::debug::DebugThread> maThreads;
    std::vector<css::code::debug::StackFrame> maFrames;
    sal_Int32 mnStoppedThread = 0; // the thread of the last stop event
    sal_Int32 mnCurrentFrame = 0; // the frame the variables are of; 0: none
    // variable rows: id -> Variable::Reference for the children, and which
    // rows have been filled already
    std::map<OUString, sal_Int32> maVarRefs;
    std::set<OUString> maVarLoaded;
    std::map<OUString, std::unique_ptr<weld::TreeIter>> maVarPending;
    sal_Int32 mnNextVarSerial = 0;
    sal_Int32 mnConsoleLength = 0;

    std::shared_ptr<EventQueue> mpEvents;
    std::shared_ptr<::code::UiNotifier> mxNotifier;

    // tells an answer that comes late that the panel is gone
    std::shared_ptr<bool> mpAlive = std::make_shared<bool>(true);
    bool mbBusy = false;
    bool mbStopRefreshWanted = false;
    bool mbFilling = false;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
