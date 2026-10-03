/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/output/XOutputFilter.hpp>
#include <com/sun/star/code/output/theOutputController.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>

#include <regex>
#include <string>

using namespace css;
using namespace css::code;

namespace codeservices
{
namespace
{
core::Severity SeverityOf(const std::string& rWord)
{
    if (rWord == "fatal error")
        return core::Severity_FATAL;
    if (rWord == "error")
        return core::Severity_ERROR;
    if (rWord == "warning")
        return core::Severity_WARNING;
    return core::Severity_INFO; // "note"
}

// Recognises the diagnostics of the compilers a build prints:
//   gcc/clang  path:line[:col]: error: text [-Wflag]
//   MSVC       path(line[,col]) : error C2065: text
// and turns each into an Error List entry.
class CompilerOutputFilter final
    : public cppu::WeakImplHelper<output::XOutputFilter>
{
public:
    OUString SAL_CALL getName() override { return u"compiler"_ustr; }
    uno::Sequence<OUString> SAL_CALL getApplicableChannels() override { return {}; }
    uno::Sequence<task::Task> SAL_CALL filterLine(const OUString& rChannel,
                                                  const output::OutputLine& rLine) override;
};

uno::Sequence<task::Task> CompilerOutputFilter::filterLine(const OUString& /*rChannel*/,
                                                            const output::OutputLine& rLine)
{
    static const std::regex aGcc(R"(^(.+?):(\d+):(?:(\d+):)?\s*(fatal error|error|warning|note):\s*(.*)$)");
    static const std::regex aMsvc(
        R"(^(.+?)\((\d+)(?:,(\d+))?\)\s*:\s*(fatal error|error|warning)\s+(\w+)\s*:\s*(.*)$)");

    const std::string aText = OUStringToOString(rLine.Text, RTL_TEXTENCODING_UTF8).getStr();
    std::smatch aMatch;

    task::Task aTask;
    aTask.Category = u"build"_ustr;
    OUString aPath;
    auto number = [](const std::ssub_match& rGroup) -> sal_Int32
    { return rGroup.matched ? std::stoi(rGroup.str()) - 1 : 0; }; // 1-based -> 0-based

    if (std::regex_match(aText, aMatch, aGcc))
    {
        aPath = OUString::fromUtf8(aMatch[1].str());
        aTask.Location.Line = number(aMatch[2]);
        aTask.Location.Column = number(aMatch[3]);
        aTask.Level = SeverityOf(aMatch[4].str());
        aTask.Description = OUString::fromUtf8(aMatch[5].str());
        aTask.Source = u"gcc"_ustr;
    }
    else if (std::regex_match(aText, aMatch, aMsvc))
    {
        aPath = OUString::fromUtf8(aMatch[1].str());
        aTask.Location.Line = number(aMatch[2]);
        aTask.Location.Column = number(aMatch[3]);
        aTask.Level = SeverityOf(aMatch[4].str());
        aTask.Code = OUString::fromUtf8(aMatch[5].str());
        aTask.Description = OUString::fromUtf8(aMatch[6].str());
        aTask.Source = u"msvc"_ustr;
    }
    else
        return {};

    // A path that does not convert (e.g. "<command-line>") is still a diagnostic,
    // just one with nowhere to jump to.
    OUString aUrl;
    if (osl::FileBase::getFileURLFromSystemPath(aPath, aUrl) == osl::FileBase::E_None)
        aTask.Url = aUrl;
    return { aTask };
}

class CompilerOutputPlugin final
    : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit CompilerOutputPlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        output::theOutputController::get(m_xContext)->registerFilter(new CompilerOutputFilter);
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.CompilerOutputPlugin"_ustr;
    }
    sal_Bool SAL_CALL supportsService(const OUString& rName) override
    {
        return cppu::supportsService(this, rName);
    }
    uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override
    {
        return { u"com.sun.star.code.core.Plugin"_ustr };
    }

private:
    uno::Reference<uno::XComponentContext> m_xContext;
};
}
}

extern "C" SAL_DLLPUBLIC_EXPORT uno::XInterface*
com_sun_star_comp_code_CompilerOutputPlugin_get_implementation(uno::XComponentContext* pContext,
                                                                uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::CompilerOutputPlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
