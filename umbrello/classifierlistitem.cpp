/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kdebug.h>
#include <klocale.h>

#include "classifierlistitem.h"

UMLClassifierListItem::UMLClassifierListItem(const UMLObject *parent, QString Name, int id)
  : UMLObject(parent, Name, id) {
}

UMLClassifierListItem::UMLClassifierListItem(const UMLObject *parent)
  : UMLObject(parent) {
}

UMLClassifierListItem::~UMLClassifierListItem() {
}

QString UMLClassifierListItem::getShortName() {
	return getName();
}

void UMLClassifierListItem::copyInto(UMLClassifierListItem *rhs) const
{
	// Call the parent.
	UMLObject::copyInto(rhs);
}


#include "classifierlistitem.moc"
