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
 * LibreStudio Code -- a visual-only prototype for a future Visual-Studio-
 * like IDE built on VCL. Nothing here opens a real file or builds a real
 * project; it exists so the shell has something to grow from. See
 * code/source/mainwindow.hxx for the composition of the shell itself.
 * =======================================================================
 */

#include <cstdio>
#include <typeinfo>

#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/ucb/UniversalContentBroker.hpp>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/bootstrap.hxx>
#include <osl/thread.h>
#include <rtl/ustring.hxx>
#include <vcl/svapp.hxx>
#include <vcl/vclmain.hxx>

#include "mainwindow.hxx"

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

}

void CodeApplication::Init()
{
    // This prop touches no UNO service of its own (no filters, no config
    // manager, nothing) -- so unlike icontest.cxx, a missing/incomplete
    // services.rdb/types.rdb (this scoped build never built the rest of the
    // office that produces them) is not fatal here. Try the same bootstrap
    // icontest does, but keep going without it if it fails; wire this back
    // to a hard requirement once a real UNO-based plugin needs it.
    try
    {
        uno::Reference<uno::XComponentContext> xContext =
            cppu::defaultBootstrap_InitialComponentContext();
        uno::Reference<lang::XMultiComponentFactory> xFactory = xContext->getServiceManager();
        uno::Reference<lang::XMultiServiceFactory> xSFactory(xFactory, uno::UNO_QUERY_THROW);
        comphelper::setProcessServiceFactory(xSFactory);

        // Same as vcl/workben/icontest.cxx: create the UCB for backwards
        // compatibility with code that still expects a default instance.
        ::ucb::UniversalContentBroker::create(comphelper::getProcessComponentContext());
    }
    catch (const uno::Exception& e)
    {
        fprintf(stderr, "warning: UNO bootstrap failed, continuing without it: %s\n",
            OUStringToOString(e.Message, osl_getThreadTextEncoding()).getStr());
    }
}

int CodeApplication::Main()
{
    try
    {
        VclPtrInstance<CodeMainWindow> xMainWindow(nullptr);
        xMainWindow->SetSizePixel(Size(1200, 800));
        xMainWindow->Show();

        Execute();

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
