/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/project/XProject.hpp>
#include <com/sun/star/code/project/XProjectItem.hpp>
#include <com/sun/star/code/project/XProjectListener.hpp>
#include <com/sun/star/ui/XSidebar.hpp>
#include <sfx2/sidebar/PanelLayout.hxx>

#include <functional>
#include <map>
#include <set>
#include <memory>

class CommandEvent;
namespace weld { class TreeView; class TreeIter; class Widget; }
namespace tools { class Rectangle; }

namespace code { class UiNotifier; }

namespace code::sidebar
{

// What Folder View and Solution Explorer have in common: a tree of the open
// projects' items, read lazily (a row's children are asked for when it is first
// expanded), rebuilt whenever a project is opened, closed or changes, with a
// double click on a file opening it in the editor.  The two differ only in
// which tree of a project they show, and in what they show for a project that
// has none.
class ProjectTreePanel : public PanelLayout
{
public:
    /// rPanelId names the root widget of rUIFile, which holds the tree ("tree").
    ProjectTreePanel(weld::Widget* pParent, const OUString& rPanelId, const OUString& rUIFile,
                     css::uno::Reference<css::ui::XSidebar> xSidebar);
    virtual ~ProjectTreePanel() override;

protected:
    /// Called by the derived class once it is fully constructed; subscribes to
    /// the project manager and fills the tree.
    void Start();

    /// The root item of the tree to show for this project; null if this panel
    /// has nothing to show for it.
    virtual css::uno::Reference<css::code::project::XProjectItem>
    RootOf(const css::uno::Reference<css::code::project::XProject>& xProject) = 0;
    /// What to say when no open project has anything to show here.
    virtual OUString EmptyText() const = 0;
    /// The row label of a project's root item.
    virtual OUString LabelOf(const css::uno::Reference<css::code::project::XProject>& xProject,
                             const css::uno::Reference<css::code::project::XProjectItem>& xRoot) const;

    /// A context menu was asked for on rItem; show it at rArea of rParent and
    /// return true, or return false if there is none for this item.
    virtual bool ShowContextMenu(const css::uno::Reference<css::code::project::XProjectItem>& xItem,
                                 weld::Widget& rParent, const tools::Rectangle& rArea);

    void Rebuild();

    /// Optional: names of the children of xParent whose rows are shown in bold.
    /// Asked once per expansion of a row, not per child.
    std::function<std::set<OUString>(const css::uno::Reference<css::code::project::XProjectItem>& xParent)>
        maEmphasisOf;

private:
    std::unique_ptr<weld::TreeIter>
    InsertItem(const weld::TreeIter* pParent,
                    const css::uno::Reference<css::code::project::XProjectItem>& xItem,
                    const OUString& rLabel);

    DECL_LINK(ExpandingHdl, const weld::TreeIter&, bool);
    DECL_LINK(ActivatedHdl, const weld::TreeIter&, bool);
    DECL_LINK(CommandHdl, const CommandEvent&, bool);

    std::unique_ptr<weld::TreeView> mxTree;
    css::uno::Reference<css::ui::XSidebar> mxSidebar;

    // Row ids are small integers; this maps them back to the items they stand for.
    std::map<OUString, css::uno::Reference<css::code::project::XProjectItem>> maItems;
    sal_Int32 mnNextId = 1;
    // rows whose children have been read: expanding a row again (collapse, expand)
    // must not read them a second time -- Qt does not switch a row's
    // children-on-demand off the way GTK does, so the handler runs every time
    std::set<OUString> maPopulated;

    std::shared_ptr<code::UiNotifier> mxNotifier;
    css::uno::Reference<css::code::project::XProjectListener> mxListener;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
