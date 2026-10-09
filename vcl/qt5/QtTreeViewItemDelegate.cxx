/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <QtTreeViewItemDelegate.hxx>
#include <QtTreeViewItemDelegate.moc>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>

QtTreeViewItemDelegate::QtTreeViewItemDelegate(
    QObject* pParent, std::function<bool(const QModelIndex& rIndex)> aStartEditingFunction,
    std::function<bool(const QModelIndex& rIndex, const QString& rNewText)> aFinishEditingFunction)
    : QStyledItemDelegate(pParent)
    , m_aStartEditingFunction(std::move(aStartEditingFunction))
    , m_aFinishEditingFunction(std::move(aFinishEditingFunction))
{
}

QWidget* QtTreeViewItemDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& rOption,
                                              const QModelIndex& rIndex) const
{
    const QStringList aChoices = rIndex.data(ROLE_CHOICES).toStringList();
    // a cell that is a list has to have its values; without them it is not edited
    // (the proxy model's own role lookup reaches the source item's data)
    if (!m_aStartEditingFunction(rIndex))
        return nullptr;

    if (!aChoices.isEmpty())
    {
        QComboBox* pCombo = new QComboBox(pParent);
        pCombo->addItems(aChoices);
        pCombo->setCurrentText(rIndex.data(Qt::DisplayRole).toString());
        // choosing a value is the edit: it is done at once, and the list closes
        connect(pCombo, &QComboBox::activated, this, [this, pCombo] {
            Q_EMIT const_cast<QtTreeViewItemDelegate*>(this)->commitData(pCombo);
            Q_EMIT const_cast<QtTreeViewItemDelegate*>(this)->closeEditor(pCombo);
        });
        m_pEditor = pCombo;
        return m_pEditor;
    }

    m_pEditor = QStyledItemDelegate::createEditor(pParent, rOption, rIndex);
    return m_pEditor;
}

void QtTreeViewItemDelegate::setEditorData(QWidget* pEditor, const QModelIndex& rIndex) const
{
    // the combo is made with its current value already
    if (qobject_cast<QComboBox*>(pEditor))
        return;
    QStyledItemDelegate::setEditorData(pEditor, rIndex);
}

void QtTreeViewItemDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel,
                                          const QModelIndex& rIndex) const
{
    // check whether new string is accepted
    if (QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(pEditor))
    {
        if (!m_aFinishEditingFunction(rIndex, pLineEdit->text()))
            return;
    }
    else if (QComboBox* pCombo = qobject_cast<QComboBox*>(pEditor))
    {
        // the same hook as for text; the value goes into the cell only if it is taken
        if (!m_aFinishEditingFunction(rIndex, pCombo->currentText()))
            return;
        pModel->setData(rIndex, pCombo->currentText(), Qt::DisplayRole);
        return;
    }

    QStyledItemDelegate::setModelData(pEditor, pModel, rIndex);
}

void QtTreeViewItemDelegate::endEditing() { closeEditor(m_pEditor); }

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
