
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*  This code generated by:
 *      Author : thomas
 *      Date   : Tue Aug 19 2003
 */

#include <kdebug.h>
#include "ownedcodeblock.h"
#include "umlobject.h"

// Constructors/Destructors
//  

OwnedCodeBlock::OwnedCodeBlock ( UMLObject * parent ) 
	: QObject ( (QObject*)parent, "anOwnedCodeBlock" ) 
{
	// one reason for being: set up the connection between
	// this code block and the parent UMLObject..when the parent
	// signals a change has been made, we automatically update
	// ourselves
	connect(parent,SIGNAL(modified()),this,SLOT(syncToParent()));
}

OwnedCodeBlock::~OwnedCodeBlock ( ) { }

//  
// Methods
//  

// Other methods
//  


/** set attributes of the node that represents this class
  * in the XMI document.
  */
void OwnedCodeBlock::setAttributesOnNode ( QDomDocument & doc, QDomElement & elem) {

        // set local class attributes
        elem.setAttribute("parentObj",QString::number(getParentObject()->getID()));

}

 /** set the class attributes of this object from
  * the passed element node.
  */
void OwnedCodeBlock::setAttributesFromNode ( QDomElement & element) {

        disconnect(getParentObject(),SIGNAL(modified()),this,SLOT(syncToParent()));

// FIX: need to SET parent object.
kdWarning()<<"PARENT OBJECT NOT SET in ownedcodeblock!!"<<endl;
        connect(getParentObject(),SIGNAL(modified()),this,SLOT(syncToParent()));
}

/**
 */
void OwnedCodeBlock::syncToParent ( ) {
	updateContent();
}

#include "ownedcodeblock.moc"
