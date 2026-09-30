/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "solutionpanel.hxx"

#include <svtools/imagemgr.hxx>
#include <tools/urlobj.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/TreeView.hxx>
#include <vcl/weld/Widget.hxx>

namespace code::sidebar
{

namespace
{
// Generic folder icon (same one a real Open dialog shows for any plain
// directory) -- SvFileInformationManager::GetFolderImageId() only touches
// the filesystem for the remote/CD/removable-device flags in VolumeInfo, a
// default-constructed one (all false) is safe to use for our fake tree.
OUString FolderIcon()
{
    return SvFileInformationManager::GetFolderImageId(svtools::VolumeInfo());
}

// Per-extension file icon, keyed purely off the name (GetFileImageId's
// bDetectFolder=false path never touches the filesystem, so a URL that
// doesn't exist on disk is fine) -- falls back to LO's generic "unknown
// file" icon for extensions it has no special mapping for (.cpp/.h today).
OUString FileIcon(const OUString& rFileName)
{
    const OUString sUrl(u"file:///"_ustr + rFileName);
    return SvFileInformationManager::GetFileImageId(INetURLObject(sUrl));
}

// insert()'s own pIconName argument isn't how real LO code sets a row's
// icon (cui's SvxScriptOrgDialog::insertEntry, cui/source/dialogs/
// scriptdlg.cxx, is the proven pattern): insert the row first, then call
// set_image() on the iterator insert() filled in.
void InsertRow(weld::TreeView& rTree, const weld::TreeIter* pParent, const OUString& rText,
    const OUString& rIconName, weld::TreeIter* pRet)
{
    rTree.insert(pParent, -1, &rText, nullptr, nullptr, nullptr, false, pRet);
    rTree.set_image(*pRet, rIconName);
}
}

SolutionPanel::SolutionPanel(weld::Widget* pParent)
    : PanelLayout(pParent, u"SolutionPanel"_ustr, u"code/ui/solutionpanel.ui"_ustr)
    , mxTree(m_xBuilder->weld_tree_view(u"tree"_ustr))
{
    // Same fake demo project as the old SolutionExplorerPanel
    // (code/source/solutionexplorer.cxx), now a real parent/child tree
    // (solutionpanel.ui's model is a GtkTreeStore) with real folder/file
    // icons instead of a flat, hand-indented list. A real tree model with
    // a real project backing it is future work.
    const OUString sFolderIcon(FolderIcon());

    std::unique_ptr<weld::TreeIter> xSolution(mxTree->make_iterator());
    InsertRow(*mxTree, nullptr, u"Solution 'DemoProject' (1 project)"_ustr, sFolderIcon,
        xSolution.get());

    std::unique_ptr<weld::TreeIter> xProject(mxTree->make_iterator());
    InsertRow(*mxTree, xSolution.get(), u"DemoProject"_ustr, sFolderIcon, xProject.get());

    std::unique_ptr<weld::TreeIter> xSourceFiles(mxTree->make_iterator());
    InsertRow(*mxTree, xProject.get(), u"Source Files"_ustr, sFolderIcon, xSourceFiles.get());
    std::unique_ptr<weld::TreeIter> xLeaf(mxTree->make_iterator());
    InsertRow(*mxTree, xSourceFiles.get(), u"main.cpp"_ustr, FileIcon(u"main.cpp"_ustr),
        xLeaf.get());

    std::unique_ptr<weld::TreeIter> xHeaderFiles(mxTree->make_iterator());
    InsertRow(*mxTree, xProject.get(), u"Header Files"_ustr, sFolderIcon, xHeaderFiles.get());
    InsertRow(*mxTree, xHeaderFiles.get(), u"utils.h"_ustr, FileIcon(u"utils.h"_ustr), xLeaf.get());

    InsertRow(*mxTree, xProject.get(), u"README.md"_ustr, FileIcon(u"README.md"_ustr), xLeaf.get());

    mxTree->expand_row(*xSolution);
    mxTree->expand_row(*xProject);
}

SolutionPanel::~SolutionPanel()
{
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
