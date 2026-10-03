/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "outputlogpanel.hxx"

#include "../uibridge.hxx"

#include <com/sun/star/code/core/theCode.hpp>
#include <com/sun/star/code/frontend/XEditorHost.hpp>
#include <com/sun/star/code/frontend/XWorkbench.hpp>
#include <com/sun/star/code/output/XOutputChannel.hpp>
#include <com/sun/star/code/output/XOutputController.hpp>
#include <com/sun/star/code/output/theOutputController.hpp>
#include <com/sun/star/code/task/Task.hpp>
#include <com/sun/star/code/task/XTaskHub.hpp>
#include <com/sun/star/code/task/theTaskHub.hpp>
#include <comphelper/processfactory.hxx>
#include <o3tl/safeint.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/ComboBox.hxx>
#include <vcl/weld/TextView.hxx>
#include <vcl/weld/Toolbar.hxx>

#include <algorithm>

using namespace css;
using namespace css::code::output;

namespace code::sidebar
{

namespace
{
uno::Reference<uno::XComponentContext> Context() { return comphelper::getProcessComponentContext(); }

// "14:32:01.153" -- the time of day only, like Visual Studio's timestamps
OUString FormatTime(const util::DateTime& rTime)
{
    auto two = [](sal_Int32 n) { return (n < 10 ? u"0"_ustr : u""_ustr) + OUString::number(n); };
    const sal_Int32 nMilli = rTime.NanoSeconds / 1000000;
    return two(rTime.Hours) + ":" + two(rTime.Minutes) + ":" + two(rTime.Seconds) + "."
           + (nMilli < 100 ? (nMilli < 10 ? u"00"_ustr : u"0"_ustr) : u""_ustr)
           + OUString::number(nMilli);
}
}

OutputLogPanel::OutputLogPanel(weld::Widget* pParent)
    : PanelLayout(pParent, u"OutputPanel"_ustr, u"code/ui/outputpanel.ui"_ustr)
    , mxSource(m_xBuilder->weld_combo_box(u"source"_ustr))
    , mxToolbar(m_xBuilder->weld_toolbar(u"toolbar"_ustr))
    , mxLog(m_xBuilder->weld_text_view(u"log"_ustr))
    , mnCurrentMessage(-1)
    , mbShowTimestamps(false)
{
    mxSource->connect_changed(LINK(this, OutputLogPanel, SourceChangedHdl));
    mxToolbar->connect_clicked(LINK(this, OutputLogPanel, ToolbarClickHdl));

    // Lines arrive on whatever thread produced them; the text view is only
    // touched here, on the main thread, once per burst.
    mxNotifier = std::make_shared<::code::UiNotifier>([this] { Refresh(); });
    mxListener = new ::code::ServiceListener(mxNotifier);
    try
    {
        theOutputController::get(Context())->addOutputListener(mxListener);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "Output cannot reach the output controller: " << rException.Message);
    }

    Refresh();
}

OutputLogPanel::~OutputLogPanel()
{
    mxNotifier->Detach();
    try
    {
        theOutputController::get(Context())->removeOutputListener(mxListener);
    }
    catch (const uno::Exception&)
    {
    }
}

uno::Reference<XOutputChannel> OutputLogPanel::CurrentChannel() const
{
    const int nActive = mxSource->get_active();
    if (nActive < 0 || o3tl::make_unsigned(nActive) >= maChannelNames.size())
        return {};
    try
    {
        return theOutputController::get(Context())->getChannel(maChannelNames[nActive]);
    }
    catch (const uno::Exception&)
    {
        return {};
    }
}

void OutputLogPanel::Refresh()
{
    // Rebuild the source list only if the set of channels changed, so that
    // a line arriving does not reset the user's selection.
    std::vector<OUString> aNames;
    std::vector<OUString> aDisplayNames;
    try
    {
        for (const auto& xChannel : theOutputController::get(Context())->getChannels())
        {
            aNames.push_back(xChannel->getName());
            aDisplayNames.push_back(xChannel->getDisplayName());
        }
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot list the output channels: " << rException.Message);
    }

    if (aNames != maChannelNames)
    {
        const OUString aKeep = mxSource->get_active() >= 0 && o3tl::make_unsigned(mxSource->get_active()) < maChannelNames.size()
                                   ? maChannelNames[mxSource->get_active()]
                                   : OUString();
        mxSource->clear();
        for (const auto& rDisplayName : aDisplayNames)
            mxSource->append_text(rDisplayName);

        // A channel that was not there before is one that has just started
        // producing output (a build, a configure run): show it, as Visual
        // Studio switches "Show output from:" to the tool that is running.
        // The very first fill is not such a case.
        const auto itNew = std::find_if(aNames.begin(), aNames.end(), [&](const OUString& rName)
                                        { return std::find(maChannelNames.begin(), maChannelNames.end(), rName)
                                                 == maChannelNames.end(); });
        // (maChannelNames still holds the previous list here)
        const auto it = std::find(aNames.begin(), aNames.end(), aKeep);
        if (!maChannelNames.empty() && itNew != aNames.end())
            mxSource->set_active(static_cast<int>(itNew - aNames.begin()));
        else if (it != aNames.end())
            mxSource->set_active(static_cast<int>(it - aNames.begin()));
        else if (!aNames.empty())
            mxSource->set_active(0);
        maChannelNames = aNames;
    }

    UpdateLogText();
}

void OutputLogPanel::UpdateLogText()
{
    maLines.clear();
    if (auto xChannel = CurrentChannel())
    {
        try
        {
            const uno::Sequence<OutputLine> aContent = xChannel->getContent();
            maLines.assign(aContent.begin(), aContent.end());
        }
        catch (const uno::Exception& rException)
        {
            SAL_WARN("code", "cannot read the output channel: " << rException.Message);
        }
    }

    if (mnCurrentMessage >= static_cast<int>(maLines.size()))
        mnCurrentMessage = static_cast<int>(maLines.size()) - 1;

    OUStringBuffer aText;
    for (size_t i = 0; i < maLines.size(); ++i)
    {
        if (i > 0)
            aText.append('\n');
        if (mbShowTimestamps)
            aText.append(FormatTime(maLines[i].Time) + " ");
        aText.append(maLines[i].Text);
    }
    mxLog->set_text(aText.makeStringAndClear());

    // Re-highlight the current message (if any) in the freshly set text --
    // selection does not survive set_text().
    if (mnCurrentMessage < 0)
        return;
    int nStart = 0;
    for (int i = 0; i < mnCurrentMessage; ++i)
    {
        nStart += maLines[i].Text.getLength() + 1; // + '\n'
        if (mbShowTimestamps)
            nStart += FormatTime(maLines[i].Time).getLength() + 1; // + ' '
    }
    int nEnd = nStart + maLines[mnCurrentMessage].Text.getLength();
    if (mbShowTimestamps)
        nEnd += FormatTime(maLines[mnCurrentMessage].Time).getLength() + 1;
    mxLog->select_region(nStart, nEnd);
}

IMPL_LINK_NOARG(OutputLogPanel, SourceChangedHdl, weld::ComboBox&, void)
{
    // The message cursor belongs to one channel's lines.
    mnCurrentMessage = -1;
    UpdateLogText();
}

// "Find Message in Code": the highlighted line is searched for among the
// tasks the output filters made of this channel's lines, and the one whose
// description the line contains is activated -- the same thing a double click
// in the Error List does.
void OutputLogPanel::JumpToTask(int nLine)
{
    if (nLine < 0 || o3tl::make_unsigned(nLine) >= maLines.size())
        return;
    const OUString& rText = maLines[nLine].Text;
    try
    {
        auto xHub = css::code::task::theTaskHub::get(Context());
        for (const auto& rTask : xHub->getTasksByCategory(u"build"_ustr))
            if (!rTask.Description.isEmpty() && rText.indexOf(rTask.Description) >= 0)
            {
                xHub->handleTask(rTask.Identifier);
                return;
            }
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot jump to the message's source: " << rException.Message);
    }
}

IMPL_LINK(OutputLogPanel, ToolbarClickHdl, const OUString&, rIdent, void)
{
    const int nLines = static_cast<int>(maLines.size());

    if (rIdent == "find")
    {
        JumpToTask(mnCurrentMessage);
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
        // Through the service, so that the channel is empty for everyone; the
        // channelCleared event brings the panel back here to redraw.
        if (auto xChannel = CurrentChannel())
        {
            try
            {
                xChannel->clear();
            }
            catch (const uno::Exception& rException)
            {
                SAL_WARN("code", "cannot clear the output channel: " << rException.Message);
            }
        }
        mnCurrentMessage = -1;
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
