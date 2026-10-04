/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "optionsdialog.hxx"
#include "optionspage.hxx"

#include <comphelper/configuration.hxx>
#include <vcl/svapp.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Container.hxx>
#include <vcl/weld/Dialog.hxx>
#include <vcl/weld/Entry.hxx>
#include <vcl/weld/TreeView.hxx>

#include <com/sun/star/util/SearchAlgorithms2.hpp>
#include <com/sun/star/util/SearchFlags.hpp>
#include <i18nutil/searchopt.hxx>
#include <unotools/textsearch.hxx>
#include <vcl/timer.hxx>

#include <map>
#include <vector>

namespace code
{
namespace
{
/// the id of the page chosen last, for the next time the dialog opens
OUString LastPage;

class OptionsDialog
{
public:
    explicit OptionsDialog(weld::Window* pParent)
        : m_xBuilder(Application::CreateBuilder(pParent, u"code/ui/optionsdialog.ui"_ustr))
        , m_xDialog(m_xBuilder->weld_dialog(u"OptionsDialog"_ustr))
        , m_xSearch(m_xBuilder->weld_entry(u"searchEntry"_ustr))
        , m_xTree(m_xBuilder->weld_tree_view(u"pages"_ustr))
        , m_xHost(m_xBuilder->weld_container(u"pagehost"_ustr))
    {
        m_xSearch->set_placeholder_text(u"Type to search"_ustr);
        m_aSearchTimer.SetInvokeHandler(LINK(this, OptionsDialog, SearchTimerHdl));
        m_aSearchTimer.SetTimeout(500);
        m_xSearch->connect_changed(LINK(this, OptionsDialog, SearchHdl));
        m_xTree->connect_selection_changed(LINK(this, OptionsDialog, SelectHdl));
        Populate(OUString());
        Select(LastPage);
    }

    bool Run()
    {
        if (m_xDialog->run() != RET_OK)
            return false;
        // only the pages the user opened can have changed
        auto xBatch = comphelper::ConfigurationChanges::create();
        for (auto& rPage : m_aPages)
            rPage.second->Store(xBatch);
        xBatch->commit();
        return true;
    }

private:
    DECL_LINK(SelectHdl, weld::ItemView&, void);
    DECL_LINK(SearchHdl, weld::TextWidget&, void);
    DECL_LINK(SearchTimerHdl, Timer*, void);

    /// the parents' titles, the page's title and the texts of its widgets, as
    /// Tools > Options of the office applications searches them
    OUString SearchableText(const OptionsPageInfo& rInfo)
    {
        OUString aText;
        for (const OptionsPageInfo* p = FindInfo(OUString::createFromAscii(rInfo.pParentId)); p;
             p = FindInfo(OUString::createFromAscii(p->pParentId)))
            aText = OUString::createFromAscii(p->pTitle) + " " + aText;
        aText += OUString::createFromAscii(rInfo.pTitle) + " "
                 + PageOf(OUString::createFromAscii(rInfo.pId)).GetAllStrings();
        return aText;
    }

    /// fills the tree with the pages that match the filter (all of them when
    /// it is empty), headings only where a page below them is left
    void Populate(const OUString& rFilter)
    {
        const OUString aKeep = m_aCurrent;
        m_bPopulating = true;
        m_xTree->clear();
        m_aIters.clear();
        const OUString aFilter = rFilter.trim();
        std::unique_ptr<utl::TextSearch> xSearch;
        if (!aFilter.isEmpty())
        {
            i18nutil::SearchOptions2 aOptions;
            aOptions.AlgorithmType2 = css::util::SearchAlgorithms2::ABSOLUTE;
            aOptions.transliterateFlags |= TransliterationFlags::IGNORE_CASE;
            aOptions.searchFlag
                |= (css::util::SearchFlags::REG_NOT_BEGINOFLINE | css::util::SearchFlags::REG_NOT_ENDOFLINE);
            aOptions.searchString = aFilter;
            xSearch = std::make_unique<utl::TextSearch>(aOptions);
        }

        std::vector<bool> aShown;
        for (const OptionsPageInfo& rInfo : GetOptionsPages())
        {
            bool bMatch = rInfo.pCreate != nullptr;
            if (bMatch && xSearch)
            {
                const OUString aText = SearchableText(rInfo);
                sal_Int32 nStart = 0;
                sal_Int32 nEnd = aText.getLength();
                bMatch = xSearch->SearchForward(aText, &nStart, &nEnd);
            }
            aShown.push_back(bMatch);
        }
        // a heading stays for the pages below it
        const auto aPages = GetOptionsPages();
        for (size_t i = 0; i < aPages.size(); ++i)
            if (aShown[i])
                for (const OptionsPageInfo* p = FindInfo(OUString::createFromAscii(aPages[i].pParentId)); p;
                     p = FindInfo(OUString::createFromAscii(p->pParentId)))
                    aShown[p - aPages.data()] = true;

        // children come after their heading in the table
        std::map<OUString, std::unique_ptr<weld::TreeIter>> aHeadings;
        for (size_t i = 0; i < aPages.size(); ++i)
        {
            if (!aShown[i])
                continue;
            const OptionsPageInfo& rInfo = aPages[i];
            const OUString aId = OUString::createFromAscii(rInfo.pId);
            const OUString aTitle = OUString::createFromAscii(rInfo.pTitle);
            auto xIter = m_xTree->make_iterator();
            const auto aParentIt = aHeadings.find(OUString::createFromAscii(rInfo.pParentId));
            m_xTree->insert(aParentIt == aHeadings.end() ? nullptr : aParentIt->second.get(), -1,
                            &aTitle, &aId, nullptr, nullptr, false, xIter.get());
            m_aIters[aId] = m_xTree->make_iterator(xIter.get());
            if (!rInfo.pCreate)
                aHeadings[aId] = std::move(xIter);
        }
        for (const auto& rHeading : aHeadings)
            m_xTree->expand_row(*rHeading.second);
        m_bPopulating = false;

        // the page on show stays when it is still listed, else the first match
        if (m_aIters.count(aKeep))
            m_xTree->select(*m_aIters[aKeep]);
        else
            for (size_t i = 0; i < aPages.size(); ++i)
                if (aShown[i] && aPages[i].pCreate)
                {
                    const OUString aId = OUString::createFromAscii(aPages[i].pId);
                    m_xTree->select(*m_aIters[aId]);
                    Show(aId);
                    break;
                }
    }

    void Select(const OUString& rId) // the first time only
    {
        const OptionsPageInfo* pInfo = rId.isEmpty() ? nullptr : FindInfo(rId);
        if (pInfo)
            m_xTree->select(*m_aIters[rId]);
        else
            m_xTree->select(0);
        Show(pInfo ? rId : OUString::createFromAscii(GetOptionsPages()[0].pId));
    }

    static const OptionsPageInfo* FindInfo(const OUString& rId)
    {
        for (const OptionsPageInfo& rInfo : GetOptionsPages())
            if (rId.equalsAscii(rInfo.pId))
                return &rInfo;
        return nullptr;
    }

    /// the page of an id, made when first needed
    OptionsPage& PageOf(const OUString& rId)
    {
        auto it = m_aPages.find(rId);
        if (it == m_aPages.end())
            it = m_aPages.emplace(rId, FindInfo(rId)->pCreate(m_xHost.get())).first;
        return *it->second;
    }

    /// shows the page of an id; a heading shows its first child's
    void Show(OUString aId)
    {
        const OptionsPageInfo* pInfo = FindInfo(aId);
        while (pInfo && !pInfo->pCreate)
        {
            const OptionsPageInfo* pChild = nullptr;
            for (const OptionsPageInfo& rInfo : GetOptionsPages())
                if (aId.equalsAscii(rInfo.pParentId))
                {
                    pChild = &rInfo;
                    break;
                }
            pInfo = pChild;
            if (pInfo)
                aId = OUString::createFromAscii(pInfo->pId);
        }
        if (!pInfo)
            return;
        for (auto& rPage : m_aPages)
            rPage.second->Show(false);
        PageOf(aId).Show(true);
        LastPage = m_aCurrent = aId;
    }

    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Dialog> m_xDialog;
    std::unique_ptr<weld::Entry> m_xSearch;
    std::unique_ptr<weld::TreeView> m_xTree;
    OUString m_aCurrent;
    bool m_bPopulating = false;
    Timer m_aSearchTimer{ "code::OptionsDialog search" };
    std::unique_ptr<weld::Container> m_xHost;
    std::map<OUString, std::unique_ptr<weld::TreeIter>> m_aIters;
    /// made when first shown
    std::map<OUString, std::unique_ptr<OptionsPage>> m_aPages;
};

IMPL_LINK(OptionsDialog, SelectHdl, weld::ItemView&, rView, void)
{
    if (m_bPopulating)
        return;
    const OUString aId = rView.get_selected_id();
    if (!aId.isEmpty())
        Show(aId);
}

// the tree is rebuilt once the user pauses typing, not on every key
IMPL_LINK_NOARG(OptionsDialog, SearchHdl, weld::TextWidget&, void) { m_aSearchTimer.Start(); }

IMPL_LINK_NOARG(OptionsDialog, SearchTimerHdl, Timer*, void) { Populate(m_xSearch->get_text()); }
}

bool RunOptionsDialog(weld::Window* pParent) { return OptionsDialog(pParent).Run(); }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
