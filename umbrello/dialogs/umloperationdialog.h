/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef UMLOPERATIONDIALOG_H
#define UMLOPERATIONDIALOG_H

//kde includes
#include <kdialogbase.h>

/**
 * @author Paul Hensgen
 */

class UMLOperation;
class ListPopupMenu;
class QGroupBox;
class QListBox;
class QButtonGroup;
class QRadioButton;
class QLabel;
class QCheckBox;
class KComboBox;
class KLineEdit;

class UMLOperationDialog : public KDialogBase {
	Q_OBJECT

public:
	/**
	*	Constructor
	*/
	UMLOperationDialog( QWidget * parent, UMLOperation * pOperation );

	/**
	*	Deconstructor
	*/
	~UMLOperationDialog();

protected:
	/**
	*	Sets up the dialog
	*/
	void setupDialog();

	/**
	 * Checks if changes are valid and applies them if they are,
	 * else returns false
	 */
	bool apply();

	/**
	* Inserts @p type into the type-combobox as well as its completion object.
	*/
	void insertType( const QString& type, int index = -1 );

	/**
	*	The operation to represent.
	*/
	UMLOperation * m_pOperation;

	/**
	* 	Menu used in paramater list box.
	*/
	ListPopupMenu * m_pMenu;

	//GUI widgets
	QGroupBox  * m_pParmsGB, * m_pGenGB;
	QListBox * m_pParmsLB;
	QButtonGroup * m_pScopeBG;
	QRadioButton * m_pPublicRB, * m_pPrivateRB,  * m_pProtectedRB;
	QLabel * m_pRtypeL, * m_pNameL;
	KComboBox * m_pRtypeCB;
	KLineEdit * m_pNameLE;
	QCheckBox * m_pAbstractCB;
	QCheckBox * m_pStaticCB;
	QPushButton* m_pDeleteButton;
	QPushButton* m_pPropertiesButton;

public slots:
	void slotParmRightButtonPressed(QListBoxItem *item, const QPoint &p);
	void slotParmRightButtonClicked(QListBoxItem *item, const QPoint &p);
	void slotParmDoubleClick(QListBoxItem *item);
	void slotParmPopupMenuSel(int id);
	void slotNewParameter();
	void slotDeleteParameter();
	void slotParameterProperties();

	/**
	 * enables or disables buttons
	 */
	void slotParamsBoxClicked(QListBoxItem* parameterItem);

	/**
	 * I don't think this is used, but if we had an apply button
	 * it would slot into here
	 */
	void slotApply();

	/**
	 * Used when the OK button is clicked.  Calls apply()
	 */
	void slotOk();
};

#endif
