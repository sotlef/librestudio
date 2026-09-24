/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// End-to-end check of the UNO core built by CMake.
//
// Building and linking the libraries proves very little on its own: the pieces
// that make UNO work — the bootstrap reading unorc, the service manager finding
// a component through services.rdb, the shared-library loader dlopen'ing it, and
// the C++ bridge marshalling a call across the ABI — only meet at run time.  So
// this program does what an application does:
//
//   1. defaultBootstrap_InitialComponentContext() reads unorc next to
//      libcppuhelpergcc3.so and opens the registries it names;
//   2. the service manager resolves a service name through services.rdb;
//   3. the component's library is loaded and instantiated;
//   4. a method is called across the bridge and the result checked.
//
// A failure at any step names the step, so the output says what is broken.

#include <sal/config.h>

#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/reflection/XIdlReflection.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/bootstrap.hxx>

#include <cstdio>
#include <string_view>

using namespace css;

namespace
{
int g_failures = 0;

void check(bool condition, std::string_view what)
{
    std::printf("  [%s] %s\n", condition ? "ok" : "FAILED", std::string(what).c_str());
    if (!condition)
        ++g_failures;
}

// createInstanceWithContext is where the whole chain meets: the name is looked
// up in services.rdb, the uri attribute is expanded through unorc, the library
// is loaded, and the component factory is called through the C++ bridge.
uno::Reference<uno::XInterface> create(const uno::Reference<lang::XMultiComponentFactory>& factory,
                                       const uno::Reference<uno::XComponentContext>& context,
                                       const char* name)
{
    return factory->createInstanceWithContext(
        rtl::OUString::createFromAscii(name), context);
}
}

int main()
{
    std::printf("LibreStudio UNO core smoke test\n");

    std::printf("bootstrap:\n");
    uno::Reference<uno::XComponentContext> context;
    try
    {
        context = cppu::defaultBootstrap_InitialComponentContext();
    }
    catch (const uno::Exception& e)
    {
        std::printf("  [FAILED] defaultBootstrap_InitialComponentContext: %s\n",
                    rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        return 1;
    }
    check(context.is(), "component context created");

    uno::Reference<lang::XMultiComponentFactory> factory = context->getServiceManager();
    check(factory.is(), "service manager obtained");

    std::printf("components from services.rdb:\n");

    // stoc: the reflection service, exercising the loader and the bridge.
    uno::Reference<uno::XInterface> reflection
        = create(factory, context, "com.sun.star.reflection.CoreReflection");
    check(reflection.is(), "com.sun.star.reflection.CoreReflection");

    // io: a service with real state, so the calls below go through the bridge.
    uno::Reference<uno::XInterface> pipeUnk = create(factory, context, "com.sun.star.io.Pipe");
    check(pipeUnk.is(), "com.sun.star.io.Pipe");

    if (pipeUnk.is())
    {
        uno::Reference<io::XOutputStream> out(pipeUnk, uno::UNO_QUERY);
        uno::Reference<io::XInputStream> in(pipeUnk, uno::UNO_QUERY);
        check(out.is() && in.is(), "Pipe implements XOutputStream and XInputStream");

        if (out.is() && in.is())
        {
            const uno::Sequence<sal_Int8> data{ 'u', 'n', 'o' };
            out->writeBytes(data);
            out->closeOutput();

            uno::Sequence<sal_Int8> read;
            const sal_Int32 count = in->readBytes(read, 3);
            check(count == 3 && read.getLength() == 3 && read[0] == 'u' && read[1] == 'n'
                      && read[2] == 'o',
                  "bytes round-tripped through the pipe service");
        }
    }

    std::printf("%s\n", g_failures == 0 ? "SMOKE TEST PASSED" : "SMOKE TEST FAILED");
    return g_failures == 0 ? 0 : 1;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
