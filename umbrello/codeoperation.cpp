
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
 *      Date   : Fri Jun 20 2003
 */

#include <kdebug.h>
#include "codeoperation.h"

#include "codedocument.h"
#include "codegenerator.h"
#include "classifiercodedocument.h"

#include "umldoc.h"
#include "umlobject.h"

// Constructors/Destructors
//  

CodeOperation::CodeOperation ( ClassifierCodeDocument * doc , UMLOperation * parentOp, QString body, QString comment)
    : CodeMethodBlock ( doc, body, comment)
{
	init(parentOp);
}

CodeOperation::CodeOperation ( ClassifierCodeDocument * doc, UMLOperation * parentOp )
    : CodeMethodBlock ( doc )
{
	init(parentOp);
}

CodeOperation::~CodeOperation ( ) { }

//  
// Methods
//  

// Accessor methods
//  

/**
 * Add a Parameter object to the m_parameterVector List
 */
/*
void CodeOperation::addParameter ( CodeParameter * add_object ) {
	m_parameterVector.append(add_object);
}
*/

/**
 * Remove a Parameter object from m_parameterVector List
 */
/*
void CodeOperation::removeParameter ( CodeParameter * remove_object ) {
	m_parameterVector.remove(remove_object);
}
*/

/**
 * Get the list of Parameter objects held by m_parameterVector
 * @return QPtrList<CodeParameter *> list of Parameter objects held by
 * m_parameterVector
 */
/*
QPtrList<CodeParameter> CodeOperation::getParameterList ( ) {
	return m_parameterVector;
}
*/

/**
 * Get the parent UMLOperation of this codeoperation.
 */
UMLOperation * CodeOperation::getParentOperation( ) {
	return m_parentOperation;
}

UMLObject * CodeOperation::getParentObject ( ) {
	return (UMLObject*)m_parentOperation;
}


// Other methods
//  

/** set attributes of the node that represents this class
 * in the XMI document.
 */
void CodeOperation::setAttributesOnNode ( QDomDocument & doc, QDomElement & elem) 
{

	CodeMethodBlock::setAttributesOnNode(doc,elem); // superclass

}

/** set the class attributes of this object from
 * the passed element node.
 */
void CodeOperation::setAttributesFromNode ( QDomElement & element) 
{

	CodeMethodBlock::setAttributesFromNode(element); // superclass

	// now set local attributes

	// oops..this is done in the parent class "ownedcodeblock".
	// we simply need to record the parent operation here
	// m_parentOperation->disconnect(this); // always disconnect from current parent

	int id = element.attribute("parent_id","-1").toInt();
	UMLObject * obj = getParentDocument()->getParentGenerator()->getDocument()->findUMLObject(id); 
	UMLOperation * op = dynamic_cast<UMLOperation*>(obj);

	m_parentOperation->disconnect(this); // always disconnect
	if(op)
		init(op); 
	else
		kdError()<<"ERROR: could'nt load code operation because of missing UMLoperation, corrupt savefile?"<<endl;

}

void CodeOperation::setAttributesFromObject(TextBlock * obj)
{

        CodeMethodBlock::setAttributesFromObject(obj);

        CodeOperation * op = dynamic_cast<CodeOperation*>(obj);
        if(op)
        {
		m_parentOperation->disconnect(this); // always disconnect
		init((UMLOperation*) op->getParentObject());
        }

}

void CodeOperation::init (UMLOperation * parentOp) 
{

	m_canDelete = false; // we cant delete these with the codeeditor, delete the UML operation instead.
	m_parentOperation = parentOp;
	setTag(CodeOperation::findTag(parentOp));

	// not needed.. done by parent "ownedcodeblock" class
//	connect(m_parentOperation,SIGNAL(modified()),this,SLOT(syncToParent()));

}

void CodeOperation::updateContent() {
        // Empty. Unlike codeaccessor methods for most (all?) languages
	// we dont auto-generate content for operations
}


#include "codeoperation.moc"
