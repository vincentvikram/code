/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2003      Brian Thomas <thomas@mail630.gsfc.nasa.gov>   *
 *   copyright (C) 2004-2008                                               *
 *   Umbrello UML Modeller Authors <uml-devel@uml.sf.net>                  *
 ***************************************************************************/

#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QDir>

#include "codegenerators/codegenpolicyext.h"
#include "codegenerationpolicy.h"
#include "umlpackagelist.h"
#include "umlclassifierlist.h"
#include "codedocumentlist.h"
#include "codeviewerstate.h"
#include "umlnamespace.h"

class UMLDoc;
class UMLObject;
class ClassifierCodeDocument;
class CodeBlock;
class CodeDocument;
class CodeViewerDialog;
class QDomDocument;
class QDomElement;

/**
 * This class collects together all of the code documents which form this project,
 * and generates code for them in a given language.
 *
 * CodeGenerator is the base class for all CodeGenerators. It
 * provides the interface through which all Generators are invoked and
 * the all the basic functionality. The only thing it doesn't do is to
 * generate code =)
 *
 * If you want to implement a CodeGenerator for some language follow
 * these steps:
 *
 * Create a class which inherits CodeGenerator. This class can have
 * any name, I use names like CppCodeGenerator for the Cpp Generator,
 * JavaCodeGenerator  for the Java Generator and so on, but you can use what
 * you want.
 *
 * The code you generate should be output to "outputDirectory" and you
 * should respect the OverwritePolicy specified. You should call
 * findFileName(..) to get an appropriate file name, and then you can
 * call openFile if you want, but if you want to do it yourself you
 *
 * Finally put your generator in a library which can be dlopened
 * together with a factory class (see below) and you are ready to go.
 *
 * Note for "simple" code generators:
 * Code can be entered into a QTextEdit widget in the ClassPropDlg. This
 * code is then stored in the respective UMLOperation, written to the
 * xmi file, and also used when generating the source files.
 * The code fragments are stored into the xmi file in the section "codegeneration"
 * with the tag "sourcecode".
 */
class CodeGenerator : public QObject
{
    Q_OBJECT

public:

    /**
     * Build a code generator.
     * @return      CodeGenerator
     */
    CodeGenerator ();

    /**
     * Build a code generator and then initialize it from an XMI element.
     * @return      CodeGenerator
     * @param       element an element from an XMI document
     */
    CodeGenerator (QDomElement & element );

    /**
     * Empty Destructor.
     */
    virtual ~CodeGenerator ( );

    /**
     * Add a CodeDocument object to the m_codedocumentVector List.
     * @return boolean - will return false if it couldnt add a document
     */
    bool addCodeDocument ( CodeDocument * add_object );

//    /**
//     * Replace (or possibly add a new) CodeDocument object to the m_codedocumentVector List.
//     * As names must be unique and each code document must have a name.
//     * @return  boolean value which will be true if the passed document was able to replace some
//     *    other document OR was added(no prior document existed..only when addIfPriorDocumentNotPresent is true).
//     *    The document which was replaced will be deleted IF deleteReplacedDocument is true.
//     */
//    bool replaceCodeDocument (  CodeDocument * replace_doc=0, bool addIfPriorDocumentNotPresent=true,
//                                bool deleteReplacedDocument=true );

    /**
     * Remove a CodeDocument object from m_codedocumentVector List.
     * @return boolean - will return false if it couldnt remove a document
     */
    bool removeCodeDocument ( CodeDocument * remove_object );

    /**
     * Get the list of CodeDocument objects held by m_codedocumentVector.
     * @return CodeDocumentList list of CodeDocument objects held by
     * m_codedocumentVector
     */
    CodeDocumentList * getCodeDocumentList ( );

    /**
     * Get a unique id for this codedocument.
     * @return   id for the codedocument
     */
    QString getUniqueID ( CodeDocument * codeDoc );

    /**
     * Save the XMI representation of this object
     */
    virtual void saveToXMI ( QDomDocument & doc, QDomElement & root );

    /**
     * Find a code document by the given id string.
     * @return  CodeDocument
     */
    CodeDocument * findCodeDocumentByID (const QString &id );

    /**
     * Write out all code documents to file as appropriate.
     */
    virtual void writeCodeToFile ( );

    /**
     * This method is here to provide class wizard the
     * ability to write out only those classes which
     * are selected by the user.
     */
    virtual void writeCodeToFile(UMLClassifierList &list);

    // these are utility methods for accessing the default
    // code gen policy object and *perhaps* should go away when we
    // finally implement the CodeGenDialog class -b.t.

    void setModifyNamePolicy(CodeGenerationPolicy::ModifyNamePolicy p);
    CodeGenerationPolicy::ModifyNamePolicy modifyNamePolicy()const;

    void setIncludeHeadings(bool i);
    bool includeHeadings() const;

    void setHeadingFileDir(const QString &);
    QString headingFileDir() const;

    void setForceDoc(bool f);
    bool forceDoc() const;

    void setForceSections(bool f);
    bool forceSections() const;

    /**
     *  Gets the heading file (as a string) to be inserted at the
     *  beginning of the generated file. you give the file type as
     *  parameter and get the string. if fileName starts with a
     *  period (.) then fileName is the extension (.cpp, .h,
     *  .java) if fileName starts with another character you are
     *  requesting a specific file (mylicensefile.txt).  The files
     *  can have parameters which are denoted by %parameter%.
     *
     *  current parameters are
     *  %author%
     *  %date%
     *  %time%
     *  %filepath%
     *
     * @return      QString
     * @param       file
     */
    virtual QString getHeadingFile (const QString &file );

    /**
     * Finds an appropriate file name for the given CodeDocument, taking into
     * account the Overwrite Policy and asking the user what to do if need be
     * (if policy == Ask).
     *
     * @param doc  the CodeDocument for which an output file name is desired.
     * @return the file name that should be used. (with extension) or
     *      NULL if none to be used
     */
    QString findFileName(CodeDocument * doc);

    /**
     * Replaces spaces with underscores and capitalises as defined in m_modname
     * @return      QString
     * @param       name
     */
    static QString cleanName ( const QString &name );

    /**
     * Format documentation for output in source files
     *
     * @param text         the documentation which has to be formatted
     * @param linePrefix   the prefix which has to be added in the beginnig of each line
     * @param lineWidth    the line width used for word-wrapping the documentation
     *
     * @return the formatted documentation text
     */
    static QString formatDoc (const QString & text, const QString & linePrefix = " *", int lineWidth = 80 );

    /**
     * Format source code for output in source files by
     * adding the correct indentation to every line of code.
     *
     * @param code          the source code block which has to be formatted
     * @param indentation   the blanks to indent
     */
    static QString formatSourceCode(const QString& code, const QString& indentation);

    /**
     * Finds all classes in the current document to which objects of class c
     * are in some way related. Possible relations are Associations (generalization,
     * composition, etc) as well as parameters to methods and return values
     * this is useful in deciding which classes/files to import/include in code generation
     * @param c the class for which relations are to be found
     * @param cList a reference to the list into which return the result
     */
    static void findObjectsRelated(UMLClassifier *c, UMLPackageList &cList);

    /**
     * A series of accessor method constructors that we need to define
     * for any particular language.
     */
    virtual CodeDocument * newClassifierCodeDocument (UMLClassifier * classifier ) = 0;

    /**
     * @param       element
     */
    virtual void loadFromXMI (QDomElement & element );

    /**
     * Create a new Code document belonging to this package.
     * @return CodeDocument   pointer to new code document.
     */
    virtual CodeDocument * newCodeDocument ( );

    /**
     * Return the unique language enum that identifies this type of code generator.
     */
    virtual Uml::Programming_Language getLanguage() = 0;

    /**
     * Find a code document by the given classifier.
     * @return      CodeDocument
     * @param       classifier
     */
    //FIX
    // NOTE: this should be 'protected' or we could have problems with CPP code generator
    CodeDocument * findCodeDocumentByClassifier (UMLClassifier * classifier );

    /**
     * Return the default datatypes for your language (bool, int etc).
     * Default implementation returns empty list.
     */
    virtual QStringList defaultDatatypes();

    /**
     * Get the editing dialog for this code document.
     */
    virtual CodeViewerDialog * getCodeViewerDialog( QWidget* parent, CodeDocument * doc,
            Settings::CodeViewerState state);

    /**
     * Check whether the given string is a reserved word for the
     * language of this code generator.
     *
     * @param rPossiblyReservedKeyword is the string to check
     *
     */
    virtual bool isReservedKeyword(const QString & rPossiblyReservedKeyword);

    /**
     * Get list of reserved keywords.
     */
    virtual const QStringList reservedKeywords() const;

    /**
     * Create the default stereotypes for your language (constructor, int etc).
     */
    virtual void createDefaultStereotypes ();

    /**
     * Initialize this code generator from its parent UMLDoc. When this is called,
     * it will (re-)generate the list of code documents for this project (generator)
     * by checking for new objects/attributes which have been added or changed in the
     * document. One or more CodeDocuments will be created/overwritten/amended as is
     * appropriate for the given language.
     * <p>
     * In this 'generic' version a ClassifierCodeDocument will exist for each and
     * every classifier that exists in our UMLDoc. IF when this is called, a code document
     * doesn't exist for the given classifier, then we will created and add a new code
     * document to our generator.
     * <p>
     * IF you want to add non-classifier related code documents at this step,
     * you will need to overload this method in the appropriate
     * code generatator (see JavaCodeGenerator for an example of this).
     */
    virtual void initFromParentDocument( );

    /**
     * Connect additional slots.
     * Only required for Advanced Code Generators.
     * To be called after constructing the code generator (see CodeGenFactory).
     */
    void connect_newcodegen_slots();

protected:

//    /**
//     * Remove (and possibly delete) all AutoGenerated content type CodeDocuments but leave
//     * the UserGenerated (and any other type) documents in this generator alone.
//     */
//    void removeAndDeleteAllAutoGeneratedCodeDocuments ( bool deleteRemovedDocs=true );

    /**
     * Returns a name that can be written to in the output directory,
     * respecting the overwrite policy.
     * If a file of the given name and extension does not exist,
     * then just returns the name.
     * If a file of the given name and extension does exist,
     * then opens an overwrite dialog. In this case the name returned
     * may be a modification of the input name.
     * This method is invoked by findFileName().
     *
     * @param name the proposed output file name
     * @param extension the extension to use
     * @return the real file name that should be used (including extension) or
     *      QString() if none to be used
     */
    QString overwritableName (const QString& name, const QString &extension );

    /**
     * Opens a file named "name" for writing in the outputDirectory.
     * If something goes wrong, it informs the user
     * if this function returns true, you know you can write to the file.
     * @return      bool
     * @param       file
     * @param       name
     */
    bool openFile (QFile& file, const QString &name);

    /**
     * The actual internal routine which writes code documents.
     */
    void writeListedCodeDocsToFile(CodeDocumentList * docs);

    static const char * hierarchicalCodeBlockNodeName;

    // map of what code documents we currently have in this generator.
    QHash<QString, CodeDocument*> m_codeDocumentDictionary;

    /**
     * Used by overwriteDialogue to know if the apply to all
     * remaining files checkbox should be checked (is by default).
     */
    bool m_applyToAllRemaining;

    /**
     * The document object.
     */
    UMLDoc* m_document;

private:

//    /**
//     * Maps CodeDocuments to filenames. Used for finding out which file
//     * each class was written to.
//     */
    // this seems silly and overkill now. -b.t.
    // QMap<CodeDocument*,QString> *m_fileMap;

    CodeDocumentList m_codedocumentVector;
    int lastIDIndex;

    void initFields() ;

    /**
     * Extract and load code for operations from xmi section.
     */
    void loadCodeForOperation(const QString& id, const QDomElement& codeDocElement);

public slots:

    /** These 2 functions check for adding or removing objects to the UMLDocument */
    virtual void checkAddUMLObject (UMLObject * obj);
    virtual void checkRemoveUMLObject (UMLObject * obj);

    /**
     * Force a synchronize of this code generator, and its present contents, to that of the parent UMLDocument.
     * "UserGenerated" code will be preserved, but Autogenerated contents will be updated/replaced
     * or removed as is apppropriate.
     */
    virtual void syncCodeToDocument ( );

signals:

    /**
     * This signal is emitted when code for a UMLClassifier has been
     * generated. Its only really used by the codegenerationwizard to
     * update its progress.
     * @param concept    The concept which was processed
     * @param generated  Flag, set to true if generation was successful
     */
    void codeGenerated(UMLClassifier* concept, bool generated);

};

#endif // CODEGENERATOR_H
