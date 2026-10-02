/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "uiobject.hxx"
#include <sfx2/sidebar/SidebarController.hxx>
#include <rtl/ustrbuf.hxx>
#include <vcl/weld/Toolbar.hxx>

namespace sfx2::sidebar
{
TabBarUIObject::TabBarUIObject(const VclPtr<TabBar>& xTabBar)
    : WindowUIObject(xTabBar)
    , mxTabBar(xTabBar)
{
}

StringMap TabBarUIObject::get_state()
{
    StringMap aMap = WindowUIObject::get_state();
    OUStringBuffer rsHighlightedTabsIds;
    for (auto const& item : mxTabBar->maItems)
    {
        if (item->mxButton->get_item_active(u"toggle"_ustr))
        {
            if (!rsHighlightedTabsIds.isEmpty())
                rsHighlightedTabsIds.append(",");
            rsHighlightedTabsIds.append(item->msDeckId);
        }
    }
    aMap[u"HighlightedTabsIds"_ustr] = rsHighlightedTabsIds;
    return aMap;
}

void TabBarUIObject::execute(const OUString& rAction, const StringMap& rParameters)
{
    if (rAction == "CLICK")
    {
        auto it = rParameters.find(u"POS"_ustr);
        // Tab-click-to-toggle is only meaningful for the real, left/right
        // Sidebar (SidebarController::OpenThenToggleDeck) -- a TabBar hosted
        // by some other IDeckHost (e.g. the "Code" app's bottom-only Output
        // dock) simply doesn't support this UITest action yet.
        if (it != rParameters.end())
            if (auto* pSidebarController = dynamic_cast<SidebarController*>(&mxTabBar->mrDeckHost))
                pSidebarController->OpenThenToggleDeck(mxTabBar->GetDeckIdForIndex(it->second.toInt32()));
    }
    else
        WindowUIObject::execute(rAction, rParameters);
}

std::unique_ptr<UIObject> TabBarUIObject::create(vcl::Window* pWindow)
{
    TabBar* pTabBar = dynamic_cast<TabBar*>(pWindow);
    assert(pTabBar);
    return std::unique_ptr<UIObject>(new TabBarUIObject(pTabBar));
}

OUString TabBarUIObject::get_name() const { return u"TabBarUIObject"_ustr; }

} // namespace sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
