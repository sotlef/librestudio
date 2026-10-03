/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The semantic-token controller with a provider of the test's own: what it
// does with the documents' versions, and with typing.

#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/semantic/SemanticTokenEvent.hpp>
#include <com/sun/star/code/semantic/SemanticTokens.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenController.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenListener.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenProvider.hpp>
#include <com/sun/star/code/semantic/theSemanticTokenController.hpp>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/implbase.hxx>
#include <osl/file.hxx>
#include <rtl/ref.hxx>

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

using namespace css;
using namespace css::code;
using namespace css::code::semantic;

namespace
{
// Answers with one token whose length is the text's, after a delay it is told.
class FakeProvider final : public cppu::WeakImplHelper<XSemanticTokenProvider>
{
public:
    OUString SAL_CALL getName() override { return u"fake"_ustr; }
    uno::Sequence<OUString> SAL_CALL getSupportedMimeTypes() override
    {
        return { u"text/x-c++src"_ustr };
    }
    SemanticTokens SAL_CALL provideSemanticTokens(const OUString&, const OUString& rText,
                                                  sal_Int32 nVersion) override
    {
        ++m_nCalls;
        std::this_thread::sleep_for(std::chrono::milliseconds(m_nDelay));
        SemanticToken aToken;
        aToken.Length = rText.getLength();
        SemanticTokens aResult;
        aResult.Version = nVersion;
        aResult.Tokens = { aToken };
        return aResult;
    }

    std::atomic<int> m_nCalls{ 0 };
    std::atomic<int> m_nDelay{ 0 };
};

class Listener final : public cppu::WeakImplHelper<XSemanticTokenListener>
{
public:
    void SAL_CALL tokensChanged(const SemanticTokenEvent& rEvent) override
    {
        std::scoped_lock aGuard(m_aMutex);
        m_aVersions.push_back(rEvent.Version);
        m_aCondition.notify_all();
    }
    void SAL_CALL disposing(const lang::EventObject&) override {}

    // waits for an event of this version; false after the timeout
    bool WaitFor(sal_Int32 nVersion)
    {
        std::unique_lock aGuard(m_aMutex);
        return m_aCondition.wait_for(aGuard, std::chrono::seconds(5),
                                     [&]
                                     {
                                         for (sal_Int32 n : m_aVersions)
                                             if (n == nVersion)
                                                 return true;
                                         return false;
                                     });
    }
    std::vector<sal_Int32> Versions()
    {
        std::scoped_lock aGuard(m_aMutex);
        return m_aVersions;
    }

private:
    std::mutex m_aMutex;
    std::condition_variable m_aCondition;
    std::vector<sal_Int32> m_aVersions;
};

class SemanticTokenControllerTest : public CppUnit::TestFixture
{
public:
    void setUp() override
    {
        m_xContext = comphelper::getProcessComponentContext();
        m_xController = theSemanticTokenController::get(m_xContext);
        m_xDocuments = document::theDocumentManager::get(m_xContext);
        m_xProvider = new FakeProvider;
        m_xListener = new Listener;
        m_xController->registerProvider(m_xProvider);
        m_xController->addSemanticTokenListener(m_xListener);

        CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, osl::FileBase::createTempFile(nullptr, nullptr, &m_aUrl));
        // a temporary file has no extension; the document takes its type from the name
        osl::File::move(m_aUrl, m_aUrl + ".cpp");
        m_aUrl += ".cpp";
        osl::File aFile(m_aUrl);
        CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, aFile.open(osl_File_OpenFlag_Write));
        sal_uInt64 nWritten;
        aFile.write("int a;", 6, nWritten);
        aFile.close();
    }

    void tearDown() override
    {
        m_xController->removeSemanticTokenListener(m_xListener);
        m_xController->unregisterProvider(u"fake"_ustr);
        m_xDocuments->closeDocument(m_aUrl, true);
        osl::File::remove(m_aUrl);
    }

    uno::Reference<document::XTextDocument> Open()
    {
        return uno::Reference<document::XTextDocument>(m_xDocuments->openDocument(m_aUrl),
                                                       uno::UNO_QUERY_THROW);
    }

    void testOpeningAnalysesTheDocument()
    {
        auto xDocument = Open();
        CPPUNIT_ASSERT(m_xListener->WaitFor(xDocument->getVersion()));
        const SemanticTokens aTokens = m_xController->getSemanticTokens(m_aUrl);
        CPPUNIT_ASSERT_EQUAL(xDocument->getVersion(), aTokens.Version);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(1), aTokens.Tokens.getLength());
        CPPUNIT_ASSERT_EQUAL(sal_Int32(6), aTokens.Tokens[0].Length);
    }

    void testNoTokensBeforeTheFirstAnalysis()
    {
        const SemanticTokens aNone = m_xController->getSemanticTokens(u"file:///no/such.cpp"_ustr);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(-1), aNone.Version);
        CPPUNIT_ASSERT(!aNone.Tokens.hasElements());
    }

    void testEveryEditRaisesTheVersion()
    {
        auto xDocument = Open();
        const sal_Int32 nBefore = xDocument->getVersion();
        xDocument->setText(u"int b;"_ustr);
        CPPUNIT_ASSERT(xDocument->getVersion() > nBefore);
        const sal_Int32 nMiddle = xDocument->getVersion();
        document::TextEdit aEdit;
        aEdit.NewText = u"x"_ustr;
        xDocument->applyEdit(aEdit);
        CPPUNIT_ASSERT(xDocument->getVersion() > nMiddle);
    }

    void testTypingIsOneAnalysis()
    {
        auto xDocument = Open();
        CPPUNIT_ASSERT(m_xListener->WaitFor(xDocument->getVersion()));
        const int nCalls = m_xProvider->m_nCalls;

        // three edits well within the pause
        xDocument->setText(u"int a1;"_ustr);
        xDocument->setText(u"int a12;"_ustr);
        xDocument->setText(u"int a123;"_ustr);
        CPPUNIT_ASSERT(m_xListener->WaitFor(xDocument->getVersion()));
        // let any wrongly started second analysis show itself
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        CPPUNIT_ASSERT_EQUAL(nCalls + 1, m_xProvider->m_nCalls.load());
        CPPUNIT_ASSERT_EQUAL(sal_Int32(9), m_xController->getSemanticTokens(m_aUrl).Tokens[0].Length);
    }

    void testAnOutdatedAnswerIsDropped()
    {
        auto xDocument = Open();
        CPPUNIT_ASSERT(m_xListener->WaitFor(xDocument->getVersion()));

        // The provider takes long; the document changes while it works.
        m_xProvider->m_nDelay = 1000;
        xDocument->setText(u"int slow;"_ustr);
        const sal_Int32 nSlow = xDocument->getVersion();
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // the analysis is running
        m_xProvider->m_nDelay = 0;
        xDocument->setText(u"int fast;"_ustr);
        const sal_Int32 nFast = xDocument->getVersion();

        CPPUNIT_ASSERT(m_xListener->WaitFor(nFast));
        std::this_thread::sleep_for(std::chrono::milliseconds(1200)); // the slow one finishes
        for (sal_Int32 n : m_xListener->Versions())
            CPPUNIT_ASSERT(n != nSlow);
        CPPUNIT_ASSERT_EQUAL(nFast, m_xController->getSemanticTokens(m_aUrl).Version);
    }

    void testClosingForgetsTheTokens()
    {
        auto xDocument = Open();
        CPPUNIT_ASSERT(m_xListener->WaitFor(xDocument->getVersion()));
        CPPUNIT_ASSERT(m_xDocuments->closeDocument(m_aUrl, true));
        CPPUNIT_ASSERT_EQUAL(sal_Int32(-1), m_xController->getSemanticTokens(m_aUrl).Version);
    }

    void testNoProviderForTheType()
    {
        m_xController->unregisterProvider(u"fake"_ustr);
        const int nCalls = m_xProvider->m_nCalls;
        auto xDocument = Open();
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        CPPUNIT_ASSERT_EQUAL(nCalls, m_xProvider->m_nCalls.load());
        CPPUNIT_ASSERT_EQUAL(sal_Int32(-1), m_xController->getSemanticTokens(m_aUrl).Version);
    }

    CPPUNIT_TEST_SUITE(SemanticTokenControllerTest);
    CPPUNIT_TEST(testOpeningAnalysesTheDocument);
    CPPUNIT_TEST(testNoTokensBeforeTheFirstAnalysis);
    CPPUNIT_TEST(testEveryEditRaisesTheVersion);
    CPPUNIT_TEST(testTypingIsOneAnalysis);
    CPPUNIT_TEST(testAnOutdatedAnswerIsDropped);
    CPPUNIT_TEST(testClosingForgetsTheTokens);
    CPPUNIT_TEST(testNoProviderForTheType);
    CPPUNIT_TEST_SUITE_END();

private:
    uno::Reference<uno::XComponentContext> m_xContext;
    uno::Reference<XSemanticTokenController> m_xController;
    uno::Reference<document::XDocumentManager> m_xDocuments;
    rtl::Reference<FakeProvider> m_xProvider;
    rtl::Reference<Listener> m_xListener;
    OUString m_aUrl;
};

CPPUNIT_TEST_SUITE_REGISTRATION(SemanticTokenControllerTest);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
