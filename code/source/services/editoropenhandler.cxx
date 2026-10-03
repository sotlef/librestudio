/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/code/core/XPlugin.hpp>
#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/frontend/XEditorHost.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/task/XTaskHandler.hpp>
#include <com/sun/star/code/task/theTaskHub.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>

using namespace css;
using namespace css::code;

namespace codeservices
{
namespace
{
// Activating a task that names a file opens it in the editor, with the caret
// on the task's position.
class EditorOpenHandler final : public cppu::WeakImplHelper<task::XTaskHandler>
{
public:
    explicit EditorOpenHandler(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    OUString SAL_CALL getName() override { return u"editor-open"_ustr; }
    uno::Sequence<OUString> SAL_CALL getCategories() override { return {}; }
    sal_Bool SAL_CALL canHandle(const task::Task& rTask) override
    {
        // Without a shell (headless) there is no editor, and saying so lets
        // handleTask() report false instead of pretending.
        return !rTask.Url.isEmpty() && editorHost().is();
    }
    void SAL_CALL handle(const task::Task& rTask) override
    {
        if (auto xHost = editorHost())
            xHost->revealPosition(rTask.Url, rTask.Location);
    }

private:
    uno::Reference<frontend::XEditorHost> editorHost() const
    {
        uno::Reference<frontend::XWorkbench> xWorkbench = core::theCode::get(m_xContext)->getWorkbench();
        return xWorkbench.is() ? xWorkbench->getEditorHost() : uno::Reference<frontend::XEditorHost>();
    }

    uno::Reference<uno::XComponentContext> m_xContext;
};

class EditorOpenPlugin final : public cppu::WeakImplHelper<core::XPlugin, lang::XServiceInfo>
{
public:
    explicit EditorOpenPlugin(uno::Reference<uno::XComponentContext> xContext)
        : m_xContext(std::move(xContext))
    {
    }

    void SAL_CALL activate() override
    {
        task::theTaskHub::get(m_xContext)->registerHandler(new EditorOpenHandler(m_xContext));
    }

    OUString SAL_CALL getImplementationName() override
    {
        return u"com.sun.star.comp.code.EditorOpenPlugin"_ustr;
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
com_sun_star_comp_code_EditorOpenPlugin_get_implementation(uno::XComponentContext* pContext,
                                                            uno::Sequence<uno::Any> const&)
{
    return cppu::acquire(new codeservices::EditorOpenPlugin(pContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
