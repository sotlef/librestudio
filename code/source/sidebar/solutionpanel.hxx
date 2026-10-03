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

// Solution Explorer: the structure the project's format describes -- the
// solution, its targets, their source groups and files -- as opposed to Folder
// View's plain directory tree.  A project whose format has no structure (an
// opened folder) shows its directory tree here too, so that the panel is never
// empty while Folder View is not.
class SolutionPanel final : public ProjectTreePanel
{
public:
    SolutionPanel(weld::Widget* pParent, css::uno::Reference<css::ui::XSidebar> xSidebar);

protected:
    virtual css::uno::Reference<css::code::project::XProjectItem>
    RootOf(const css::uno::Reference<css::code::project::XProject>& xProject) override;
    virtual bool ShowContextMenu(const css::uno::Reference<css::code::project::XProjectItem>& xItem,
                                 weld::Widget& rParent, const tools::Rectangle& rArea) override;
    virtual OUString EmptyText() const override;
    virtual OUString LabelOf(const css::uno::Reference<css::code::project::XProject>& xProject,
                             const css::uno::Reference<css::code::project::XProjectItem>& xRoot) const override;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
