/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/window.hxx>
#include <vcl/vclptr.hxx>
#include <tools/link.hxx>
#include <rtl/ustring.hxx>
#include <vector>

class TabBar;

// The "document" area: a single visible window whose content is swapped
// when the tab selection changes, exactly like basctl's Shell/
// ModulWindowLayout does with its own TabBar + WindowTable -- one real
// widget kept alive at a time, not one page per tab. Paints fixed fake
// source text; not an editable control. Swap for a real text-edit widget
// once this stops being a prop.
class EditorContentWindow final : public vcl::Window
{
public:
    explicit EditorContentWindow(vcl::Window* pParent);
    virtual ~EditorContentWindow() override;

    virtual void Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect) override;

    void SetSourceLines(std::vector<OUString> aLines);

private:
    std::vector<OUString> maLines;
};

class EditorAreaPanel final : public vcl::Window
{
public:
    explicit EditorAreaPanel(vcl::Window* pParent);
    virtual ~EditorAreaPanel() override;
    virtual void dispose() override;
    virtual void Resize() override;

private:
    DECL_LINK(TabSelectHdl, TabBar*, void);
    void UpdateContentForCurrentTab();

    VclPtr<TabBar> mxTabBar;
    VclPtr<EditorContentWindow> mxContent;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
