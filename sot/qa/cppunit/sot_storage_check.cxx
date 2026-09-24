/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Reads real OLE2 compound documents through sot.
//
// Upstream checks the same thing in test_sot.cxx, but that test is built on
// unotest's FiltersTest fixture, and unotest links vcl — so it cannot run until
// the GUI half of the tree is ported.  This is the part of it that does not
// need a toolkit: open every file in sot/qa/cppunit/data/pass/ as a SotStorage,
// walk the storages and streams inside, and read each stream twice — forwards
// and backwards, byte by byte — because a bug in the sector chain usually shows
// up only when reading out of order.
//
// It is a much stronger check than the UNO smoke test: that one only proves that
// a service can be instantiated, whereas this parses real files with real
// defects in their history (the names are bug numbers from the tracker).

#include <sal/config.h>

#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/bootstrap.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <sot/storage.hxx>
#include <sot/storinfo.hxx>

using namespace css;
#include <tools/stream.hxx>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{
int g_failures = 0;
int g_files = 0;
int g_streams = 0;
int g_shortReads = 0;

void fail(const std::string& what)
{
    std::printf("  [FAILED] %s\n", what.c_str());
    ++g_failures;
}

// Reads one stream twice, in both directions, comparing against the first pass.
// Upstream's checkStream does the same; the backwards pass is what catches a
// broken sector chain.
bool checkStream(const rtl::Reference<SotStorage>& storage, const OUString& name, sal_uInt64 size)
{
    ++g_streams;

    rtl::Reference<SotStorageStream> stream(storage->OpenSotStream(name));
    if (!stream.is() || storage->GetError())
    {
        fail("cannot open stream");
        return false;
    }

    std::vector<unsigned char> data(size);
    sal_uInt64 readable = 0;
    if (size > 0)
    {
        stream->Seek(0);
        const sal_uInt64 remaining = stream->GetSize() - stream->Tell();
        if (remaining != size)
        {
            fail("stream reports a different size than the directory entry");
            return false;
        }
    }

    // A short read is not a failure here.  Several of the files under data/pass
    // are damaged on purpose — badchain-1.compound has a directory sector that
    // is not even a directory — and upstream reads them the same way, taking as
    // much as it can and checking only that what came back reads the same
    // again.  Requiring the full size would report those files as broken when
    // the point of keeping them is that they still open.
    readable = size > 0 ? stream->ReadBytes(data.data(), size) : 0;
    if (readable < size)
        ++g_shortReads;

    // Now read it back one byte at a time, from the end.
    rtl::Reference<SotStorageStream> backwards(storage->OpenSotStream(name));
    if (backwards.is())
    {
        for (sal_uInt64 i = readable; i > 0; --i)
        {
            backwards->Seek(i - 1);
            unsigned char c = 0;
            if (backwards->ReadBytes(&c, 1) != 1 || c != data[i - 1])
            {
                fail("byte read backwards differs from the forward read");
                return false;
            }
        }
    }

    return true;
}

bool checkStorage(const rtl::Reference<SotStorage>& storage, const std::string& indent)
{
    SvStorageInfoList entries;
    storage->FillInfoList(&entries);

    for (const SvStorageInfo& entry : entries)
    {
        const OUString name = entry.GetName();
        const std::string narrow
            = std::string(indent) + rtl::OUStringToOString(name, RTL_TEXTENCODING_UTF8).getStr();

        if (entry.IsStorage())
        {
            rtl::Reference<SotStorage> child(storage->OpenSotStorage(name));
            if (!child.is() || storage->GetError())
            {
                fail("cannot open substorage " + narrow);
                continue;
            }
            if (!checkStorage(child, indent + "  "))
                fail("in substorage " + narrow);
        }
        else if (entry.IsStream())
        {
            std::printf("  %s (%" SAL_PRIuUINT64 " bytes)\n", narrow.c_str(),
                        static_cast<sal_uInt64>(entry.GetSize()));
            if (!checkStream(storage, name, entry.GetSize()))
                fail("in stream " + narrow);
        }
    }
    return true;
}

bool checkFile(const std::string& path)
{
    std::printf("%s\n", path.c_str());
    ++g_files;

    SvFileStream file(rtl::OUString::createFromAscii(path.c_str()), StreamMode::READ);
    if (file.GetError())
    {
        fail("cannot open file");
        return false;
    }

    rtl::Reference<SotStorage> storage = new SotStorage(file);
    if (!storage.is() || storage->GetError())
    {
        fail("cannot open as an OLE2 storage");
        return false;
    }
    if (!storage->Validate())
    {
        fail("storage does not validate");
        return false;
    }

    return checkStorage(storage, "");
}

// The concrete expectation upstream records in test_sot.cxx's testSize: the
// Book stream of this file is 13312 bytes, and seeking behaves.
bool checkKnownSizes(const std::string& passDir)
{
    const std::string path = passDir + "/fdo84229-1.compound";
    std::printf("%s (known sizes)\n", path.c_str());

    SvFileStream file(rtl::OUString::createFromAscii(path.c_str()), StreamMode::READ);
    rtl::Reference<SotStorage> storage = new SotStorage(file);
    if (!storage.is() || storage->GetError())
    {
        fail("cannot open fdo84229-1.compound");
        return false;
    }

    rtl::Reference<SotStorageStream> book
        = storage->OpenSotStream(rtl::OUString::createFromAscii("Book"));
    if (!book.is() || storage->GetError() || book->GetError())
    {
        fail("cannot open the Book stream");
        return false;
    }

    const sal_uInt64 size = book->GetSize();
    if (size != 13312)
    {
        fail("Book stream is " + std::to_string(size) + " bytes, expected 13312");
        return false;
    }
    std::printf("  Book: %" SAL_PRIuUINT64 " bytes\n", static_cast<sal_uInt64>(size));

    book->Seek(STREAM_SEEK_TO_END);
    if (book->GetError() || book->Tell() != book->GetSize())
        fail("seeking to the end misbehaves");
    book->Seek(STREAM_SEEK_TO_BEGIN);
    if (book->GetError() || book->Tell() != 0)
        fail("seeking to the beginning misbehaves");

    return true;
}
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf("usage: %s <sot/qa/cppunit/data>\n", argv[0]);
        return 2;
    }

    const std::string dataDir = argv[1];
    const std::string passDir = dataDir + "/pass";
    std::printf("sot storage check\n");

    // SotStorage reaches for the UNO context (it is built on the embed services),
    // so the process has to be bootstrapped first — upstream's fixture does the
    // same thing before running the test body.
    try
    {
        css::uno::Reference<css::uno::XComponentContext> context
            = cppu::defaultBootstrap_InitialComponentContext();
        if (!context.is())
        {
            std::printf("cannot bootstrap the UNO context\n");
            return 2;
        }

        // Publishing the context process-wide is not optional here.  The
        // upper-casing in StgEntry::Load goes through CharClass, which asks
        // comphelper::getProcessComponentContext() for the i18n service rather
        // than taking a context as a parameter — inside the office the desktop
        // has already set that up.  desktop/qa/desktop_app/test_desktop_app.cxx
        // is the same three lines for the same reason.
        css::uno::Reference<css::lang::XMultiComponentFactory> factory(
            context->getServiceManager());
        css::uno::Reference<css::lang::XMultiServiceFactory> serviceManager(
            factory, css::uno::UNO_QUERY_THROW);
        comphelper::setProcessServiceFactory(serviceManager);
    }
    catch (const css::uno::Exception& e)
    {
        std::printf("cannot bootstrap the UNO context: %s\n",
                    rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
        return 2;
    }

    // The directory listing is read through the shell rather than a directory
    // API: sot has no such thing and inventing one here would be testing the
    // wrong layer.
    const std::string command = "ls " + passDir + "/*.compound 2>/dev/null";
    FILE* listing = popen(command.c_str(), "r");
    if (!listing)
    {
        std::printf("cannot list %s\n", passDir.c_str());
        return 2;
    }

    std::vector<std::string> files;
    char line[4096];
    while (fgets(line, sizeof line, listing))
    {
        std::string name(line);
        while (!name.empty() && (name.back() == '\n' || name.back() == '\r'))
            name.pop_back();
        if (!name.empty())
            files.push_back(name);
    }
    pclose(listing);

    if (files.empty())
    {
        std::printf("no .compound files found in %s\n", passDir.c_str());
        return 2;
    }

    for (const std::string& file : files)
        checkFile(file);

    checkKnownSizes(passDir);

    std::printf("\n%d files, %d streams, %d failures, %d streams short\n", g_files, g_streams,
                g_failures, g_shortReads);
    std::printf("%s\n", g_failures == 0 ? "SOT CHECK PASSED" : "SOT CHECK FAILED");
    return g_failures == 0 ? 0 : 1;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
