
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
 *      Date   : Thu Jun 19 2003
 */

#include <kdebug.h>
#include <qregexp.h>

#include "codegenerator.h"
#include "classifiercodedocument.h"
#include "association.h"
#include "attribute.h"
#include "class.h"
#include "interface.h"
#include "umlrole.h"

// Constructors/Destructors
//  

ClassifierCodeDocument::ClassifierCodeDocument ( UMLClassifier * parent , CodeGenerator * gen ) :
       CodeDocument ( gen ) 
{
	init (parent); 
}

ClassifierCodeDocument::~ClassifierCodeDocument ( ) { }

//  
// Methods
//  


// Accessor methods
//  

/** get a list of codeclassifier objects held by this classifiercodedocument that meet the passed criteria.
 */
QPtrList<CodeClassField> ClassifierCodeDocument::getSpecificClassFields (CodeClassField::ClassFieldType cfType) 
{
        QPtrList<CodeClassField> list;
        for (CodeClassField * cf = m_classfieldVector.first(); cf; cf = m_classfieldVector.next())
                if (cf->getClassFieldType() == cfType)
                        list.append(cf);
        return list;
}

/** get a list of codeclassifier objects held by this classifiercodedocument that meet the passed criteria.
 */
QPtrList<CodeClassField> ClassifierCodeDocument::getSpecificClassFields (CodeClassField::ClassFieldType cfType, bool isStatic)
{
	QPtrList<CodeClassField> list;
	list.setAutoDelete(false);
        for (CodeClassField * cf = m_classfieldVector.first(); cf; cf = m_classfieldVector.next())
                if (cf->getClassFieldType() == cfType && cf->getStatic() == isStatic)
                        list.append(cf);
        return list;
}

/** get a list of codeclassifier objects held by this classifiercodedocument that meet the passed criteria.
 */
QPtrList<CodeClassField> ClassifierCodeDocument::getSpecificClassFields (CodeClassField::ClassFieldType cfType, Uml::Scope visibility)
{
	QPtrList<CodeClassField> list;
	list.setAutoDelete(false);
        for (CodeClassField * cf = m_classfieldVector.first(); cf; cf = m_classfieldVector.next())
                if (cf->getClassFieldType() == cfType && cf->getVisibility() == visibility)
                        list.append(cf);
       	return list;
}

/** get a list of codeclassifier objects held by this classifiercodedocument that meet the passed criteria.
 */
QPtrList<CodeClassField> ClassifierCodeDocument::getSpecificClassFields (CodeClassField::ClassFieldType cfType, bool isStatic, Uml::Scope visibility)
{
	QPtrList<CodeClassField> list;
	list.setAutoDelete(false);
        for (CodeClassField * cf = m_classfieldVector.first(); cf; cf = m_classfieldVector.next())
                if (cf->getClassFieldType() == cfType && cf->getVisibility() == visibility && cf->getStatic() == isStatic )
                        list.append(cf);
       	return list;
}

// do we have accessor methods for lists of objects?
// (as opposed to lists of primitive types like 'int' or 'float', etc)
bool ClassifierCodeDocument::hasObjectVectorClassFields() {
        for (CodeClassField * cf = m_classfieldVector.first(); cf; cf = m_classfieldVector.next())
		if(cf->getClassFieldType() != CodeClassField::Attribute)
		{
			UMLRole * role = dynamic_cast<UMLRole*>(cf->getParentObject()); 	
			QString multi = role->getMultiplicity();
			if ( 
				multi.contains(QRegExp("[23456789\\*]")) ||
				multi.contains(QRegExp("1\\d"))
                           )
				return true;
		}	
	return false;
}

bool ClassifierCodeDocument::hasClassFields() {
	if(m_classfieldVector.count() > 0 ) 
		return true;
	return false;
}

/**
 * Tell if one or more codeclassfields are derived from associations.
 */
bool ClassifierCodeDocument::hasAssociationClassFields() {
	QPtrList<CodeClassField> list = getSpecificClassFields(CodeClassField::Attribute);
	return (m_classfieldVector.count() - list.count()) > 0 ? true : false;
}

/**
 * Tell if one or more codeclassfields are derived from attributes.
 */
bool ClassifierCodeDocument::hasAttributeClassFields() {
	QPtrList<CodeClassField> list = getSpecificClassFields(CodeClassField::Attribute);
	return list.count() > 0 ? true : false;
}

/**
 * Add a CodeClassField object to the m_classfieldVector List
 * @return boolean value if successfull in adding
 */
// We DONT add methods of the code classfield here because we need to allow
// the codegenerator writer the liberty to organize their document as they desire.
bool ClassifierCodeDocument::addCodeClassField ( CodeClassField * add_object ) {
	UMLObject * umlobj = add_object->getParentObject();
        if(!(m_classFieldMap->contains(umlobj)))
	{
		m_classfieldVector.append(add_object);
        	m_classFieldMap->insert(umlobj,add_object);

		return true;
	}
	return false;
}

// this is a slot..should only be called from a signal
void ClassifierCodeDocument::addAttributeClassField (UMLObject *obj, bool syncToParentIfAdded) {
	UMLAttribute *at = (UMLAttribute*)obj;
	CodeClassField * cf = newCodeClassField(at);
	if(cf)
		if (addCodeClassField(cf) && syncToParentIfAdded) 
			updateContent();
}

/**
 * Remove a CodeClassField object from m_classfieldVector List
 */
bool ClassifierCodeDocument::removeCodeClassField ( CodeClassField * remove_object ) {
	UMLObject * umlobj = remove_object->getParentObject();
        if(m_classFieldMap->contains(umlobj))
	{
		if (m_classfieldVector.removeRef(remove_object)) 
		{
			// remove from our classfield map
        		m_classFieldMap->remove(umlobj);

			// Now remove all of the methods and declaration block it 'owns'
			removeTextBlock(remove_object->getDeclarationCodeBlock());

			QPtrList<CodeAccessorMethod> * methods = remove_object->getMethodList();
			for (CodeAccessorMethod * m = methods->first(); m ; m = methods->next())
				removeTextBlock(m);

			return true;
		} 
	}
	return false;
}

void ClassifierCodeDocument::removeAttributeClassField(UMLObject *obj) 
{
        CodeClassField * remove_object = (*m_classFieldMap)[obj];
        if(remove_object)
                removeCodeClassField(remove_object);
}

void ClassifierCodeDocument::removeAssociationClassField (UMLAssociation *assoc )
{

	// the object could be either role a or b. We should check 
	// both parts of the association.
        CodeClassField * remove_object = (*m_classFieldMap)[assoc->getUMLRoleA()];
        if(remove_object)
                removeCodeClassField(remove_object);
	else { // check role b
        	remove_object = (*m_classFieldMap)[assoc->getUMLRoleB()];
        	if(remove_object)
                	removeCodeClassField(remove_object);
	}

}

/**
 * Get the list of CodeClassField objects held by m_classfieldVector
 * @return QPtrList<CodeClassField> list of CodeClassField objects held by
 * m_classfieldVector
 */
QPtrList<CodeClassField> * ClassifierCodeDocument::getCodeClassFieldList ( ) {
	return &m_classfieldVector;
}

/**
 * Get the value of m_parentclassifier
 * @return the value of m_parentclassifier
 */
UMLClassifier * ClassifierCodeDocument::getParentClassifier ( ) {
	return m_parentclassifier;
}

/**
 * @return      QPtrList<CodeOperation>
 */
QPtrList<CodeOperation> ClassifierCodeDocument::getCodeOperations ( ) {

	QPtrList<CodeOperation> list;
	list.setAutoDelete(false);

	QPtrList<TextBlock> * tlist = getTextBlockList();
	for (TextBlock *tb = tlist->first(); tb; tb=tlist->next())
	{
		CodeOperation * cop = dynamic_cast<CodeOperation*>(tb);
		if(cop)
			list.append(cop);
	}
	return list;
}

/**
 * @param       op
 */
void ClassifierCodeDocument::addOperation (UMLObject * op ) {
	QString tag = CodeOperation::findTag((UMLOperation*)op);
	if(!findTextBlockByTag(tag, true)) 
	{
		CodeOperation *opblock = getParentGenerator()->newCodeOperation(this, (UMLOperation*)op);
		if(!addCodeOperation(opblock)) // wont add if already present
			delete opblock; // delete unused operations
	}
}

/**
 * @param       op
 */
void ClassifierCodeDocument::removeOperation (UMLObject * op ) {
// FIX
kdWarning()<<"REMOVE OPERATION CALLED for op:"<<op<<endl;
}

// Other methods
//  

void ClassifierCodeDocument::addCodeClassFieldMethods(QPtrList<CodeClassField> &list )
{

        for (CodeClassField * field = list.first(); field ; field = list.next())
        {
                QPtrList <CodeAccessorMethod> * list = field->getMethodList();
                for (CodeAccessorMethod * method = list->first(); method; method = list->next())
                {
                        QString tag = method->getTag();
                        if(tag.isEmpty())
			{
				tag = getUniqueTag();
				method->setTag(tag);
			}

                        addTextBlock(method); // wont add if already exists in document;

                }

        }
 
}

bool ClassifierCodeDocument::parentIsInterface () {
	UMLClassifier * c = getParentClassifier();
	return dynamic_cast<UMLInterface*>(c) ? true : false; 
}

/**
 * @return	CodeClassField
 * @param	attribute attribute which is parent of this class field.
 */
CodeClassField * ClassifierCodeDocument::newCodeClassField ( UMLAttribute * attribute ) {
	return getParentGenerator()->newCodeClassField(this,attribute);
}

/**
 * @return	CodeClassField
 * @param	role 
 */
CodeClassField * ClassifierCodeDocument::newCodeClassField (UMLRole * role ) {
	return getParentGenerator()->newCodeClassField(this,role);
}

/**
 * Init from a UMLClassifier object.
 * @param	classifier 
 * @param	package 
 */
void ClassifierCodeDocument::init (UMLClassifier * c ) 
{

kdWarning()<<" **> Start INIT Classifier code doc"<<endl;

  	m_parentclassifier = c;
	m_classfieldVector.setAutoDelete(true);
        m_classFieldMap = new QMap<UMLObject *, CodeClassField*>;

	updateHeader(); 
	syncNamesToParent();
	initCodeClassFields(); 

	// slots
	connect(c,SIGNAL(attributeAdded(UMLObject*)),this,SLOT(addAttributeClassField(UMLObject*)));
	connect(c,SIGNAL(attributeRemoved(UMLObject*)),this,SLOT(removeAttributeClassField(UMLObject*)));
	connect(c,SIGNAL(sigAssociationAdded(UMLAssociation*)),this,SLOT(addAssociationClassField(UMLAssociation*)));
	connect(c,SIGNAL(sigAssociationRemoved(UMLAssociation*)),this,SLOT(removeAssociationClassField(UMLAssociation*)));
	connect(c,SIGNAL(operationAdded(UMLObject*)),this,SLOT(addOperation(UMLObject*)));
	connect(c,SIGNAL(operationRemoved(UMLObject*)),this,SLOT(removeOperation(UMLObject*)));

	connect(c,SIGNAL(modified()),this,SLOT(syncToParent()));

kdWarning()<<" **> END INIT Classifier code doc"<<endl;
}

// IF the classifier object is modified, this will get called. 
// Possible mods include changing the filename and package
// the classifier has. 
void ClassifierCodeDocument::syncNamesToParent( ) {

 	setFileName(m_parentclassifier->getName());
	setPackage(m_parentclassifier->getPackage());
}

void ClassifierCodeDocument::syncronize( ) {

kdWarning()<<" Syncronize classifier code doc:"<<this<<endl;

	updateHeader(); // doing this insures time/date stamp is at the time of this call
	syncNamesToParent(); 
	updateContent();
	updateOperations();

}

void ClassifierCodeDocument::updateOperations( ) {

        QPtrList<UMLOperation> *opList = getParentClassifier()->getFilteredOperationsList();
	for (UMLOperation *op = opList->first(); op; op = opList->next())
        {
		QString tag = CodeOperation::findTag(op);
        	if(!findTextBlockByTag(tag, true))
		{
			CodeOperation * codeOp = getParentGenerator()->newCodeOperation(this, op);
			if(!addCodeOperation(codeOp)) //wont add IF already present
				delete codeOp; // delete unused operations
		}
        }
}

void ClassifierCodeDocument::syncToParent( ) {
	syncronize();
}

/**
 * add codeclassfields to this classifiercodedocument. IF a codeclassfield
 * already exists, it is not added.
 */
void ClassifierCodeDocument::initCodeClassFields ( ) {

kdWarning()<<" INIT CODE CLASSFIELDS IN CLASSIFIER DOC"<<endl;
	UMLClassifier * c = getParentClassifier();
	// first, do the code classifields that arise from attributes
        if (!parentIsInterface()) {
                UMLClass * mclass = dynamic_cast<UMLClass*>(c);
                QPtrList<UMLAttribute>* alist = mclass->getFilteredAttributeList();
                for(UMLAttribute * at = alist->first(); at; at = alist->next())
                {
                        CodeClassField * field = newCodeClassField(at);
                        addCodeClassField(field);
                }

        }

	// now, do the code classifields that arise from associations
	QPtrList<UMLAssociation> ap = c->getSpecificAssocs(Uml::at_Association);
	QPtrList<UMLAssociation> ag = c->getAggregations();
	QPtrList<UMLAssociation> ac = c->getCompositions();

	updateAssociationClassFields(ap);
	updateAssociationClassFields(ag);
	updateAssociationClassFields(ac);

kdWarning()<<" FINISH - INIT CODE CLASSFIELDS IN CLASSIFIER DOC"<<endl;

}

void ClassifierCodeDocument::updateAssociationClassFields ( QPtrList<UMLAssociation> &assocList )
{
        QPtrList<CodeClassField> list;
        for(UMLAssociation * a=assocList.first(); a; a=assocList.next())
		addAssociationClassField(a, false); // syncToParent later 
}

void ClassifierCodeDocument::addAssociationClassField (UMLAssociation * a, bool syncToParentIfAdded) 
{

kdWarning()<<"ADD ASSOCIATION CLASSFIELD CALLED FOR :"<<a<<endl;
	int cid = getParentClassifier()->getID(); // so we know who 'we' are 
	bool printRoleA = false, printRoleB = false;
	// it may seem counter intuitive, but you want to insert the role of the
	// *other* class into *this* class.
	if (a->getRoleAId() == cid)
		printRoleB = true;

	if (a->getRoleBId() == cid)
		printRoleA = true;

	// grab RoleB decl
	if (printRoleB)
	{

		UMLRole * role = a->getUMLRoleB();
        	if(!(m_classFieldMap->contains((UMLObject*)role)))
		{
                	CodeClassField * classfield = newCodeClassField(role);
			if( addCodeClassField(classfield) && syncToParentIfAdded)
				syncToParent(); // needed for a slot add 
		}
	}

	// print RoleA decl
	if (printRoleA)
	{
		UMLRole * role = a->getUMLRoleA();
		if(!(m_classFieldMap->contains((UMLObject*)role)))
		{
			CodeClassField * classfield = newCodeClassField(role);
			if( addCodeClassField(classfield) && syncToParentIfAdded)
				syncToParent(); // needed for a slot add 
		}
	}

}

/** set the class attributes of this object from
 * the passed element node.
 */
void ClassifierCodeDocument::setAttributesFromNode ( QDomElement & elem ) 
{

	// call super-class
	CodeDocument::setAttributesFromNode(elem);

        // set attributes
	int parent_id = elem.attribute("parent_class","-1").toInt();
kdWarning()<<"setAttributeFromNode in classifiercodedoc got parent class id:"<<parent_id<<endl;
// FIX

}

/** set attributes of the node that represents this class
 * in the XMI document.
 */
void ClassifierCodeDocument::setAttributesOnNode ( QDomDocument & doc, QDomElement & blockElement)
{

	// do super-class first
	CodeDocument::setAttributesOnNode(doc, blockElement);

	// cache parent class id
        blockElement.setAttribute("parent_class",QString::number(getParentClassifier()->getID()));

}


#include "classifiercodedocument.moc"
