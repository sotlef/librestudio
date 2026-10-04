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
#include <com/sun/star/code/core/Range.hpp>
#include <com/sun/star/code/document/XTextDocument.hpp>
#include <com/sun/star/code/semantic/SemanticToken.hpp>
#include <tools/link.hxx>
#include <vcl/font.hxx>
#include <vcl/timer.hxx>
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
/// The editor of one document: line numbers on the left, the text coloured by
/// the semantic tokens the analysis service has for it, a caret, a selection,
/// typing.  The text belongs to the document, never to the view: every edit is
/// an XTextDocument::applyEdit, and the view reads the lines back from it.
/// Refresh() is how the view is told that the text or the tokens may have
/// changed behind its back.
///
/// The view is the widget (a weld::CustomWidgetController) and owns the
/// scrolled window it sits in; scrolling is the view's own, in pixels.
///
/// Not yet: several carets, rectangular selection, IME, wrapping, folding.
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
    /// Scrolls so that the position is in view, and puts the caret there.
    void RevealPosition(const css::code::core::Position& rPosition);
    OUString GetText() const;

    /// Reads the Tools > Options settings again and lays the text out anew.
    void SettingsChanged();

    void Undo();
    void Redo();

    // weld::CustomWidgetController
    virtual void SetDrawingArea(weld::DrawingArea* pDrawingArea) override;
    virtual void Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect) override;
    virtual void Resize() override;
    virtual void StyleUpdated() override;
    virtual bool MouseButtonDown(const MouseEvent& rEvent) override;
    virtual bool MouseMove(const MouseEvent& rEvent) override;
    virtual bool MouseButtonUp(const MouseEvent& rEvent) override;
    virtual bool KeyInput(const KeyEvent& rEvent) override;
    virtual void GetFocus() override;
    virtual void LoseFocus() override;

private:
    using Position = css::code::core::Position;
    using Range = css::code::core::Range;

    // what one edit did, enough to take it back and to do it again
    struct UndoStep
    {
        Position aStart;
        OUString aOld; // what was there
        OUString aNew; // what is there now
        Position aCaretBefore;
        Position aCaretAfter;
        // Part of the same user action as the step before it (one edit per line
        // of a rectangular selection): undone and redone together with it.
        bool bChained = false;
    };

    DECL_LINK(ScrollHdl, weld::ScrolledWindow&, void);
    DECL_LINK(BlinkHdl, Timer*, void);

    static void LoadSettings();
    void MeasureFont(vcl::RenderContext& rRenderContext);
    void ConfigureScrollBars();
    // width in pixels of the column of line numbers, with its padding
    tools::Long GutterWidth() const;
    // pixel position of the left edge of a text column, in the drawing area
    tools::Long ColumnX(const OUString& rLine, sal_Int32 nColumn) const;
    // visual column (tabs expanded) of a UTF-16 column of a line
    static sal_Int32 VisualColumn(const OUString& rLine, sal_Int32 nColumn);
    // the column whose left edge is nearest to a visual column
    static sal_Int32 ColumnOfVisual(const OUString& rLine, sal_Int32 nVisual);
    static OUString ExpandTabs(const OUString& rLine);

    // geometry of the text itself
    Position PositionAt(const Point& rPoint) const;
    Position ClampPosition(Position aPosition) const;
    static bool Before(const Position& rA, const Position& rB);
    static Position EndOf(const Position& rStart, const OUString& rText);
    OUString TextIn(const Range& rRange) const;
    bool HasSelection() const;
    Range Selection() const; // ordered

    // the caret
    void MoveCaret(Position aPosition, bool bExtend, bool bKeepWantedColumn = false);
    void ScrollInto(const Position& rPosition, bool bCentre);
    void ResetBlink();
    Position WordLeft(Position aPosition) const;
    Position WordRight(Position aPosition) const;
    Position PreviousPosition(const Position& rPosition) const;
    Position NextPosition(const Position& rPosition) const;
    void SelectWordAt(const Position& rPosition);

    // editing; everything goes through Replace
    void Replace(const Range& rRange, const OUString& rNewText, bool bRecord, bool bMerge = false);
    void ShiftTokens(const Range& rRange, const OUString& rNewText);
    void InsertText(const OUString& rText, bool bTyping);
    void DeleteBackward(bool bWord);
    void DeleteForward(bool bWord);
    void InsertNewLine();
    void Copy();
    void Cut();
    void Paste();
    bool HandleKey(const KeyEvent& rEvent);

    // The rectangular selection: the lines between the anchor's and the caret's,
    // in the columns (visual, so beyond the end of short lines too) between
    // m_nAnchorVisual and m_nCaretVisual.  Whatever is typed, pasted or deleted
    // is done on every one of its lines, as one undo step.
    tools::Long VisualX(sal_Int32 nVisual) const;
    sal_Int32 VisualAt(const Point& rPoint) const;
    sal_Int32 RectFirstLine() const;
    sal_Int32 RectLastLine() const;
    sal_Int32 RectLeft() const;
    sal_Int32 RectRight() const;
    void BeginRect();
    void MoveRect(sal_Int32 nLine, sal_Int32 nVisual);
    void CollapseRect();
    // Replaces, on each line, the cells nVLow..nVHigh by the text for that line
    // (one text: the same on all), and leaves a zero-width rectangle at nNewVisual.
    void RectReplace(sal_Int32 nVLow, sal_Int32 nVHigh, const std::vector<OUString>& rTexts,
                     sal_Int32 nNewVisual);
    OUString RectText() const;

    css::uno::Reference<css::code::document::XTextDocument> m_xDocument;
    std::unique_ptr<weld::ScrolledWindow> m_xScroll;
    std::unique_ptr<weld::CustomWeld> m_xDrawing;

    std::vector<OUString> m_aLines;
    // the tokens by line, in column order
    std::vector<std::vector<css::code::semantic::SemanticToken>> m_aTokens;
    sal_Int32 m_nMaxColumns = 0;
    // The document version of the text this view produced by its own last edit:
    // tokens of an older version are then still the best guess (shifted by the
    // edit), tokens of another text are wrong.
    sal_Int32 m_nOwnVersion = -2;

    Position m_aCaret;
    Position m_aAnchor; // the other end of the selection; equal to the caret if none
    sal_Int32 m_nWantedColumn = 0; // visual, kept over up and down
    bool m_bRect = false;
    sal_Int32 m_nAnchorVisual = 0;
    sal_Int32 m_nCaretVisual = 0;
    bool m_bChain = false; // the next recorded undo step belongs to the one before
    bool m_bDragging = false;
    bool m_bFocused = false;
    bool m_bCaretOn = true;
    AutoTimer m_aBlink;

    std::vector<UndoStep> m_aUndo;
    std::vector<UndoStep> m_aRedo;

    vcl::Font m_aFont;
    tools::Long m_nCharWidth = 0;
    tools::Long m_nLineHeight = 0;
    bool m_bFontMeasured = false;
    bool m_bConfiguring = false;
    bool m_bRevealPending = false;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
