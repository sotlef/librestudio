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
#include <rtl/character.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <com/sun/star/datatransfer/DataFlavor.hpp>
#include <com/sun/star/datatransfer/XTransferable.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboard.hpp>
#include <vcl/event.hxx>
#include <vcl/keycodes.hxx>
#include <vcl/unohelp2.hxx>
#include <vcl/ptrstyle.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/ScrolledWindow.hxx>

#include <algorithm>
#include <cstdlib>

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
    , m_aBlink("code::CodeEditView caret blink")
{
    m_aBlink.SetTimeout(530);
    m_aBlink.SetInvokeHandler(LINK(this, CodeEditView, BlinkHdl));
    m_xScroll->connect_vadjustment_value_changed(LINK(this, CodeEditView, ScrollHdl));
    m_xScroll->connect_hadjustment_value_changed(LINK(this, CodeEditView, ScrollHdl));
    m_xDrawing = std::make_unique<weld::CustomWeld>(rBuilder, u"editview"_ustr, *this);
    Refresh();
}

CodeEditView::~CodeEditView() { m_aBlink.Stop(); }

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

sal_Int32 CodeEditView::VisualColumn(const OUString& rLine, sal_Int32 nColumn)
{
    sal_Int32 nVisual = 0;
    for (sal_Int32 i = 0; i < nColumn && i < rLine.getLength(); ++i)
        nVisual += rLine[i] == '\t' ? TabWidth - nVisual % TabWidth : 1;
    // a column beyond the line's end is that many cells further
    if (nColumn > rLine.getLength())
        nVisual += nColumn - rLine.getLength();
    return nVisual;
}

sal_Int32 CodeEditView::ColumnOfVisual(const OUString& rLine, sal_Int32 nVisual)
{
    sal_Int32 nCell = 0;
    for (sal_Int32 i = 0; i < rLine.getLength(); ++i)
    {
        const sal_Int32 nWidth = rLine[i] == '\t' ? TabWidth - nCell % TabWidth : 1;
        if (nVisual < nCell + (nWidth + 1) / 2)
            return i;
        nCell += nWidth;
    }
    return rLine.getLength();
}

// ---- the text -------------------------------------------------------------

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

    // Tokens of another version of the text would colour the wrong words.  The
    // text this view has just edited itself is the exception: the tokens it
    // had, shifted by its edits (ShiftTokens), are still a good guess until the
    // analysis catches up.  After any other change the text is plain.
    if (aTokens.Version == nVersion)
    {
        m_aTokens.assign(m_aLines.size(), {});
        for (const SemanticToken& rToken : aTokens.Tokens)
            if (rToken.Start.Line >= 0 && static_cast<size_t>(rToken.Start.Line) < m_aTokens.size())
                m_aTokens[rToken.Start.Line].push_back(rToken);
        for (auto& rLine : m_aTokens)
            std::stable_sort(rLine.begin(), rLine.end(),
                             [](const SemanticToken& a, const SemanticToken& b)
                             { return a.Start.Column < b.Start.Column; });
    }
    else if (nVersion != m_nOwnVersion || m_aTokens.size() != m_aLines.size())
        m_aTokens.assign(m_aLines.size(), {});

    // somebody else changed the text: what could be undone is no longer there
    if (nVersion != m_nOwnVersion)
    {
        m_aUndo.clear();
        m_aRedo.clear();
    }

    m_aCaret = ClampPosition(m_aCaret);
    m_aAnchor = ClampPosition(m_aAnchor);

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

bool CodeEditView::Before(const Position& rA, const Position& rB)
{
    return rA.Line < rB.Line || (rA.Line == rB.Line && rA.Column < rB.Column);
}

CodeEditView::Position CodeEditView::EndOf(const Position& rStart, const OUString& rText)
{
    Position aEnd = rStart;
    sal_Int32 nLast = 0;
    for (sal_Int32 n = rText.indexOf('\n'); n >= 0; n = rText.indexOf('\n', n + 1))
    {
        ++aEnd.Line;
        nLast = n + 1;
    }
    aEnd.Column = aEnd.Line == rStart.Line ? rStart.Column + rText.getLength()
                                           : rText.getLength() - nLast;
    return aEnd;
}

CodeEditView::Position CodeEditView::ClampPosition(Position aPosition) const
{
    aPosition.Line = std::clamp<sal_Int32>(aPosition.Line, 0, m_aLines.size() - 1);
    aPosition.Column = std::clamp<sal_Int32>(aPosition.Column, 0, m_aLines[aPosition.Line].getLength());
    return aPosition;
}

OUString CodeEditView::TextIn(const Range& rRange) const
{
    if (rRange.Start.Line == rRange.End.Line)
        return m_aLines[rRange.Start.Line].copy(rRange.Start.Column,
                                                rRange.End.Column - rRange.Start.Column);
    OUStringBuffer aText;
    aText.append(m_aLines[rRange.Start.Line].subView(rRange.Start.Column));
    for (sal_Int32 nLine = rRange.Start.Line + 1; nLine < rRange.End.Line; ++nLine)
        aText.append('\n').append(m_aLines[nLine]);
    aText.append('\n').append(m_aLines[rRange.End.Line].subView(0, rRange.End.Column));
    return aText.makeStringAndClear();
}

bool CodeEditView::HasSelection() const
{
    if (m_bRect)
        return m_nAnchorVisual != m_nCaretVisual;
    return m_aCaret.Line != m_aAnchor.Line || m_aCaret.Column != m_aAnchor.Column;
}

CodeEditView::Range CodeEditView::Selection() const
{
    Range aRange;
    if (Before(m_aCaret, m_aAnchor))
    {
        aRange.Start = m_aCaret;
        aRange.End = m_aAnchor;
    }
    else
    {
        aRange.Start = m_aAnchor;
        aRange.End = m_aCaret;
    }
    return aRange;
}

CodeEditView::Position CodeEditView::PreviousPosition(const Position& rPosition) const
{
    Position aResult = rPosition;
    if (aResult.Column > 0)
    {
        --aResult.Column;
        const OUString& rLine = m_aLines[aResult.Line];
        if (aResult.Column > 0 && rtl::isLowSurrogate(rLine[aResult.Column])
            && rtl::isHighSurrogate(rLine[aResult.Column - 1]))
            --aResult.Column;
    }
    else if (aResult.Line > 0)
    {
        --aResult.Line;
        aResult.Column = m_aLines[aResult.Line].getLength();
    }
    return aResult;
}

CodeEditView::Position CodeEditView::NextPosition(const Position& rPosition) const
{
    Position aResult = rPosition;
    const OUString& rLine = m_aLines[aResult.Line];
    if (aResult.Column < rLine.getLength())
    {
        if (rtl::isHighSurrogate(rLine[aResult.Column]) && aResult.Column + 1 < rLine.getLength()
            && rtl::isLowSurrogate(rLine[aResult.Column + 1]))
            ++aResult.Column;
        ++aResult.Column;
    }
    else if (static_cast<size_t>(aResult.Line) + 1 < m_aLines.size())
    {
        ++aResult.Line;
        aResult.Column = 0;
    }
    return aResult;
}

namespace
{
enum class CharClass
{
    Space,
    Word,
    Other
};
CharClass ClassOf(sal_Unicode c)
{
    if (c == ' ' || c == '\t')
        return CharClass::Space;
    if (c == '_' || rtl::isAsciiAlphanumeric(c) || c > 0x7F)
        return CharClass::Word;
    return CharClass::Other;
}
}

CodeEditView::Position CodeEditView::WordLeft(Position aPosition) const
{
    // over the blanks, then over the run of one kind before them; a line
    // break counts as a step of its own
    if (aPosition.Column == 0)
        return PreviousPosition(aPosition);
    const OUString& rLine = m_aLines[aPosition.Line];
    sal_Int32 nColumn = aPosition.Column;
    while (nColumn > 0 && ClassOf(rLine[nColumn - 1]) == CharClass::Space)
        --nColumn;
    if (nColumn > 0)
    {
        const CharClass eClass = ClassOf(rLine[nColumn - 1]);
        while (nColumn > 0 && ClassOf(rLine[nColumn - 1]) == eClass)
            --nColumn;
    }
    aPosition.Column = nColumn;
    return aPosition;
}

CodeEditView::Position CodeEditView::WordRight(Position aPosition) const
{
    const OUString& rLine = m_aLines[aPosition.Line];
    if (aPosition.Column >= rLine.getLength())
        return NextPosition(aPosition);
    sal_Int32 nColumn = aPosition.Column;
    const CharClass eClass = ClassOf(rLine[nColumn]);
    if (eClass != CharClass::Space)
        while (nColumn < rLine.getLength() && ClassOf(rLine[nColumn]) == eClass)
            ++nColumn;
    while (nColumn < rLine.getLength() && ClassOf(rLine[nColumn]) == CharClass::Space)
        ++nColumn;
    aPosition.Column = nColumn;
    return aPosition;
}

void CodeEditView::SelectWordAt(const Position& rPosition)
{
    const OUString& rLine = m_aLines[rPosition.Line];
    sal_Int32 nStart = rPosition.Column;
    sal_Int32 nEnd = rPosition.Column;
    if (!rLine.isEmpty())
    {
        const sal_Int32 nAt = std::min<sal_Int32>(rPosition.Column, rLine.getLength() - 1);
        const CharClass eClass = ClassOf(rLine[nAt]);
        nStart = nAt;
        nEnd = nAt + 1;
        while (nStart > 0 && ClassOf(rLine[nStart - 1]) == eClass)
            --nStart;
        while (nEnd < rLine.getLength() && ClassOf(rLine[nEnd]) == eClass)
            ++nEnd;
    }
    m_aAnchor = { rPosition.Line, nStart };
    m_aCaret = { rPosition.Line, nEnd };
    m_nWantedColumn = VisualColumn(rLine, nEnd);
}

// ---- geometry and scrolling -----------------------------------------------

tools::Long CodeEditView::GutterWidth() const
{
    sal_Int32 nDigits = 3;
    for (size_t n = m_aLines.size(); n >= 1000; n /= 10)
        ++nDigits;
    return nDigits * m_nCharWidth + 2 * Padding;
}

tools::Long CodeEditView::ColumnX(const OUString& rLine, sal_Int32 nColumn) const
{
    return GutterWidth() + Padding + VisualColumn(rLine, nColumn) * m_nCharWidth
           - m_xScroll->hadjustment_get_value();
}

CodeEditView::Position CodeEditView::PositionAt(const Point& rPoint) const
{
    if (!m_bFontMeasured)
        return {};
    const tools::Long nY = rPoint.Y() + m_xScroll->vadjustment_get_value();
    Position aPosition;
    aPosition.Line = std::clamp<sal_Int32>(nY / m_nLineHeight, 0, m_aLines.size() - 1);
    if (nY < 0)
        aPosition.Line = 0;
    const tools::Long nX = rPoint.X() - GutterWidth() - Padding + m_xScroll->hadjustment_get_value();
    const sal_Int32 nVisual
        = static_cast<sal_Int32>(std::max<tools::Long>(0, (nX + m_nCharWidth / 2) / m_nCharWidth));
    aPosition.Column = ColumnOfVisual(m_aLines[aPosition.Line], nVisual);
    return aPosition;
}

void CodeEditView::ConfigureScrollBars()
{
    if (!m_bFontMeasured || m_bConfiguring)
        return; // the first Paint measures, and configures
    // Configuring the bars may resize the drawing area (the Qt scrolled window
    // sets its content to the page size), which comes back here.
    m_bConfiguring = true;
    const Size aSize = GetOutputSizePixel();
    const tools::Long nGutter = GutterWidth();
    // The page is the whole area, as the toolkits want it; the gutter does not
    // scroll, so it is part of the extent instead.
    const tools::Long nHeight = static_cast<tools::Long>(m_aLines.size()) * m_nLineHeight;
    const tools::Long nWidth = nGutter + (m_nMaxColumns + 1) * m_nCharWidth;
    m_xScroll->vadjustment_configure(
        std::min<tools::Long>(m_xScroll->vadjustment_get_value(),
                              std::max<tools::Long>(0, nHeight - aSize.Height())),
        nHeight, m_nLineHeight, aSize.Height() * 9 / 10, aSize.Height());
    m_xScroll->hadjustment_configure(
        std::min<tools::Long>(m_xScroll->hadjustment_get_value(),
                              std::max<tools::Long>(0, nWidth - aSize.Width())),
        nWidth, m_nCharWidth * 4, aSize.Width() * 9 / 10, aSize.Width());
    m_bConfiguring = false;
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

void CodeEditView::ScrollInto(const Position& rPosition, bool bCentre)
{
    if (!m_bFontMeasured || GetOutputSizePixel().Height() <= 0)
    {
        m_bRevealPending = true; // the first Paint knows the sizes
        return;
    }
    const Size aSize = GetOutputSizePixel();
    const Position aPosition = ClampPosition(rPosition);

    const tools::Long nTop = aPosition.Line * m_nLineHeight;
    const tools::Long nValue = m_xScroll->vadjustment_get_value();
    if (nTop < nValue)
        m_xScroll->vadjustment_set_value(
            std::max<tools::Long>(0, bCentre ? nTop - aSize.Height() / 3 : nTop));
    else if (nTop + m_nLineHeight > nValue + aSize.Height())
        m_xScroll->vadjustment_set_value(bCentre ? nTop - aSize.Height() / 3
                                                 : nTop + m_nLineHeight - aSize.Height());

    // the text area is what is right of the numbers
    const tools::Long nTextWidth = aSize.Width() - GutterWidth() - Padding;
    const tools::Long nLeft = VisualColumn(m_aLines[aPosition.Line], aPosition.Column) * m_nCharWidth;
    const tools::Long nHValue = m_xScroll->hadjustment_get_value();
    if (nLeft < nHValue)
        m_xScroll->hadjustment_set_value(std::max<tools::Long>(0, nLeft - nTextWidth / 4));
    else if (nLeft + m_nCharWidth > nHValue + nTextWidth)
        m_xScroll->hadjustment_set_value(nLeft + m_nCharWidth * 2 - nTextWidth);
    Invalidate();
}

void CodeEditView::RevealPosition(const Position& rPosition)
{
    MoveCaret(rPosition, false);
    ScrollInto(m_aCaret, true);
}

// ---- the caret ------------------------------------------------------------

void CodeEditView::ResetBlink()
{
    m_bCaretOn = true;
    if (m_bFocused)
        m_aBlink.Start();
}

IMPL_LINK_NOARG(CodeEditView, BlinkHdl, Timer*, void)
{
    m_bCaretOn = !m_bCaretOn;
    if (!m_bFontMeasured || !GetDrawingArea())
        return;
    // only the caret's own line, or the lines of the rectangle
    const sal_Int32 nFrom = m_bRect ? RectFirstLine() : m_aCaret.Line;
    const sal_Int32 nTo = m_bRect ? RectLastLine() : m_aCaret.Line;
    const tools::Long nY = nFrom * m_nLineHeight - m_xScroll->vadjustment_get_value();
    Invalidate(tools::Rectangle(
        Point(0, nY), Size(GetOutputSizePixel().Width(), (nTo - nFrom + 1) * m_nLineHeight)));
}

void CodeEditView::GetFocus()
{
    m_bFocused = true;
    ResetBlink();
    Invalidate();
}

void CodeEditView::LoseFocus()
{
    m_bFocused = false;
    m_aBlink.Stop();
    m_bCaretOn = true;
    Invalidate();
}

void CodeEditView::MoveCaret(Position aPosition, bool bExtend, bool bKeepWantedColumn)
{
    m_bRect = false;
    m_aCaret = ClampPosition(aPosition);
    if (!bExtend)
        m_aAnchor = m_aCaret;
    if (!bKeepWantedColumn)
        m_nWantedColumn = VisualColumn(m_aLines[m_aCaret.Line], m_aCaret.Column);
    ScrollInto(m_aCaret, false);
    ResetBlink();
    Invalidate();
}

// ---- editing --------------------------------------------------------------

// What was at a range moves tokens that lay after it, and drops those that
// overlap it: after an edit the old colours of what is left of the line are
// right, the new text is plain, and the analysis soon says the rest.
void CodeEditView::ShiftTokens(const Range& rRange, const OUString& rNewText)
{
    const Position aEndNew = EndOf(rRange.Start, rNewText);
    const sal_Int32 nLineDelta = aEndNew.Line - rRange.End.Line;
    std::vector<std::vector<SemanticToken>> aNew(
        std::max<sal_Int32>(1, static_cast<sal_Int32>(m_aTokens.size()) + nLineDelta));
    for (size_t nLine = 0; nLine < m_aTokens.size(); ++nLine)
        for (const SemanticToken& rToken : m_aTokens[nLine])
        {
            const sal_Int32 nColumn = rToken.Start.Column;
            const bool bBefore = static_cast<sal_Int32>(nLine) < rRange.Start.Line
                                 || (static_cast<sal_Int32>(nLine) == rRange.Start.Line
                                     && nColumn + rToken.Length <= rRange.Start.Column);
            const bool bAfter = static_cast<sal_Int32>(nLine) > rRange.End.Line
                                || (static_cast<sal_Int32>(nLine) == rRange.End.Line
                                    && nColumn >= rRange.End.Column);
            SemanticToken aToken = rToken;
            if (bBefore)
            {
            }
            else if (bAfter)
            {
                aToken.Start.Line += nLineDelta;
                if (static_cast<sal_Int32>(nLine) == rRange.End.Line)
                    aToken.Start.Column += aEndNew.Column - rRange.End.Column;
            }
            else
                continue;
            if (aToken.Start.Line >= 0 && static_cast<size_t>(aToken.Start.Line) < aNew.size())
                aNew[aToken.Start.Line].push_back(aToken);
        }
    m_aTokens = std::move(aNew);
}

void CodeEditView::Replace(const Range& rRange, const OUString& rNewText, bool bRecord, bool bMerge)
{
    const Position aCaretBefore = m_aCaret;
    const OUString aOld = TextIn(rRange);
    if (aOld.isEmpty() && rNewText.isEmpty())
        return;

    css::code::document::TextEdit aEdit;
    aEdit.Range = rRange;
    aEdit.NewText = rNewText;
    try
    {
        m_xDocument->applyEdit(aEdit);
        m_nOwnVersion = m_xDocument->getVersion();
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot edit the document: " << rException.Message);
        return;
    }
    ShiftTokens(rRange, rNewText);
    const Position aEndNew = EndOf(rRange.Start, rNewText);

    if (bRecord)
    {
        m_aRedo.clear();
        bool bMerged = false;
        if (bMerge && !m_bChain && !m_aUndo.empty() && aOld.isEmpty() && rNewText.getLength() == 1)
        {
            UndoStep& rLast = m_aUndo.back();
            const Position aLastEnd = EndOf(rLast.aStart, rLast.aNew);
            if (rLast.aOld.isEmpty() && !rLast.aNew.isEmpty() && aLastEnd.Line == rRange.Start.Line
                && aLastEnd.Column == rRange.Start.Column
                && rLast.aCaretAfter.Line == aCaretBefore.Line
                && rLast.aCaretAfter.Column == aCaretBefore.Column
                && rLast.aNew.indexOf('\n') < 0
                // a word is one step, the blanks and punctuation after it another
                && (ClassOf(rLast.aNew[rLast.aNew.getLength() - 1]) == ClassOf(rNewText[0])))
            {
                rLast.aNew += rNewText;
                rLast.aCaretAfter = aEndNew;
                bMerged = true;
            }
        }
        if (!bMerged)
        {
            m_aUndo.push_back({ rRange.Start, aOld, rNewText, aCaretBefore, aEndNew, m_bChain });
            if (m_aUndo.size() > 1000)
                m_aUndo.erase(m_aUndo.begin());
        }
    }

    m_aCaret = aEndNew;
    m_aAnchor = aEndNew;
    Refresh();
    m_nWantedColumn = VisualColumn(m_aLines[m_aCaret.Line], m_aCaret.Column);
    ScrollInto(m_aCaret, false);
    ResetBlink();
}

void CodeEditView::Undo()
{
    // a chain of steps is one action: back to the one that began it
    while (!m_aUndo.empty())
    {
        const UndoStep aStep = m_aUndo.back();
        m_aUndo.pop_back();
        Range aCurrent;
        aCurrent.Start = aStep.aStart;
        aCurrent.End = EndOf(aStep.aStart, aStep.aNew);
        Replace(aCurrent, aStep.aOld, false);
        // Replace() ends with the caret after the restored text, and keeps the
        // outcome of Refresh() for the undo stack: ours, not "somebody else's"
        MoveCaret(aStep.aCaretBefore, false);
        m_aRedo.push_back(aStep);
        if (!aStep.bChained)
            break;
    }
}

void CodeEditView::Redo()
{
    bool bFirst = true;
    while (!m_aRedo.empty() && (bFirst || m_aRedo.back().bChained))
    {
        bFirst = false;
        const UndoStep aStep = m_aRedo.back();
        m_aRedo.pop_back();
        Range aCurrent;
        aCurrent.Start = aStep.aStart;
        aCurrent.End = EndOf(aStep.aStart, aStep.aOld);
        Replace(aCurrent, aStep.aNew, false);
        MoveCaret(aStep.aCaretAfter, false);
        m_aUndo.push_back(aStep);
    }
}

// ---- the rectangular selection ----------------------------------------------

tools::Long CodeEditView::VisualX(sal_Int32 nVisual) const
{
    return GutterWidth() + Padding + nVisual * m_nCharWidth - m_xScroll->hadjustment_get_value();
}

sal_Int32 CodeEditView::VisualAt(const Point& rPoint) const
{
    if (!m_bFontMeasured)
        return 0;
    const tools::Long nX = rPoint.X() - GutterWidth() - Padding + m_xScroll->hadjustment_get_value();
    return static_cast<sal_Int32>(std::max<tools::Long>(0, (nX + m_nCharWidth / 2) / m_nCharWidth));
}

sal_Int32 CodeEditView::RectFirstLine() const { return std::min(m_aAnchor.Line, m_aCaret.Line); }
sal_Int32 CodeEditView::RectLastLine() const { return std::max(m_aAnchor.Line, m_aCaret.Line); }
sal_Int32 CodeEditView::RectLeft() const { return std::min(m_nAnchorVisual, m_nCaretVisual); }
sal_Int32 CodeEditView::RectRight() const { return std::max(m_nAnchorVisual, m_nCaretVisual); }

void CodeEditView::BeginRect()
{
    if (m_bRect)
        return;
    m_nAnchorVisual = VisualColumn(m_aLines[m_aAnchor.Line], m_aAnchor.Column);
    m_nCaretVisual = VisualColumn(m_aLines[m_aCaret.Line], m_aCaret.Column);
    m_bRect = true;
}

void CodeEditView::MoveRect(sal_Int32 nLine, sal_Int32 nVisual)
{
    nLine = std::clamp<sal_Int32>(nLine, 0, m_aLines.size() - 1);
    m_nCaretVisual = std::max<sal_Int32>(0, nVisual);
    m_aCaret = { nLine, ColumnOfVisual(m_aLines[nLine], m_nCaretVisual) };
    m_nWantedColumn = m_nCaretVisual;
    ScrollInto({ nLine, m_aCaret.Column }, false);
    ResetBlink();
    Invalidate();
}

void CodeEditView::CollapseRect()
{
    if (!m_bRect)
        return;
    m_bRect = false;
    m_aAnchor = m_aCaret;
}

OUString CodeEditView::RectText() const
{
    OUStringBuffer aText;
    for (sal_Int32 nLine = RectFirstLine(); nLine <= RectLastLine(); ++nLine)
    {
        if (nLine != RectFirstLine())
            aText.append('\n');
        const OUString& rLine = m_aLines[nLine];
        const sal_Int32 nFrom = ColumnOfVisual(rLine, RectLeft());
        const sal_Int32 nTo = ColumnOfVisual(rLine, RectRight());
        aText.append(rLine.subView(nFrom, std::max<sal_Int32>(0, nTo - nFrom)));
    }
    return aText.makeStringAndClear();
}

void CodeEditView::RectReplace(sal_Int32 nVLow, sal_Int32 nVHigh,
                               const std::vector<OUString>& rTexts, sal_Int32 nNewVisual)
{
    const sal_Int32 nFirst = RectFirstLine();
    const sal_Int32 nLast = RectLastLine();
    const sal_Int32 nAnchorLine = m_aAnchor.Line;
    const sal_Int32 nCaretLine = m_aCaret.Line;
    const Position aCaretBefore = m_aCaret;
    m_bChain = false;
    // from the bottom up: an edit leaves the lines above it where they are
    for (sal_Int32 nLine = nLast; nLine >= nFirst; --nLine)
    {
        const OUString& rLine = m_aLines[nLine];
        OUString aText = rTexts.size() == 1 ? rTexts[0] : rTexts[nLine - nFirst];
        const sal_Int32 nStart = ColumnOfVisual(rLine, nVLow);
        const sal_Int32 nEnd = nVHigh > nVLow ? ColumnOfVisual(rLine, nVHigh) : nStart;
        // a line too short to reach the rectangle is padded out to it
        const sal_Int32 nLength = VisualColumn(rLine, rLine.getLength());
        if (!aText.isEmpty() && nLength < nVLow)
        {
            OUStringBuffer aPad;
            for (sal_Int32 n = nLength; n < nVLow; ++n)
                aPad.append(' ');
            aText = aPad.makeStringAndClear() + aText;
        }
        if (nStart == nEnd && aText.isEmpty())
            continue;
        m_aCaret = aCaretBefore;
        Replace(Range{ { nLine, nStart }, { nLine, nEnd } }, aText, true);
        m_bChain = true; // the rest of the lines undo together with the first edit
    }
    m_bChain = false;
    m_bRect = true;
    m_nAnchorVisual = m_nCaretVisual = std::max<sal_Int32>(0, nNewVisual);
    m_aAnchor = { nAnchorLine, ColumnOfVisual(m_aLines[nAnchorLine], m_nCaretVisual) };
    m_aCaret = { nCaretLine, ColumnOfVisual(m_aLines[nCaretLine], m_nCaretVisual) };
    m_nWantedColumn = m_nCaretVisual;
    ScrollInto(m_aCaret, false);
    ResetBlink();
    Invalidate();
}

void CodeEditView::InsertText(const OUString& rText, bool bTyping)
{
    // one kind of line break inside: what the document's own lines are split at
    OUString aText = rText.replaceAll("\r\n", "\n").replaceAll("\r", "\n");
    if (m_bRect)
    {
        std::vector<OUString> aTexts;
        for (sal_Int32 nStart = 0;;)
        {
            const sal_Int32 nEol = aText.indexOf('\n', nStart);
            aTexts.push_back(aText.copy(nStart, nEol < 0 ? aText.getLength() - nStart : nEol - nStart));
            if (nEol < 0)
                break;
            nStart = nEol + 1;
        }
        const size_t nLines = RectLastLine() - RectFirstLine() + 1;
        if (aTexts.size() == 1 || aTexts.size() == nLines)
        {
            sal_Int32 nWidth = 0;
            for (const OUString& rPart : aTexts)
                nWidth = std::max(nWidth, VisualColumn(rPart, rPart.getLength()));
            // one text for every line, or one per line (a rectangle pasted into one)
            RectReplace(RectLeft(), RectRight(), aTexts, RectLeft() + nWidth);
            return;
        }
        // text that fits no rectangle goes where the caret is
        CollapseRect();
    }
    if (aText.isEmpty() && !HasSelection())
        return;
    const bool bSelection = HasSelection();
    Range aRange = bSelection ? Selection() : Range{ m_aCaret, m_aCaret };
    Replace(aRange, aText, true, bTyping && !bSelection);
}

void CodeEditView::InsertNewLine()
{
    if (m_bRect)
    {
        if (HasSelection())
            RectReplace(RectLeft(), RectRight(), { OUString() }, RectLeft());
        CollapseRect();
    }
    const OUString& rLine = m_aLines[Selection().Start.Line];
    const sal_Int32 nColumn = Selection().Start.Column;
    sal_Int32 nIndent = 0;
    while (nIndent < rLine.getLength() && nIndent < nColumn
           && (rLine[nIndent] == ' ' || rLine[nIndent] == '\t'))
        ++nIndent;
    OUString aText = u"\n"_ustr + rLine.copy(0, nIndent);
    // after an opening brace the next line goes one level in
    sal_Int32 nBefore = nColumn;
    while (nBefore > 0 && (rLine[nBefore - 1] == ' ' || rLine[nBefore - 1] == '\t'))
        --nBefore;
    if (nBefore > 0 && rLine[nBefore - 1] == '{')
        aText += u"    "_ustr;
    InsertText(aText, false);
}

void CodeEditView::DeleteBackward(bool bWord)
{
    if (m_bRect)
    {
        if (HasSelection())
            RectReplace(RectLeft(), RectRight(), { OUString() }, RectLeft());
        else if (RectLeft() > 0)
            RectReplace(RectLeft() - 1, RectLeft(), { OUString() }, RectLeft() - 1);
        return;
    }
    if (HasSelection())
    {
        Replace(Selection(), OUString(), true);
        return;
    }
    if (m_aCaret.Line == 0 && m_aCaret.Column == 0)
        return;
    Range aRange;
    aRange.Start = bWord ? WordLeft(m_aCaret) : PreviousPosition(m_aCaret);
    aRange.End = m_aCaret;
    Replace(aRange, OUString(), true);
}

void CodeEditView::DeleteForward(bool bWord)
{
    if (m_bRect)
    {
        if (HasSelection())
            RectReplace(RectLeft(), RectRight(), { OUString() }, RectLeft());
        else
            RectReplace(RectLeft(), RectLeft() + 1, { OUString() }, RectLeft());
        return;
    }
    if (HasSelection())
    {
        Replace(Selection(), OUString(), true);
        return;
    }
    Range aRange;
    aRange.Start = m_aCaret;
    aRange.End = bWord ? WordRight(m_aCaret) : NextPosition(m_aCaret);
    if (!Before(aRange.Start, aRange.End))
        return;
    Replace(aRange, OUString(), true);
}

void CodeEditView::Copy()
{
    if (!HasSelection() || !GetDrawingArea())
        return;
    try
    {
        vcl::unohelper::TextDataObject::CopyStringTo(m_bRect ? RectText() : TextIn(Selection()),
                                                     GetDrawingArea()->get_clipboard());
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot copy: " << rException.Message);
    }
}

void CodeEditView::Cut()
{
    if (!HasSelection())
        return;
    Copy();
    if (m_bRect)
        RectReplace(RectLeft(), RectRight(), { OUString() }, RectLeft());
    else
        Replace(Selection(), OUString(), true);
}

void CodeEditView::Paste()
{
    if (!GetDrawingArea())
        return;
    try
    {
        uno::Reference<datatransfer::clipboard::XClipboard> xClipboard
            = GetDrawingArea()->get_clipboard();
        if (!xClipboard.is())
            return;
        uno::Reference<datatransfer::XTransferable> xContents = xClipboard->getContents();
        if (!xContents.is())
            return;
        datatransfer::DataFlavor aFlavor;
        aFlavor.MimeType = u"text/plain;charset=utf-16"_ustr;
        aFlavor.HumanPresentableName = u"Unicode text"_ustr;
        aFlavor.DataType = cppu::UnoType<OUString>::get();
        if (!xContents->isDataFlavorSupported(aFlavor))
            return;
        OUString aText;
        xContents->getTransferData(aFlavor) >>= aText;
        InsertText(aText, false);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("code", "cannot paste: " << rException.Message);
    }
}

// ---- input ----------------------------------------------------------------

bool CodeEditView::HandleKey(const KeyEvent& rEvent)
{
    const vcl::KeyCode& rCode = rEvent.GetKeyCode();
    const sal_uInt16 nKey = rCode.GetCode();
    const bool bShift = rCode.IsShift();
    const bool bCtrl = rCode.IsMod1();
    // Alt+Shift+arrows grow a rectangular selection; any other Alt is the menu's
    if (rCode.IsMod2())
    {
        if (!bShift || (nKey != KEY_LEFT && nKey != KEY_RIGHT && nKey != KEY_UP && nKey != KEY_DOWN))
            return false;
        BeginRect();
        const sal_Int32 nLine = m_aCaret.Line;
        if (nKey == KEY_LEFT)
            MoveRect(nLine, std::max<sal_Int32>(0, m_nCaretVisual - 1));
        else if (nKey == KEY_RIGHT)
            MoveRect(nLine, m_nCaretVisual + 1);
        else
            MoveRect(nLine + (nKey == KEY_UP ? -1 : 1), m_nCaretVisual);
        return true;
    }
    // moving the caret ends a rectangular selection, where the caret is
    if (m_bRect)
        switch (nKey)
        {
            case KEY_LEFT:
            case KEY_RIGHT:
            case KEY_UP:
            case KEY_DOWN:
            case KEY_PAGEUP:
            case KEY_PAGEDOWN:
            case KEY_HOME:
            case KEY_END:
                CollapseRect();
                break;
            default:
                break;
        }

    const sal_Int32 nPage
        = std::max<sal_Int32>(1, GetOutputSizePixel().Height() / std::max<tools::Long>(1, m_nLineHeight) - 1);
    const Range aSelection = Selection();

    switch (nKey)
    {
        case KEY_LEFT:
            if (HasSelection() && !bShift && !bCtrl)
                MoveCaret(aSelection.Start, false);
            else
                MoveCaret(bCtrl ? WordLeft(m_aCaret) : PreviousPosition(m_aCaret), bShift);
            return true;
        case KEY_RIGHT:
            if (HasSelection() && !bShift && !bCtrl)
                MoveCaret(aSelection.End, false);
            else
                MoveCaret(bCtrl ? WordRight(m_aCaret) : NextPosition(m_aCaret), bShift);
            return true;
        case KEY_UP:
        case KEY_DOWN:
        case KEY_PAGEUP:
        case KEY_PAGEDOWN:
        {
            const sal_Int32 nStep = nKey == KEY_UP ? -1 : nKey == KEY_DOWN ? 1
                                    : nKey == KEY_PAGEUP ? -nPage : nPage;
            const sal_Int32 nLine = std::clamp<sal_Int32>(m_aCaret.Line + nStep, 0, m_aLines.size() - 1);
            Position aPosition{ nLine, ColumnOfVisual(m_aLines[nLine], m_nWantedColumn) };
            // up on the first line goes to its start, down on the last to its end
            if (nLine == m_aCaret.Line && nStep < 0)
                aPosition.Column = 0;
            else if (nLine == m_aCaret.Line && nStep > 0)
                aPosition.Column = m_aLines[nLine].getLength();
            const bool bEdge = nLine == m_aCaret.Line;
            MoveCaret(aPosition, bShift, !bEdge);
            return true;
        }
        case KEY_HOME:
        {
            if (bCtrl)
            {
                MoveCaret({ 0, 0 }, bShift);
                return true;
            }
            const OUString& rLine = m_aLines[m_aCaret.Line];
            sal_Int32 nFirst = 0;
            while (nFirst < rLine.getLength() && (rLine[nFirst] == ' ' || rLine[nFirst] == '\t'))
                ++nFirst;
            // first the first word, again for the very start of the line
            MoveCaret({ m_aCaret.Line, m_aCaret.Column == nFirst ? 0 : nFirst }, bShift);
            return true;
        }
        case KEY_END:
            if (bCtrl)
                MoveCaret({ static_cast<sal_Int32>(m_aLines.size()) - 1,
                            m_aLines.back().getLength() },
                          bShift);
            else
                MoveCaret({ m_aCaret.Line, m_aLines[m_aCaret.Line].getLength() }, bShift);
            return true;
        case KEY_RETURN:
            InsertNewLine();
            return true;
        case KEY_BACKSPACE:
            DeleteBackward(bCtrl);
            return true;
        case KEY_DELETE:
            if (bShift && !bCtrl)
                Cut();
            else
                DeleteForward(bCtrl);
            return true;
        case KEY_TAB:
        {
            if (bCtrl)
                return false;
            if (bShift)
            {
                // one level out of the caret's line
                const sal_Int32 nLine = m_aCaret.Line;
                const OUString& rLine = m_aLines[nLine];
                sal_Int32 nCut = 0;
                if (!rLine.isEmpty() && rLine[0] == '\t')
                    nCut = 1;
                else
                    while (nCut < TabWidth && nCut < rLine.getLength() && rLine[nCut] == ' ')
                        ++nCut;
                if (nCut > 0)
                    Replace(Range{ { nLine, 0 }, { nLine, nCut } }, OUString(), true);
                return true;
            }
            // to the next tab stop
            const sal_Int32 nVisual
                = m_bRect ? RectLeft()
                          : VisualColumn(m_aLines[Selection().Start.Line], Selection().Start.Column);
            InsertText(u"    "_ustr.copy(0, TabWidth - nVisual % TabWidth), false);
            return true;
        }
        default:
            break;
    }

    if (bCtrl)
    {
        switch (nKey)
        {
            case KEY_A:
                m_bRect = false;
                m_aAnchor = { 0, 0 };
                MoveCaret({ static_cast<sal_Int32>(m_aLines.size()) - 1, m_aLines.back().getLength() },
                          true);
                return true;
            case KEY_C:
            case KEY_INSERT:
                Copy();
                return true;
            case KEY_X:
                Cut();
                return true;
            case KEY_V:
                Paste();
                return true;
            case KEY_Z:
                if (bShift)
                    Redo();
                else
                    Undo();
                return true;
            case KEY_Y:
                Redo();
                return true;
            default:
                return false;
        }
    }
    if (bShift && nKey == KEY_INSERT)
    {
        Paste();
        return true;
    }

    const sal_Unicode cChar = rEvent.GetCharCode();
    if (cChar >= 32 && cChar != 127)
    {
        InsertText(OUString(cChar), true);
        return true;
    }
    return false;
}

bool CodeEditView::KeyInput(const KeyEvent& rEvent) { return HandleKey(rEvent); }

bool CodeEditView::MouseButtonDown(const MouseEvent& rEvent)
{
    if (!rEvent.IsLeft() && !rEvent.IsMiddle())
        return false;
    GrabFocus();
    const Position aPosition = PositionAt(rEvent.GetPosPixel());
    // Alt+click, or the middle button: drag out a rectangle
    if (rEvent.IsMiddle() || rEvent.IsMod2())
    {
        m_aAnchor = m_aCaret = aPosition;
        m_bRect = true;
        m_nAnchorVisual = m_nCaretVisual = VisualAt(rEvent.GetPosPixel());
        m_nWantedColumn = m_nCaretVisual;
        m_bDragging = true;
        ResetBlink();
        Invalidate();
        return true;
    }
    if (rEvent.GetClicks() == 2)
        SelectWordAt(aPosition);
    else if (rEvent.GetClicks() >= 3)
    {
        m_aAnchor = { aPosition.Line, 0 };
        m_aCaret = static_cast<size_t>(aPosition.Line) + 1 < m_aLines.size()
                       ? Position{ aPosition.Line + 1, 0 }
                       : Position{ aPosition.Line, m_aLines[aPosition.Line].getLength() };
    }
    else
    {
        MoveCaret(aPosition, rEvent.IsShift());
        m_bDragging = true;
        return true;
    }
    ResetBlink();
    Invalidate();
    return true;
}

bool CodeEditView::MouseMove(const MouseEvent& rEvent)
{
    if (!m_bDragging)
        return false;
    if (m_bRect)
        MoveRect(PositionAt(rEvent.GetPosPixel()).Line, VisualAt(rEvent.GetPosPixel()));
    else
        MoveCaret(PositionAt(rEvent.GetPosPixel()), true);
    return true;
}

bool CodeEditView::MouseButtonUp(const MouseEvent&)
{
    const bool bWasDragging = m_bDragging;
    m_bDragging = false;
    return bWasDragging;
}

// ---- drawing --------------------------------------------------------------

void CodeEditView::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle&)
{
    const StyleSettings& rStyle = Application::GetSettings().GetStyleSettings();
    const bool bFirst = !m_bFontMeasured;
    MeasureFont(rRenderContext);
    if (bFirst)
    {
        ConfigureScrollBars();
        if (m_bRevealPending)
        {
            m_bRevealPending = false;
            ScrollInto(m_aCaret, true);
        }
    }
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

    // the selection, under the text
    if (m_bRect)
    {
        if (HasSelection())
        {
            Color aSelection = rStyle.GetHighlightColor();
            aSelection.Merge(rStyle.GetFieldColor(), m_bFocused ? 110 : 60);
            rRenderContext.SetFillColor(aSelection);
            const tools::Long nX = std::max(VisualX(RectLeft()), nGutter);
            const tools::Long nRight = VisualX(RectRight());
            if (nRight > nX)
                for (sal_Int32 nLine = std::max(nFirst, RectFirstLine());
                     nLine <= std::min(nLast, RectLastLine()); ++nLine)
                    rRenderContext.DrawRect(tools::Rectangle(
                        Point(nX, nLine * m_nLineHeight - nTop), Size(nRight - nX, m_nLineHeight)));
        }
    }
    else if (HasSelection())
    {
        Color aSelection = rStyle.GetHighlightColor();
        aSelection.Merge(rStyle.GetFieldColor(), m_bFocused ? 110 : 60);
        rRenderContext.SetFillColor(aSelection);
        const Range aRange = Selection();
        for (sal_Int32 nLine = std::max(nFirst, aRange.Start.Line);
             nLine <= std::min(nLast, aRange.End.Line); ++nLine)
        {
            const OUString& rLine = m_aLines[nLine];
            const sal_Int32 nFrom = nLine == aRange.Start.Line ? aRange.Start.Column : 0;
            const sal_Int32 nTo = nLine == aRange.End.Line ? aRange.End.Column : rLine.getLength();
            tools::Long nX = ColumnX(rLine, nFrom);
            tools::Long nRight = ColumnX(rLine, nTo);
            if (nLine < aRange.End.Line) // the line break is selected, too
                nRight += m_nCharWidth;
            nX = std::max(nX, nGutter);
            if (nRight > nX)
                rRenderContext.DrawRect(tools::Rectangle(
                    Point(nX, nLine * m_nLineHeight - nTop), Size(nRight - nX, m_nLineHeight)));
        }
    }

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
            drawRun(nDone, nStart, rStyle.GetFieldTextColor());
            drawRun(nStart, nEnd, ColorOf(aPalette, rToken));
            nDone = std::max(nDone, nEnd);
        }
        drawRun(nDone, rLine.getLength(), rStyle.GetFieldTextColor());
    }

    // the caret; a rectangle has one on each of its lines
    if (m_bRect)
    {
        if (m_bFocused && m_bCaretOn)
        {
            rRenderContext.SetFillColor(rStyle.GetFieldTextColor());
            for (sal_Int32 nLine = std::max(nFirst, RectFirstLine());
                 nLine <= std::min(nLast, RectLastLine()); ++nLine)
                rRenderContext.DrawRect(tools::Rectangle(
                    Point(VisualX(m_nCaretVisual), nLine * m_nLineHeight - nTop),
                    Size(2, m_nLineHeight)));
        }
    }
    else if (m_bFocused && m_bCaretOn && m_aCaret.Line >= nFirst && m_aCaret.Line <= nLast)
    {
        const tools::Long nX = ColumnX(m_aLines[m_aCaret.Line], m_aCaret.Column);
        rRenderContext.SetFillColor(rStyle.GetFieldTextColor());
        rRenderContext.DrawRect(tools::Rectangle(
            Point(nX, m_aCaret.Line * m_nLineHeight - nTop), Size(2, m_nLineHeight)));
    }
    rRenderContext.SetClipRegion();

    // the line numbers
    rRenderContext.SetFillColor(rStyle.GetFaceColor());
    rRenderContext.DrawRect(tools::Rectangle(Point(0, 0), Size(nGutter - 1, aSize.Height())));
    for (sal_Int32 nLine = nFirst; nLine <= nLast; ++nLine)
    {
        const OUString aNumber = OUString::number(nLine + 1);
        // the caret's line stands out
        rRenderContext.SetTextColor(nLine == m_aCaret.Line ? rStyle.GetFieldTextColor()
                                                           : rStyle.GetDisableColor());
        rRenderContext.DrawText(
            Point(nGutter - Padding - aNumber.getLength() * m_nCharWidth,
                  nLine * m_nLineHeight - nTop + 1),
            aNumber);
    }
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
