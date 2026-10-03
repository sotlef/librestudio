/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/code/core/Position.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/code/semantic/SemanticToken.hpp>
#include <tools/link.hxx>
#include <vcl/font.hxx>
#include <vcl/weld/customweld.hxx>

#include <memory>
#include <vector>

namespace weld
{
class Builder;
class ScrolledWindow;
}

namespace code
{
/// What shows the text of one document: line numbers on the left, the text
/// coloured by the semantic tokens the analysis service has for it.  It only
/// displays; the text, and the tokens, belong to the services, and Refresh()
/// is how the view is told they may have changed.  Not yet: caret, selection,
/// typing.
///
/// The view is the widget (a weld::CustomWidgetController) and owns the
/// scrolled window it sits in; scrolling is the view's own, in pixels.
class CodeEditView final : public weld::CustomWidgetController
{
public:
    /// rBuilder holds a scrolled window "EditorDocumentContent" with the
    /// drawing area "editview" in it.
    CodeEditView(weld::Builder& rBuilder,
                 css::uno::Reference<css::code::document::XTextDocument> xDocument);
    virtual ~CodeEditView() override;

    /// Reads the text and the tokens again.  Main thread.
    void Refresh();
    /// Scrolls so that the position is in view.
    void RevealPosition(const css::code::core::Position& rPosition);
    OUString GetText() const;

    // weld::CustomWidgetController
    virtual void SetDrawingArea(weld::DrawingArea* pDrawingArea) override;
    virtual void Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect) override;
    virtual void Resize() override;
    virtual void StyleUpdated() override;

private:
    DECL_LINK(ScrollHdl, weld::ScrolledWindow&, void);

    void MeasureFont(vcl::RenderContext& rRenderContext);
    void ConfigureScrollBars();
    // width in pixels of the column of line numbers, with its padding
    tools::Long GutterWidth() const;
    // visual column (tabs expanded) of a UTF-16 column of a line
    sal_Int32 VisualColumn(const OUString& rLine, sal_Int32 nColumn) const;
    static OUString ExpandTabs(const OUString& rLine);

    css::uno::Reference<css::code::document::XTextDocument> m_xDocument;
    std::unique_ptr<weld::ScrolledWindow> m_xScroll;
    std::unique_ptr<weld::CustomWeld> m_xDrawing;

    std::vector<OUString> m_aLines;
    // the tokens by line, in column order
    std::vector<std::vector<css::code::semantic::SemanticToken>> m_aTokens;
    sal_Int32 m_nMaxColumns = 0;

    vcl::Font m_aFont;
    tools::Long m_nCharWidth = 0;
    tools::Long m_nLineHeight = 0;
    bool m_bFontMeasured = false;
    bool m_bConfiguring = false;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
