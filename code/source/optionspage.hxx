/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/ustring.hxx>
#include <initializer_list>
#include <memory>
#include <span>

namespace comphelper { class ConfigurationChanges; }
namespace weld
{
class Builder;
class Container;
}

namespace code
{
/// One page of Tools > Options.  A page owns the widgets of its .ui file
/// (built into the dialog's page area), reads its settings from officecfg when
/// it is made and writes them into a configuration batch when the dialog is
/// closed with OK.
class OptionsPage
{
public:
    /// rUiFile is below code/ui/; rRootId is the id of the .ui's root widget.
    OptionsPage(weld::Container* pHost, const OUString& rUiFile, const OUString& rRootId);
    virtual ~OptionsPage();

    void Show(bool bShow);
    virtual void Store(const std::shared_ptr<comphelper::ConfigurationChanges>& rBatch) = 0;
    /// The texts of the page's labels and check buttons, which Tools > Options'
    /// search looks in besides the titles.  A page names the widgets that carry
    /// text: GetStrings({labels}, {check buttons}).
    virtual OUString GetAllStrings() = 0;

protected:
    weld::Builder& GetBuilder() { return *m_xBuilder; }
    OUString GetStrings(std::initializer_list<OUString> aLabels,
                        std::initializer_list<OUString> aCheckButtons);

private:
    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Container> m_xRoot;
};

/// A node of the options tree.  A node without a page is only a heading: the
/// first of its children is shown when it is chosen.  To add a settings page:
/// write its .ui and its OptionsPage subclass in optionspages.cxx, and add one
/// line to GetOptionsPages().
struct OptionsPageInfo
{
    /// unique, and what the dialog remembers the last chosen page by
    const char* pId;
    /// the id of the heading this page is below, "" at the top level
    const char* pParentId;
    const char* pTitle;
    /// null for a heading
    std::unique_ptr<OptionsPage> (*pCreate)(weld::Container* pHost);
};

/// The tree, in the order it is shown; a parent comes before its children.
std::span<const OptionsPageInfo> GetOptionsPages();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
