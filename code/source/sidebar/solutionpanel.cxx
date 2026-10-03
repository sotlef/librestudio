/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "solutionpanel.hxx"

#include <com/sun/star/code/project/ItemKind.hpp>
#include <com/sun/star/code/project/XProjectFiles.hpp>
#include <com/sun/star/code/project/XProjectTargets.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Menu.hxx>
#include <vcl/weld/Widget.hxx>
#include <com/sun/star/code/build/XBuildController.hpp>
#include <com/sun/star/code/build/XBuildable.hpp>
#include <com/sun/star/code/build/XCompilable.hpp>
#include <com/sun/star/code/build/theBuildController.hpp>
#include <com/sun/star/code/document/XDocumentManager.hpp>
#include <com/sun/star/code/document/theDocumentManager.hpp>
#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/frontend/XEditorHost.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <vector>

using namespace css;
using namespace css::code::project;

namespace code::sidebar
{

namespace
{
// The solution: the one row above a project's targets.  It is the panel's own
// notion (a project says what its targets are, not that they sit in a
// solution), so it is made here, from the targets, rather than by the project.
class SolutionItem final : public cppu::WeakImplHelper<XProjectItem>
{
public:
    SolutionItem(OUString aName, uno::Reference<XProject> xProject, const uno::Sequence<uno::Reference<XProjectTarget>>& rTargets)
        : m_aName(std::move(aName))
        , m_xProject(std::move(xProject))
    {
        for (const auto& xTarget : rTargets)
            m_aChildren.push_back(uno::Reference<XProjectItem>(xTarget, uno::UNO_QUERY));
    }

    /// the solution is built by building its project
    const uno::Reference<XProject>& GetProject() const { return m_xProject; }

    OUString SAL_CALL getName() override { return m_aName; }
    ItemKind SAL_CALL getKind() override { return ItemKind_FOLDER; }
    OUString SAL_CALL getUrl() override { return {}; }
    uno::Reference<XProjectItem> SAL_CALL getParent() override { return {}; }
    uno::Sequence<uno::Reference<XProjectItem>> SAL_CALL getChildren() override
    {
        return comphelper::containerToSequence(m_aChildren);
    }

private:
    const OUString m_aName;
    const uno::Reference<XProject> m_xProject;
    std::vector<uno::Reference<XProjectItem>> m_aChildren;
};
}

SolutionPanel::SolutionPanel(weld::Widget* pParent, uno::Reference<ui::XSidebar> xSidebar)
    : ProjectTreePanel(pParent, u"SolutionPanel"_ustr, u"code/ui/solutionpanel.ui"_ustr, std::move(xSidebar))
{
    Start();
}

uno::Reference<XProjectItem> SolutionPanel::RootOf(const uno::Reference<XProject>& xProject)
{
    // the project's targets, once it has any (a configured CMake project) ...
    if (uno::Reference<XProjectTargets> xTargets{ xProject, uno::UNO_QUERY })
    {
        const uno::Sequence<uno::Reference<XProjectTarget>> aTargets = xTargets->getTargets();
        if (aTargets.hasElements())
            return new SolutionItem(xProject->getName(), xProject, aTargets);
    }
    // ... otherwise the directory tree
    if (uno::Reference<XProjectFiles> xFiles{ xProject, uno::UNO_QUERY })
        return xFiles->getRootItem();
    return {};
}

bool SolutionPanel::ShowContextMenu(const uno::Reference<XProjectItem>& xItem, weld::Widget& rParent,
                                    const tools::Rectangle& rArea)
{
    const uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
    // what an item can do is what it can be asked for: no interface, no command
    const SolutionItem* pSolution = dynamic_cast<SolutionItem*>(xItem.get());
    const uno::Reference<css::code::build::XBuildable> xBuildable(pSolution ? uno::Reference<uno::XInterface>(pSolution->GetProject())
                                                                            : uno::Reference<uno::XInterface>(xItem),
                                                                  uno::UNO_QUERY);
    const uno::Reference<css::code::build::XCompilable> xCompilable(xItem, uno::UNO_QUERY);
    const bool bFile = xItem->getKind() == ItemKind_FILE;
    if (!xBuildable.is() && !xCompilable.is() && !bFile)
        return false;

    std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(&rParent, u"code/ui/solutionpanel.ui"_ustr));
    std::unique_ptr<weld::Menu> xMenu = xBuilder->weld_menu(u"itemmenu"_ustr);
    const bool bBuilding = [&]
    {
        try
        {
            return css::code::build::theBuildController::get(xContext)->getRunningBuild().is();
        }
        catch (const uno::Exception&)
        {
            return false;
        }
    }();
    // one build at a time: while one runs, the commands that start one are off
    if (!bFile)
        xMenu->remove(u"open"_ustr);
    if (!bFile || !xCompilable.is())
        xMenu->remove(u"compile"_ustr);
    if (!bFile || !xCompilable.is())
        xMenu->remove(u"filesep"_ustr);
    if (!xBuildable.is())
        for (const OUString& rIdent : { u"build"_ustr, u"rebuild"_ustr, u"clean"_ustr })
            xMenu->remove(rIdent);
    else if (bBuilding)
        for (const OUString& rIdent : { u"build"_ustr, u"rebuild"_ustr, u"clean"_ustr })
            xMenu->set_sensitive(rIdent, false);
    if (xCompilable.is() && bBuilding)
        xMenu->set_sensitive(u"compile"_ustr, false);

    const OUString aCommand = xMenu->popup_at_rect(&rParent, rArea);
    if (aCommand.isEmpty())
        return true;
    try
    {
        if (aCommand == "open")
        {
            uno::Reference<css::code::frontend::XWorkbench> xWorkbench
                = css::code::core::theCode::get(xContext)->getWorkbench();
            if (xWorkbench.is() && xWorkbench->getEditorHost().is())
                xWorkbench->getEditorHost()->openDocument(xItem->getUrl());
            return true;
        }
        // what is on disk is what gets built
        css::code::document::theDocumentManager::get(xContext)->saveAll();
        auto xBuild = css::code::build::theBuildController::get(xContext);
        if (aCommand == "rebuild")
            xBuild->rebuild(xBuildable);
        else if (aCommand == "clean")
            xBuild->clean(xBuildable);
        else if (aCommand == "compile")
            xBuild->compile(xCompilable);
        else
            xBuild->build(xBuildable);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "menu command failed: " << rException.Message);
    }
    return true;
}

OUString SolutionPanel::EmptyText() const { return u"No project open (File > Open > Project)"_ustr; }

OUString SolutionPanel::LabelOf(const uno::Reference<XProject>& xProject, const uno::Reference<XProjectItem>& xRoot) const
{
    // the directory tree's root is the project itself; the solution row says what it is
    return dynamic_cast<SolutionItem*>(xRoot.get()) ? "Solution '" + xProject->getName() + "'" : xProject->getName();
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
