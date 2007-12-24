/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2004-2007                                               *
 *   Umbrello UML Modeller Authors <uml-devel@uml.sf.net>                  *
 ***************************************************************************/

/*  This code generated by:
 *      Author : thomas
 *      Date   : Thu Jun 19 2003
 */

// own header
#include "codegenerator.h"

// system includes
#include <cstdlib> //to get the user name

// qt includes
#include <QtCore/QDateTime>
#include <QtCore/QRegExp>
#include <QtCore/QDir>
#include <QtCore/QTextStream>

// kde includes
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdialog.h>
#include <kapplication.h>

// app includes
#include "dialogs/overwritedialogue.h"
#include "dialogs/codeviewerdialog.h"
#include "codegenerators/simplecodegenerator.h"
#include "attribute.h"
#include "association.h"
#include "classifier.h"
#include "classifiercodedocument.h"
#include "codedocument.h"
#include "codegenerationpolicy.h"
#include "operation.h"
#include "uml.h"
#include "umldoc.h"
#include "umlobject.h"
#include "umlattributelist.h"
#include "umloperationlist.h"
#include "model_utils.h"


CodeGenerator::CodeGenerator ()
        : QObject (UMLApp::app()->getDocument())
{
    initFields();
}

// FIX
// hmm. this should be pure virtual so that implemented in sub-class
CodeGenerator::CodeGenerator (QDomElement & element )
        : QObject (UMLApp::app()->getDocument())
{
    initFields();
    loadFromXMI(element); // hmm. cant call this here.. its 'pure' virtual
}

CodeGenerator::~CodeGenerator ()
{
    // destroy all owned codedocuments
    qDeleteAll(m_codedocumentVector);
    m_codedocumentVector.clear();
}

QString CodeGenerator::getUniqueID(CodeDocument * codeDoc)
{
    QString id = codeDoc->getID();

    // does this document already exist? then just return its present id
    if (!id.isEmpty() && findCodeDocumentByID(id))
        return id;

    // approach now differs by whether or not its a classifier code document
    ClassifierCodeDocument * classDoc = dynamic_cast<ClassifierCodeDocument*>(codeDoc);
    if (classDoc) {
        UMLClassifier *c = classDoc->getParentClassifier();
        id = ID2STR(c->getID()); // this is supposed to be unique already..
    } else {

        QString prefix = "doc";
        QString id = prefix + "_0";
        int number = lastIDIndex;
        for ( ; findCodeDocumentByID(id); number++) {
            id = prefix + '_' + QString::number(number);
        }
        lastIDIndex = number;
    }

    return id;
}

CodeDocument * CodeGenerator::findCodeDocumentByID( const QString &tag )
{
    CodeDocument* doc = m_codeDocumentDictionary.value(tag);
    if (doc)
        return doc;
    else
        return NULL;
}

bool CodeGenerator::addCodeDocument ( CodeDocument * doc )
{
    QString tag = doc->getID();

    // assign a tag if one doesn't already exist
    if (tag.isEmpty())
    {
        tag = getUniqueID(doc);
        doc->setID(tag);
    }

    if (m_codeDocumentDictionary.contains(tag))
        return false; // return false, we already have some object with this tag in the list
    else
        m_codeDocumentDictionary.insert(tag, doc);

    m_codedocumentVector.append(doc);
    return true;
}

bool CodeGenerator::removeCodeDocument ( CodeDocument * remove_object )
{
    QString tag = remove_object->getID();
    if (!(tag.isEmpty()))
        m_codeDocumentDictionary.remove(tag);
    else
        return false;

    m_codedocumentVector.removeAll(remove_object);
    return true;
}

CodeDocumentList * CodeGenerator::getCodeDocumentList ()
{
    return &m_codedocumentVector;
}

// the vanilla version
CodeViewerDialog * CodeGenerator::getCodeViewerDialog ( QWidget* parent, CodeDocument *doc,
        Settings::CodeViewerState state)
{
    return new CodeViewerDialog(parent, doc, state);
}

void CodeGenerator::loadFromXMI (QDomElement & qElement )
{
    // look for our particular child element
    QDomNode node = qElement.firstChild();
    QDomElement element = node.toElement();
    QString langType = Model_Utils::progLangToString( getLanguage() );

    if (qElement.tagName() != "codegenerator"
            || qElement.attribute("language", "UNKNOWN") != langType)
        return;
    // got our code generator element, now load
    // codedocuments
    QDomNode codeDocNode = qElement.firstChild();
    QDomElement codeDocElement = codeDocNode.toElement();
    while (!codeDocElement.isNull())
    {
        QString docTag = codeDocElement.tagName();
        QString id = codeDocElement.attribute( "id", "-1" );
        if (docTag == "sourcecode")
        {
            loadCodeForOperation(id, codeDocElement);
        }
        else if (docTag == "codedocument" || docTag == "classifiercodedocument")
        {
            CodeDocument * codeDoc = findCodeDocumentByID(id);
            if (codeDoc) {
                codeDoc->loadFromXMI(codeDocElement);
            }
            else {
                uWarning() << "missing code document for id:" << id;
            }
        } else {
            uWarning() << "got strange codegenerator child node:" << docTag << ", ignoring.";
        }
        codeDocNode = codeDocElement.nextSibling();
        codeDocElement = codeDocNode.toElement();
    }
}

// probably we have code which was entered in classpropdlg for an operation
void CodeGenerator::loadCodeForOperation(const QString& idStr, QDomElement codeDocElement)
{
    Uml::IDType id = STR2ID(idStr);
    UMLObject *obj = m_document->findObjectById(id);
    if (obj)
    {
        uDebug() << "found UMLObject for id:" << idStr;
        QString value = codeDocElement.attribute("value", "");

        Uml::Object_Type t = obj->getBaseType();
        if (t == Uml::ot_Operation)
        {
            UMLOperation *op = static_cast<UMLOperation*>(obj);
            op->setSourceCode(value);
        }
        else
            uError() << "sourcecode id " << idStr << " has unexpected type " << t;
    }
    else
    {
        uError() << "unknown sourcecode id " << idStr;
    }
}

void CodeGenerator::saveToXMI ( QDomDocument & doc, QDomElement & root )
{
    QString langType = Model_Utils::progLangToString( getLanguage() );
    QDomElement docElement = doc.createElement( "codegenerator" );
    docElement.setAttribute("language",langType);

    if (dynamic_cast<SimpleCodeGenerator*>(this))
    {
        UMLClassifierList concepts = m_document->getClassesAndInterfaces();
        foreach (UMLClassifier *c, concepts)
        {
            UMLOperationList operations = c->getOpList();
            foreach (UMLOperation *op, operations)
            {
                // save the source code
                QString code = op->getSourceCode();
                if (code.isEmpty())
                    continue;
                QDomElement codeElement = doc.createElement("sourcecode");
                codeElement.setAttribute("id", ID2STR(op->getID()));
                codeElement.setAttribute("value", code);
                docElement.appendChild( codeElement );
            }
        }
    }
    else
    {
        CodeDocumentList * docList = getCodeDocumentList();
        CodeDocumentList::iterator it = docList->begin();
        CodeDocumentList::iterator end = docList->end();
        for ( ; it != end; ++it ) {
            (*it)->saveToXMI(doc, docElement);
        }
    }
    root.appendChild( docElement );
}

void CodeGenerator::initFromParentDocument()
{
    // Walk through the document converting classifiers into
    // classifier code documents as needed (e.g only if doesn't exist)
    UMLClassifierList concepts = m_document->getClassesAndInterfaces();
    foreach (UMLClassifier *c , concepts)
    {
        // Doesn't exist? Then build one.
        CodeDocument * codeDoc = findCodeDocumentByClassifier(c);
        if (!codeDoc)
        {
            codeDoc = newClassifierCodeDocument(c);
            addCodeDocument(codeDoc); // this will also add a unique tag to the code document
        }
    }
}

void CodeGenerator::syncCodeToDocument ()
{
    CodeDocumentList::iterator it = m_codedocumentVector.begin();
    CodeDocumentList::iterator end = m_codedocumentVector.end();
    for ( ; it != end; ++it )
        (*it)->synchronize();
}

// in this 'vanilla' version, we only worry about adding classifier
// documents
void CodeGenerator::checkAddUMLObject (UMLObject * obj)
{
    if (!obj)
        return;

    UMLClassifier * c = dynamic_cast<UMLClassifier*>(obj);
    if (c) {
        CodeDocument * cDoc = newClassifierCodeDocument(c);
        addCodeDocument(cDoc);
    }
}

void CodeGenerator::checkRemoveUMLObject (UMLObject * obj)
{
    if (!obj)
        return;

    UMLClassifier * c = dynamic_cast<UMLClassifier*>(obj);
    if (c) {
        ClassifierCodeDocument * cDoc = (ClassifierCodeDocument*) findCodeDocumentByClassifier(c);
        if (cDoc)
            removeCodeDocument(cDoc);
    }
}

CodeDocument * CodeGenerator::findCodeDocumentByClassifier ( UMLClassifier * classifier )
{
    return findCodeDocumentByID(ID2STR(classifier->getID()));
}

void CodeGenerator::writeCodeToFile ( )
{
    writeListedCodeDocsToFile(&m_codedocumentVector);
}

void CodeGenerator::writeCodeToFile ( UMLClassifierList & concepts)
{
    CodeDocumentList docs;

    foreach (UMLClassifier *concept, concepts ) {
        CodeDocument * doc = findCodeDocumentByClassifier(concept);
        if (doc)
            docs.append(doc);
    }

    writeListedCodeDocsToFile(&docs);
}

// Main method. Will write out passed code documents to file as appropriate.
void CodeGenerator::writeListedCodeDocsToFile ( CodeDocumentList * docs )
{
    // iterate thru all code documents
    CodeDocumentList::iterator it = docs->begin();
    CodeDocumentList::iterator end = docs->end();
    for ( ; it != end; ++it )
    {
        // we need this so we know when to emit a 'codeGenerated' signal
        ClassifierCodeDocument * cdoc = dynamic_cast<ClassifierCodeDocument *>(*it);
        bool codeGenSuccess = false;

        // we only write the document, if so requested
        if ((*it)->getWriteOutCode())
        {
            QString filename = findFileName(*it);
            // check that we may open that file for writing
            QFile file;
            if ( openFile(file,filename) ) {
                QTextStream stream(&file);
                stream << (*it)->toString() << endl;
                file.close();
                codeGenSuccess = true; // we wrote the code OK
            } else {
                uWarning() << "Cannot open file :"<<filename<<" for writing ";
            }
        }

        if (cdoc)
            emit codeGenerated(cdoc->getParentClassifier(), codeGenSuccess);
    }
}

CodeDocument * CodeGenerator::newCodeDocument ()
{
    CodeDocument * newCodeDoc = new CodeDocument ();
    return newCodeDoc;
}

QString CodeGenerator::getHeadingFile( const QString &file )
{
    return UMLApp::app()->getCommonPolicy()->getHeadingFile(file);
}

QString CodeGenerator::overwritableName(const QString& name, const QString &extension )
{
    CodeGenerationPolicy *pol = UMLApp::app()->getCommonPolicy();
    QDir outputDirectory = pol->getOutputDirectory();
    QString filename = name + extension;

    if (!outputDirectory.exists(filename)) {
        return filename;
    }

    int suffix;
    OverwriteDialogue overwriteDialog( name, outputDirectory.absolutePath(),
                                         m_applyToAllRemaining, kapp->activeWindow() );
    switch (pol->getOverwritePolicy()) {  //if it exists, check the OverwritePolicy we should use
    case CodeGenerationPolicy::Ok:              //ok to overwrite file
        filename = name + extension;
        break;
    case CodeGenerationPolicy::Ask:            //ask if we can overwrite
        switch(overwriteDialog.exec()) {
        case KDialog::Yes:  //overwrite file
            if ( overwriteDialog.applyToAllRemaining() ) {
                pol->setOverwritePolicy(CodeGenerationPolicy::Ok);
                filename = name + extension;
            } else {
                m_applyToAllRemaining = false;
            }
            break;
        case KDialog::No: //generate similar name
            suffix = 1;
            while (1) {
                filename = name + "__" + QString::number(suffix) + extension;
                if (!outputDirectory.exists(filename))
                    break;
                suffix++;
            }
            if ( overwriteDialog.applyToAllRemaining() ) {
                pol->setOverwritePolicy(CodeGenerationPolicy::Never);
            } else {
                m_applyToAllRemaining = false;
            }
            break;
        case KDialog::Cancel: //don't output anything
            if ( overwriteDialog.applyToAllRemaining() ) {
                pol->setOverwritePolicy(CodeGenerationPolicy::Cancel);
            } else {
                m_applyToAllRemaining = false;
            }
            return QString();
            break;
        }

        break;
    case CodeGenerationPolicy::Never: //generate similar name
        suffix = 1;
        while (1) {
            filename = name + "__" + QString::number(suffix) + extension;
            if (!outputDirectory.exists(filename))
                break;
            suffix++;
        }
        break;
    case CodeGenerationPolicy::Cancel: //don't output anything
        return QString();
        break;
    }

    return filename;
}

bool CodeGenerator::openFile (QFile & file, const QString &fileName )
{
    //open files for writing.
    if (fileName.isEmpty()) {
        uWarning() << "cannot find a file name";
        return false;
    } else {
        QDir outputDirectory = UMLApp::app()->getCommonPolicy()->getOutputDirectory();
        file.setFileName(outputDirectory.absoluteFilePath(fileName));
        if(!file.open(QIODevice::WriteOnly)) {
            KMessageBox::sorry(0,i18n("Cannot open file %1 for writing. Please make sure the folder exists and you have permissions to write to it.", file.fileName()),i18n("Cannot Open File"));
            return false;
        }
        return true;
    }
}

QString CodeGenerator::cleanName ( const QString &name )
{
    QString retval = name;
    retval.replace(QRegExp("\\W+"), "_");
    return retval;
}

QString CodeGenerator::findFileName ( CodeDocument * codeDocument )
{
    // Get the path name
    QString path = codeDocument->getPath();

    // if path is given add this as a directory to the file name
    QString name;
    if (!path.isEmpty()) {
        path.replace(QRegExp("::"), "/"); // Simple hack!
        name = path + '/' + codeDocument->getFileName();
        path = '/' + path;
    } else {  // determine the "natural" file name
        name = codeDocument->getFileName();
    }

    // Convert all "::" to "/" : Platform-specific path separator
    name.replace(QRegExp("::"), "/"); // Simple hack!

    // if a path name exists check the existence of the path directory
    if (!path.isEmpty()) {
        QDir outputDirectory = UMLApp::app()->getCommonPolicy()->getOutputDirectory();
        QDir pathDir(outputDirectory.absolutePath() + path);

        // does our complete output directory exist yet? if not, try to create it
        if (!pathDir.exists())
        {
            // ugh. dir separator here is UNIX specific..
            QStringList dirs = pathDir.absolutePath().split("/");
            QString currentDir;

            QStringList::iterator end(dirs.end());
            for (QStringList::iterator dir(dirs.begin()); dir != end; ++dir)
            {
                currentDir += '/' + *dir;
                if (! (pathDir.exists(currentDir)
                        || pathDir.mkdir(currentDir) ) )
                {
                    KMessageBox::error(0, i18n("Cannot create the folder:\n") +
                                       pathDir.absolutePath() + i18n("\nPlease check the access rights"),
                                       i18n("Cannot Create Folder"));
                    return NULL;
                }
            }
        }
    }

    name.simplified();
    name.replace(QRegExp(" "),"_");

    return overwritableName( name, codeDocument->getFileExtension() );
}

void CodeGenerator::findObjectsRelated(UMLClassifier *c, UMLPackageList &cList)
{
    UMLPackage *temp;
    UMLAssociationList associations = c->getAssociations();

    foreach (UMLAssociation *a , associations ) {
        temp = 0;
        switch (a->getAssocType()) {
        case Uml::at_Generalization:
        case Uml::at_Realization:
            // only the "b" end is seen by the "a" end, not other way around
            {
                UMLObject *objB = a->getObject(Uml::B);
                if (objB != c)
                    temp = (UMLPackage*)objB;
            }
            break;
        case Uml::at_Dependency:
        case Uml::at_UniAssociation:
            {
                UMLObject *objA = a->getObject(Uml::A);
                UMLObject *objB = a->getObject(Uml::B);
                if (objA == c)
                    temp = static_cast<UMLPackage*>(objB);
            }
            break;
        case Uml::at_Aggregation:
        case Uml::at_Composition:
        case Uml::at_Association:
            {
                UMLObject *objA = a->getObject(Uml::A);
                UMLObject *objB = a->getObject(Uml::B);
                if (objA == c && objB->getBaseType() != Uml::ot_Datatype)
                    temp = static_cast<UMLPackage*>(objB);
            }
            break;
        default: /* all others.. like for state diagrams..we currently don't use */
            break;
        }

        // now add in list ONLY if its not already there
        if(temp  && !cList.count(temp))
            cList.append(temp);
    }

    //operations
    UMLOperationList opl(c->getOpList());
    foreach(UMLOperation *op , opl ) {
        temp =0;
        //check return value
        temp =(UMLClassifier*) op->getType();
        if (temp && temp->getBaseType() != Uml::ot_Datatype && !cList.count(temp) )
            cList.append(temp);
        //check parameters
        UMLAttributeList atl = op->getParmList();
        foreach (UMLAttribute *at , atl ) {
            temp = (UMLClassifier*)at->getType();
            if (temp && temp->getBaseType() != Uml::ot_Datatype && !cList.count(temp) )
                cList.append(temp);
        }
    }

    //attributes
    if (!c->isInterface()) {
        UMLAttributeList atl = c->getAttributeList();
        foreach (UMLAttribute *at , atl ) {
            temp=0;
            temp = (UMLClassifier*) at->getType();
            if (temp && temp->getBaseType() != Uml::ot_Datatype && !cList.count(temp) )
                cList.append(temp);
        }
    }
}

QString CodeGenerator::formatDoc(const QString &text, const QString &linePrefix, int lineWidth)
{
    const QString endLine = UMLApp::app()->getCommonPolicy()->getNewLineEndingChars();
    QString output = QString();
    QStringList lines = text.split(endLine);
    for (QStringList::ConstIterator lit = lines.begin(); lit != lines.end(); ++lit) {
        QString input = *lit;
        input.remove( QRegExp("\\s+$") );
        if (input.length() < lineWidth) {
            output += linePrefix + input + endLine;
            continue;
        }
        int index;
        while ((index = input.lastIndexOf(" ", lineWidth)) >= 0) {
            output += linePrefix + input.left(index) + endLine; // add line
            input.remove(0, index + 1); // remove processed string, including white space
        }
        if (!input.isEmpty())
            output += linePrefix + input + endLine;
    }
    return output;
}

QString CodeGenerator::formatSourceCode(const QString& code, const QString& indentation)
{
    const QString endLine = UMLApp::app()->getCommonPolicy()->getNewLineEndingChars();
    QString output = QString();
    if (! code.isEmpty()) {
        QStringList lines = code.split(endLine);
        for (int i = 0; i < lines.size(); ++i) {
            output += indentation + lines.at(i) + endLine;
        }
    }
    return output;
}

void CodeGenerator::initFields()
{
    m_document = UMLApp::app()->getDocument();
    m_applyToAllRemaining = true;
    lastIDIndex = 0;

    // initial population of our project generator
    // CANT Be done here because we would call pure virtual method
    // of newClassifierDocument (bad!).
    // We should only call from the child
    // initFromParentDocument();
}

void CodeGenerator::connect_newcodegen_slots()
{
    connect(m_document, SIGNAL(sigObjectCreated(UMLObject*)),
            this, SLOT(checkAddUMLObject(UMLObject*)));
    connect(m_document, SIGNAL(sigObjectRemoved(UMLObject*)),
            this, SLOT(checkRemoveUMLObject(UMLObject*)));
    CodeGenerationPolicy *commonPolicy = UMLApp::app()->getCommonPolicy();
    connect(commonPolicy, SIGNAL(modifiedCodeContent()),
            this, SLOT(syncCodeToDocument()));
}

// these are utility methods for accessing the default
// code gen policy object and should go away when we
// finally implement the CodeGenDialog class -b.t.

void CodeGenerator::setForceDoc(bool f)
{
    UMLApp::app()->getCommonPolicy()->setCodeVerboseDocumentComments(f);
}

bool CodeGenerator::forceDoc() const
{
    return UMLApp::app()->getCommonPolicy()->getCodeVerboseDocumentComments();
}

void CodeGenerator::setForceSections(bool f)
{
    UMLApp::app()->getCommonPolicy()->setCodeVerboseSectionComments(f);
}

bool CodeGenerator::forceSections() const
{
    return UMLApp::app()->getCommonPolicy()->getCodeVerboseSectionComments();
}

QStringList CodeGenerator::defaultDatatypes()
{
    return QStringList();
    //empty by default, override in your code generator
}

bool CodeGenerator::isReservedKeyword(const QString & keyword)
{
    const QStringList keywords = reservedKeywords();
    return keywords.contains(keyword);
}

const QStringList CodeGenerator::reservedKeywords() const
{
    static QStringList emptyList;
    return emptyList;
}

void CodeGenerator::createDefaultStereotypes()
{
    //empty by default, override in your code generator
    //e.g.  m_document->createDefaultStereotypes("constructor");
}

#include "codegenerator.moc"
