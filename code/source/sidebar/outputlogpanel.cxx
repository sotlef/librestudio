/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "outputlogpanel.hxx"

#include <vcl/weld/Builder.hxx>
#include <vcl/weld/TextView.hxx>

namespace code::sidebar
{

OutputLogPanel::OutputLogPanel(weld::Widget* pParent)
    : PanelLayout(pParent, u"OutputPanel"_ustr, u"code/ui/outputpanel.ui"_ustr)
    , mxLog(m_xBuilder->weld_text_view(u"log"_ustr))
{
    // Same fake demo build log as the old OutputPanel
    // (code/source/outputpanel.cxx's maLines); a real log sink that reacts
    // to an actual build is future work.
    mxLog->set_text(
        u"1>------ Build started: Project: DemoProject, Configuration: Debug x64 ------\n"
        "1>main.cpp\n"
        "1>DemoProject.vcxproj -> C:\\DemoProject\\bin\\Debug\\DemoProject.exe\n"
        "========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped =========="_ustr);
}

OutputLogPanel::~OutputLogPanel()
{
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
