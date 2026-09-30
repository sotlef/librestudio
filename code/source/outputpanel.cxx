/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "outputpanel.hxx"

#include <tools/color.hxx>
#include <tools/fontenum.hxx>
#include <vcl/outdev.hxx>

OutputPanel::OutputPanel(vcl::Window* pParent)
    : DockingWindow(pParent, WB_BORDER)
{
    SetText(u"Output"_ustr);

    maLines = {
        u"1>------ Build started: Project: DemoProject, Configuration: Debug x64 ------"_ustr,
        u"1>main.cpp"_ustr,
        u"1>DemoProject.vcxproj -> C:\\DemoProject\\bin\\Debug\\DemoProject.exe"_ustr,
        u"========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped =========="_ustr,
    };
}

OutputPanel::~OutputPanel()
{
    disposeOnce();
}

void OutputPanel::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle&)
{
    vcl::Font aFont(rRenderContext.GetFont());
    aFont.SetFamilyName(u"Liberation Mono"_ustr);
    aFont.SetFamily(FAMILY_MODERN);
    aFont.SetPitch(PITCH_FIXED);
    aFont.SetFontSize(Size(0, 13));
    aFont.SetColor(COL_BLACK);
    rRenderContext.SetFont(aFont);

    rRenderContext.SetFillColor(COL_WHITE);
    rRenderContext.SetLineColor();
    rRenderContext.DrawRect(tools::Rectangle(Point(0, 0), GetOutputSizePixel()));

    tools::Long nY = 4;
    const tools::Long nLineHeight = rRenderContext.GetTextHeight() + 3;
    for (const OUString& rLine : maLines)
    {
        rRenderContext.DrawText(Point(6, nY), rLine);
        nY += nLineHeight;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
