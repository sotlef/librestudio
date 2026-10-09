/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "projectpropertiesdialog.hxx"

#include <com/sun/star/code/project/ProjectProperty.hpp>
#include <com/sun/star/code/project/ProjectPropertyPage.hpp>
#include <com/sun/star/code/project/XProjectPropertyEditor.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Dialog.hxx>
#include <vcl/font.hxx>
#include <vcl/settings.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/weld/Label.hxx>
#include <vcl/weld/MessageDialog.hxx>
#include <vcl/weld/TreeView.hxx>
#include <vcl/weld/Widget.hxx>

#include <map>
#include <memory>
#include <vector>

using namespace css;
using namespace css::code::project;

namespace code::sidebar
{
namespace
{
class PropertiesDialog
{
public:
    PropertiesDialog(weld::Widget* pParent, const OUString& rTitle,
                     const uno::Reference<XProjectProperties>& xProperties,
                     const uno::Sequence<ProjectPropertyPage>& rPages)
        : m_xBuilder(Application::CreateBuilder(pParent, u"code/ui/projectpropertiesdialog.ui"_ustr))
        , m_xDialog(m_xBuilder->weld_dialog(u"ProjectPropertiesDialog"_ustr))
        , m_xPages(m_xBuilder->weld_tree_view(u"pages"_ustr))
        , m_xProperties(m_xBuilder->weld_tree_view(u"properties"_ustr))
        , m_xDescriptionName(m_xBuilder->weld_label(u"descriptionname"_ustr))
        , m_xDescriptionText(m_xBuilder->weld_label(u"descriptiontext"_ustr))
        , m_xSource(xProperties)
        , m_xEditor(xProperties, uno::UNO_QUERY)
        , m_aPages(rPages)
    {
        m_xDialog->set_title(rTitle);
        vcl::Font aBold = Application::GetSettings().GetStyleSettings().GetLabelFont();
        aBold.SetWeight(WEIGHT_BOLD);
        m_xDescriptionName->set_font(aBold);
        m_xProperties->set_column_fixed_widths({ 220 });
        // The value column is edited for the cells the provider marks editable
        // (text or a typed value) and for the cells that are lists of values
        // (the tree makes the list, set_cell_choices); a list on a property that
        // is not editable is only looked at, the cell goes back to what it had.
        m_xProperties->set_column_editables({ false, true });
        m_xProperties->connect_editing(LINK(this, PropertiesDialog, EditingStartedHdl),
                                       LINK(this, PropertiesDialog, EditingDoneHdl));
        m_xPages->connect_selection_changed(LINK(this, PropertiesDialog, PageHdl));
        m_xProperties->connect_selection_changed(LINK(this, PropertiesDialog, PropertyHdl));
        FillPages();
    }

    ~PropertiesDialog()
    {
        if (m_pRefreshEvent)
            Application::RemoveUserEvent(m_pRefreshEvent);
    }

    void Run() { m_xDialog->run(); }

private:
    DECL_LINK(PageHdl, weld::ItemView&, void);
    DECL_LINK(PropertyHdl, weld::ItemView&, void);
    DECL_LINK(RefreshHdl, void*, void);

    // "C/C++/Preprocessor" is the page Preprocessor below the heading C/C++;
    // a heading no page names is made, so the tree has the shape of the paths
    void FillPages()
    {
        std::map<OUString, std::unique_ptr<weld::TreeIter>> aNodes;
        std::map<OUString, sal_Int32> aPageOfPath;
        for (sal_Int32 nPage = 0; nPage < m_aPages.getLength(); ++nPage)
            aPageOfPath[m_aPages[nPage].Path] = nPage;
        m_xPages->freeze();
        for (sal_Int32 nPage = 0; nPage < m_aPages.getLength(); ++nPage)
        {
            const OUString& rPath = m_aPages[nPage].Path;
            const weld::TreeIter* pParent = nullptr;
            OUString aSoFar;
            for (sal_Int32 nIndex = 0; nIndex >= 0;)
            {
                const OUString aPart = rPath.getToken(0, '/', nIndex);
                aSoFar = aSoFar.isEmpty() ? aPart : aSoFar + "/" + aPart;
                auto it = aNodes.find(aSoFar);
                if (it == aNodes.end())
                {
                    // the id is the page's number; a heading that is no page has none
                    const auto itPage = aPageOfPath.find(aSoFar);
                    const OUString aId = itPage == aPageOfPath.end() ? OUString() : OUString::number(itPage->second);
                    auto xIter = m_xPages->make_iterator();
                    m_xPages->insert(pParent, -1, &aPart, &aId, nullptr, nullptr, false, xIter.get());
                    it = aNodes.emplace(aSoFar, std::move(xIter)).first;
                }
                pParent = it->second.get();
            }
        }
        m_xPages->thaw();
        for (const auto& rNode : aNodes)
            m_xPages->expand_row(*rNode.second);
        if (m_xPages->n_children())
        {
            m_xPages->select(0);
            ShowPage();
        }
    }

    void ShowPage()
    {
        // a repaint of the same page keeps the chosen row
        const OUString aKeptId = m_xProperties->get_selected_id();
        m_xProperties->clear();
        m_xDescriptionName->set_label(OUString());
        m_xDescriptionText->set_label(OUString());
        const OUString aId = m_xPages->get_selected_id();
        if (aId.isEmpty())
            return; // a heading: nothing of its own
        const ProjectPropertyPage& rPage = m_aPages[aId.toInt32()];
        m_xProperties->freeze();
        for (sal_Int32 i = 0; i < rPage.Properties.getLength(); ++i)
        {
            const ProjectProperty& rProperty = rPage.Properties[i];
            // one line in the list
            const OUString aLine = rProperty.Value.replaceAll("\n", " ");
            const OUString aId = OUString::number(i);
            auto xIter = m_xProperties->make_iterator();
            m_xProperties->insert(nullptr, -1, &rProperty.Name, &aId, nullptr, nullptr, false, xIter.get());
            m_xProperties->set_text(*xIter, aLine, 1);
            // bold: set here, not inherited and not the tool's default
            m_xProperties->set_text_emphasis(*xIter, rProperty.Emphasized, 0);
            m_xProperties->set_text_emphasis(*xIter, rProperty.Emphasized, 1);
            // a property that is one of a set is a drop-down list in its value cell
            if (rProperty.Choices.hasElements())
                m_xProperties->set_cell_choices(*xIter, 1, std::vector<OUString>(rProperty.Choices.begin(), rProperty.Choices.end()),
                                                rProperty.SelectedChoice);
        }
        m_xProperties->thaw();
        if (m_xProperties->n_children())
        {
            int nRow = 0;
            if (!aKeptId.isEmpty() && aKeptId.toInt32() < m_xProperties->n_children())
                nRow = aKeptId.toInt32(); // the row id is the property's index and rows are in that order
            m_xProperties->select(nRow);
            m_xProperties->scroll_to_row(nRow);
            ShowValue();
        }
    }

    // Under the list: the name of the chosen property in bold, and what it
    // is.  A property that has no description (what a project says of itself)
    // shows its value there instead, a value of several parts one to a line:
    // the list cuts a long one short.
    void ShowValue()
    {
        const OUString aId = m_xProperties->get_selected_id();
        const OUString aPageId = m_xPages->get_selected_id();
        if (aId.isEmpty() || aPageId.isEmpty())
        {
            m_xDescriptionName->set_label(OUString());
            m_xDescriptionText->set_label(OUString());
            return;
        }
        const ProjectProperty& rProperty = m_aPages[aPageId.toInt32()].Properties[aId.toInt32()];
        m_xDescriptionName->set_label(rProperty.Name);
        m_xDescriptionText->set_label(rProperty.Description.isEmpty() ? rProperty.Value.replaceAll("; ", "\n")
                                                                      : rProperty.Description);
    }

    DECL_LINK(EditingStartedHdl, const weld::TreeIter&, bool);
    DECL_LINK(EditingDoneHdl, const weld::TreeView::IterColText&, bool);

    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Dialog> m_xDialog;
    std::unique_ptr<weld::TreeView> m_xPages;
    std::unique_ptr<weld::TreeView> m_xProperties;
    std::unique_ptr<weld::Label> m_xDescriptionName;
    std::unique_ptr<weld::Label> m_xDescriptionText;
    // what the provider said, read again after each accepted edit
    uno::Reference<XProjectProperties> m_xSource;
    // null when the provider cannot be written to: nothing is editable then
    uno::Reference<XProjectPropertyEditor> m_xEditor;
    uno::Sequence<ProjectPropertyPage> m_aPages;
    ImplSVEvent* m_pRefreshEvent = nullptr;
};

IMPL_LINK_NOARG(PropertiesDialog, PageHdl, weld::ItemView&, void) { ShowPage(); }
IMPL_LINK_NOARG(PropertiesDialog, PropertyHdl, weld::ItemView&, void) { ShowValue(); }
// The tree opens the editor (a text cell or the list) only for the cells that
// say yes here: the ones the provider lets be written, and the lists.
IMPL_LINK(PropertiesDialog, EditingStartedHdl, const weld::TreeIter&, rIter, bool)
{
    const OUString aPageId = m_xPages->get_selected_id();
    if (aPageId.isEmpty())
        return false;
    const ProjectProperty& rProperty = m_aPages[aPageId.toInt32()].Properties[m_xProperties->get_id(rIter).toInt32()];
    return (rProperty.Editable && m_xEditor.is()) || rProperty.Choices.hasElements();
}

// Whatever is typed or chosen goes to the provider, which decides.  The value
// column only: the names are never edited.  Empty text is a reset to the default.
// True is answered when it took it (the tree then keeps the text), false when
// it refused or the property is only looked at (the cell keeps what it had).
// What the cell shows after a take is not the typed text for certain (a reset,
// a value the provider writes another way, Emphasized) so the page is read
// again; that cannot be done in here, the tree is still in the middle of the
// edit and must not be cleared under its own signal, so it is posted.
IMPL_LINK(PropertiesDialog, EditingDoneHdl, const weld::TreeView::IterColText&, rEdit, bool)
{
    if (rEdit.m_nColumn != 1)
        return false;
    const OUString aPageId = m_xPages->get_selected_id();
    if (aPageId.isEmpty())
        return false;
    const ProjectPropertyPage& rPage = m_aPages[aPageId.toInt32()];
    const ProjectProperty& rProperty = rPage.Properties[m_xProperties->get_id(rEdit.m_rIter).toInt32()];
    if (!rProperty.Editable || !m_xEditor.is())
        return false; // a list that is only looked at
    OUString aError;
    bool bFailed = false;
    try
    {
        m_xEditor->setProperty(rPage.Path, rProperty.Name, rEdit.m_sText);
    }
    catch (const lang::IllegalArgumentException& rException)
    {
        aError = rException.Message;
        bFailed = true;
    }
    catch (const io::IOException& rException)
    {
        aError = rException.Message;
        bFailed = true;
    }
    catch (const uno::Exception& rException)
    {
        aError = rException.Message;
        bFailed = true;
    }
    if (bFailed)
    {
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(
            m_xDialog.get(), VclMessageType::Error, VclButtonsType::Ok, rProperty.Name));
        xBox->set_secondary_text(aError);
        xBox->run();
        return false;
    }
    if (!m_pRefreshEvent)
        m_pRefreshEvent = Application::PostUserEvent(LINK(this, PropertiesDialog, RefreshHdl));
    return true;
}

IMPL_LINK_NOARG(PropertiesDialog, RefreshHdl, void*, void)
{
    m_pRefreshEvent = nullptr;
    try
    {
        m_aPages = m_xSource->getPropertyPages();
    }
    catch (const uno::Exception& rException)
    {
        // the old pages stay; the cell shows what was typed
        SAL_WARN("code", "cannot read the properties again: " << rException.Message);
        return;
    }
    ShowPage();
}
}

void RunProjectPropertiesDialog(weld::Widget* pParent, const OUString& rTitle,
                                const uno::Reference<XProjectProperties>& xProperties)
{
    uno::Sequence<ProjectPropertyPage> aPages;
    try
    {
        aPages = xProperties->getPropertyPages();
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot read the properties of " << rTitle << ": " << rException.Message);
    }
    PropertiesDialog(pParent, rTitle, xProperties, aPages).Run();
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
