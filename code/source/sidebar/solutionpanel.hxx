/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/project/XProjectItem.hpp>
#include <com/sun/star/code/project/XProjectListener.hpp>
#include <com/sun/star/ui/XSidebar.hpp>
#include <sfx2/sidebar/PanelLayout.hxx>

#include <map>
#include <memory>

namespace weld { class TreeView; class TreeIter; }

namespace code { class UiNotifier; }

namespace code::sidebar
{

// Solution Explorer: one root row per open project (theProjectManager), its
// file tree below it, read lazily -- a folder's children are asked for when
// the row is first expanded.  Double-clicking a file opens it in the editor.
// Rebuilt whenever a project is opened, closed or activated.
class SolutionPanel final : public PanelLayout
{
public:
    SolutionPanel(weld::Widget* pParent,
        css::uno::Reference<css::ui::XSidebar> xSidebar);
    virtual ~SolutionPanel() override;

private:
    void Rebuild();
    void InsertItem(const weld::TreeIter* pParent, const css::uno::Reference<css::code::project::XProjectItem>& xItem,
                    const OUString& rLabel);

    DECL_LINK(ExpandingHdl, const weld::TreeIter&, bool);
    DECL_LINK(ActivatedHdl, const weld::TreeIter&, bool);

    std::unique_ptr<weld::TreeView> mxTree;
    css::uno::Reference<css::ui::XSidebar> mxSidebar;

    // Row ids are small integers; this maps them back to the items they stand for.
    std::map<OUString, css::uno::Reference<css::code::project::XProjectItem>> maItems;
    sal_Int32 mnNextId = 1;

    std::shared_ptr<::code::UiNotifier> mxNotifier;
    css::uno::Reference<css::code::project::XProjectListener> mxListener;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
