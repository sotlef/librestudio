/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/dockwin.hxx>
#include <rtl/ustring.hxx>
#include <vector>

// Stand-in for the Output/build-log panel: paints a fixed fake log, nothing
// real is ever built. Swap for a real log sink once there is something to
// log.
class OutputPanel final : public DockingWindow
{
public:
    explicit OutputPanel(vcl::Window* pParent);
    virtual ~OutputPanel() override;

    virtual void Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect) override;

private:
    std::vector<OUString> maLines;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
