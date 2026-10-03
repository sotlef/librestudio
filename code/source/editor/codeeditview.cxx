/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "codeeditview.hxx"

#include <com/sun/star/code/semantic/SemanticTokenKind.hpp>
#include <com/sun/star/code/semantic/SemanticTokenModifiers.hpp>
#include <com/sun/star/code/semantic/SemanticTokens.hpp>
#include <com/sun/star/code/semantic/XSemanticTokenController.hpp>
#include <com/sun/star/code/semantic/theSemanticTokenController.hpp>
#include <comphelper/processfactory.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <vcl/event.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/ScrolledWindow.hxx>

#include <algorithm>

using namespace css;
using namespace css::code::semantic;

namespace code
{
namespace
{
constexpr sal_Int32 TabWidth = 4;
constexpr tools::Long Padding = 6; // around the line numbers, and before the text

struct Palette
{
    Color aText, aKeyword, aComment, aString, aNumber, aPreprocessor, aType, aFunction, aVariable,
        aNamespace, aMember, aOperator;
};

// The colours of the usual light and dark editor themes; later the user's own
// (the style table the plan names) takes their place.
Palette PaletteFor(bool bDark)
{
    if (bDark)
        return { COL_WHITE,         Color(0x56, 0x9C, 0xD6), Color(0x6A, 0x99, 0x55),
                 Color(0xCE, 0x91, 0x78), Color(0xB5, 0xCE, 0xA8), Color(0xC5, 0x86, 0xC0),
                 Color(0x4E, 0xC9, 0xB0), Color(0xDC, 0xDC, 0xAA), Color(0x9C, 0xDC, 0xFE),
                 Color(0x4E, 0xC9, 0xB0), Color(0x9C, 0xDC, 0xFE), Color(0xD4, 0xD4, 0xD4) };
    return { COL_BLACK,          Color(0x00, 0x00, 0xFF), Color(0x00, 0x80, 0x00),
             Color(0xA3, 0x15, 0x15), Color(0x09, 0x86, 0x58), Color(0xAF, 0x00, 0xDB),
             Color(0x26, 0x7F, 0x99), Color(0x79, 0x5E, 0x26), Color(0x00, 0x10, 0x80),
             Color(0x26, 0x7F, 0x99), Color(0x00, 0x10, 0x80), Color(0x40, 0x40, 0x40) };
}

Color ColorOf(const Palette& rPalette, const SemanticToken& rToken)
{
    switch (rToken.Kind)
    {
        case SemanticTokenKind_KEYWORD:
            return rPalette.aKeyword;
        case SemanticTokenKind_COMMENT:
            return rPalette.aComment;
        case SemanticTokenKind_STRING:
            return rPalette.aString;
        case SemanticTokenKind_NUMBER:
            return rPalette.aNumber;
        case SemanticTokenKind_OPERATOR:
            return rPalette.aOperator;
        case SemanticTokenKind_PREPROCESSOR:
        case SemanticTokenKind_MACRO:
            return rPalette.aPreprocessor;
        case SemanticTokenKind_NAMESPACE:
            return rPalette.aNamespace;
        case SemanticTokenKind_TYPE:
        case SemanticTokenKind_CLASS:
        case SemanticTokenKind_STRUCT:
        case SemanticTokenKind_ENUM:
        case SemanticTokenKind_INTERFACE:
        case SemanticTokenKind_TYPE_PARAMETER:
            return rPalette.aType;
        case SemanticTokenKind_FUNCTION:
        case SemanticTokenKind_METHOD:
            return rPalette.aFunction;
        case SemanticTokenKind_ENUM_MEMBER:
        case SemanticTokenKind_PROPERTY:
            return rPalette.aMember;
        case SemanticTokenKind_VARIABLE:
        case SemanticTokenKind_PARAMETER:
        case SemanticTokenKind_LABEL:
            return rPalette.aVariable;
    }
    return rPalette.aText;
}
}

CodeEditView::CodeEditView(weld::Builder& rBuilder,
                           uno::Reference<css::code::document::XTextDocument> xDocument)
    : m_xDocument(std::move(xDocument))
    // the view scrolls itself: the window only holds the bars
    , m_xScroll(rBuilder.weld_scrolled_window(u"EditorDocumentContent"_ustr, true))
{
    m_xScroll->connect_vadjustment_value_changed(LINK(this, CodeEditView, ScrollHdl));
    m_xScroll->connect_hadjustment_value_changed(LINK(this, CodeEditView, ScrollHdl));
    m_xDrawing = std::make_unique<weld::CustomWeld>(rBuilder, u"editview"_ustr, *this);
    Refresh();
}

CodeEditView::~CodeEditView() = default;

void CodeEditView::SetDrawingArea(weld::DrawingArea* pDrawingArea)
{
    weld::CustomWidgetController::SetDrawingArea(pDrawingArea);
    pDrawingArea->set_cursor(PointerStyle::Text);
    pDrawingArea->get_ref_device().SetBackground(
        Application::GetSettings().GetStyleSettings().GetFieldColor());
}

void CodeEditView::MeasureFont(vcl::RenderContext& rRenderContext)
{
    if (m_bFontMeasured)
        return;
    m_aFont = Application::GetSettings().GetStyleSettings().GetFieldFont();
    m_aFont.SetFamilyName(u"monospace"_ustr);
    m_aFont.SetPitch(PITCH_FIXED);
    rRenderContext.SetFont(m_aFont);
    m_nCharWidth = std::max<tools::Long>(1, rRenderContext.GetTextWidth(u"0000000000"_ustr) / 10);
    m_nLineHeight = std::max<tools::Long>(1, rRenderContext.GetTextHeight() + 2);
    m_bFontMeasured = true;
}

OUString CodeEditView::ExpandTabs(const OUString& rLine)
{
    if (rLine.indexOf('\t') < 0)
        return rLine;
    OUStringBuffer aResult;
    sal_Int32 nColumn = 0;
    for (sal_Int32 i = 0; i < rLine.getLength(); ++i)
    {
        if (rLine[i] == '\t')
        {
            const sal_Int32 nSpaces = TabWidth - nColumn % TabWidth;
            for (sal_Int32 n = 0; n < nSpaces; ++n)
                aResult.append(' ');
            nColumn += nSpaces;
        }
        else
        {
            aResult.append(rLine[i]);
            ++nColumn;
        }
    }
    return aResult.makeStringAndClear();
}

sal_Int32 CodeEditView::VisualColumn(const OUString& rLine, sal_Int32 nColumn) const
{
    sal_Int32 nVisual = 0;
    for (sal_Int32 i = 0; i < nColumn && i < rLine.getLength(); ++i)
        nVisual += rLine[i] == '\t' ? TabWidth - nVisual % TabWidth : 1;
    return nVisual;
}

void CodeEditView::Refresh()
{
    OUString aText;
    SemanticTokens aTokens;
    sal_Int32 nVersion = -1;
    try
    {
        aText = m_xDocument->getText();
        nVersion = m_xDocument->getVersion();
        aTokens = theSemanticTokenController::get(comphelper::getProcessComponentContext())
                      ->getSemanticTokens(m_xDocument->getUrl());
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot read the document: " << rException.Message);
    }

    m_aLines.clear();
    m_nMaxColumns = 0;
    for (sal_Int32 nStart = 0;;)
    {
        const sal_Int32 nEol = aText.indexOf('\n', nStart);
        sal_Int32 nEnd = nEol < 0 ? aText.getLength() : nEol;
        const sal_Int32 nNext = nEol < 0 ? -1 : nEol + 1;
        if (nEnd > nStart && aText[nEnd - 1] == '\r')
            --nEnd;
        m_aLines.push_back(aText.copy(nStart, nEnd - nStart));
        m_nMaxColumns = std::max(m_nMaxColumns, VisualColumn(m_aLines.back(), nEnd - nStart));
        if (nNext < 0)
            break;
        nStart = nNext;
    }

    // Tokens of another version of the text would colour the wrong words;
    // until the analysis has caught up the text is plain.
    m_aTokens.assign(m_aLines.size(), {});
    if (aTokens.Version == nVersion)
        for (const SemanticToken& rToken : aTokens.Tokens)
            if (rToken.Start.Line >= 0 && static_cast<size_t>(rToken.Start.Line) < m_aTokens.size())
                m_aTokens[rToken.Start.Line].push_back(rToken);
    for (auto& rLine : m_aTokens)
        std::stable_sort(rLine.begin(), rLine.end(),
                         [](const SemanticToken& a, const SemanticToken& b)
                         { return a.Start.Column < b.Start.Column; });

    if (GetDrawingArea())
    {
        ConfigureScrollBars();
        Invalidate();
    }
}

OUString CodeEditView::GetText() const
{
    OUStringBuffer aText;
    for (size_t i = 0; i < m_aLines.size(); ++i)
    {
        if (i)
            aText.append('\n');
        aText.append(m_aLines[i]);
    }
    return aText.makeStringAndClear();
}

tools::Long CodeEditView::GutterWidth() const
{
    sal_Int32 nDigits = 3;
    for (size_t n = m_aLines.size(); n >= 1000; n /= 10)
        ++nDigits;
    return nDigits * m_nCharWidth + 2 * Padding;
}

void CodeEditView::ConfigureScrollBars()
{
    if (!m_bFontMeasured)
        return; // the first Paint measures, and configures
    const Size aSize = GetOutputSizePixel();
    const tools::Long nTextWidth = std::max<tools::Long>(0, aSize.Width() - GutterWidth());
    m_xScroll->vadjustment_configure(
        std::min<tools::Long>(m_xScroll->vadjustment_get_value(),
                              std::max<tools::Long>(0, m_aLines.size() * m_nLineHeight - aSize.Height())),
        m_aLines.size() * m_nLineHeight, m_nLineHeight, aSize.Height() * 9 / 10, aSize.Height());
    m_xScroll->hadjustment_configure(
        std::min<tools::Long>(m_xScroll->hadjustment_get_value(),
                              std::max<tools::Long>(0, (m_nMaxColumns + 1) * m_nCharWidth - nTextWidth)),
        (m_nMaxColumns + 1) * m_nCharWidth, m_nCharWidth * 4, nTextWidth * 9 / 10, nTextWidth);
}

void CodeEditView::Resize()
{
    ConfigureScrollBars();
    Invalidate();
}

void CodeEditView::StyleUpdated()
{
    m_bFontMeasured = false;
    if (GetDrawingArea())
        GetDrawingArea()->get_ref_device().SetBackground(
            Application::GetSettings().GetStyleSettings().GetFieldColor());
    Invalidate();
}

IMPL_LINK_NOARG(CodeEditView, ScrollHdl, weld::ScrolledWindow&, void) { Invalidate(); }

void CodeEditView::RevealPosition(const css::code::core::Position& rPosition)
{
    if (!m_bFontMeasured)
        return;
    const Size aSize = GetOutputSizePixel();
    const sal_Int32 nLine = std::clamp<sal_Int32>(rPosition.Line, 0, m_aLines.size() - 1);
    const tools::Long nTop = nLine * m_nLineHeight;
    const tools::Long nValue = m_xScroll->vadjustment_get_value();
    if (nTop < nValue || nTop + m_nLineHeight > nValue + aSize.Height())
        m_xScroll->vadjustment_set_value(std::max<tools::Long>(0, nTop - aSize.Height() / 3));

    const tools::Long nLeft
        = VisualColumn(m_aLines[nLine], rPosition.Column) * m_nCharWidth;
    const tools::Long nTextWidth = aSize.Width() - GutterWidth();
    const tools::Long nHValue = m_xScroll->hadjustment_get_value();
    if (nLeft < nHValue || nLeft + m_nCharWidth > nHValue + nTextWidth)
        m_xScroll->hadjustment_set_value(std::max<tools::Long>(0, nLeft - nTextWidth / 3));
    Invalidate();
}

void CodeEditView::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle&)
{
    const StyleSettings& rStyle = Application::GetSettings().GetStyleSettings();
    const bool bFirst = !m_bFontMeasured;
    MeasureFont(rRenderContext);
    if (bFirst)
        ConfigureScrollBars();
    rRenderContext.SetFont(m_aFont);

    const Size aSize = GetOutputSizePixel();
    const Palette aPalette = PaletteFor(rStyle.GetFieldColor().IsDark());
    const tools::Long nGutter = GutterWidth();

    rRenderContext.SetLineColor();
    rRenderContext.SetFillColor(rStyle.GetFieldColor());
    rRenderContext.DrawRect(tools::Rectangle(Point(0, 0), aSize));

    const tools::Long nTop = m_xScroll->vadjustment_get_value();
    const tools::Long nLeft = m_xScroll->hadjustment_get_value();
    const sal_Int32 nFirst = static_cast<sal_Int32>(nTop / m_nLineHeight);
    const sal_Int32 nLast = std::min<sal_Int32>(m_aLines.size() - 1,
                                                (nTop + aSize.Height()) / m_nLineHeight);

    // the text, clipped so that a scrolled line does not run under the numbers
    rRenderContext.SetClipRegion(vcl::Region(tools::Rectangle(Point(nGutter, 0), aSize)));
    for (sal_Int32 nLine = nFirst; nLine <= nLast; ++nLine)
    {
        const tools::Long nY = nLine * m_nLineHeight - nTop + 1;
        const OUString& rLine = m_aLines[nLine];

        // The line is cut into runs: the text between tokens in the plain colour,
        // each token in its own.  Positions are visual columns, so tabs line up.
        sal_Int32 nDone = 0;
        auto drawRun = [&](sal_Int32 nFrom, sal_Int32 nTo, const Color& rColor)
        {
            if (nTo <= nFrom)
                return;
            rRenderContext.SetTextColor(rColor);
            const OUString aRun = ExpandTabs(rLine.copy(0, nTo)).copy(VisualColumn(rLine, nFrom));
            rRenderContext.DrawText(
                Point(nGutter + Padding + VisualColumn(rLine, nFrom) * m_nCharWidth - nLeft, nY),
                aRun);
        };
        for (const SemanticToken& rToken : m_aTokens[nLine])
        {
            const sal_Int32 nStart = std::clamp(rToken.Start.Column, nDone, rLine.getLength());
            const sal_Int32 nEnd = std::min(nStart + rToken.Length, rLine.getLength());
            drawRun(nDone, nStart, aPalette.aText);
            drawRun(nStart, nEnd, ColorOf(aPalette, rToken));
            nDone = std::max(nDone, nEnd);
        }
        drawRun(nDone, rLine.getLength(), aPalette.aText);
    }
    rRenderContext.SetClipRegion();

    // the line numbers
    rRenderContext.SetFillColor(rStyle.GetFaceColor());
    rRenderContext.DrawRect(tools::Rectangle(Point(0, 0), Size(nGutter - 1, aSize.Height())));
    rRenderContext.SetTextColor(rStyle.GetDisableColor());
    for (sal_Int32 nLine = nFirst; nLine <= nLast; ++nLine)
    {
        const OUString aNumber = OUString::number(nLine + 1);
        rRenderContext.DrawText(
            Point(nGutter - Padding - aNumber.getLength() * m_nCharWidth,
                  nLine * m_nLineHeight - nTop + 1),
            aNumber);
    }
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
