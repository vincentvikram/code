/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *  copyright (C) 2006                                                     *
 *  Umbrello UML Modeller Authors <uml-devel@ uml.sf.net>                  *
 ***************************************************************************/

#ifndef OBJECT_FACTORY__H
#define OBJECT_FACTORY__H

#include <qstring.h>
#include "umlnamespace.h"

class UMLObject;
class UMLPackage;
class UMLClassifier;
class UMLClassifierListItem;
class UMLAttribute;
class UMLOperation;

namespace Object_Factory {

/**
 * Creates a UMLObject of the given type.
 *
 * @param type      The type of @ref UMLObject to create.
 * @param n         A name to give to the object (optional.)
 *                  If not given then an input dialog prompts
 *                  the user to supply a name.
 * @param parentPkg The object's parent package (optional.)
 * @param prepend   Set true if it is desired to prepend the new
 *                  object to the list of objects.
 *                  The default is to append.
 */
UMLObject* createUMLObject(Uml::Object_Type type,
                           const QString &n = QString::null,
                           UMLPackage *parentPkg = 0,
                           bool prepend = false);

/**
 * Creates an operation, attribute, template, or enum literal
 * for the parent classifier.
 *
 * @param parent    The parent concept
 * @param type      The type to create
 * @return      Pointer to the UMLClassifierListItem created
 */
UMLClassifierListItem* createChildObject(UMLClassifier *parent, Uml::Object_Type type);

UMLAttribute *createAttribute(UMLObject *parent, QString name);

UMLOperation *createOperation(UMLClassifier *parent, QString name);

/**
 * Control whether the createUMLObject() solicits a new unique ID for the
 * created object.
 * By default, unique ID generation is turned on.
 *
 * @param yesno   False turns UID generation off, true turns it on.
 */
void assignUniqueIdOnCreation(bool yesno);

/**
 * Return whether unique ID generation is on or off.
 */
bool assignUniqueIdOnCreation();

/**
 * Make a new UMLObject according to the given XMI tag.
 * Used by loadFromXMI and clipboard paste.
 */
UMLObject* makeObjectFromXMI(const QString &xmiTag);

}

#endif
