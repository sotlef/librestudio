/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreStudio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>

#include <algorithm>

/**
 * The scroll area a GtkScrolledWindow of a .ui becomes.
 *
 * Its size hint is its minimum, as Gtk's is (a scrolled window does not
 * propagate its child's natural size); see QtBuilder.cxx.
 *
 * It can also be user managed, which is what weld_scrolled_window(id, true) asks
 * for: the owner of the window draws the scrolled content itself (a custom
 * drawing area that paints from an offset) and tells the window how big that
 * content is.  Qt's own idea of scrolling -- the child is a big widget that
 * moves behind the viewport, the range of the bars is the child's size minus
 * the viewport's -- does not fit that.  A user managed area therefore
 *  - keeps the range its owner configured, which QScrollArea would recompute from
 *    the child's size at every resize,
 *  - does not move the child when a bar moves (the owner reacts to the bar's
 *    value instead),
 * and its child stays the size of the viewport (widgetResizable).
 */
class QtScrolledArea : public QScrollArea
{
public:
    // one per orientation: what the owner configured
    struct Axis
    {
        bool bSet = false;
        int nUpper = 0; // the extent of the content
        int nPage = 0; // the size of what is visible of it
        int nPageStep = 0;
        // what the owner asked for; AsNeeded is decided here, from the extents
        Qt::ScrollBarPolicy ePolicy = Qt::ScrollBarAsNeeded;
    };

    using QScrollArea::QScrollArea;

    QSize sizeHint() const override { return minimumSizeHint(); }

    void setUserManaged(bool bUserManaged) { m_bUserManaged = bUserManaged; }
    bool isUserManaged() const { return m_bUserManaged; }

    Axis& axis(Qt::Orientation eOrientation)
    {
        return eOrientation == Qt::Horizontal ? m_aHorizontal : m_aVertical;
    }

    // Makes the bars show the configured extents.
    void applyExtents()
    {
        if (!m_bUserManaged)
            return;
        apply(horizontalScrollBar(), m_aHorizontal);
        apply(verticalScrollBar(), m_aVertical);
        applyPolicies();
    }

    // Whether a bar is there is not left to QScrollArea: it hides a bar whose
    // range it has computed as 0, and that range is the child's size minus the
    // viewport's, which is nothing to a drawing area that paints from an
    // offset.  Hiding it gives the viewport the bar's room, showing it again
    // (with the owner's range) takes it back, and so on, forever.  So a bar is
    // always on or always off, and which is decided here from the extents.
    void applyPolicies()
    {
        QScrollBar* pH = horizontalScrollBar();
        QScrollBar* pV = verticalScrollBar();
        // the room the other bar takes is part of what the content has to fit into
        const int nBarWidth = pV->isVisible() ? pV->sizeHint().width() : 0;
        const int nBarHeight = pH->isVisible() ? pH->sizeHint().height() : 0;
        setPolicy(Qt::Horizontal, wanted(m_aHorizontal, nBarWidth));
        setPolicy(Qt::Vertical, wanted(m_aVertical, nBarHeight));
    }

protected:
    // QScrollArea recomputes the range of its bars from the size of the child
    // whenever the area or the layout changes, and a range of 0 takes the value
    // of the bar to 0 with it.  Whatever it did, the owner's range and the value
    // the bar had are put back.
    void resizeEvent(QResizeEvent* pEvent) override
    {
        const Values aBefore = values();
        QScrollArea::resizeEvent(pEvent);
        restore(aBefore);
    }

    bool event(QEvent* pEvent) override
    {
        const Values aBefore = values();
        const bool bHandled = QScrollArea::event(pEvent);
        restore(aBefore);
        return bHandled;
    }

    bool eventFilter(QObject* pObject, QEvent* pEvent) override
    {
        const Values aBefore = values();
        const bool bHandled = QScrollArea::eventFilter(pObject, pEvent); // the child's resizes
        restore(aBefore);
        return bHandled;
    }

    void scrollContentsBy(int nDx, int nDy) override
    {
        if (!m_bUserManaged)
            QScrollArea::scrollContentsBy(nDx, nDy);
    }

private:
    static Qt::ScrollBarPolicy wanted(const Axis& rAxis, int nRoomOfTheOtherBar)
    {
        if (rAxis.ePolicy != Qt::ScrollBarAsNeeded)
            return rAxis.ePolicy;
        // the page the owner gave is what is left of the area with the other bar in
        // it; without the bar there would be more, so compare with that
        return rAxis.bSet && rAxis.nUpper > rAxis.nPage + nRoomOfTheOtherBar
                   ? Qt::ScrollBarAlwaysOn
                   : Qt::ScrollBarAlwaysOff;
    }
    void setPolicy(Qt::Orientation eOrientation, Qt::ScrollBarPolicy ePolicy)
    {
        if (eOrientation == Qt::Horizontal)
        {
            if (horizontalScrollBarPolicy() != ePolicy)
                setHorizontalScrollBarPolicy(ePolicy);
        }
        else if (verticalScrollBarPolicy() != ePolicy)
            setVerticalScrollBarPolicy(ePolicy);
    }

    struct Values
    {
        int nHorizontal = 0;
        int nVertical = 0;
    };
    Values values() const { return { horizontalScrollBar()->value(), verticalScrollBar()->value() }; }
    void restore(const Values& rValues)
    {
        if (!m_bUserManaged)
            return;
        applyExtents();
        if (m_aHorizontal.bSet)
            horizontalScrollBar()->setValue(rValues.nHorizontal);
        if (m_aVertical.bSet)
            verticalScrollBar()->setValue(rValues.nVertical);
    }

    static void apply(QScrollBar* pBar, const Axis& rAxis)
    {
        if (!pBar || !rAxis.bSet)
            return;
        pBar->setRange(0, std::max(0, rAxis.nUpper - rAxis.nPage));
        if (rAxis.nPageStep > 0)
            pBar->setPageStep(rAxis.nPageStep);
    }

    bool m_bUserManaged = false;
    Axis m_aHorizontal;
    Axis m_aVertical;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
