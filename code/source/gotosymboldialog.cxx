/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gotosymboldialog.hxx"

#include "workbench.hxx"

#include <com/sun/star/code/core/Position.hpp>
#include <com/sun/star/code/index/SymbolKind.hpp>
#include <com/sun/star/code/index/XSymbolIndex.hpp>
#include <com/sun/star/code/index/theSymbolIndex.hpp>
#include <comphelper/processfactory.hxx>
#include <osl/file.hxx>
#include <sal/log.hxx>
#include <vcl/svapp.hxx>
#include <vcl/timer.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/Dialog.hxx>
#include <vcl/weld/Entry.hxx>
#include <vcl/weld/TreeView.hxx>

#include <memory>
#include <vector>

using namespace css;
using namespace css::code::index;

namespace code
{
namespace
{
// how many symbols the list shows: a query that matches thousands is not finished
constexpr sal_Int32 MaxResults = 200;

enum Column
{
    COL_NAME = 0,
    COL_KIND,
    COL_FILE,
    COL_LINE
};

OUString KindName(SymbolKind eKind)
{
    switch (eKind)
    {
        case SymbolKind_NAMESPACE:
            return u"namespace"_ustr;
        case SymbolKind_CLASS:
            return u"class"_ustr;
        case SymbolKind_STRUCT:
            return u"struct"_ustr;
        case SymbolKind_UNION:
            return u"union"_ustr;
        case SymbolKind_ENUM:
            return u"enum"_ustr;
        case SymbolKind_ENUM_MEMBER:
            return u"enumerator"_ustr;
        case SymbolKind_INTERFACE:
            return u"interface"_ustr;
        case SymbolKind_TYPEDEF:
            return u"typedef"_ustr;
        case SymbolKind_FUNCTION:
            return u"function"_ustr;
        case SymbolKind_METHOD:
            return u"method"_ustr;
        case SymbolKind_CONSTRUCTOR:
            return u"constructor"_ustr;
        case SymbolKind_FIELD:
            return u"field"_ustr;
        case SymbolKind_VARIABLE:
            return u"variable"_ustr;
        case SymbolKind_MACRO:
            return u"macro"_ustr;
        case SymbolKind_TEMPLATE_PARAMETER:
            return u"template parameter"_ustr;
        default:
            return OUString();
    }
}

OUString FileNameOf(const OUString& rUrl)
{
    return rUrl.copy(rUrl.lastIndexOf('/') + 1);
}

uno::Reference<XSymbolIndex> Index()
{
    return theSymbolIndex::get(comphelper::getProcessComponentContext());
}

void Say(const OUString& rMessage)
{
    if (Workbench* pWorkbench = Workbench::Instance())
        pWorkbench->showMessage(rMessage, 6000);
}

// "name (what)" as the list shows it: the scope is part of what tells two apart
OUString Title(const Symbol& rSymbol)
{
    OUString aTitle = rSymbol.QualifiedName;
    if (!rSymbol.Detail.isEmpty() && rSymbol.Kind != SymbolKind_FIELD
        && rSymbol.Kind != SymbolKind_VARIABLE)
        aTitle = rSymbol.Container.isEmpty() ? rSymbol.Detail
                                             : rSymbol.Container + "::" + rSymbol.Detail;
    return aTitle;
}

class SymbolList
{
public:
    SymbolList(weld::Window* pParent, bool bSearch, OUString aTitle, uno::Sequence<Symbol> aSymbols)
        : m_xBuilder(Application::CreateBuilder(pParent, u"code/ui/gotosymboldialog.ui"_ustr))
        , m_xDialog(m_xBuilder->weld_dialog(u"GotoSymbolDialog"_ustr))
        , m_xQuery(m_xBuilder->weld_entry(u"query"_ustr))
        , m_xTable(m_xBuilder->weld_tree_view(u"results"_ustr))
        , m_bSearch(bSearch)
        , m_aSymbols(std::move(aSymbols))
    {
        m_xDialog->set_title(aTitle);
        m_xTable->connect_item_activated(LINK(this, SymbolList, ActivatedHdl));
        if (bSearch)
        {
            m_aTimer.SetTimeout(120);
            m_aTimer.SetInvokeHandler(LINK(this, SymbolList, TimerHdl));
            m_xQuery->connect_changed(LINK(this, SymbolList, QueryHdl));
            m_xQuery->grab_focus();
        }
        else
        {
            // the choice of a few: no query
            m_xQuery->set_visible(false);
            m_xTable->grab_focus();
        }
        Fill();
    }

    /// true if the user picked a symbol; it is then in rSymbol
    bool Run(Symbol& rSymbol)
    {
        if (m_xDialog->run() != RET_OK)
            return false;
        const int nRow = m_xTable->get_selected_index();
        if (nRow < 0 || static_cast<size_t>(nRow) >= m_aShown.size())
            return false;
        rSymbol = m_aShown[nRow];
        return true;
    }

private:
    DECL_LINK(QueryHdl, weld::TextWidget&, void);
    DECL_LINK(TimerHdl, Timer*, void);
    DECL_LINK(ActivatedHdl, const weld::TreeIter&, bool);

    void Fill()
    {
        if (m_bSearch)
        {
            const OUString aQuery = m_xQuery->get_text().trim();
            try
            {
                m_aShown = aQuery.isEmpty()
                               ? std::vector<Symbol>()
                               : [&]
                {
                    const uno::Sequence<Symbol> aFound = Index()->findSymbols(aQuery, MaxResults);
                    return std::vector<Symbol>(aFound.begin(), aFound.end());
                }();
            }
            catch (const uno::Exception& rException)
            {
                SAL_WARN("code", "cannot find symbols: " << rException.Message);
                m_aShown.clear();
            }
        }
        else
            m_aShown.assign(m_aSymbols.begin(), m_aSymbols.end());

        m_xTable->freeze();
        m_xTable->clear();
        for (const Symbol& rSymbol : m_aShown)
        {
            const int nRow = m_xTable->n_children();
            m_xTable->append();
            m_xTable->set_text(nRow, Title(rSymbol), COL_NAME);
            m_xTable->set_text(nRow, KindName(rSymbol.Kind), COL_KIND);
            m_xTable->set_text(nRow, FileNameOf(rSymbol.Url), COL_FILE);
            m_xTable->set_text(nRow, OUString::number(rSymbol.Range.Start.Line + 1), COL_LINE);
        }
        m_xTable->thaw();
        if (!m_aShown.empty())
            m_xTable->select(0);
    }

    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Dialog> m_xDialog;
    std::unique_ptr<weld::Entry> m_xQuery;
    std::unique_ptr<weld::TreeView> m_xTable;
    Timer m_aTimer{ "code::SymbolList" };
    bool m_bSearch;
    uno::Sequence<Symbol> m_aSymbols;
    std::vector<Symbol> m_aShown;
};

// a pause after the last key: the index is asked once per word, not per letter
IMPL_LINK_NOARG(SymbolList, QueryHdl, weld::TextWidget&, void) { m_aTimer.Start(); }

IMPL_LINK_NOARG(SymbolList, TimerHdl, Timer*, void) { Fill(); }

IMPL_LINK_NOARG(SymbolList, ActivatedHdl, const weld::TreeIter&, bool)
{
    m_xDialog->response(RET_OK);
    return true;
}
}

void ShowSymbol(const Symbol& rSymbol)
{
    Workbench* pWorkbench = Workbench::Instance();
    if (!pWorkbench || rSymbol.Url.isEmpty())
        return;
    pWorkbench->revealRange(rSymbol.Url, rSymbol.Range, true);
}

void RunGoToSymbolDialog(weld::Window* pParent)
{
    SymbolList aDialog(pParent, true, u"Go to Symbol"_ustr, {});
    Symbol aSymbol;
    if (aDialog.Run(aSymbol))
        ShowSymbol(aSymbol);
}

void GoToDefinition(weld::Window* pParent)
{
    Workbench* pWorkbench = Workbench::Instance();
    if (!pWorkbench)
        return;
    const OUString aUrl = pWorkbench->CurrentDocumentUrl();
    css::code::core::Position aCaret;
    if (aUrl.isEmpty() || !pWorkbench->CurrentCaret(aCaret))
    {
        Say(u"Open a source file first."_ustr);
        return;
    }

    uno::Sequence<Symbol> aFound;
    try
    {
        aFound = Index()->findDefinition(aUrl, aCaret);
    }
    catch (const uno::Exception& rException)
    {
        Say(u"Cannot find the definition: "_ustr + rException.Message);
        return;
    }
    if (!aFound.hasElements())
    {
        Say(Index()->getState() == IndexState_INDEXING
                ? u"No definition found yet: the symbols are still being indexed."_ustr
                : u"No definition found."_ustr);
        return;
    }
    if (aFound.getLength() == 1)
    {
        ShowSymbol(aFound[0]);
        return;
    }
    SymbolList aDialog(pParent, false, u"Go to Definition"_ustr, aFound);
    Symbol aSymbol;
    if (aDialog.Run(aSymbol))
        ShowSymbol(aSymbol);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
