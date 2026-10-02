/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <sal/config.h>

#include <QtFrame.hxx>
#include <QtObject.hxx>
#include <QtObject.moc>

#include <QtCore/QLibraryInfo>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QVBoxLayout>

QtObject::QtObject(QtFrame* pParent, bool bShow, bool bWidgetHost)
    : m_pParent(pParent)
    , m_pQWindow(nullptr)
    , m_pQWidget(nullptr)
    , m_bForwardKey(false)
{
    if (!m_pParent)
        return;

    if (bWidgetHost)
    {
        // Hosting a native Qt widget subtree: a plain child QWidget is all
        // that's needed -- the children composite into the toplevel's
        // backing store like any other widget. The video/OpenGL branches
        // below must NOT be used for this: createWindowContainer() embeds a
        // raw native QWindow stacked above all sibling widget content, and
        // with no video sink/GL context ever rendering into it, its
        // uninitialized surface shows up as garbage covering the hosted
        // widgets; WA_NoSystemBackground/WA_OpaquePaintEvent likewise
        // suppress exactly the background erase a widget host needs.
        m_pQWidget = new QtObjectWidget(*this);

        // Fill the host's own background: the hosted subtree's partial
        // repaints (e.g. a tab bar's hover highlight; document-mode
        // inactive tabs paint no background of their own) would
        // otherwise show whatever VCL last left in the toplevel's
        // backing store underneath this region -- visible as stripes of
        // stale splitter/window pixels inside the MDI tab bar.
        m_pQWidget->setAutoFillBackground(true);

        // An own cursor, so the hosted subtree doesn't inherit the
        // frame-wide one VCL last set: QtFrame::SetPointer() puts VCL's
        // pointer on the whole toplevel, and once the mouse crosses from
        // a vcl window straight onto this native subtree, VCL gets no
        // further MouseMove to reset it -- hovering the MDI tabs showed
        // the splitter's V-split cursor forever. The hosted Qt widgets
        // manage their own cursors (QLineEdit's I-beam etc.) on top of
        // this neutral default, and gtk3 behaves the same way because
        // the embedded subtree's GdkWindow likewise never follows the
        // frame cursor.
        m_pQWidget->setCursor(Qt::ArrowCursor);
    }
    else if (QLibraryInfo::version().majorVersion() > 5)
    {
        m_pQWindow = new QWindow;
        m_pQWidget = QWidget::createWindowContainer(m_pQWindow, &pParent->GetQWidget());
    }
    else
    {
        // with the qt5 VCL plugin, the above would cause issues with video playback (s. tdf#148864, tdf#125517),
        // which is not a problem with the QtMultimedia approach that the qt6 VCL plugin uses;
        // stay with the QtObjectWidget introduced in commit 4366e0605214260e55a937173b0c2e02225dc843
        m_pQWidget = new QtObjectWidget(*this);

        // a foreign rendering surface owns all of its pixels, Qt must neither
        // erase the background nor composite anything over it
        m_pQWidget->setAttribute(Qt::WA_NoSystemBackground);
        m_pQWidget->setAttribute(Qt::WA_OpaquePaintEvent);

        // invoke QWidget::winId() to ensure a native window for OpenGL rendering is available on X11,
        // don't do it on Wayland, as that breaks rendering otherwise, s.a. QtFrame::ResolveWindowHandle
        if (QGuiApplication::platformName() == "xcb")
            m_pQWidget->winId();
        m_pQWindow = m_pQWidget->windowHandle();
    }

    // set layout, used for video playback, see QtPlayer::createPlayerWindow
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    m_pQWidget->setLayout(layout);

    if (bShow)
        m_pQWidget->show();

    QtFrame::FillSystemEnvData(m_aSystemData, m_pQWidget);
}

QtObject::~QtObject()
{
    if (m_pQWidget)
    {
        m_pQWidget->setParent(nullptr);
        delete m_pQWidget;
    }
}

QWindow* QtObject::windowHandle() const { return m_pQWindow; }

void QtObject::ResetClipRegion()
{
    if (m_pQWidget)
        m_pRegion = QRegion(m_pQWidget->geometry());
    else
        m_pRegion = QRegion();
}

void QtObject::BeginSetClipRegion(sal_uInt32) { m_pRegion = QRegion(); }

void QtObject::UnionClipRegion(tools::Long nX, tools::Long nY, tools::Long nWidth,
                               tools::Long nHeight)
{
    m_pRegion += QRect(nX, nY, nWidth, nHeight);
}

void QtObject::EndSetClipRegion()
{
    if (m_pQWidget)
        m_pRegion = m_pRegion.intersected(m_pQWidget->geometry());
}

void QtObject::SetPosSize(tools::Long nX, tools::Long nY, tools::Long nWidth, tools::Long nHeight)
{
    if (m_pQWidget)
    {
        // VCL talks device pixels, Qt logical ones -- same conversion every
        // QtFrame::SetPosSize/SetMinClientSize/... geometry call does; without
        // it everything lands at ratio-times the intended position and size
        // under HiDPI
        const qreal fRatio = m_pParent->devicePixelRatioF();
        m_pQWidget->move(round(nX / fRatio), round(nY / fRatio));
        m_pQWidget->setFixedSize(round(nWidth / fRatio), round(nHeight / fRatio));
    }
}

void QtObject::Show(bool bVisible)
{
    if (m_pQWidget)
        m_pQWidget->setVisible(bVisible);
}

void QtObject::SetForwardKey(bool bEnable) { m_bForwardKey = bEnable; }

void QtObject::Reparent(SalFrame* pFrame)
{
    QtFrame* pNewParent = static_cast<QtFrame*>(pFrame);
    if (m_pParent == pNewParent)
        return;
    m_pParent = pNewParent;
    m_pQWidget->setParent(&m_pParent->GetQWidget());
}

QtObjectWidget::QtObjectWidget(QtObject& rParent)
    : QWidget(&rParent.frame()->GetQWidget())
    , m_rParent(rParent)
{
    assert(m_rParent.frame());
}

void QtObjectWidget::focusInEvent(QFocusEvent*)
{
    SolarMutexGuard aGuard;
    m_rParent.CallCallback(SalObjEvent::GetFocus);
}

void QtObjectWidget::focusOutEvent(QFocusEvent*)
{
    SolarMutexGuard aGuard;
    m_rParent.CallCallback(SalObjEvent::LoseFocus);
}

void QtObjectWidget::mousePressEvent(QMouseEvent* pEvent)
{
    SolarMutexGuard aGuard;
    m_rParent.CallCallback(SalObjEvent::ToTop);

    if (m_rParent.forwardKey())
        pEvent->ignore();
}

void QtObjectWidget::mouseReleaseEvent(QMouseEvent* pEvent)
{
    if (m_rParent.forwardKey())
        pEvent->ignore();
}

void QtObjectWidget::keyReleaseEvent(QKeyEvent* pEvent)
{
    if (m_rParent.forwardKey())
        pEvent->ignore();
}

void QtObjectWidget::keyPressEvent(QKeyEvent* pEvent)
{
    if (m_rParent.forwardKey())
        pEvent->ignore();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
