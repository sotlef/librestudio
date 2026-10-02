/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "outputlogpanel.hxx"

#include <algorithm>

#include <o3tl/safeint.hxx>
#include <rtl/ustrbuf.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/TextView.hxx>
#include <vcl/weld/Toolbar.hxx>

namespace code::sidebar
{

OutputLogPanel::OutputLogPanel(weld::Widget* pParent)
    : PanelLayout(pParent, u"OutputPanel"_ustr, u"code/ui/outputpanel.ui"_ustr)
    , mxSource(m_xBuilder->weld_combo_box(u"source"_ustr))
    , mxToolbar(m_xBuilder->weld_toolbar(u"toolbar"_ustr))
    , mxLog(m_xBuilder->weld_text_view(u"log"_ustr))
    , mnCurrentMessage(-1)
    , mbShowTimestamps(false)
{
    // Same fake demo build log as the old OutputPanel
    // (code/source/outputpanel.cxx's maLines); a real log sink that reacts
    // to an actual build is future work -- which is also why the Debug
    // source below is empty and the per-line timestamps are canned values:
    // there is no real event (and no real wall clock moment) behind any of
    // these lines yet.
    maSources.push_back(
        { u"Build"_ustr,
          { { u"14:32:01.153"_ustr,
              u"1>------ Build started: Project: DemoProject, Configuration: Debug x64 ------"_ustr },
            { u"14:32:01.289"_ustr, u"1>main.cpp"_ustr },
            { u"14:32:02.735"_ustr,
              u"1>DemoProject.vcxproj -> C:\\DemoProject\\bin\\Debug\\DemoProject.exe"_ustr },
            { u"14:32:02.761"_ustr,
              u"========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped =========="_ustr } } });
    maSources.push_back({ u"Debug"_ustr, {} });

    for (const auto& rSource : maSources)
        mxSource->append_text(rSource.msName);
    mxSource->set_active(0);
    mxSource->connect_changed(LINK(this, OutputLogPanel, SourceChangedHdl));

    mxToolbar->connect_clicked(LINK(this, OutputLogPanel, ToolbarClickHdl));

    UpdateLogText();
}

OutputLogPanel::~OutputLogPanel()
{
}

int OutputLogPanel::CurrentSourceIndex() const
{
    const int nActive = mxSource->get_active();
    return (nActive >= 0 && o3tl::make_unsigned(nActive) < maSources.size()) ? nActive : 0;
}

void OutputLogPanel::UpdateLogText()
{
    const LogSource& rSource = maSources[CurrentSourceIndex()];

    if (mnCurrentMessage >= static_cast<int>(rSource.maLines.size()))
        mnCurrentMessage = static_cast<int>(rSource.maLines.size()) - 1;

    OUStringBuffer aText;
    for (size_t i = 0; i < rSource.maLines.size(); ++i)
    {
        if (i > 0)
            aText.append('\n');
        if (mbShowTimestamps)
            aText.append(rSource.maLines[i].msTimestamp + " ");
        aText.append(rSource.maLines[i].msText);
    }
    mxLog->set_text(aText.makeStringAndClear());

    // Re-highlight the current message (if any) in the freshly set text --
    // selection does not survive set_text().
    if (mnCurrentMessage < 0)
        return;
    int nStart = 0;
    for (int i = 0; i < mnCurrentMessage; ++i)
    {
        nStart += rSource.maLines[i].msText.getLength() + 1; // + '\n'
        if (mbShowTimestamps)
            nStart += rSource.maLines[i].msTimestamp.getLength() + 1; // + ' '
    }
    int nEnd = nStart + rSource.maLines[mnCurrentMessage].msText.getLength();
    if (mbShowTimestamps)
        nEnd += rSource.maLines[mnCurrentMessage].msTimestamp.getLength() + 1;
    mxLog->select_region(nStart, nEnd);
}

IMPL_LINK_NOARG(OutputLogPanel, SourceChangedHdl, weld::ComboBox&, void)
{
    // Each source keeps its own log; the message cursor is per-source in
    // spirit but not worth persisting for canned demo content -- reset it.
    mnCurrentMessage = -1;
    UpdateLogText();
}

IMPL_LINK(OutputLogPanel, ToolbarClickHdl, const OUString&, rIdent, void)
{
    LogSource& rSource = maSources[CurrentSourceIndex()];
    const int nLines = static_cast<int>(rSource.maLines.size());

    if (rIdent == "find")
    {
        // "Find Message in Code": jumping from a build message to its
        // source location needs real build messages carrying file/line
        // information -- nothing but canned demo text exists yet, so this
        // is an honest no-op until a real log sink lands.
    }
    else if (rIdent == "prev")
    {
        if (nLines == 0)
            return;
        mnCurrentMessage = (mnCurrentMessage <= 0) ? 0 : mnCurrentMessage - 1;
        UpdateLogText();
    }
    else if (rIdent == "next")
    {
        if (nLines == 0)
            return;
        mnCurrentMessage = (mnCurrentMessage < 0) ? 0 : std::min(mnCurrentMessage + 1, nLines - 1);
        UpdateLogText();
    }
    else if (rIdent == "clear")
    {
        rSource.maLines.clear();
        mnCurrentMessage = -1;
        UpdateLogText();
    }
    else if (rIdent == "wordwrap")
    {
        mxLog->set_word_wrap(mxToolbar->get_item_active(u"wordwrap"_ustr));
    }
    else if (rIdent == "timestamp")
    {
        mbShowTimestamps = mxToolbar->get_item_active(u"timestamp"_ustr);
        UpdateLogText();
    }
}

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
