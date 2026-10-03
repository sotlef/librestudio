/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/output/OutputLine.hpp>
#include <com/sun/star/code/output/XOutputChannel.hpp>
#include <com/sun/star/code/output/XOutputListener.hpp>
#include <sfx2/sidebar/PanelLayout.hxx>

#include <memory>
#include <vector>

namespace weld { class ComboBox; class TextView; class Toolbar; }

namespace code { class UiNotifier; }

namespace code::sidebar
{

// The Output window, like Visual Studio's: a toolbar strip ("Show output
// from:" source selector + find / prev / next / clear / word-wrap /
// timestamp buttons) above the log.  The sources are the channels of
// theOutputController, the log is the selected channel's content; both follow
// the service (a channel appearing, a line arriving, a channel being cleared).
class OutputLogPanel final : public PanelLayout
{
public:
    explicit OutputLogPanel(weld::Widget* pParent);
    virtual ~OutputLogPanel() override;

private:
    // Re-reads the channel list and the selected channel.  Everything the
    // service tells the panel funnels into this; it runs on the main thread.
    void Refresh();
    // The log text of the selected channel; also records where each line starts.
    void UpdateLogText();
    css::uno::Reference<css::code::output::XOutputChannel> CurrentChannel() const;
    void JumpToTask(int nLine);

    DECL_LINK(SourceChangedHdl, weld::ComboBox&, void);
    DECL_LINK(ToolbarClickHdl, const OUString&, void);

    std::unique_ptr<weld::ComboBox> mxSource;
    std::unique_ptr<weld::Toolbar> mxToolbar;
    std::unique_ptr<weld::TextView> mxLog;

    // names of the channels, parallel to the combo box entries
    std::vector<OUString> maChannelNames;
    // the lines shown now (to find a line's own text again when highlighting it)
    std::vector<css::code::output::OutputLine> maLines;
    // Index of the line highlighted by "Go to Previous/Next Message", -1 = none.
    int mnCurrentMessage;
    bool mbShowTimestamps;

    std::shared_ptr<::code::UiNotifier> mxNotifier;
    css::uno::Reference<css::code::output::XOutputListener> mxListener;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
