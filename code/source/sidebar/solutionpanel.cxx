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
    SolutionItem(OUString aName, const uno::Sequence<uno::Reference<XProjectTarget>>& rTargets)
        : m_aName(std::move(aName))
    {
        for (const auto& xTarget : rTargets)
            m_aChildren.push_back(uno::Reference<XProjectItem>(xTarget, uno::UNO_QUERY));
    }

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
            return new SolutionItem(xProject->getName(), aTargets);
    }
    // ... otherwise the directory tree
    if (uno::Reference<XProjectFiles> xFiles{ xProject, uno::UNO_QUERY })
        return xFiles->getRootItem();
    return {};
}

OUString SolutionPanel::EmptyText() const { return u"No project open (File > Open > Project)"_ustr; }

OUString SolutionPanel::LabelOf(const uno::Reference<XProject>& xProject, const uno::Reference<XProjectItem>& xRoot) const
{
    // the directory tree's root is the project itself; the solution row says what it is
    return dynamic_cast<SolutionItem*>(xRoot.get()) ? "Solution '" + xProject->getName() + "'" : xProject->getName();
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
