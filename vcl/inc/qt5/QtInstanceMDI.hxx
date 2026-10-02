/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include "QtInstanceContainer.hxx"
#include "QtInstanceWidget.hxx"

#include <vcl/weld/MDI.hxx>

#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>

#include <map>

// Qt counterpart to gtk3's GtkInstanceMDI: wraps a real QTabWidget,
// using its native setTabsClosable()/tabCloseRequested for the close
// button (Qt has no native pin concept, so the pin button is a small
// QToolButton placed via QTabBar::setTabButton on the other side).
class QtInstanceMDI : public QtInstanceWidget, public virtual weld::MDI
{
    Q_OBJECT

    QTabWidget* m_pTabWidget;
    int m_nNextDocId;

    std::map<int, QWidget*> m_aDocumentPages;
    std::map<int, QToolButton*> m_aDocumentPinButtons;
    std::map<int, bool> m_aDocumentDirty;
    std::map<int, bool> m_aDocumentPinned;
    std::map<int, OUString> m_aDocumentLabelText;

    mutable std::map<int, std::unique_ptr<QtInstanceContainer>> m_aPageContainerInstances;

    int docIdForIndex(int nIndex) const;
    int indexForDocId(int nDocId) const;
    void updateTabLabel(int nDocId);
    void updatePinIcon(int nDocId);

public:
    explicit QtInstanceMDI(QTabWidget* pTabWidget);

    virtual int insert_document(int nPos, const OUString& rLabel,
                                const OUString* pIconName) override;
    virtual void close_document(int nDocId) override;

    virtual void set_document_label(int nDocId, const OUString& rLabel) override;
    virtual void set_document_dirty(int nDocId, bool bDirty) override;
    virtual bool get_document_dirty(int nDocId) const override;
    virtual void set_document_pinned(int nDocId, bool bPinned) override;
    virtual bool get_document_pinned(int nDocId) const override;

    virtual int get_current_document() const override;
    virtual void set_current_document(int nDocId) override;
    virtual int get_document_count() const override;
    virtual int get_document_id(int nIndex) const override;

    virtual weld::Container* get_document_page(int nDocId) const override;

private Q_SLOTS:
    void currentTabChanged(int nIndex);
    void tabCloseRequested(int nIndex);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
