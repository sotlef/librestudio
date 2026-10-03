/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "solutionpanel.hxx"

#include "../uibridge.hxx"

#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/frontend/XEditorHost.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/project/ItemKind.hpp>
#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectFiles.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <svtools/imagemgr.hxx>
#include <tools/urlobj.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/TreeView.hxx>
#include <vcl/weld/Widget.hxx>

using namespace css;
using namespace css::code::project;

namespace code::sidebar
{

namespace
{
// Generic folder icon (same one a real Open dialog shows for any plain
// directory) -- SvFileInformationManager::GetFolderImageId() only touches
// the filesystem for the remote/CD/removable-device flags in VolumeInfo, a
// default-constructed one (all false) is safe to use.
OUString FolderIcon()
{
    return SvFileInformationManager::GetFolderImageId(svtools::VolumeInfo());
}

// Per-file icon, keyed purely off the URL (GetFileImageId's bDetectFolder=false
// path never touches the filesystem).
OUString FileIcon(const OUString& rUrl)
{
    return SvFileInformationManager::GetFileImageId(INetURLObject(rUrl));
}
}

SolutionPanel::SolutionPanel(weld::Widget* pParent,
    css::uno::Reference<css::ui::XSidebar> xSidebar)
    : PanelLayout(pParent, u"SolutionPanel"_ustr, u"code/ui/solutionpanel.ui"_ustr)
    , mxTree(m_xBuilder->weld_tree_view(u"tree"_ustr))
    , mxSidebar(std::move(xSidebar))
{
    mxTree->connect_expanding(LINK(this, SolutionPanel, ExpandingHdl));
    mxTree->connect_item_activated(LINK(this, SolutionPanel, ActivatedHdl));

    // Projects open and close on any thread; the tree is only touched on this one.
    mxNotifier = std::make_shared<::code::UiNotifier>([this] { Rebuild(); });
    mxListener = new ::code::ServiceListener(mxNotifier);
    try
    {
        css::code::project::theProjectManager::get(comphelper::getProcessComponentContext())
            ->addProjectListener(mxListener);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "Solution Explorer cannot reach the project manager: " << rException.Message);
    }

    Rebuild();
}

SolutionPanel::~SolutionPanel()
{
    mxNotifier->Detach();
    try
    {
        css::code::project::theProjectManager::get(comphelper::getProcessComponentContext())
            ->removeProjectListener(mxListener);
    }
    catch (const uno::Exception&)
    {
    }
}

void SolutionPanel::InsertItem(const weld::TreeIter* pParent,
                               const uno::Reference<XProjectItem>& xItem, const OUString& rLabel)
{
    const ItemKind eKind = xItem->getKind();
    const bool bFile = eKind == ItemKind_FILE;
    const OUString aId = OUString::number(mnNextId++);
    maItems[aId] = xItem;

    const OUString aIcon = bFile ? FileIcon(xItem->getUrl()) : FolderIcon();
    std::unique_ptr<weld::TreeIter> xRow(mxTree->make_iterator());
    // Everything that is not a file can have children; they are read when the
    // row is expanded, not before.
    mxTree->insert(pParent, -1, &rLabel, &aId, nullptr, nullptr, !bFile, xRow.get());
    mxTree->set_image(*xRow, aIcon);
}

void SolutionPanel::Rebuild()
{
    mxTree->freeze();
    mxTree->clear();
    maItems.clear();

    uno::Sequence<uno::Reference<XProject>> aProjects;
    try
    {
        aProjects = css::code::project::theProjectManager::get(comphelper::getProcessComponentContext())
                        ->getOpenProjects();
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot list the open projects: " << rException.Message);
    }

    if (!aProjects.hasElements())
    {
        // A row without an id: nothing to open behind it.
        const OUString aHint(u"No folder open (File > Open Folder)"_ustr);
        mxTree->insert(nullptr, -1, &aHint, nullptr, nullptr, nullptr, false, nullptr);
    }

    for (const auto& xProject : aProjects)
    {
        uno::Reference<XProjectFiles> xFiles(xProject, uno::UNO_QUERY);
        if (!xFiles.is())
            continue; // a project with no file tree has nothing to show here
        InsertItem(nullptr, xFiles->getRootItem(), xProject->getName());
    }
    mxTree->thaw();

    // Open the project rows, the way a solution opens: you see its top level.
    std::unique_ptr<weld::TreeIter> xIter(mxTree->make_iterator());
    if (mxTree->get_iter_first(*xIter))
    {
        do
        {
            if (!mxTree->get_id(*xIter).isEmpty())
                mxTree->expand_row(*xIter);
        } while (mxTree->iter_next_sibling(*xIter));
    }

    if (mxSidebar.is())
        mxSidebar->requestLayout();
}

IMPL_LINK(SolutionPanel, ExpandingHdl, const weld::TreeIter&, rIter, bool)
{
    const auto it = maItems.find(mxTree->get_id(rIter));
    if (it == maItems.end())
        return true;

    try
    {
        for (const auto& xChild : it->second->getChildren())
            InsertItem(&rIter, xChild, xChild->getName());
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot list " << it->second->getUrl() << ": " << rException.Message);
    }
    return true;
}

IMPL_LINK(SolutionPanel, ActivatedHdl, const weld::TreeIter&, rIter, bool)
{
    const auto it = maItems.find(mxTree->get_id(rIter));
    if (it == maItems.end() || it->second->getKind() != ItemKind_FILE)
        return false; // a folder: let the tree expand or collapse it

    try
    {
        uno::Reference<css::code::frontend::XWorkbench> xWorkbench
            = css::code::core::theCode::get(comphelper::getProcessComponentContext())->getWorkbench();
        if (xWorkbench.is() && xWorkbench->getEditorHost().is())
            xWorkbench->getEditorHost()->openDocument(it->second->getUrl());
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot open " << it->second->getUrl() << ": " << rException.Message);
    }
    return true;
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
