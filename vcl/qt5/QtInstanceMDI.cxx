/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <QtInstanceMDI.hxx>
#include <QtInstanceMDI.moc>

#include <vcl/qt/QtUtils.hxx>

#include <QtGui/QIcon>
#include <QtWidgets/QStyle>
#include <QtWidgets/QTabBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace
{
// property on each tab's page widget holding its opaque int document id
const char* const PROPERTY_DOC_ID = "code-mdi-doc-id";
}

QtInstanceMDI::QtInstanceMDI(QTabWidget* pTabWidget)
    : QtInstanceWidget(pTabWidget)
    , m_pTabWidget(pTabWidget)
    , m_nNextDocId(1)
{
    assert(m_pTabWidget);

    // Native close button -- Qt already has exactly this feature, no need
    // to build one by hand the way the gtk3 implementation does.
    m_pTabWidget->setTabsClosable(true);

    // Qt's native "these tabs are documents, not dialog pages" mode: flat,
    // compact tab strip without the tab-pane frame. Styles use it to drop
    // most of the regular QTabBar padding (Breeze: 42px -> ~2/3 of that for
    // a 13px-high label font), which is exactly the Visual-Studio-like look
    // wanted here.
    m_pTabWidget->setDocumentMode(true);

    connect(m_pTabWidget, &QTabWidget::currentChanged, this, &QtInstanceMDI::currentTabChanged);
    connect(m_pTabWidget, &QTabWidget::tabCloseRequested, this, &QtInstanceMDI::tabCloseRequested);
}

int QtInstanceMDI::docIdForIndex(int nIndex) const
{
    QWidget* pPage = m_pTabWidget->widget(nIndex);
    if (!pPage)
        return 0;
    QVariant aIdVariant = pPage->property(PROPERTY_DOC_ID);
    return aIdVariant.canConvert<int>() ? aIdVariant.toInt() : 0;
}

int QtInstanceMDI::indexForDocId(int nDocId) const
{
    auto it = m_aDocumentPages.find(nDocId);
    if (it == m_aDocumentPages.end())
        return -1;
    return m_pTabWidget->indexOf(it->second);
}

void QtInstanceMDI::updateTabLabel(int nDocId)
{
    const int nIndex = indexForDocId(nDocId);
    if (nIndex < 0)
        return;
    OUString sText = m_aDocumentLabelText[nDocId];
    if (m_aDocumentDirty[nDocId])
        sText = u"● "_ustr + sText;
    m_pTabWidget->setTabText(nIndex, toQString(sText));
}

void QtInstanceMDI::updatePinIcon(int nDocId)
{
    QToolButton* pPinButton = m_aDocumentPinButtons[nDocId];
    if (!pPinButton)
        return;
    // QIcon::fromTheme(), not loadQPixmapIcon(): the latter resolves
    // through vcl's own bundled ImageTree (a literal file lookup in this
    // tree's own icon-theme packages), which has no "starred-symbolic"/
    // "non-starred-symbolic"-style freedesktop names at all -- confirmed
    // empirically (gtk3's own GtkInstanceMDI hit the exact same problem,
    // blank icons, with its LO-side equivalent). QIcon::fromTheme() goes
    // through Qt's native system icon-theme resolution instead, same
    // spirit as gtk3's own fix (see the long comment on
    // GtkInstanceMDI::SetSystemThemeIcon).
    const bool bPinned = m_aDocumentPinned[nDocId];
    pPinButton->setIcon(QIcon::fromTheme(bPinned ? QStringLiteral("starred-symbolic")
                                                  : QStringLiteral("non-starred-symbolic")));
}

int QtInstanceMDI::insert_document(int nPos, const OUString& rLabel, const OUString* pIconName)
{
    SolarMutexGuard g;

    int nDocId = 0;
    GetQtInstance().RunInMainThread([&] {
        nDocId = m_nNextDocId++;

        QWidget* pPage = new QWidget;
        // zero the QVBoxLayout's default contents margins: the document
        // content should fill its page edge-to-edge, same as the bare
        // GtkBox page the gtk3 implementation uses (GtkBox has no implicit
        // margins; a default-constructed Qt layout does)
        QVBoxLayout* pPageLayout = new QVBoxLayout;
        pPageLayout->setContentsMargins(0, 0, 0, 0);
        pPageLayout->setSpacing(0);
        pPage->setLayout(pPageLayout);
        pPage->setProperty(PROPERTY_DOC_ID, nDocId);

        int nIndex = m_pTabWidget->insertTab(nPos, pPage, vclToQtStringWithAccelerator(rLabel));
        if (pIconName && !pIconName->isEmpty())
            m_pTabWidget->setTabIcon(nIndex, loadQPixmapIcon(*pIconName));

        QToolButton* pPinButton = new QToolButton;
        pPinButton->setAutoRaise(true);
        // Left to its own devices a QToolButton's sizeHint is massive relative
        // to a tab label (34px for a 13px-high font under Breeze), and
        // QTabBar grows every tab to fit the side widget -- size it like the
        // style's own native tab close button instead, so the pin can never
        // be the tallest thing in the tab.
        const int nCloseW
            = m_pTabWidget->style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, nullptr, pPage);
        const int nCloseH = m_pTabWidget->style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight,
                                                               nullptr, pPage);
        pPinButton->setFixedSize(nCloseW, nCloseH);
        pPinButton->setIconSize(QSize(nCloseW, nCloseH));
        connect(pPinButton, &QToolButton::clicked, this, [this, nDocId] {
            set_document_pinned(nDocId, !get_document_pinned(nDocId));
            signal_document_pin_toggled(nDocId);
        });
        m_pTabWidget->tabBar()->setTabButton(nIndex, QTabBar::LeftSide, pPinButton);

        m_aDocumentPages[nDocId] = pPage;
        m_aDocumentPinButtons[nDocId] = pPinButton;
        m_aDocumentDirty[nDocId] = false;
        m_aDocumentPinned[nDocId] = false;
        m_aDocumentLabelText[nDocId] = rLabel;

        updatePinIcon(nDocId);
    });

    return nDocId;
}

void QtInstanceMDI::close_document(int nDocId)
{
    SolarMutexGuard g;
    GetQtInstance().RunInMainThread([&] {
        const int nIndex = indexForDocId(nDocId);
        if (nIndex >= 0)
            m_pTabWidget->removeTab(nIndex);
    });

    m_aDocumentPages.erase(nDocId);
    m_aDocumentPinButtons.erase(nDocId);
    m_aDocumentDirty.erase(nDocId);
    m_aDocumentPinned.erase(nDocId);
    m_aDocumentLabelText.erase(nDocId);
    m_aPageContainerInstances.erase(nDocId);
}

void QtInstanceMDI::set_document_label(int nDocId, const OUString& rLabel)
{
    SolarMutexGuard g;
    m_aDocumentLabelText[nDocId] = rLabel;
    GetQtInstance().RunInMainThread([&] { updateTabLabel(nDocId); });
}

void QtInstanceMDI::set_document_dirty(int nDocId, bool bDirty)
{
    SolarMutexGuard g;
    m_aDocumentDirty[nDocId] = bDirty;
    GetQtInstance().RunInMainThread([&] { updateTabLabel(nDocId); });
}

bool QtInstanceMDI::get_document_dirty(int nDocId) const
{
    auto it = m_aDocumentDirty.find(nDocId);
    return it != m_aDocumentDirty.end() && it->second;
}

void QtInstanceMDI::set_document_pinned(int nDocId, bool bPinned)
{
    SolarMutexGuard g;
    m_aDocumentPinned[nDocId] = bPinned;
    GetQtInstance().RunInMainThread([&] { updatePinIcon(nDocId); });
}

bool QtInstanceMDI::get_document_pinned(int nDocId) const
{
    auto it = m_aDocumentPinned.find(nDocId);
    return it != m_aDocumentPinned.end() && it->second;
}

int QtInstanceMDI::get_current_document() const
{
    SolarMutexGuard g;
    int nDocId = 0;
    GetQtInstance().RunInMainThread([&] {
        const int nIndex = m_pTabWidget->currentIndex();
        if (nIndex >= 0)
            nDocId = docIdForIndex(nIndex);
    });
    return nDocId;
}

void QtInstanceMDI::set_current_document(int nDocId)
{
    SolarMutexGuard g;
    GetQtInstance().RunInMainThread([&] {
        const int nIndex = indexForDocId(nDocId);
        if (nIndex >= 0)
            m_pTabWidget->setCurrentIndex(nIndex);
    });
}

int QtInstanceMDI::get_document_count() const
{
    SolarMutexGuard g;
    int nCount = 0;
    GetQtInstance().RunInMainThread([&] { nCount = m_pTabWidget->count(); });
    return nCount;
}

int QtInstanceMDI::get_document_id(int nIndex) const
{
    SolarMutexGuard g;
    int nDocId = 0;
    GetQtInstance().RunInMainThread([&] { nDocId = docIdForIndex(nIndex); });
    return nDocId;
}

weld::Container* QtInstanceMDI::get_document_page(int nDocId) const
{
    SolarMutexGuard g;

    auto it = m_aDocumentPages.find(nDocId);
    if (it == m_aDocumentPages.end())
        return nullptr;
    QWidget* pWidget = it->second;

    if (!m_aPageContainerInstances.contains(nDocId))
        m_aPageContainerInstances.emplace(nDocId, std::make_unique<QtInstanceContainer>(pWidget));

    return m_aPageContainerInstances.at(nDocId).get();
}

void QtInstanceMDI::currentTabChanged(int nIndex)
{
    SolarMutexGuard g;
    signal_current_document_changed(docIdForIndex(nIndex));
}

void QtInstanceMDI::tabCloseRequested(int nIndex)
{
    SolarMutexGuard g;
    signal_document_close_request(docIdForIndex(nIndex));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
