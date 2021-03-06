/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2002-2014                                               *
 *   Umbrello UML Modeller Authors <umbrello-devel@kde.org>                *
 ***************************************************************************/

#ifndef UMLOPERATIONDIALOG_H
#define UMLOPERATIONDIALOG_H

//kde includes
#include "singlepagedialogbase.h"

//qt includes
#include <QListWidgetItem>

class DocumentationWidget;
class UMLOperation;
class ListPopupMenu;
class QGroupBox;
class QListWidget;
class QLabel;
class QRadioButton;
class QPushButton;
class QCheckBox;
class KComboBox;
class KLineEdit;
class UMLDoc;
class QToolButton;
class UMLStereotypeWidget;
class VisibilityEnumWidget;

/**
 * @author Paul Hensgen
 * Bugs and comments to umbrello-devel@kde.org or http://bugs.kde.org
 */
class UMLOperationDialog : public SinglePageDialogBase
{
    Q_OBJECT
public:

    UMLOperationDialog(QWidget * parent, UMLOperation * pOperation);
    ~UMLOperationDialog();

protected:

    void setupDialog();

    bool apply();

    void insertTypesSorted(const QString& type = QString());

    UMLOperation*  m_operation;  ///< The operation to represent.
    UMLDoc*        m_doc;        ///< The UMLDocument where all objects live.
    ListPopupMenu* m_menu;       ///< Menu used in parameter list box.

    //GUI widgets
    QGroupBox*    m_pParmsGB;
    QGroupBox*    m_pGenGB;
    QListWidget*  m_pParmsLW;
    QLabel*       m_pRtypeL;
    QLabel*       m_pNameL;
    KComboBox*    m_pRtypeCB;
    UMLStereotypeWidget* m_stereotypeWidget;
    VisibilityEnumWidget* m_visibilityEnumWidget;
    KLineEdit*    m_pNameLE;
    QCheckBox*    m_pAbstractCB;
    QCheckBox*    m_pStaticCB;
    QCheckBox*    m_pQueryCB;
    QPushButton*  m_pDeleteButton;
    QPushButton*  m_pPropertiesButton;
    QToolButton*  m_pUpButton;
    QToolButton*  m_pDownButton;
    DocumentationWidget*  m_docWidget;

public slots:
    void slotParmRightButtonPressed(const QPoint &p);
    void slotParmDoubleClick(QListWidgetItem *item);
    void slotMenuSelection(QAction* action);
    void slotNewParameter();
    void slotDeleteParameter();
    void slotParameterProperties();
    void slotParameterUp();
    void slotParameterDown();
    void slotParamsBoxClicked(QListWidgetItem* parameterItem);
    void slotNameChanged(const QString &);

};

#endif
