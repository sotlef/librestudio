/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "projecttreepanel.hxx"

namespace code::sidebar
{

// Folder View: the open projects' files exactly as they lie on disk, whatever
// the project's format makes of them.  Always available while anything is open.
class FolderViewPanel final : public ProjectTreePanel
{
public:
    FolderViewPanel(weld::Widget* pParent, css::uno::Reference<css::ui::XSidebar> xSidebar);

protected:
    virtual css::uno::Reference<css::code::project::XProjectItem>
    RootOf(const css::uno::Reference<css::code::project::XProject>& xProject) override;
    virtual OUString EmptyText() const override;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
