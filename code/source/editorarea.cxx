/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "editorarea.hxx"

#include <svtools/tabbar.hxx>
#include <tools/color.hxx>
#include <tools/fontenum.hxx>
#include <vcl/outdev.hxx>

namespace {
    constexpr sal_uInt16 TAB_MAIN_CPP = 1;
    constexpr sal_uInt16 TAB_UTILS_H = 2;
    constexpr sal_uInt16 TAB_README = 3;
}

EditorContentWindow::EditorContentWindow(vcl::Window* pParent)
    : vcl::Window(pParent, WB_CLIPCHILDREN)
{
}

EditorContentWindow::~EditorContentWindow()
{
    disposeOnce();
}

void EditorContentWindow::SetSourceLines(std::vector<OUString> aLines)
{
    maLines = std::move(aLines);
    Invalidate();
}

void EditorContentWindow::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle&)
{
    vcl::Font aFont(rRenderContext.GetFont());
    aFont.SetFamilyName(u"Liberation Mono"_ustr);
    aFont.SetFamily(FAMILY_MODERN);
    aFont.SetPitch(PITCH_FIXED);
    aFont.SetFontSize(Size(0, 14));
    aFont.SetColor(COL_BLACK);
    rRenderContext.SetFont(aFont);

    rRenderContext.SetFillColor(COL_WHITE);
    rRenderContext.SetLineColor();
    rRenderContext.DrawRect(tools::Rectangle(Point(0, 0), GetOutputSizePixel()));

    tools::Long nY = 6;
    const tools::Long nLineHeight = rRenderContext.GetTextHeight() + 4;
    for (const OUString& rLine : maLines)
    {
        rRenderContext.DrawText(Point(8, nY), rLine);
        nY += nLineHeight;
    }
}

EditorAreaPanel::EditorAreaPanel(vcl::Window* pParent)
    : vcl::Window(pParent, WB_CLIPCHILDREN)
{
    mxTabBar = VclPtr<TabBar>::Create(this, WB_BORDER);
    mxTabBar->InsertPage(TAB_MAIN_CPP, u"main.cpp"_ustr);
    mxTabBar->InsertPage(TAB_UTILS_H, u"utils.h"_ustr);
    mxTabBar->InsertPage(TAB_README, u"README.md"_ustr);
    mxTabBar->SetCurPageId(TAB_MAIN_CPP);
    mxTabBar->SetSelectHdl(LINK(this, EditorAreaPanel, TabSelectHdl));
    mxTabBar->Show();

    mxContent = VclPtr<EditorContentWindow>::Create(this);
    mxContent->Show();

    UpdateContentForCurrentTab();
}

EditorAreaPanel::~EditorAreaPanel()
{
    disposeOnce();
}

void EditorAreaPanel::dispose()
{
    mxContent.disposeAndClear();
    mxTabBar.disposeAndClear();
    vcl::Window::dispose();
}

void EditorAreaPanel::Resize()
{
    vcl::Window::Resize();
    if (!mxTabBar || !mxContent)
        return;

    const Size aSize = GetOutputSizePixel();
    const tools::Long nTabBarHeight = 24;
    mxTabBar->SetPosSizePixel(Point(0, 0), Size(aSize.Width(), nTabBarHeight));
    mxContent->SetPosSizePixel(Point(0, nTabBarHeight),
        Size(aSize.Width(), aSize.Height() - nTabBarHeight));
}

void EditorAreaPanel::UpdateContentForCurrentTab()
{
    if (!mxTabBar || !mxContent)
        return;

    std::vector<OUString> aLines;
    switch (mxTabBar->GetCurPageId())
    {
        case TAB_MAIN_CPP:
            aLines = {
                u"#include <iostream>"_ustr,
                u""_ustr,
                u"int main()"_ustr,
                u"{"_ustr,
                u"    std::cout << \"Hello, LibreStudio Code!\" << std::endl;"_ustr,
                u"    return 0;"_ustr,
                u"}"_ustr,
            };
            break;
        case TAB_UTILS_H:
            aLines = {
                u"#pragma once"_ustr,
                u""_ustr,
                u"int add(int a, int b);"_ustr,
            };
            break;
        default:
            aLines = { u"LibreStudio Code -- work in progress. Nothing here is real yet."_ustr };
            break;
    }
    mxContent->SetSourceLines(std::move(aLines));
}

IMPL_LINK_NOARG(EditorAreaPanel, TabSelectHdl, TabBar*, void)
{
    UpdateContentForCurrentTab();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
