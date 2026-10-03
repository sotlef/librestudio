/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "folderviewpanel.hxx"

#include <com/sun/star/code/project/XProjectFiles.hpp>

using namespace css;
using namespace css::code::project;

namespace code::sidebar
{

FolderViewPanel::FolderViewPanel(weld::Widget* pParent, uno::Reference<ui::XSidebar> xSidebar)
    : ProjectTreePanel(pParent, u"FolderViewPanel"_ustr, u"code/ui/folderviewpanel.ui"_ustr, std::move(xSidebar))
{
    Start();
}

uno::Reference<XProjectItem> FolderViewPanel::RootOf(const uno::Reference<XProject>& xProject)
{
    if (uno::Reference<XProjectFiles> xFiles{ xProject, uno::UNO_QUERY })
        return xFiles->getRootItem();
    return {};
}

OUString FolderViewPanel::EmptyText() const { return u"No folder open (File > Open > Folder)"_ustr; }

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
