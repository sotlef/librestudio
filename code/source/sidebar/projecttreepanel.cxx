/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "projecttreepanel.hxx"

#include "../uibridge.hxx"

#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/frontend/XEditorHost.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/project/ItemKind.hpp>
#include <com/sun/star/code/project/TargetType.hpp>
#include <com/sun/star/code/project/XProjectManager.hpp>
#include <com/sun/star/code/project/XProjectTarget.hpp>
#include <com/sun/star/code/project/theProjectManager.hpp>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <svtools/imagemgr.hxx>
#include <tools/urlobj.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/TreeView.hxx>
#include <vcl/weld/Widget.hxx>

using namespace css;
using namespace css::code::project;

namespace code::sidebar
{

namespace
{
// Generic folder icon (the one a real Open dialog shows for any plain
// directory) -- SvFileInformationManager::GetFolderImageId() only touches the
// filesystem for the remote/CD/removable-device flags in VolumeInfo, a
// default-constructed one (all false) is safe to use.
OUString FolderIcon() { return SvFileInformationManager::GetFolderImageId(svtools::VolumeInfo()); }

// Per-file icon, keyed purely off the URL (GetFileImageId's bDetectFolder=false
// path never touches the filesystem).
OUString FileIcon(const OUString& rUrl) { return SvFileInformationManager::GetFileImageId(INetURLObject(rUrl)); }
}

ProjectTreePanel::ProjectTreePanel(weld::Widget* pParent, const OUString& rPanelId,
                                   const OUString& rUIFile, css::uno::Reference<css::ui::XSidebar> xSidebar)
    : PanelLayout(pParent, rPanelId, rUIFile)
    , mxTree(m_xBuilder->weld_tree_view(u"tree"_ustr))
    , mxSidebar(std::move(xSidebar))
{
    mxTree->connect_expanding(LINK(this, ProjectTreePanel, ExpandingHdl));
    mxTree->connect_item_activated(LINK(this, ProjectTreePanel, ActivatedHdl));
    mxTree->connect_command(LINK(this, ProjectTreePanel, CommandHdl));
}

ProjectTreePanel::~ProjectTreePanel()
{
    if (mxNotifier)
        mxNotifier->Detach();
    try
    {
        if (mxListener.is())
            theProjectManager::get(comphelper::getProcessComponentContext())->removeProjectListener(mxListener);
    }
    catch (const uno::Exception&)
    {
    }
}

void ProjectTreePanel::Start()
{
    // Projects open, close and change on any thread; the tree is only touched on this one.
    mxNotifier = std::make_shared<::code::UiNotifier>([this] { Rebuild(); });
    mxListener = new ::code::ServiceListener(mxNotifier);
    try
    {
        theProjectManager::get(comphelper::getProcessComponentContext())->addProjectListener(mxListener);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "the project tree cannot reach the project manager: " << rException.Message);
    }
    Rebuild();
}

OUString ProjectTreePanel::LabelOf(const uno::Reference<XProject>& xProject,
                                   const uno::Reference<XProjectItem>&) const
{
    return xProject->getName();
}

void ProjectTreePanel::InsertItem(const weld::TreeIter* pParent, const uno::Reference<XProjectItem>& xItem,
                                  const OUString& rLabel)
{
    const ItemKind eKind = xItem->getKind();
    const OUString aId = OUString::number(mnNextId++);
    maItems[aId] = xItem;

    // A file shows its own icon; every other kind of item is a folder as far
    // as the tree is concerned.
    const OUString aIcon = eKind == ItemKind_FILE ? FileIcon(xItem->getUrl()) : FolderIcon();
    // Anything that is not a file can have children; they are read when the
    // row is expanded, not before.
    const bool bExpandable = eKind != ItemKind_FILE;
    std::unique_ptr<weld::TreeIter> xRow(mxTree->make_iterator());
    mxTree->insert(pParent, -1, &rLabel, &aId, nullptr, nullptr, bExpandable, xRow.get());
    mxTree->set_image(*xRow, aIcon);
}

void ProjectTreePanel::Rebuild()
{
    mxTree->freeze();
    mxTree->clear();
    maItems.clear();
    maPopulated.clear();

    uno::Sequence<uno::Reference<XProject>> aProjects;
    try
    {
        aProjects = theProjectManager::get(comphelper::getProcessComponentContext())->getOpenProjects();
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot list the open projects: " << rException.Message);
    }

    bool bAny = false;
    for (const auto& xProject : aProjects)
    {
        uno::Reference<XProjectItem> xRoot = RootOf(xProject);
        if (!xRoot.is())
            continue; // this project has nothing to show here
        InsertItem(nullptr, xRoot, LabelOf(xProject, xRoot));
        bAny = true;
    }
    if (!bAny)
    {
        // A row without an id: nothing to open behind it.
        const OUString aHint = EmptyText();
        mxTree->insert(nullptr, -1, &aHint, nullptr, nullptr, nullptr, false, nullptr);
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

IMPL_LINK(ProjectTreePanel, ExpandingHdl, const weld::TreeIter&, rIter, bool)
{
    const OUString aRowId = mxTree->get_id(rIter);
    const auto it = maItems.find(aRowId);
    if (it == maItems.end())
        return true;
    // read a row's children once
    if (!maPopulated.insert(aRowId).second)
        return true;

    try
    {
        for (const auto& xChild : it->second->getChildren())
            InsertItem(&rIter, xChild, xChild->getName());
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot list " << it->second->getName() << ": " << rException.Message);
    }
    return true;
}

bool ProjectTreePanel::ShowContextMenu(const uno::Reference<XProjectItem>&, weld::Widget&,
                                       const tools::Rectangle&)
{
    return false;
}

IMPL_LINK(ProjectTreePanel, CommandHdl, const CommandEvent&, rEvent, bool)
{
    if (rEvent.GetCommand() != CommandEventId::ContextMenu)
        return false;
    // a keyboard-invoked menu has no position: use the cursor row
    std::unique_ptr<weld::TreeIter> xRow = rEvent.IsMouseEvent()
        ? mxTree->get_dest_row_at_pos(rEvent.GetMousePosPixel(), false, false)
        : mxTree->get_cursor();
    if (!xRow)
        return false;
    const auto it = maItems.find(mxTree->get_id(*xRow));
    if (it == maItems.end())
        return false;
    mxTree->select(*xRow);
    const tools::Rectangle aArea = rEvent.IsMouseEvent()
                                       ? tools::Rectangle(rEvent.GetMousePosPixel(), Size(1, 1))
                                       : mxTree->get_row_area(*xRow);
    return ShowContextMenu(it->second, *mxTree, aArea);
}

IMPL_LINK(ProjectTreePanel, ActivatedHdl, const weld::TreeIter&, rIter, bool)
{
    const auto it = maItems.find(mxTree->get_id(rIter));
    if (it == maItems.end() || it->second->getKind() != ItemKind_FILE)
        return false; // not a file: let the tree expand or collapse it

    try
    {
        uno::Reference<css::code::frontend::XWorkbench> xWorkbench
            = ::css::code::core::theCode::get(comphelper::getProcessComponentContext())->getWorkbench();
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
