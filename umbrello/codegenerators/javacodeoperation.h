
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
 *      Date   : Mon Jun 30 2003
 */


#ifndef JAVACODEOPERATION_H
#define JAVACODEOPERATION_H

#include <qstring.h>
#include "../codeoperation.h"

class JavaClassifierCodeDocument;

class JavaCodeOperation : virtual public CodeOperation
{
	Q_OBJECT
public:

	// Constructors/Destructors
	//  

	/**
	 * Empty Constructor
	 */
	JavaCodeOperation ( JavaClassifierCodeDocument * doc, UMLOperation * op, const QString & body = "", const QString & comment = "");

	/**
	 * Empty Destructor
	 */
	virtual ~JavaCodeOperation ( );

        /**
         * Save the XMI representation of this object
         * @return      bool    status of save
         */
        virtual bool saveToXMI ( QDomDocument & doc, QDomElement & root );

        /**
         * load params from the appropriate XMI element node.
         */
        virtual void loadFromXMI ( QDomElement & root );

protected:

       /** set attributes of the node that represents this class
         * in the XMI document.
         */
        virtual void setAttributesOnNode ( QDomDocument & doc, QDomElement & blockElement);

        /** set the class attributes of this object from
         * the passed element node.
         */
        virtual void setAttributesFromNode ( QDomElement & element);

        void updateMethodDeclaration();

private:
	void init (JavaClassifierCodeDocument * doc );


};

#endif // JAVACODEOPERATION_H
