/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/sidebar/PanelLayout.hxx>

#include <vector>

namespace weld { class ComboBox; class TextView; class Toolbar; }

namespace code::sidebar
{

// Real sfx2::sidebar::PanelLayout replacement for the old, hand-docked
// OutputPanel (code/source/outputpanel.hxx, now retired) -- same fake
// build-log content, ported from a raw DockingWindow::Paint() to a real
// weld::TextView, the same way SolutionPanel ported the old
// SolutionExplorerPanel's raw ListBox to a weld::TreeView.
//
// Composite like Visual Studio's Output window: a toolbar strip above the
// log ("Show output from:" source selector + find / prev / next / clear /
// word-wrap / timestamp buttons), then the text view.
class OutputLogPanel final : public PanelLayout
{
public:
    explicit OutputLogPanel(weld::Widget* pParent);
    virtual ~OutputLogPanel() override;

private:
    // One line of the (demo) log: the message itself plus the timestamp
    // shown in front of it while "Show Timestamp" is active.
    struct LogLine
    {
        OUString msTimestamp;
        OUString msText;
    };

    // One entry of the "Show output from:" selector with its own log.
    struct LogSource
    {
        OUString msName;
        std::vector<LogLine> maLines;
    };

    void UpdateLogText();
    int CurrentSourceIndex() const;

    DECL_LINK(SourceChangedHdl, weld::ComboBox&, void);
    DECL_LINK(ToolbarClickHdl, const OUString&, void);

    std::unique_ptr<weld::ComboBox> mxSource;
    std::unique_ptr<weld::Toolbar> mxToolbar;
    std::unique_ptr<weld::TextView> mxLog;

    std::vector<LogSource> maSources;
    // Index of the line highlighted by "Go to Previous/Next Message" in the
    // current source's log, -1 = nothing selected yet.
    int mnCurrentMessage;
    bool mbShowTimestamps;
};

} // namespace code::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
