/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlayout.h>
#include <klocale.h>
#include <kdebug.h>

#include "../umlview.h"
#include "../classwidget.h"
#include "../interfacewidget.h"
#include "classoptionspage.h"

ClassOptionsPage::ClassOptionsPage(QWidget* pParent, UMLWidget* pWidget, UMLWidget_Type type) : QWidget( pParent ) {
	if (type == wt_Class) {
		m_pClassWidget = static_cast<ClassWidget*>(pWidget);
		m_pInterfaceWidget = 0;
		setupClassPage();
	} else if (type == wt_Interface) {
		m_pInterfaceWidget = static_cast<InterfaceWidget*>(pWidget);
		m_pClassWidget = 0;
		setupInterfacePage();
	}
	m_options = 0;
	
}

ClassOptionsPage::ClassOptionsPage(QWidget* pParent, SettingsDlg::OptionState *options) : QWidget( pParent )
{
	m_options = options;
	m_pClassWidget = 0;
	m_pInterfaceWidget = 0;
	setupClassPageOption();
}

ClassOptionsPage::~ClassOptionsPage() {}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ClassOptionsPage::setupClassPage() {
	int margin = fontMetrics().height();

	bool sig = false;
	Uml::Signature_Type sigtype;

	QVBoxLayout * topLayout = new QVBoxLayout(this);
	topLayout -> setSpacing(6);
	m_pVisibilityGB = new QGroupBox(i18n("Show"), this);
	topLayout -> addWidget(m_pVisibilityGB);
	QGridLayout * visibilityLayout = new QGridLayout(m_pVisibilityGB);
	visibilityLayout -> setSpacing(10);
	visibilityLayout -> setMargin(margin);

	m_pShowOpsCB = new QCheckBox(i18n("Operatio&ns"), m_pVisibilityGB);
	m_pShowOpsCB -> setChecked(m_pClassWidget -> getShowOps());
	visibilityLayout -> addWidget(m_pShowOpsCB, 0, 0);

	sigtype = m_pClassWidget -> getShowOpSigs();
	if(sigtype == Uml::st_NoSig || sigtype == Uml::st_NoSigNoScope)
		sig = false;
	else
		sig = true;
	m_pShowOpSigCB = new QCheckBox(i18n("O&peration signature"), m_pVisibilityGB);
	m_pShowOpSigCB -> setChecked(sig);
	visibilityLayout -> addWidget(m_pShowOpSigCB, 1, 0);
	visibilityLayout -> setRowStretch(3, 1);

	m_pShowAttsCB = new QCheckBox(i18n("Att&ributes"), m_pVisibilityGB);
	m_pShowAttsCB -> setChecked(m_pClassWidget -> getShowAtts());
	visibilityLayout -> addWidget(m_pShowAttsCB, 2, 0);

	m_pShowAttSigCB = new QCheckBox(i18n("Attr&ibute signature"), m_pVisibilityGB);
	sigtype = m_pClassWidget -> getShowAttSigs();
	if(sigtype == Uml::st_NoSig || sigtype == Uml::st_NoSigNoScope)
		sig = false;
	else
		sig = true;
	m_pShowAttSigCB -> setChecked(sig);
	visibilityLayout -> addWidget(m_pShowAttSigCB, 3, 0);

	m_pShowScopeCB = new QCheckBox(i18n("&Visibility"), m_pVisibilityGB);
	m_pShowScopeCB -> setChecked(m_pClassWidget -> getShowScope());
	visibilityLayout -> addWidget(m_pShowScopeCB, 0, 1);

	m_pShowPackageCB = new QCheckBox(i18n("Pac&kage"), m_pVisibilityGB);
	m_pShowPackageCB -> setChecked(m_pClassWidget -> getShowPackage());
	visibilityLayout -> addWidget(m_pShowPackageCB, 1, 1);

	m_pShowStereotypeCB = new QCheckBox(i18n("Stereot&ype"), m_pVisibilityGB);
	m_pShowStereotypeCB -> setChecked(m_pClassWidget -> getShowStereotype());
	visibilityLayout -> addWidget(m_pShowStereotypeCB, 2, 1);

}

void ClassOptionsPage::setupClassPageOption() {

	int margin = fontMetrics().height();

// 	bool sig = false;
// 	Uml::Signature_Type sigtype;

	QVBoxLayout * topLayout = new QVBoxLayout(this);
	topLayout -> setSpacing(6);
	m_pVisibilityGB = new QGroupBox(i18n("Show"), this);
	topLayout -> addWidget(m_pVisibilityGB);
	QGridLayout * visibilityLayout = new QGridLayout(m_pVisibilityGB);
	visibilityLayout -> setSpacing(10);
	visibilityLayout -> setMargin(margin);

	m_pShowOpsCB = new QCheckBox(i18n("Operatio&ns"), m_pVisibilityGB);
	m_pShowOpsCB -> setChecked( m_options->classState.showOps );
	visibilityLayout -> addWidget(m_pShowOpsCB, 0, 0);
/*###
	if (m_) {
		if (m_ShowOpSigs == Uml::st_NoSigNoScope)
			m_ShowOpSigs = Uml::st_NoSig;
		else if (m_ShowOpSigs == Uml::st_SigNoScope)
			m_ShowOpSigs = Uml::st_ShowSig;

		if (m_ShowAttSigs == Uml::st_NoSigNoScope)
			m_ShowAttSigs = Uml::st_NoSig;
		else if (m_ShowAttSigs == Uml::st_SigNoScope)
			m_ShowAttSigs = Uml::st_ShowSig;
	} else {
		if (m_ShowOpSigs == Uml::st_ShowSig)
			m_ShowOpSigs = Uml::st_SigNoScope;
		else if (m_ShowOpSigs == Uml::st_NoSig)
			m_ShowOpSigs = Uml::st_NoSigNoScope;

		if (m_ShowAttSigs == Uml::st_ShowSig)
			m_ShowAttSigs = Uml::st_SigNoScope;
		else if(m_ShowAttSigs == Uml::st_NoSig)
			m_ShowAttSigs = Uml::st_NoSigNoScope;
	}

###
	sigtype = m_options->classState.showOpSig;*/
/*	if(sigtype == Uml::st_NoSig || sigtype == Uml::st_NoSigNoScope)
		sig = false;
	else
		sig = true;*/
	m_pShowOpSigCB = new QCheckBox(i18n("O&peration signature"), m_pVisibilityGB);
	m_pShowOpSigCB -> setChecked(m_options->classState.showOpSig);
	visibilityLayout -> addWidget(m_pShowOpSigCB, 1, 0);
	visibilityLayout -> setRowStretch(3, 1);

	m_pShowAttsCB = new QCheckBox(i18n("Att&ributes"), m_pVisibilityGB);
	m_pShowAttsCB -> setChecked(m_options->classState.showAtts );
	visibilityLayout -> addWidget(m_pShowAttsCB, 2, 0);

	m_pShowAttSigCB = new QCheckBox(i18n("Attr&ibute signature"), m_pVisibilityGB);
/*	sigtype = m_options->classState.showAttSig;
	if(sigtype == Uml::st_NoSig || sigtype == Uml::st_NoSigNoScope)
		sig = false;
	else
		sig = true;*/
	m_pShowAttSigCB -> setChecked(m_options->classState.showAttSig);
	visibilityLayout -> addWidget(m_pShowAttSigCB, 3, 0);

	m_pShowScopeCB = new QCheckBox(i18n("&Visibility"), m_pVisibilityGB);
	m_pShowScopeCB -> setChecked(m_options->classState.showScope);
	visibilityLayout -> addWidget(m_pShowScopeCB, 0, 1);

	m_pShowPackageCB = new QCheckBox(i18n("Pac&kage"), m_pVisibilityGB);
	m_pShowPackageCB -> setChecked(m_options->classState.showPackage);
	visibilityLayout -> addWidget(m_pShowPackageCB, 1, 1);

	m_pShowStereotypeCB = new QCheckBox(i18n("Stereot&ype"), m_pVisibilityGB);
	m_pShowStereotypeCB -> setChecked(m_options->classState.showStereoType);
	visibilityLayout -> addWidget(m_pShowStereotypeCB, 2, 1);

}

void ClassOptionsPage::setupInterfacePage() {
	int margin = fontMetrics().height();

	bool sig = false;
	Uml::Signature_Type sigtype;

	QVBoxLayout * topLayout = new QVBoxLayout(this);
	topLayout -> setSpacing(6);
	m_pVisibilityGB = new QGroupBox(i18n("Show"), this);
	topLayout -> addWidget(m_pVisibilityGB);
	QGridLayout * visibilityLayout = new QGridLayout(m_pVisibilityGB);
	visibilityLayout -> setSpacing(10);
	visibilityLayout -> setMargin(margin);

	m_pShowOpsCB = new QCheckBox(i18n("Operatio&ns"), m_pVisibilityGB);
	m_pShowOpsCB -> setChecked(m_pInterfaceWidget -> getShowOps());
	visibilityLayout -> addWidget(m_pShowOpsCB, 0, 0);

	sigtype = m_pInterfaceWidget -> getShowOpSigs();
	if(sigtype == Uml::st_NoSig || sigtype == Uml::st_NoSigNoScope)
		sig = false;
	else
		sig = true;
	m_pShowOpSigCB = new QCheckBox(i18n("O&peration signature"), m_pVisibilityGB);
	m_pShowOpSigCB -> setChecked(sig);
	visibilityLayout -> addWidget(m_pShowOpSigCB, 1, 0);
	visibilityLayout -> setRowStretch(3, 1);

	m_pShowScopeCB = new QCheckBox(i18n("&Visibility"), m_pVisibilityGB);
	m_pShowScopeCB -> setChecked(m_pInterfaceWidget -> getShowScope());
	visibilityLayout -> addWidget(m_pShowScopeCB, 0, 1);

	m_pShowPackageCB = new QCheckBox(i18n("Pac&kage"), m_pVisibilityGB);
	m_pShowPackageCB -> setChecked(m_pInterfaceWidget -> getShowPackage());
	visibilityLayout -> addWidget(m_pShowPackageCB, 1, 1);

	m_pDrawAsCircleCB = new QCheckBox(i18n("Draw as circle"), m_pVisibilityGB);
	m_pDrawAsCircleCB->setChecked( m_pInterfaceWidget->getDrawAsCircle() );
	visibilityLayout->addWidget(m_pDrawAsCircleCB, 2, 0);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ClassOptionsPage::updateUMLWidget() {
	if (m_pClassWidget) {
		updateClassWidget();
	} else if (m_pInterfaceWidget) {
		updateInterfaceWidget();
	} else if (m_options) {
		updateOptionState();
	}
}

void ClassOptionsPage::updateClassWidget() {
	m_pClassWidget->setShowScope( m_pShowScopeCB->isChecked() );
	m_pClassWidget->setShowAtts( m_pShowAttsCB->isChecked() );
	m_pClassWidget->setShowOps( m_pShowOpsCB->isChecked() );
	m_pClassWidget->setShowStereotype( m_pShowStereotypeCB->isChecked() );
	m_pClassWidget->setShowPackage( m_pShowPackageCB->isChecked() );
	m_pClassWidget->setShowAttSigs( m_pShowAttSigCB->isChecked() );
	m_pClassWidget->setShowOpSigs( m_pShowOpSigCB->isChecked() );
}

void ClassOptionsPage::updateInterfaceWidget() {
	m_pInterfaceWidget->setShowScope( m_pShowScopeCB->isChecked() );
	m_pInterfaceWidget->setShowOps( m_pShowOpsCB->isChecked() );
	m_pInterfaceWidget->setShowPackage( m_pShowPackageCB->isChecked() );
	m_pInterfaceWidget->setShowOpSigs( m_pShowOpSigCB->isChecked() );
	m_pInterfaceWidget->setDrawAsCircle( m_pDrawAsCircleCB->isChecked() );
}

void ClassOptionsPage::updateOptionState() {
	m_options->classState.showScope =    m_pShowScopeCB->isChecked();
	m_options->classState.showAtts  =    m_pShowAttsCB->isChecked();
	m_options->classState.showOps   =    m_pShowOpsCB->isChecked();
	m_options->classState.showStereoType = m_pShowStereotypeCB->isChecked();
	m_options->classState.showPackage    = m_pShowPackageCB->isChecked();
	m_options->classState.showAttSig     = m_pShowAttSigCB->isChecked();
	m_options->classState.showOpSig      = m_pShowOpSigCB->isChecked();
}


//#include "classoptionspage.moc"
