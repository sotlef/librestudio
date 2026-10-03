/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <QtInstanceScrolledWindow.hxx>
#include <QtScrolledArea.hxx>
#include <QtInstanceScrolledWindow.moc>

#include <QtWidgets/QScrollBar>

QtInstanceScrolledWindow::QtInstanceScrolledWindow(QScrollArea* pScrollArea,
                                                   bool bUserManagedScrolling)
    : QtInstanceWidget(pScrollArea)
    , m_pScrollArea(pScrollArea)
{
    assert(m_pScrollArea);

    if (bUserManagedScrolling)
    {
        m_pUserManaged = dynamic_cast<QtScrolledArea*>(pScrollArea);
        if (m_pUserManaged)
            m_pUserManaged->setUserManaged(true);
    }

    if (QScrollBar* pHorizontalScrollBar = m_pScrollArea->horizontalScrollBar())
        connect(pHorizontalScrollBar, &QScrollBar::valueChanged, this,
                &QtInstanceScrolledWindow::handleHorizontalScrollValueChanged);

    if (QScrollBar* pVerticalScrollBar = m_pScrollArea->verticalScrollBar())
        connect(pVerticalScrollBar, &QScrollBar::valueChanged, this,
                &QtInstanceScrolledWindow::handleVerticalScrollValueChanged);
}

void QtInstanceScrolledWindow::configureUserManaged(Qt::Orientation eOrientation, int nValue,
                                                     int nUpper, int nStepIncrement,
                                                     int nPageIncrement, int nPageSize)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        QtScrolledArea::Axis& rAxis = m_pUserManaged->axis(eOrientation);
        rAxis.bSet = true;
        rAxis.nUpper = nUpper;
        rAxis.nPage = nPageSize;
        rAxis.nPageStep = nPageIncrement;
        QScrollBar* pBar = eOrientation == Qt::Horizontal ? m_pScrollArea->horizontalScrollBar()
                                                          : m_pScrollArea->verticalScrollBar();
        pBar->setSingleStep(nStepIncrement);
        m_pUserManaged->applyExtents();
        pBar->setValue(nValue);
    });
}

void QtInstanceScrolledWindow::hadjustment_configure(int nValue, int nUpper, int nStepIncrement,
                                                     int nPageIncrement, int nPageSize)
{
    if (m_pUserManaged)
        configureUserManaged(Qt::Horizontal, nValue, nUpper, nStepIncrement, nPageIncrement,
                             nPageSize);
    else
        weld::ScrolledWindow::hadjustment_configure(nValue, nUpper, nStepIncrement,
                                                    nPageIncrement, nPageSize);
}

void QtInstanceScrolledWindow::vadjustment_configure(int nValue, int nUpper, int nStepIncrement,
                                                     int nPageIncrement, int nPageSize)
{
    if (m_pUserManaged)
        configureUserManaged(Qt::Vertical, nValue, nUpper, nStepIncrement, nPageIncrement,
                             nPageSize);
    else
        weld::ScrolledWindow::vadjustment_configure(nValue, nUpper, nStepIncrement,
                                                    nPageIncrement, nPageSize);
}

int QtInstanceScrolledWindow::hadjustment_get_value() const
{
    SolarMutexGuard g;

    int nValue = 0;
    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->horizontalScrollBar())
            nValue = pScrollBar->value();
    });

    return nValue;
}

void QtInstanceScrolledWindow::hadjustment_set_value(int nValue)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->horizontalScrollBar())
            pScrollBar->setValue(nValue);
    });
}

int QtInstanceScrolledWindow::hadjustment_get_upper() const
{
    if (m_pUserManaged)
        return m_pUserManaged->axis(Qt::Horizontal).nUpper;

    SolarMutexGuard g;

    int nMax = 0;
    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->horizontalScrollBar())
            nMax = pScrollBar->maximum();
    });

    return nMax;
}

void QtInstanceScrolledWindow::hadjustment_set_upper(int nUpper)
{
    if (m_pUserManaged)
    {
        SolarMutexGuard g;
        GetQtInstance().RunInMainThread([&] {
            m_pUserManaged->axis(Qt::Horizontal).nUpper = nUpper;
            m_pUserManaged->axis(Qt::Horizontal).bSet = true;
            m_pUserManaged->applyExtents();
        });
        return;
    }

    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->horizontalScrollBar())
            pScrollBar->setMaximum(nUpper);
    });
}

int QtInstanceScrolledWindow::hadjustment_get_page_size() const
{
    if (m_pUserManaged)
        return m_pUserManaged->axis(Qt::Horizontal).nPage;

    SolarMutexGuard g;

    int nPageSize = 0;
    GetQtInstance().RunInMainThread([&] {
        if (QWidget* pWidget = m_pScrollArea->widget())
            nPageSize = pWidget->width();
    });

    return nPageSize;
}

void QtInstanceScrolledWindow::hadjustment_set_page_size(int nSize)
{
    if (m_pUserManaged)
    {
        // the visible part is the viewport's; the child is never resized to it
        SolarMutexGuard g;
        GetQtInstance().RunInMainThread([&] {
            m_pUserManaged->axis(Qt::Horizontal).nPage = nSize;
            m_pUserManaged->axis(Qt::Horizontal).bSet = true;
            m_pUserManaged->applyExtents();
        });
        return;
    }

    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QWidget* pWidget = m_pScrollArea->widget())
            pWidget->resize(nSize, pWidget->height());
    });
}

void QtInstanceScrolledWindow::hadjustment_set_page_increment(int nSize)
{
    if (m_pUserManaged)
    {
        SolarMutexGuard g;
        GetQtInstance().RunInMainThread([&] {
            m_pUserManaged->axis(Qt::Horizontal).nPageStep = nSize;
            m_pUserManaged->applyExtents();
        });
        return;
    }

    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->horizontalScrollBar())
            pScrollBar->setPageStep(nSize);
    });
}

void QtInstanceScrolledWindow::hadjustment_set_step_increment(int nSize)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->horizontalScrollBar())
            pScrollBar->setSingleStep(nSize);
    });
}

void QtInstanceScrolledWindow::set_hpolicy(VclPolicyType eHPolicy)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread(
        [&] { m_pScrollArea->setHorizontalScrollBarPolicy(toQtPolicy(eHPolicy)); });
}

VclPolicyType QtInstanceScrolledWindow::get_hpolicy() const
{
    SolarMutexGuard g;

    VclPolicyType ePolicy = VclPolicyType::AUTOMATIC;
    GetQtInstance().RunInMainThread(
        [&] { ePolicy = toVclPolicy(m_pScrollArea->horizontalScrollBarPolicy()); });

    return ePolicy;
}

int QtInstanceScrolledWindow::vadjustment_get_value() const
{
    SolarMutexGuard g;

    int nValue = 0;
    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->verticalScrollBar())
            nValue = pScrollBar->value();
    });

    return nValue;
}

void QtInstanceScrolledWindow::vadjustment_set_value(int nValue)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->verticalScrollBar())
            pScrollBar->setValue(nValue);
    });
}

int QtInstanceScrolledWindow::vadjustment_get_upper() const
{
    if (m_pUserManaged)
        return m_pUserManaged->axis(Qt::Vertical).nUpper;

    SolarMutexGuard g;

    int nMax = 0;
    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->verticalScrollBar())
            nMax = pScrollBar->maximum();
    });

    return nMax;
}

void QtInstanceScrolledWindow::vadjustment_set_upper(int nUpper)
{
    if (m_pUserManaged)
    {
        SolarMutexGuard g;
        GetQtInstance().RunInMainThread([&] {
            m_pUserManaged->axis(Qt::Vertical).nUpper = nUpper;
            m_pUserManaged->axis(Qt::Vertical).bSet = true;
            m_pUserManaged->applyExtents();
        });
        return;
    }

    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->verticalScrollBar())
            pScrollBar->setMaximum(nUpper);
    });
}

int QtInstanceScrolledWindow::vadjustment_get_page_size() const
{
    if (m_pUserManaged)
        return m_pUserManaged->axis(Qt::Vertical).nPage;

    SolarMutexGuard g;

    int nPageSize = 0;
    GetQtInstance().RunInMainThread([&] {
        if (QWidget* pWidget = m_pScrollArea->widget())
            nPageSize = pWidget->height();
    });

    return nPageSize;
}

void QtInstanceScrolledWindow::vadjustment_set_page_size(int nSize)
{
    if (m_pUserManaged)
    {
        // the visible part is the viewport's; the child is never resized to it
        SolarMutexGuard g;
        GetQtInstance().RunInMainThread([&] {
            m_pUserManaged->axis(Qt::Vertical).nPage = nSize;
            m_pUserManaged->axis(Qt::Vertical).bSet = true;
            m_pUserManaged->applyExtents();
        });
        return;
    }

    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QWidget* pWidget = m_pScrollArea->widget())
            pWidget->resize(pWidget->width(), nSize);
    });
}

void QtInstanceScrolledWindow::vadjustment_set_page_increment(int nSize)
{
    if (m_pUserManaged)
    {
        SolarMutexGuard g;
        GetQtInstance().RunInMainThread([&] {
            m_pUserManaged->axis(Qt::Vertical).nPageStep = nSize;
            m_pUserManaged->applyExtents();
        });
        return;
    }

    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->verticalScrollBar())
            pScrollBar->setPageStep(nSize);
    });
}

void QtInstanceScrolledWindow::vadjustment_set_step_increment(int nSize)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pScrollBar = m_pScrollArea->verticalScrollBar())
            pScrollBar->setSingleStep(nSize);
    });
}

void QtInstanceScrolledWindow::set_vpolicy(VclPolicyType eVPolicy)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread(
        [&] { m_pScrollArea->setVerticalScrollBarPolicy(toQtPolicy(eVPolicy)); });
}

VclPolicyType QtInstanceScrolledWindow::get_vpolicy() const
{
    SolarMutexGuard g;

    VclPolicyType ePolicy = VclPolicyType::AUTOMATIC;
    GetQtInstance().RunInMainThread(
        [&] { ePolicy = toVclPolicy(m_pScrollArea->verticalScrollBarPolicy()); });

    return ePolicy;
}

int QtInstanceScrolledWindow::get_scroll_thickness() const
{
    SolarMutexGuard g;

    int nThickness = 0;
    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pVerticalScrollBar = m_pScrollArea->verticalScrollBar())
            nThickness = pVerticalScrollBar->width();
        else if (QScrollBar* pHorizontalScrollBar = m_pScrollArea->horizontalScrollBar())
            nThickness = pHorizontalScrollBar->height();
    });

    return nThickness;
}

void QtInstanceScrolledWindow::set_scroll_thickness(int nThickness)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QScrollBar* pVerticalScrollBar = m_pScrollArea->verticalScrollBar())
            pVerticalScrollBar->resize(nThickness, pVerticalScrollBar->height());
        else if (QScrollBar* pHorizontalScrollBar = m_pScrollArea->horizontalScrollBar())
            pHorizontalScrollBar->resize(pHorizontalScrollBar->width(), nThickness);
    });
}

void QtInstanceScrolledWindow::customize_scrollbars(const Color& rBackgroundColor,
                                                    const Color& rShadowColor,
                                                    const Color& rFaceColor)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        for (QScrollBar* pScrollBar :
             { m_pScrollArea->horizontalScrollBar(), m_pScrollArea->verticalScrollBar() })
        {
            if (pScrollBar)
            {
                QPalette aPalette = pScrollBar->palette();
                aPalette.setColor(QPalette::ColorRole::Base, toQColor(rBackgroundColor));
                aPalette.setColor(QPalette::ColorRole::Shadow, toQColor(rShadowColor));
                aPalette.setColor(QPalette::ColorRole::Button, toQColor(rFaceColor));
                pScrollBar->setPalette(aPalette);
            }
        }
    });
}

Qt::ScrollBarPolicy QtInstanceScrolledWindow::toQtPolicy(VclPolicyType eVclPolicy)
{
    switch (eVclPolicy)
    {
        case VclPolicyType::ALWAYS:
            return Qt::ScrollBarAlwaysOn;
        case VclPolicyType::AUTOMATIC:
            return Qt::ScrollBarAsNeeded;
        case VclPolicyType::NEVER:
            return Qt::ScrollBarAlwaysOff;
        default:
            assert(false && "Unhandled scroll bar policy");
            return Qt::ScrollBarAsNeeded;
    }
}

VclPolicyType QtInstanceScrolledWindow::toVclPolicy(Qt::ScrollBarPolicy eQtPolicy)
{
    switch (eQtPolicy)
    {
        case Qt::ScrollBarAlwaysOn:
            return VclPolicyType::ALWAYS;
        case Qt::ScrollBarAsNeeded:
            return VclPolicyType::AUTOMATIC;
        case Qt::ScrollBarAlwaysOff:
            return VclPolicyType::NEVER;
        default:
            assert(false && "Unhandled scroll bar policy");
            return VclPolicyType::AUTOMATIC;
    }
}

void QtInstanceScrolledWindow::handleHorizontalScrollValueChanged()
{
    SolarMutexGuard aGuard;
    signal_hadjustment_value_changed();
}

void QtInstanceScrolledWindow::handleVerticalScrollValueChanged()
{
    SolarMutexGuard aGuard;
    signal_vadjustment_value_changed();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
