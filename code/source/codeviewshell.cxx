/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "codeviewshell.hxx"

#include "codedocshell.hxx"
#include "editormdipanel.hxx"
#include "workbench.hxx"

#include <com/sun/star/code/build/XBuildController.hpp>
#include <com/sun/star/code/build/XBuildable.hpp>
#include <com/sun/star/code/build/theBuildController.hpp>
#include <com/sun/star/code/core/XBackgroundJob.hpp>
#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/frontend/FileFilter.hpp>
#include <com/sun/star/code/frontend/XDialogs.hpp>
#include <com/sun/star/code/project/XProjectFormatReader.hpp>
#include <com/sun/star/code/project/XProjectFormatRegistry.hpp>
#include <com/sun/star/code/project/theProjectFormatRegistry.hpp>
#include <comphelper/sequence.hxx>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <comphelper/processfactory.hxx>
#include "sidebar/outputchildwindow.hxx"
#include "sidebar/outputdockingwindow.hxx"

#include <com/sun/star/code/core/JobEvent.hpp>
#include <com/sun/star/code/core/XBackgroundJobListener.hpp>
#include <com/sun/star/code/debug/theDebugManager.hpp>
#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>
#include <sfx2/sidebar/SidebarController.hxx>
#include <vcl/svapp.hxx>

#include <codeids.hxx>
#include <startuptarget.hxx>
#include "runcontroller.hxx"
#include "optionsdialog.hxx"
#include "finddialog.hxx"
#include "gotosymboldialog.hxx"
#include "vcsstatusbarcontrol.hxx"
#include <sfx2/objface.hxx>
#include <mutex>
#include <vector>
#include <sfx2/request.hxx>
#include <sfx2/frame.hxx>
#include <sfx2/sfxsids.hrc>
#include <svl/stritem.hxx>
#include <svl/voiditem.hxx>
#include <sfx2/sidebar/SidebarChildWindow.hxx>
#include <sfx2/viewfrm.hxx>
#include <vcl/EnumContext.hxx>

// See codedocshell.cxx for why this is needed.
extern SFX2_DLLPUBLIC constinit const SfxType0 aSfxVoidItem_Impl;

#ifdef _WIN32
// Unlike the ELF build, where the process finds aSfxUnoAnyItem_Impl in one of
// the shared libraries, a PE import has to be backed by an exported definition
// and none of them exports this one; SFX_TYPEMAP makes the slot header define
// it here, the way sc/sd/basctl do with their typemap.cxx.
#define SFX_TYPEMAP
#endif
#define ShellClass_code_Shell
#include <codeslots.hxx>

namespace uno = css::uno;

namespace code
{
namespace
{
// ---- Start (Without) Debugging ----------------------------------------------
//
// The startup target is built first, as Visual Studio does, and starts when
// the build succeeds (the Build channel already says why when it does not).
// The build job reports on its own thread; the start is posted to the main
// thread, where the sidebar and the Debug panel live.

void ShowStartMessage(const OUString& rText)
{
    if (Workbench* pWorkbench = Workbench::Instance())
        pWorkbench->showMessage(rText, 6000);
}

// main thread
void StartNow(const uno::Reference<uno::XComponentContext>& xContext,
              const css::code::debug::LaunchDescriptor& rDescriptor, bool bWithDebugger)
{
    if (bWithDebugger)
    {
        try
        {
            // The request first: a living Debug panel hears it at once, and
            // one the deck switch creates below hears it on subscribing.
            css::code::debug::theDebugManager::get(xContext)->requestLaunch(rDescriptor);
        }
        catch (const uno::Exception& rException)
        {
            ShowStartMessage(u"Cannot start debugging: "_ustr + rException.Message);
            return;
        }
        // the Debug tab of the bottom dock
        if (SfxViewFrame* pFrame = SfxViewFrame::Current())
        {
            pFrame->ShowChildWindow(sidebar::OutputChildWindow::GetChildWindowId());
            if (SfxChildWindow* pChild
                = pFrame->GetChildWindow(sidebar::OutputChildWindow::GetChildWindowId()))
                if (auto* pDock = dynamic_cast<sidebar::OutputDockingWindow*>(pChild->GetWindow()))
                    pDock->ShowDeck(u"DebugDeck");
        }
    }
    else if (!run::Start(xContext, rDescriptor))
        ShowStartMessage(u"A program is already running."_ustr);
}

class StartAfterBuild final
    : public cppu::WeakImplHelper<css::code::core::XBackgroundJobListener>
{
public:
    StartAfterBuild(uno::Reference<uno::XComponentContext> xContext,
                    css::code::debug::LaunchDescriptor aDescriptor, bool bWithDebugger)
        : m_xContext(std::move(xContext))
        , m_aDescriptor(std::move(aDescriptor))
        , m_bWithDebugger(bWithDebugger)
    {
    }

    void Watch(const uno::Reference<css::code::core::XBackgroundJob>& xJob)
    {
        m_xJob = xJob;
        xJob->addJobListener(this);
        // the job may have ended before the listener was in place
        Check(xJob->getStatus());
    }

    void SAL_CALL jobChanged(const css::code::core::JobEvent& rEvent) override
    {
        Check(rEvent.Status);
    }
    void SAL_CALL disposing(const css::lang::EventObject&) override {}

private:
    void Check(css::code::core::JobStatus eStatus)
    {
        if (eStatus == css::code::core::JobStatus_RUNNING)
            return;
        {
            std::scoped_lock aGuard(m_aMutex);
            if (m_bDone)
                return;
            m_bDone = true;
        }
        try
        {
            m_xJob->removeJobListener(this);
        }
        catch (const uno::Exception&)
        {
        }
        if (eStatus != css::code::core::JobStatus_SUCCEEDED)
            return;
        // to the main thread, alive until the handler ran
        acquire();
        Application::PostUserEvent(LINK(this, StartAfterBuild, StartHdl));
    }

    DECL_LINK(StartHdl, void*, void);

    const uno::Reference<uno::XComponentContext> m_xContext;
    const css::code::debug::LaunchDescriptor m_aDescriptor;
    const bool m_bWithDebugger;
    uno::Reference<css::code::core::XBackgroundJob> m_xJob;
    std::mutex m_aMutex;
    bool m_bDone = false;
};

IMPL_LINK_NOARG(StartAfterBuild, StartHdl, void*, void)
{
    StartNow(m_xContext, m_aDescriptor, m_bWithDebugger);
    release(); // the acquire() of Check()
}
}

SFX_IMPL_NAMED_VIEWFACTORY(Shell, "Default")
{
    SFX_VIEW_REGISTRATION(DocShell);
}

SFX_IMPL_INTERFACE(code_Shell, SfxViewShell)

void code_Shell::InitInterface_Impl()
{
    // Lets the real Sidebar (its "Solution" deck now hosts what used to be
    // the in-content SolutionExplorerPanel, see code/source/sidebar/) attach
    // its SfxChildWindow to this view -- the same call
    // sw/source/uibase/uiview/view0.cxx makes for Writer.
    GetStaticInterface()->RegisterChildWindow(
        sfx2::sidebar::SidebarChildWindow::GetChildWindowId());

    // Same for the bottom-only Output dock (code/source/sidebar/) --
    // the in-content OutputPanel it replaces is gone, see LayoutPanels()
    // below.
    GetStaticInterface()->RegisterChildWindow(
        code::sidebar::OutputChildWindow::GetChildWindowId());
}

Shell::Shell(SfxViewFrame& rFrame, SfxViewShell* /*pOldSh*/)
    : SfxViewShell(rFrame, SfxViewShellFlags::NO_NEWWINDOW)
{
    // Without this, SidebarController (sfx2/source/sidebar/SidebarController.cxx)
    // sits registered as a valid css::ui::XContextChangeEventListener but
    // never gets its first, deck-selecting ContextChangeEventObject: that
    // broadcast is gated on SfxShell::maContextChangeBroadcaster's stored
    // context name (sfx2/source/control/shell.cxx, BroadcastContextForActivation())
    // being non-empty, and it starts empty until SetContextName() is
    // called -- every real view shell (sw's textsh.cxx, sc's cellsh.cxx, ...)
    // does this in its own constructor. Shell activation itself already
    // happens normally through SfxDispatcher's ordinary shell-stack push,
    // so this is the one missing piece, not another Appear()-style cascade
    // to fake.
    SetContextName(vcl::EnumContext::GetContextName(vcl::EnumContext::Context::Default));

    InitPanels();
}

Shell::~Shell()
{
    // Unregister before destroying it -- same order as SwView's dtor, so
    // the base class never holds a dangling window pointer.
    SetWindow(nullptr);
    mxEditorArea.disposeAndClear();
}

void Shell::InitPanels()
{
    vcl::Window& rContentWindow = GetViewFrame().GetWindow();

    // The editor is a child of the frame's own content window, and -- like
    // every real app's edit window (sw's SwEditWin, basctl's window) -- it,
    // NOT the content window itself, is what gets registered via
    // SetWindow(). Two reasons, both load-bearing:
    //
    // - SfxViewFrame::DoAdjustPosSizePixel() (sfx2/source/view/viewfrm.cxx)
    //   only runs the OuterResizePixel()/SetBorderPixel() layout cascade at
    //   all when SfxViewShell::GetWindow() is non-null; our standalone
    //   Frame never resizes this view without it.
    //
    // - SfxViewFrame::SetBorderPixelImpl() positions GetWindow() at the
    //   edit area -- (0,0, content-window size) for a shell with no inner
    //   tools -- as a CHILD inside the content window. Registering the
    //   content window itself here made that call reposition the content
    //   window to (0,0) within the sfx2 WorkWindow, clobbering the
    //   tool-space border offset SfxFrame::SetToolSpaceBorderPixel_Impl()
    //   had just applied: any LEFT/TOP-docked strip (e.g. the collapsed
    //   sidebar's 8px fade-in splitter) ended up covered by the editor.
    //   RIGHT/BOTTOM docks survived only because (0,0) happens to be the
    //   correct position when the border is on those sides.
    //
    // With the editor registered instead, SetBorderPixelImpl() lays the
    // editor out inside the content window on every resize, so there is no
    // hand-written OuterResizePixel()/LayoutPanels() layout here anymore.
    // (The Output dock and Solution deck live outside this content area
    // entirely, as real sfx2 docks -- see code/source/sidebar/.)
    mxEditorArea = VclPtr<EditorMDIPanel>::Create(&rContentWindow);
    mxEditorArea->Show();
    SetWindow(mxEditorArea.get());

    // First paint happens before any resize reaches the cascade above;
    // start out filling the content window like SetBorderPixelImpl() will.
    mxEditorArea->SetPosSizePixel(Point(0, 0), rContentWindow.GetOutputSizePixel());
}

void Shell::Execute(SfxRequest& rReq)
{
    switch (rReq.GetSlot())
    {
        case SID_CODE_BUILD:
        case SID_CODE_REBUILD:
        case SID_CODE_CLEAN:
        {
            uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
            try
            {
                auto xProject = css::code::project::theProjectManager::get(xContext)->getActiveProject();
                if (!xProject.is())
                {
                    if (Workbench* pWorkbench = Workbench::Instance())
                        pWorkbench->showMessage(u"Open a project to build."_ustr, 6000);
                    break;
                }
                css::uno::Reference<css::code::build::XBuildable> xBuildable(xProject, css::uno::UNO_QUERY);
                if (!xBuildable.is())
                {
                    if (Workbench* pWorkbench = Workbench::Instance())
                        pWorkbench->showMessage(u"This project cannot be built."_ustr, 6000);
                    break;
                }
                // what is on disk is what gets built
                css::code::document::theDocumentManager::get(xContext)->saveAll();
                auto xBuild = css::code::build::theBuildController::get(xContext);
                if (rReq.GetSlot() == SID_CODE_REBUILD)
                    xBuild->rebuild(xBuildable);
                else if (rReq.GetSlot() == SID_CODE_CLEAN)
                    xBuild->clean(xBuildable);
                else
                    xBuild->build(xBuildable);
            }
            catch (const uno::Exception& rException)
            {
                if (Workbench* pWorkbench = Workbench::Instance())
                    pWorkbench->showMessage(u"Cannot build: "_ustr + rException.Message, 8000);
            }
            break;
        }
        case SID_CODE_START_DEBUGGING:
        case SID_CODE_START_WITHOUT_DEBUGGING:
        {
            const bool bWithDebugger = rReq.GetSlot() == SID_CODE_START_DEBUGGING;
            uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
            try
            {
                // the active project, or the one open: the choice the startup box shows
                auto xManager = css::code::project::theProjectManager::get(xContext);
                auto xProject = xManager->getActiveProject();
                if (!xProject.is())
                    if (const auto aOpen = xManager->getOpenProjects(); aOpen.hasElements())
                        xProject = aOpen[0];
                const auto xTarget = startup::ResolveStartupTarget(xContext, xProject);
                css::uno::Reference<css::code::debug::XLaunchable> xLaunchable(xTarget, css::uno::UNO_QUERY);
                if (!xLaunchable.is())
                {
                    ShowStartMessage(u"No startup target to run."_ustr);
                    break;
                }
                const css::code::debug::LaunchDescriptor aDescriptor = xLaunchable->createLaunchDescriptor();
                if (aDescriptor.Program.isEmpty())
                {
                    ShowStartMessage(u"The startup target has nothing to run."_ustr);
                    break;
                }
                if (!bWithDebugger && run::IsRunning())
                {
                    ShowStartMessage(u"A program is already running."_ustr);
                    break;
                }

                // built first, as Visual Studio does; what is on disk is what gets built
                css::uno::Reference<css::code::build::XBuildable> xBuildable(xTarget, css::uno::UNO_QUERY);
                auto xBuild = css::code::build::theBuildController::get(xContext);
                if (!xBuildable.is())
                {
                    StartNow(xContext, aDescriptor, bWithDebugger); // nothing to build with
                    break;
                }
                if (xBuild->getRunningBuild().is())
                {
                    ShowStartMessage(u"A build is already running."_ustr);
                    break;
                }
                css::code::document::theDocumentManager::get(xContext)->saveAll();
                rtl::Reference<StartAfterBuild> xStart
                    = new StartAfterBuild(xContext, aDescriptor, bWithDebugger);
                xStart->Watch(xBuild->build(xBuildable));
            }
            catch (const uno::Exception& rException)
            {
                ShowStartMessage(u"Cannot start: "_ustr + rException.Message);
            }
            break;
        }
        case SID_CODE_OPTIONS:
            if (RunOptionsDialog(rReq.GetFrameWeld()) && mxEditorArea)
                mxEditorArea->SettingsChanged();
            break;
        case SID_CODE_FINDINFILES:
        case SID_CODE_REPLACEINFILES:
        case SID_CODE_MODIFYFIND:
        {
            css::code::search::FindOptions aInitial;
            bool bInitial = false;
            if (rReq.GetSlot() == SID_CODE_MODIFYFIND)
            {
                const SfxUnoAnyItem* pOptions
                    = rReq.GetArg<SfxUnoAnyItem>(SID_CODE_ARG_FINDOPTIONS);
                bInitial = pOptions && (pOptions->GetValue() >>= aInitial);
            }
            RunFindDialog(
                rReq.GetFrameWeld(),
                rReq.GetSlot() == SID_CODE_REPLACEINFILES || (bInitial && aInitial.Replace),
                [this]
                {
                    // the results come in the bottom dock, on the Find in Files tab
                    SfxViewFrame& rFrame = GetViewFrame();
                    rFrame.ShowChildWindow(sidebar::OutputChildWindow::GetChildWindowId());
                    if (SfxChildWindow* pChild = rFrame.GetChildWindow(
                            sidebar::OutputChildWindow::GetChildWindowId()))
                        if (auto* pDock = dynamic_cast<sidebar::OutputDockingWindow*>(
                                pChild->GetWindow()))
                            pDock->ShowDeck(u"FindResultsDeck");
                },
                bInitial ? &aInitial : nullptr);
            break;
        }
        case SID_COPY:
        case SID_CUT:
        case SID_PASTE:
        case SID_UNDO:
        case SID_REDO:
        case SID_SELECTALL:
            if (Workbench* pWorkbench = Workbench::Instance())
            {
                static const std::pair<sal_uInt16, EditCommand> aCommands[]
                    = { { SID_COPY, EditCommand::Copy },     { SID_CUT, EditCommand::Cut },
                        { SID_PASTE, EditCommand::Paste },   { SID_UNDO, EditCommand::Undo },
                        { SID_REDO, EditCommand::Redo },     { SID_SELECTALL, EditCommand::SelectAll } };
                for (const auto& [nSlot, eCommand] : aCommands)
                    if (nSlot == rReq.GetSlot())
                        pWorkbench->DoEditCommand(eCommand);
            }
            break;
        case SID_CODE_GOTODEFINITION:
            GoToDefinition(rReq.GetFrameWeld());
            break;
        case SID_CODE_GOTOSYMBOL:
            RunGoToSymbolDialog(rReq.GetFrameWeld());
            break;
        case SID_CODE_CANCELBUILD:
            try
            {
                css::code::build::theBuildController::get(comphelper::getProcessComponentContext())->cancelBuild();
            }
            catch (const uno::Exception&)
            {
            }
            break;
        case SID_CODE_OPENPROJECT:
        {
            uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
            try
            {
                uno::Reference<css::code::frontend::XWorkbench> xWorkbench
                    = css::code::core::theCode::get(xContext)->getWorkbench();
                if (!xWorkbench.is() || !xWorkbench->getDialogs().is())
                    break;

                // One filter per format the registered readers know, and in front
                // of them the union of all: "every project or solution I can open".
                const uno::Sequence<uno::Reference<css::code::project::XProjectFormatReader>> aReaders
                    = css::code::project::theProjectFormatRegistry::get(xContext)->getReaders();
                std::vector<css::code::frontend::FileFilter> aFilters;
                std::vector<OUString> aAll;
                for (const auto& xReader : aReaders)
                {
                    const uno::Sequence<OUString> aPatterns = xReader->getFilePatterns();
                    if (!aPatterns.hasElements())
                        continue;
                    aFilters.push_back({ xReader->getDisplayName(), aPatterns });
                    aAll.insert(aAll.end(), aPatterns.begin(), aPatterns.end());
                }
                aFilters.insert(aFilters.begin(),
                                { u"All supported project and solution formats"_ustr,
                                  comphelper::containerToSequence(aAll) });

                const OUString aUrl = xWorkbench->getDialogs()->pickFile(
                    u"Open Project"_ustr, comphelper::containerToSequence(aFilters));
                if (aUrl.isEmpty())
                    break;
                css::code::project::theProjectManager::get(xContext)->openProject(aUrl);
            }
            catch (const uno::Exception& rException)
            {
                if (Workbench* pWorkbench = Workbench::Instance())
                    pWorkbench->showMessage(u"Cannot open project: "_ustr + rException.Message, 8000);
            }
            break;
        }
        case SID_CODE_OPENFOLDER:
        {
            // The project manager decides what a folder is (a registered reader
            // that recognises it, else a plain file tree); this only asks where.
            uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
            try
            {
                uno::Reference<css::code::frontend::XWorkbench> xWorkbench
                    = css::code::core::theCode::get(xContext)->getWorkbench();
                if (!xWorkbench.is() || !xWorkbench->getDialogs().is())
                    break;
                const OUString aUrl = xWorkbench->getDialogs()->pickFolder(u"Open Folder"_ustr);
                if (aUrl.isEmpty())
                    break;
                css::code::project::theProjectManager::get(xContext)->openProject(aUrl);
            }
            catch (const uno::Exception& rException)
            {
                if (Workbench* pWorkbench = Workbench::Instance())
                    pWorkbench->showMessage(u"Cannot open folder: "_ustr + rException.Message, 8000);
            }
            break;
        }
        case SID_CODE_SAVEALL:
            try
            {
                css::code::document::theDocumentManager::get(comphelper::getProcessComponentContext())
                    ->saveAll();
            }
            catch (const uno::Exception& rException)
            {
                if (Workbench* pWorkbench = Workbench::Instance())
                    pWorkbench->showMessage(u"Cannot save: "_ustr + rException.Message, 8000);
            }
            break;
        case SID_SAVEDOC: // File > Save and Ctrl+S
            try
            {
                if (Workbench* pWorkbench = Workbench::Instance())
                    pWorkbench->SaveCurrentDocument();
            }
            catch (const uno::Exception& rException)
            {
                if (Workbench* pWorkbench = Workbench::Instance())
                    pWorkbench->showMessage(u"Cannot save: "_ustr + rException.Message, 8000);
            }
            break;
        default:
            // SID_NEWDOC is still decorative.
            break;
    }
    rReq.Done();
}

void Shell::GetState(SfxItemSet& rSet)
{
    // SID_NEWDOC/SID_SAVEDOC: leaving them untouched keeps both slots
    // enabled; nothing here depends on document state yet.

    // Status bar items. The text is the workbench's current message
    // (XStatusBar::showMessage, notifications), "Ready" when it has none; the
    // workbench invalidates this slot whenever that changes. The bubble
    // button's popover is still the constant "No background tasks running."
    // Build is possible with a project open and nothing building; Cancel only while one runs.
    if (rSet.GetItemState(SID_CODE_BUILD, false) != SfxItemState::UNKNOWN
        || rSet.GetItemState(SID_CODE_REBUILD, false) != SfxItemState::UNKNOWN
        || rSet.GetItemState(SID_CODE_CLEAN, false) != SfxItemState::UNKNOWN
        || rSet.GetItemState(SID_CODE_CANCELBUILD, false) != SfxItemState::UNKNOWN)
    {
        bool bProject = false;
        bool bBuilding = false;
        try
        {
            auto xContext = comphelper::getProcessComponentContext();
            bProject = css::uno::Reference<css::code::build::XBuildable>(
                css::code::project::theProjectManager::get(xContext)->getActiveProject(), css::uno::UNO_QUERY).is();
            bBuilding = css::code::build::theBuildController::get(xContext)->getRunningBuild().is();
        }
        catch (const uno::Exception&)
        {
        }
        if (rSet.GetItemState(SID_CODE_BUILD, false) != SfxItemState::UNKNOWN && (!bProject || bBuilding))
            rSet.DisableItem(SID_CODE_BUILD);
        if (rSet.GetItemState(SID_CODE_REBUILD, false) != SfxItemState::UNKNOWN && (!bProject || bBuilding))
            rSet.DisableItem(SID_CODE_REBUILD);
        if (rSet.GetItemState(SID_CODE_CLEAN, false) != SfxItemState::UNKNOWN && (!bProject || bBuilding))
            rSet.DisableItem(SID_CODE_CLEAN);
        if (rSet.GetItemState(SID_CODE_CANCELBUILD, false) != SfxItemState::UNKNOWN && !bBuilding)
            rSet.DisableItem(SID_CODE_CANCELBUILD);
    }
    if (rSet.GetItemState(SID_CODE_CONFIGURATION, false) != SfxItemState::UNKNOWN)
        rSet.Put(SfxVoidItem(SID_CODE_CONFIGURATION));
    if (rSet.GetItemState(SID_CODE_STARTUP, false) != SfxItemState::UNKNOWN)
        rSet.Put(SfxVoidItem(SID_CODE_STARTUP));
    // Enabled with any project open: whether its startup target can really
    // run is answered (with a message) on Execute -- resolving it here would
    // read the configuration store on every status round.
    if (rSet.GetItemState(SID_CODE_START_DEBUGGING, false) != SfxItemState::UNKNOWN
        || rSet.GetItemState(SID_CODE_START_WITHOUT_DEBUGGING, false) != SfxItemState::UNKNOWN)
    {
        bool bProject = false;
        try
        {
            auto xManager = css::code::project::theProjectManager::get(
                comphelper::getProcessComponentContext());
            bProject = xManager->getActiveProject().is() || xManager->getOpenProjects().hasElements();
        }
        catch (const uno::Exception&)
        {
        }
        if (!bProject)
        {
            if (rSet.GetItemState(SID_CODE_START_DEBUGGING, false) != SfxItemState::UNKNOWN)
                rSet.DisableItem(SID_CODE_START_DEBUGGING);
            if (rSet.GetItemState(SID_CODE_START_WITHOUT_DEBUGGING, false) != SfxItemState::UNKNOWN)
                rSet.DisableItem(SID_CODE_START_WITHOUT_DEBUGGING);
        }
    }
    if (rSet.GetItemState(SID_CODE_STATUS_TASKS, false) != SfxItemState::UNKNOWN)
        rSet.Put(SfxVoidItem(SID_CODE_STATUS_TASKS));
    if (rSet.GetItemState(SID_CODE_STATUS_VCS, false) != SfxItemState::UNKNOWN)
        rSet.Put(SfxStringItem(SID_CODE_STATUS_VCS, VcsStatusText()));
    if (rSet.GetItemState(SID_CODE_STATUS_READY, false) != SfxItemState::UNKNOWN)
        rSet.Put(SfxStringItem(SID_CODE_STATUS_READY,
                               Workbench::Instance() ? Workbench::Instance()->GetStatusText()
                                                     : u"Ready"_ustr));
}

} // namespace code

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
