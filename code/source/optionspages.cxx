/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "optionspage.hxx"

#include <comphelper/configuration.hxx>
#include <officecfg/Office/Code.hxx>
#include <rtl/ustrbuf.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/CheckButton.hxx>
#include <vcl/weld/Container.hxx>
#include <vcl/weld/Entry.hxx>
#include <vcl/weld/Label.hxx>
#include <vcl/weld/SpinButton.hxx>

namespace code
{
OptionsPage::OptionsPage(weld::Container* pHost, const OUString& rUiFile, const OUString& rRootId)
    : m_xBuilder(Application::CreateBuilder(pHost, u"code/ui/"_ustr + rUiFile))
    , m_xRoot(m_xBuilder->weld_container(rRootId))
{
    m_xRoot->hide();
}

OptionsPage::~OptionsPage() = default;

void OptionsPage::Show(bool bShow)
{
    if (bShow)
        m_xRoot->show();
    else
        m_xRoot->hide();
}

OUString OptionsPage::GetStrings(std::initializer_list<OUString> aLabels,
                                 std::initializer_list<OUString> aCheckButtons)
{
    OUStringBuffer aAll;
    for (const OUString& rId : aLabels)
        if (auto xLabel = m_xBuilder->weld_label(rId))
            aAll.append(xLabel->get_label() + " ");
    for (const OUString& rId : aCheckButtons)
        if (auto xCheck = m_xBuilder->weld_check_button(rId))
            aAll.append(xCheck->get_label() + " ");
    // the mnemonic marks are not part of the words
    return aAll.makeStringAndClear().replaceAll("_", "");
}

namespace
{
using Editor = officecfg::Office::Code::Editor;

class EditorFontPage final : public OptionsPage
{
public:
    explicit EditorFontPage(weld::Container* pHost)
        : OptionsPage(pHost, u"optionspage_editorfont.ui"_ustr, u"EditorFontPage"_ustr)
        , m_xFontName(GetBuilder().weld_entry(u"fontname"_ustr))
        , m_xFontSize(GetBuilder().weld_spin_button(u"fontsize"_ustr))
    {
        m_xFontName->set_text(Editor::FontName::get());
        m_xFontSize->set_value(Editor::FontSize::get());
    }

    virtual void Store(const std::shared_ptr<comphelper::ConfigurationChanges>& rBatch) override
    {
        Editor::FontName::set(m_xFontName->get_text().trim(), rBatch);
        Editor::FontSize::set(static_cast<sal_Int32>(m_xFontSize->get_value()), rBatch);
    }

    virtual OUString GetAllStrings() override
    {
        return GetStrings({ u"fontlabel"_ustr, u"fontsizelabel"_ustr }, {});
    }

private:
    std::unique_ptr<weld::Entry> m_xFontName;
    std::unique_ptr<weld::SpinButton> m_xFontSize;
};

class EditorGeneralPage final : public OptionsPage
{
public:
    explicit EditorGeneralPage(weld::Container* pHost)
        : OptionsPage(pHost, u"optionspage_editorgeneral.ui"_ustr, u"EditorGeneralPage"_ustr)
        , m_xTabWidth(GetBuilder().weld_spin_button(u"tabwidth"_ustr))
        , m_xInsertSpaces(GetBuilder().weld_check_button(u"insertspaces"_ustr))
        , m_xLineNumbers(GetBuilder().weld_check_button(u"linenumbers"_ustr))
    {
        m_xTabWidth->set_value(Editor::TabWidth::get());
        m_xInsertSpaces->set_active(Editor::InsertSpaces::get());
        m_xLineNumbers->set_active(Editor::LineNumbers::get());
    }

    virtual void Store(const std::shared_ptr<comphelper::ConfigurationChanges>& rBatch) override
    {
        Editor::TabWidth::set(static_cast<sal_Int32>(m_xTabWidth->get_value()), rBatch);
        Editor::InsertSpaces::set(m_xInsertSpaces->get_active(), rBatch);
        Editor::LineNumbers::set(m_xLineNumbers->get_active(), rBatch);
    }

    virtual OUString GetAllStrings() override
    {
        return GetStrings({ u"tabwidthlabel"_ustr },
                          { u"insertspaces"_ustr, u"linenumbers"_ustr });
    }

private:
    std::unique_ptr<weld::SpinButton> m_xTabWidth;
    std::unique_ptr<weld::CheckButton> m_xInsertSpaces;
    std::unique_ptr<weld::CheckButton> m_xLineNumbers;
};

template <class Page> std::unique_ptr<OptionsPage> Make(weld::Container* pHost)
{
    return std::make_unique<Page>(pHost);
}

const OptionsPageInfo aPages[] = {
    { "editor", "", "Text Editor", nullptr },
    { "editor.general", "editor", "General", &Make<EditorGeneralPage> },
    { "editor.font", "editor", "Font", &Make<EditorFontPage> },
};
}

std::span<const OptionsPageInfo> GetOptionsPages() { return aPages; }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
