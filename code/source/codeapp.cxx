/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * =======================================================================
 * LibreStudio Code -- v2: a real sfx2/framework shell. The menu bar,
 * toolbar and status bar are no longer hand-built (see codeviewshell.cxx
 * for what code::Shell actually owns: the Editor/Output content panels)
 * -- they come from code/uiconfig/code/{menubar,toolbar,statusbar}/*.xml
 * via framework's own LayoutManager, the same mechanism Writer/Calc/basctl
 * use. Solution Explorer is a real sfx2::sidebar deck now too (see
 * code/source/sidebar/), not a third hand-docked content panel.
 *
 * code::DocShell is constructed directly here and handed to the standard
 * office FrameLoader with an already-built model -- the same pattern
 * basctl's own SID_BASICIDE_APPEAR uses (sfx2/source/appl/appserv.cxx) --
 * rather than going through the generic private:factory/scode ->
 * UNO-service-name resolution path, which would require code::DocShell to
 * be registered as a dlopen()able UNO component. That is why "code" has no
 * .component file and stays a plain executable.
 * =======================================================================
 */

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <typeinfo>

#include <execinfo.h>

#include <com/sun/star/awt/XWindow2.hpp>
#include <com/sun/star/frame/Frame.hpp>
#include <com/sun/star/frame/XFrame2.hpp>
#include <com/sun/star/frame/XModel3.hpp>
#include <com/sun/star/frame/XSynchronousFrameLoader.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <comphelper/configuration.hxx>
#include <comphelper/namedvaluecollection.hxx>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/bootstrap.hxx>
#include <officecfg/Office/Common.hxx>
#include <officecfg/Setup.hxx>
#include <osl/thread.h>
#include <rtl/ustring.hxx>
#include <sfx2/app.hxx>
#include <sfx2/chalign.hxx>
#include <sfx2/childwin.hxx>
#include <sfx2/sidebar/SidebarChildWindow.hxx>
#include <sfx2/sidebar/SidebarController.hxx>
#include <sfx2/viewfrm.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <unotools/bootstrap.hxx>
#include <unotools/configmgr.hxx>
#include <unotools/resmgr.hxx>
#include <vcl/svapp.hxx>
#include <vcl/vclmain.hxx>
#include <vcl/wrkwin.hxx>

#include "codedocshell.hxx"
#include "codeviewshell.hxx"

using namespace com::sun::star;

namespace {

class CodeApplication : public Application
{
public:
    virtual void Init() override;
    virtual int Main() override;

private:
    int mnExitCode = EXIT_SUCCESS;
};

// A great many actions here still dispatch into UNO services that were
// never built (see the "code" porting log in project memory for the running
// list of components found missing this way -- uui, filterconfig, ...).
// Those show up as an *uncaught* UNO/std exception escaping VCL's own event
// dispatch (ImplWindowFrameProc et al. have no top-level catch of their
// own), which libstdc++'s default terminate handler turns into a bare
// "terminate called after throwing an instance of 'X'" with no message and
// no backtrace -- diagnosing each one has meant reproducing it under gdb by
// hand. This replaces that handler so every future one is self-diagnosing.
void CodeTerminateHandler()
{
    fprintf(stderr, "\n=== LibreStudio Code: unhandled exception, terminating ===\n");
    if (std::exception_ptr eptr = std::current_exception())
    {
        try
        {
            std::rethrow_exception(eptr);
        }
        catch (const uno::Exception& e)
        {
            fprintf(stderr, "type: %s\nmessage: %s\n", typeid(e).name(),
                OUStringToOString(e.Message, osl_getThreadTextEncoding()).getStr());
        }
        catch (const std::exception& e)
        {
            fprintf(stderr, "type: %s\nmessage: %s\n", typeid(e).name(), e.what());
        }
        catch (...)
        {
            fprintf(stderr, "type: <unknown, neither css::uno::Exception nor std::exception>\n");
        }
    }
    else
    {
        fprintf(stderr, "(std::terminate called with no active exception)\n");
    }

    void* aFrames[64];
    int nFrames = backtrace(aFrames, sizeof(aFrames) / sizeof(aFrames[0]));
    fprintf(stderr, "--- backtrace (%d frames) ---\n", nFrames);
    backtrace_symbols_fd(aFrames, nFrames, fileno(stderr));
    fflush(stderr);

    std::abort();
}

// %PRODUCTNAME%/%OOOVENDOR%/%ABOUTBOXPRODUCTVERSION(SUFFIX)% et al. in .ui
// files and resource strings (e.g. cui's aboutdialog.ui) are left as literal
// text unless something installs a Translate::SetReadStringHook() --
// vcl/source/window/builder.cxx checks Translate::GetReadStringHook() for
// every .ui-loaded string, but nothing sets it by default. Real soffice.bin
// installs one from desktop::Desktop::Main() (desktop/source/appl/app.cxx's
// static, desktop-module-private ReplaceStringHookProc); "code" doesn't
// link desktop at all, so this is the same substitution copied verbatim.
OUString CodeReplaceStringHookProc(const OUString& rStr)
{
    const static OUString sBuildId(utl::Bootstrap::getBuildIdData(u"development"_ustr)),
        sBrandName(utl::ConfigManager::getProductName()),
        sVersion(utl::ConfigManager::getProductVersion()),
        sAboutBoxVersion(utl::ConfigManager::getAboutBoxProductVersion()),
        sAboutBoxVersionSuffix(utl::ConfigManager::getAboutBoxProductVersionSuffix()),
        sExtension(utl::ConfigManager::getProductExtension());

    OUString sRet(rStr);
    if (sRet.indexOf("%PRODUCT") != -1 || sRet.indexOf("%ABOUTBOX") != -1)
    {
        sRet = sRet.replaceAll("%PRODUCTNAME", sBrandName);
        sRet = sRet.replaceAll("%PRODUCTVERSION", sVersion);
        sRet = sRet.replaceAll("%BUILDID", sBuildId);
        sRet = sRet.replaceAll("%ABOUTBOXPRODUCTVERSIONSUFFIX", sAboutBoxVersionSuffix);
        sRet = sRet.replaceAll("%ABOUTBOXPRODUCTVERSION", sAboutBoxVersion);
        sRet = sRet.replaceAll("%PRODUCTEXTENSION", sExtension);
    }

    if (sRet.indexOf("%OOOVENDOR") != -1)
    {
        const static OUString sOOOVendor = utl::ConfigManager::getVendor();
        sRet = sRet.replaceAll("%OOOVENDOR", sOOOVendor);
    }

    return sRet;
}

}

void CodeApplication::Init()
{
    std::set_terminate(&CodeTerminateHandler);
    Translate::SetReadStringHook(&CodeReplaceStringHookProc);

    // Unlike v1, this is no longer optional: SfxApplication and the whole
    // sfx2/framework machinery need a real UNO context to exist at all.
    uno::Reference<uno::XComponentContext> xContext =
        cppu::defaultBootstrap_InitialComponentContext();
    uno::Reference<lang::XMultiComponentFactory> xFactory = xContext->getServiceManager();
    uno::Reference<lang::XMultiServiceFactory> xSFactory(xFactory, uno::UNO_QUERY_THROW);
    comphelper::setProcessServiceFactory(xSFactory);
}

int CodeApplication::Main()
{
    VclPtr<WorkWindow> xMainWindow;

    try
    {
        SfxApplication::GetOrCreate();

        // ShowTipOfTheDay: not what actually fixed the icon-theme crash
        // (that turned out to be postprocess_images never having been
        // built here, see project memory project_librestudio) -- kept
        // anyway as a genuine, independent UX choice.
        //
        // WhatsNew: this is the real one -- SfxViewFrame's own startup
        // code (sfx2/source/view/viewfrm.cxx, the wantsWhatsNew branch)
        // shows a real "Welcome to %PRODUCTNAME%!" wizard (welcomedialog.ui,
        // 3 tabs: Welcome/User Interface/Appearance) whenever
        // utl::isProductVersionNew() sees no prior-version marker in the
        // user profile -- true every run here, since this process has no
        // persisted user profile at all. Gating on WhatsNew is what that
        // code itself checks first; false skips the dialog, the "what's
        // new" alternative dialog, and its infobar fallback in one go.
        // Both settings are scoped to this process only, not a shared
        // config file.
        {
            auto xBatch = comphelper::ConfigurationChanges::create();
            officecfg::Office::Common::Misc::ShowTipOfTheDay::set(false, xBatch);
            officecfg::Setup::Product::WhatsNew::set(false, xBatch);
            xBatch->commit();
        }

        // basctl does this same registration inside its own Dll::Dll()
        // constructor (basctl/source/basicide/iderdll.cxx), reached via
        // EnsureIde() the first time anything touches the Basic IDE. "code"
        // has no UNO-component entry point to hang an equivalent lazy
        // singleton off, so it just does it directly here, once, before the
        // first (and only) DocShell/Shell of the process is created.
        code::DocShell::RegisterInterface();
        code::Shell::RegisterFactory(CODE_INTERFACE_VIEWSH);
        code::Shell::RegisterInterface();

        // bVisible=true: this app has no persisted user profile for a
        // saved ChildWindow state to override (see UserInstallation in
        // cmake/LibreStudioUnorc.cmake), so the factory default below is
        // what actually governs whether the Solution sidebar shows up on
        // first run -- real Writer passes false here and relies on other,
        // session-persisted state instead (sw/source/uibase/app/swmodule.cxx).
        sfx2::sidebar::SidebarChildWindow::RegisterChildWindow(true, nullptr);

        // SidebarChildWindow's own constructor used to hardcode
        // SfxChildAlignment::RIGHT unconditionally -- correct for Writer/
        // Calc/Impress' Properties-style deck, but the user asked for the
        // Solution Explorer-style left placement Visual Studio uses
        // instead. SetDefaultAlignment() only changes the *fallback* used
        // when there is no real persisted/dragged state to restore (which
        // is always true here -- no user profile, see above); genuine
        // saved state still wins, same as every other app. Must be called
        // before the sidebar's first ShowChildWindow() below.
        sfx2::sidebar::SidebarChildWindow::SetDefaultAlignment(SfxChildAlignment::LEFT);

        // Also basctl's Dll::Dll(): SfxBaseModel::getSupportedServiceNames()
        // (sfx2/source/doc/sfxbasemodel.cxx) delegates to
        // GetObjectShell()->GetFactory().GetDocumentServiceName(), and
        // framework's ModuleManager::identify() (framework/source/services/
        // modulemanager.cxx) works purely by matching that reported service
        // name against the officecfg Setup.xcu factory nodes -- without this
        // call our model reports no service name at all, identify() throws
        // UnknownModuleException, and that exception crosses enough shared
        // library boundaries that it stops matching `catch (css::uno::
        // Exception&)` by the time it reaches vcl's scheduler, landing in
        // its catch (...) instead, which is an abort(), not a warning.
        code::DocShell::Factory().SetDocumentServiceName(u"com.sun.star.code.CodeDocument"_ustr);

        xMainWindow = VclPtr<WorkWindow>::Create(nullptr,
            WB_APP | WB_STDWORK | WB_SIZEABLE | WB_CLOSEABLE | WB_CLIPCHILDREN);
        xMainWindow->SetText(u"LibreStudio Code"_ustr);
        xMainWindow->SetSizePixel(Size(1200, 800));

        // LayoutManager::implts_doLayout() (framework/source/layoutmanager/
        // layoutmanager.cxx) bails out immediately whenever m_bParentWindowVisible
        // is false, and that flag is latched once, synchronously, from
        // pContainerWindow->IsVisible() inside setDockingAreaAcceptor() -- which
        // Frame::initListeners() calls as part of initialize() below. The later
        // windowShown() listener callback (fired when we Show() the window) only
        // flips the flag back to true; it never re-runs the layout pass. Real
        // Desktop-mediated loading never hits this because Desktop's own
        // TaskCreatorService shows the container window as part of creating it.
        // So the container window must already be visible *before*
        // xFrame->initialize() below, or the FrameAction_COMPONENT_ATTACHED-
        // triggered layout during xLoader->load() silently does nothing -- no
        // menubar/toolbar/statusbar creation, no content-window sizing.
        xMainWindow->Show();

        uno::Reference<awt::XWindow2> xWin(
            VCLUnoHelper::GetInterface(xMainWindow.get()), uno::UNO_QUERY_THROW);

        uno::Reference<uno::XComponentContext> xContext(comphelper::getProcessComponentContext());
        uno::Reference<frame::XFrame2> xFrame(frame::Frame::create(xContext));
        xFrame->initialize(xWin);

        rtl::Reference<SfxObjectShell> xDocShell = new code::DocShell();
        xDocShell->DoInitNew();
        xDocShell->SetModified(false);

        uno::Reference<frame::XSynchronousFrameLoader> xLoader(
            xContext->getServiceManager()->createInstanceWithContext(
                u"com.sun.star.comp.office.FrameLoader"_ustr, xContext),
            uno::UNO_QUERY_THROW);

        comphelper::NamedValueCollection aLoadArgs;
        aLoadArgs.put(u"Model"_ustr, xDocShell->GetModel());
        aLoadArgs.put(u"URL"_ustr, u"private:factory/scode"_ustr);

        uno::Reference<frame::XFrame> xFrameBase(xFrame, uno::UNO_QUERY_THROW);
        xLoader->load(aLoadArgs.getPropertyValues(), xFrameBase);

        // AppearWithUpdate(), not plain Appear(): SfxFrame::Appear() only
        // shows the windows. The actual "private:resource/menubar/menubar"
        // creation happens inside SfxDispatcher::SetMenu_Impl()
        // (sfx2/source/control/dispatch.cxx), called from Update_Impl() --
        // real Desktop-mediated loading reaches that via the window-focus
        // activation cascade (SfxFrameWindow_Impl::GetFocus() ->
        // MakeActive_Impl(), sfx2/source/view/frame2.cxx) that our hand-built
        // Frame/FrameLoader sequence never triggers on its own.
        // AppearWithUpdate() calls Update_Impl(true) explicitly, bypassing
        // that cascade instead of trying to fake it.
        SfxViewFrame* pViewFrame = SfxViewFrame::GetFirst(xDocShell.get(), false);
        if (pViewFrame)
            pViewFrame->GetFrame().AppearWithUpdate();

        // Same story as AppearWithUpdate() above: RegisterChildWindow's own
        // bVisible=true factory default is what a normal Desktop-mediated
        // session's own startup cascade (SfxWorkWindow's initial
        // UpdateChildWindow pass) consults to auto-create default-visible
        // child windows -- our hand-built Frame/FrameLoader sequence never
        // runs that cascade either, so ask for the sidebar explicitly.
        if (pViewFrame)
            pViewFrame->ShowChildWindow(sfx2::sidebar::SidebarChildWindow::GetChildWindowId());

        // PropertyDeck (officecfg's Sidebar.xcu) is scoped "any, any,
        // visible" -- a generic default meant for real document modules
        // (Writer/Calc/...), not ours, and it has no content that makes
        // sense for "Code" (it shows character/paragraph/cell formatting
        // controls, all bound to the active text/draw selection). Worse,
        // being a second tab in the same deck, it takes part in the deck's
        // width negotiation alongside Solution. Hide it for our module only
        // -- leaving the shared "any" entry other modules still rely on
        // untouched -- via SidebarController::RegisterHiddenDeckForApplication()
        // (sfx2/source/sidebar/SidebarController.cxx). A one-shot HideDeck()
        // call on the current controller is NOT enough: SfxWorkWindow::
        // ToggleChildWindow_Impl() (sfx2/source/appl/workwin.cxx), the code
        // behind every ".uno:Sidebar" menu/toolbar toggle, unconditionally
        // destroys and recreates the docking window/SidebarController/
        // ResourceManager on every OFF->ON cycle -- unlike
        // SfxWorkWindow::UpdateChildWindows_Impl, it never consults
        // SfxChildWindow::IsHideNotDelete() at all, so a hide applied to one
        // instance is lost the moment the user toggles the checkbox.
        // RegisterHiddenDeckForApplication() is a static, process-wide
        // registry consulted fresh by every controller instance's own
        // UpdateConfigurations(), so it survives any number of recreations;
        // call it once, anywhere before the sidebar is first shown.
        sfx2::sidebar::SidebarController::RegisterHiddenDeckForApplication(
            u"com.sun.star.code.CodeDocument", u"PropertyDeck");

        // SidebarController (sfx2/source/sidebar/SidebarController.cxx)
        // registers itself as a css::ui::XContextChangeEventListener on our
        // controller in its own constructor, then just waits: the first,
        // deck-selecting UpdateConfigurations()/SwitchToDeck() call only
        // happens once a ContextChangeEventObject actually arrives. That
        // broadcast is normally sent from SfxShell::Activate() (sfx2/source/
        // control/shell.cxx), called when SfxDispatcher flushes a newly
        // pushed shell onto its stack -- traced with a temporary diagnostic:
        // AppearWithUpdate()'s Update_Impl(true) above does flush the
        // dispatcher and fires exactly two Activate() calls, but neither
        // this pointer is ever code::Shell's own -- some other piece of the
        // normal push-activation wiring our hand-built Frame/FrameLoader
        // sequence relies on isn't happening either. code::Shell::Shell()
        // already calls SetContextName() (codeviewshell.cxx), so the only
        // missing step is the broadcast call itself.
        if (pViewFrame)
        {
            if (SfxViewShell* pCodeShell = pViewFrame->GetViewShell())
                pCodeShell->BroadcastContextForActivation(true);
        }

        // SolutionPanel already calls XSidebar::requestLayout() itself once
        // its content is built (code/source/sidebar/solutionpanel.cxx) --
        // that's the real, already-built "size the sidebar to fit its
        // content" feature (officecfg's Office.UI.Sidebar.General.
        // MinimumWidth, on by default; SidebarController::requestLayout()
        // computes and applies Deck::GetMinimalWidth()), not something to
        // hand-compute. But that call happens *during* panel construction,
        // itself nested inside the BroadcastContextForActivation() call
        // above's own SwitchToDeck()/CreateDeck() sequence, which still has
        // a later step of its own (SwitchToDeck's mpCurrentDeck->
        // setPosSizePixel() using the *current* window width) that runs
        // after and overwrites it. Ask again now that everything has
        // actually settled.
        if (pViewFrame)
        {
            if (SfxViewShell* pCodeShell = pViewFrame->GetViewShell())
            {
                if (auto* pSidebarController
                    = sfx2::sidebar::SidebarController::GetSidebarControllerForView(pCodeShell))
                    pSidebarController->requestLayout();
            }
        }

        Execute();

        xDocShell->DoClose();
        xMainWindow.disposeAndClear();
    }
    catch (const uno::Exception& e)
    {
        fprintf(stderr, "fatal error [%s]: %s\n", typeid(e).name(),
            OUStringToOString(e.Message, osl_getThreadTextEncoding()).getStr());
        mnExitCode = EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "fatal error: %s\n", e.what());
        mnExitCode = EXIT_FAILURE;
    }

    return mnExitCode;
}

void vclmain::createApplication()
{
    static CodeApplication aApp;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
