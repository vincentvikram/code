/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2002-2013                                               *
 *   Umbrello UML Modeller Authors <umbrello-devel@kde.org>                *
 ***************************************************************************/

// own header
#include "umlscene.h"

// application specific includes
#include "activitywidget.h"
#include "actorwidget.h"
#include "artifactwidget.h"
#include "association.h"
#include "associationwidget.h"
#include "assocrules.h"
#include "attribute.h"
#include "boxwidget.h"
#include "classifier.h"
#include "classifierwidget.h"
#include "classoptionspage.h"
#include "cmds.h"
#include "componentwidget.h"
#include "datatypewidget.h"
#include "debug_utils.h"
#include "docwindow.h"
#include "entity.h"
#include "entitywidget.h"
#include "enumwidget.h"
#include "floatingtextwidget.h"
#include "folder.h"
#include "foreignkeyconstraint.h"
#include "forkjoinwidget.h"
#include "idchangelog.h"
#include "layoutgenerator.h"
#include "layoutgrid.h"
#include "listpopupmenu.h"
#include "messagewidget.h"
#include "model_utils.h"
#include "notewidget.h"
#include "object_factory.h"
#include "objectnodewidget.h"
#include "objectwidget.h"
#include "package.h"
#include "packagewidget.h"
#include "pinwidget.h"
#include "seqlinewidget.h"
#include "signalwidget.h"
#include "statewidget.h"
#include "toolbarstatefactory.h"
#include "toolbarstate.h"
#include "uml.h"
#include "umldoc.h"
#include "umldragdata.h"
#include "umllistview.h"
#include "umllistviewitem.h"
#include "umlobject.h"
#include "umlobjectlist.h"
#include "umlrole.h"
#include "umlview.h"
#include "umlviewimageexporter.h"
#include "umlwidget.h"
#include "uniqueid.h"
#include "widget_factory.h"
#include "widget_utils.h"
#include "widgetlist_utils.h"

//kde include files
#include <ktemporaryfile.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kcursor.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <klocale.h>

// include files for Qt
#include <QColor>
#include <QPainter>
#include <QPixmap>
#include <QPrinter>
#include <QString>
#include <QStringList>

// system includes
#include <cmath>  // for ceil

// static members
const UMLSceneValue UMLScene::defaultCanvasSize = 5000;
bool UMLScene::m_showDocumentationIndicator = false;

using namespace Uml;

DEBUG_REGISTER(UMLScene)

/**
 * Constructor.
 */
UMLScene::UMLScene(UMLFolder *parentFolder, UMLView *view)
  : QGraphicsScene(0, 0, defaultCanvasSize, defaultCanvasSize),
    m_nLocalID(Uml::ID::None),
    m_nID(Uml::ID::None),
    m_Type(Uml::DiagramType::Undefined),
    m_Name(QString()),
    m_Documentation(QString()),
    m_Options(Settings::optionState()),
    m_bUseSnapToGrid(false),
    m_bUseSnapComponentSizeToGrid(false),
    m_isOpen(true),
    m_nCollaborationId(0),
    m_bCreateObject(false),
    m_bDrawSelectedOnly(false),
    m_bPaste(false),
    m_bStartedCut(false),
    m_view(view),
    m_pFolder(parentFolder),
    m_pIDChangesLog(0),
    m_isActivated(false),
    m_bPopupShowing(false)
{
    //m_AssociationList.setAutoDelete(true);
    //m_WidgetList.setAutoDelete(true);
    //m_MessageList.setAutoDelete(true);

    m_PastePoint = UMLScenePoint(0, 0);

    m_pImageExporter = new UMLViewImageExporter(this);

    // setup signals
    connect(UMLApp::app(), SIGNAL(sigCutSuccessful()),
            this, SLOT(slotCutSuccessful()));
    // Create the ToolBarState factory. This class is not a singleton, because it
    // needs a pointer to this object.
    m_pToolBarStateFactory = new ToolBarStateFactory();
    m_pToolBarState = m_pToolBarStateFactory->getState(WorkToolBar::tbb_Arrow, this);

    m_doc = UMLApp::app()->document();

//    // settings for background
//    setBackgroundBrush(QColor(195, 195, 195));
    m_layoutGrid = new LayoutGrid(this);
}

/**
 * Destructor.
 */
UMLScene::~UMLScene()
{
    delete m_pImageExporter;
    m_pImageExporter = 0;
    delete m_pIDChangesLog;
    m_pIDChangesLog = 0;

    // before we can delete the QCanvas, all widgets must be explicitly
    // removed
    // otherwise the implicit remove of the contained widgets will cause
    // events which would demand a valid connected QCanvas
    // ==> this causes umbrello to crash for some - larger?? - projects
    // first avoid all events, which would cause some update actions
    // on deletion of each removed widget
    blockSignals(true);
    removeAllWidgets();

    delete m_pToolBarStateFactory;
    m_pToolBarStateFactory = 0;
}

/**
 * Return the UMLFolder in which this diagram lives.
 */
UMLFolder* UMLScene::folder() const
{
    return m_pFolder;
}

/**
 * Set the UMLFolder in which this diagram lives.
 */
void UMLScene::setFolder(UMLFolder *folder)
{
    m_pFolder = folder;
}

/**
 * Returns the active view associated with this scene.
 */
UMLView* UMLScene::activeView() const
{
    return m_view;
}

/**
 * Return the documentation of the diagram.
 */
QString UMLScene::documentation() const
{
    return m_Documentation;
}

/**
 * Set the documentation of the diagram.
 */
void UMLScene::setDocumentation(const QString &doc)
{
    m_Documentation = doc;
}

/**
 * Return the name of the diagram.
 */
QString UMLScene::name() const
{
    return m_Name;
}

/**
 * Set the name of the diagram.
 */
void UMLScene::setName(const QString &name)
{
    m_Name = name;
}

/**
 * Returns the type of the diagram.
 */
DiagramType::Enum UMLScene::type() const
{
    return m_Type;
}

/**
 * Set the type of diagram.
 */
void UMLScene::setType(DiagramType::Enum type)
{
    m_Type = type;
}

/**
 * Returns the ID of the diagram.
 */
Uml::ID::Type UMLScene::ID() const
{
    return m_nID;
}

/**
 * Sets the ID of the diagram.
 */
void UMLScene::setID(Uml::ID::Type id)
{
    m_nID = id;
}

/**
 * Returns the position of the diagram.
 */
UMLScenePoint UMLScene::pos() const
{
    return m_Pos;
}

/**
 * Sets the position of the diagram.
 */
void UMLScene::setPos(const UMLScenePoint &pos)
{
    m_Pos = pos;
}

/**
 * Returns the fill color to use.
 */
const QColor& UMLScene::fillColor() const
{
    return m_Options.uiState.fillColor;
}

/**
 * Set the background color.
 *
 * @param color  The color to use.
 */
void UMLScene::setFillColor(const QColor &color)
{
    m_Options.uiState.fillColor = color;
    emit sigFillColorChanged(ID());
}

/**
 * Returns the line color to use.
 */
const QColor& UMLScene::lineColor() const
{
    return m_Options.uiState.lineColor;
}

/**
 * Sets the line color.
 *
 * @param color  The color to use.
 */
void UMLScene::setLineColor(const QColor &color)
{
    m_Options.uiState.lineColor = color;
    emit sigLineColorChanged(ID());
}

/**
 * Returns the line width to use.
 */
uint UMLScene::lineWidth() const
{
    return m_Options.uiState.lineWidth;
}

/**
 * Sets the line width.
 *
 * @param width  The width to use.
 */
void UMLScene::setLineWidth(uint width)
{
    m_Options.uiState.lineWidth = width;
    emit sigLineWidthChanged(ID());
}

/**
 * Returns the text color to use.
 */
const QColor& UMLScene::textColor() const
{
    return m_Options.uiState.textColor;
}

/**
 * Sets the text color.
 *
 * @param color  The color to use.
 */
void UMLScene::setTextColor(const QColor& color)
{
    m_Options.uiState.textColor = color;
    emit sigTextColorChanged(ID());
}

/**
 * return grid dot color
 *
 * @return Color
 */
const QColor& UMLScene::gridDotColor() const
{
    return m_layoutGrid->gridDotColor();
}

/**
 * set grid dot color
 *
 * @param color grid dot color
 */
void UMLScene::setGridDotColor(const QColor& color)
{
    m_layoutGrid->setGridDotColor(color);
}

/**
 * Returns the options being used.
 */
const Settings::OptionState& UMLScene::optionState() const
{
    return m_Options;
}

/**
 * Sets the options to be used.
 */
void UMLScene::setOptionState(const Settings::OptionState& options)
{
    m_Options = options;
}

/**
 * Returns a reference to the association list.
 */
AssociationWidgetList& UMLScene::associationList()
{
    return m_AssociationList;
}

/**
 * Returns a reference to the widget list.
 */
UMLWidgetList& UMLScene::widgetList()
{
    return m_WidgetList;
}

/**
 * Returns a reference to the message list.
 */
MessageWidgetList& UMLScene::messageList()
{
    return m_MessageList;
}

/**
 * Used for creating unique name of collaboration messages.
 */
int UMLScene::generateCollaborationId()
{
    return ++m_nCollaborationId;
}

/**
 * Returns the open state.
 * @return   when true diagram is shown to the user
 */
bool UMLScene::isOpen() const
{
    return m_isOpen;
}

/**
 * Sets the flag 'isOpen'.
 * @param isOpen   flag indicating that the diagram is shown to the user
 */
void UMLScene::setIsOpen(bool isOpen)
{
    m_isOpen = isOpen;
}

/**
 * Contains the implementation for printing functionality.
 */
void UMLScene::print(QPrinter *pPrinter, QPainter & pPainter)
{
    bool isFooter = optionState().generalState.footerPrinting;

    // The printer will probably use a different font with different font metrics,
    // force the widgets to update accordingly on paint
    forceUpdateWidgetFontMetrics(&pPainter);

    QRectF source = diagramRect();
    QRect paper = pPrinter->paperRect();
    QRect page = pPrinter->pageRect();

    // use the painter font metrics, not the screen fm!
    QFontMetrics fm = pPainter.fontMetrics(); 
    int fontHeight  = fm.lineSpacing();

    if (paper == page) {
        QSize margin = page.size() * 0.025;
        page.adjust(margin.width(), margin.height(), -margin.width(), -margin.height());
    }

    if (isFooter) {
        int margin = 3 + 3 * fontHeight;
        page.adjust(0, 0, 0, -margin);
    }

    getDiagram(pPainter, QRectF(source), QRectF(page));

    //draw foot note
    if (isFooter) {
        page.adjust(0, 0, 0, fontHeight);
        QString string = i18n("Diagram: %2 Page %1", 1, name());
        QColor textColor(50, 50, 50);
        pPainter.setPen(textColor);
        pPainter.drawLine(page.left(), page.bottom()    , page.right(), page.bottom());
        pPainter.drawText(page.left(), page.bottom() + 3, page.right(), 2*fontHeight, Qt::AlignLeft, string);
    }

    // next painting will most probably be to a different device (i.e. the screen)
    forceUpdateWidgetFontMetrics(0);
}

/**
 * Initialize and announce a newly created widget.
 * Auxiliary to contentsMouseReleaseEvent().
 */
void UMLScene::setupNewWidget(UMLWidget *w)
{
    w->setX(m_Pos.x());
    w->setY(m_Pos.y());
    w->setVisible(true);
    w->setActivated();
    w->setFont(font());
    w->slotFillColorChanged(ID());
    w->slotTextColorChanged(ID());
    w->slotLineWidthChanged(ID());
    resizeSceneToItems();
    m_WidgetList.append(w);
    m_doc->setModified();

    UMLApp::app()->executeCommand(new CmdCreateWidget(w));
}

/**
 * Return whether we are currently creating an object.
 */
bool UMLScene::getCreateObject() const
{
    return m_bCreateObject;
}

/**
 * Set whether we are currently creating an object.
 */
void UMLScene::setCreateObject(bool bCreate)
{
    m_bCreateObject = bCreate;
}

/**
 * Overrides the standard operation.
 */
void UMLScene::showEvent(QShowEvent* /*se*/)
{
    connect(m_doc, SIGNAL(sigObjectCreated(UMLObject*)),
            this, SLOT(slotObjectCreated(UMLObject*)));
    connect(this, SIGNAL(sigAssociationRemoved(AssociationWidget*)),
            UMLApp::app()->docWindow(), SLOT(slotAssociationRemoved(AssociationWidget*)));
    connect(this, SIGNAL(sigWidgetRemoved(UMLWidget*)),
            UMLApp::app()->docWindow(), SLOT(slotWidgetRemoved(UMLWidget*)));
}

/**
 * Overrides the standard operation.
 */
void UMLScene::hideEvent(QHideEvent* /*he*/)
{
    disconnect(m_doc, SIGNAL(sigObjectCreated(UMLObject*)), this, SLOT(slotObjectCreated(UMLObject*)));
    disconnect(this, SIGNAL(sigAssociationRemoved(AssociationWidget*)),
               UMLApp::app()->docWindow(), SLOT(slotAssociationRemoved(AssociationWidget*)));
    disconnect(this, SIGNAL(sigWidgetRemoved(UMLWidget*)),
               UMLApp::app()->docWindow(), SLOT(slotWidgetRemoved(UMLWidget*)));
}

/**
 * Changes the current tool to the selected tool.
 * The current tool is cleaned and the selected tool initialized.
 */
void UMLScene::slotToolBarChanged(int c)
{
    m_pToolBarState->cleanBeforeChange();
    m_pToolBarState = m_pToolBarStateFactory->getState((WorkToolBar::ToolBar_Buttons)c, this);
    m_pToolBarState->init();

    m_bPaste = false;
}

/**
 * Slot called when an object is created.
 * @param o   created UML object
 */
void UMLScene::slotObjectCreated(UMLObject* o)
{
    DEBUG(DBG_SRC) << "scene=" << name() << " / object=" << o->name();
    m_bPaste = false;
    //check to see if we want the message
    //may be wanted by someone else e.g. list view

    if (!m_bCreateObject) {
        return;
    }

    UMLWidget* newWidget = Widget_Factory::createWidget(this, o);

    if (!newWidget) {
        return;
    }

    newWidget->setActivated();
    newWidget->setFont(font());
    newWidget->slotFillColorChanged(ID());
    newWidget->slotTextColorChanged(ID());
    newWidget->slotLineWidthChanged(ID());
    newWidget->updateGeometry();

    m_WidgetList.append(newWidget);
    newWidget->activate();

    m_bCreateObject = false;

    switch (o->baseType()) {
        case UMLObject::ot_Actor:
        case UMLObject::ot_UseCase:
        case UMLObject::ot_Class:
        case UMLObject::ot_Package:
        case UMLObject::ot_Component:
        case UMLObject::ot_Node:
        case UMLObject::ot_Artifact:
        case UMLObject::ot_Interface:
        case UMLObject::ot_Enum:
        case UMLObject::ot_Entity:
        case UMLObject::ot_Datatype:
        case UMLObject::ot_Category:
            createAutoAssociations(newWidget);
            // We need to invoke createAutoAttributeAssociations()
            // on all other widgets again because the newly created
            // widget might saturate some latent attribute assocs.
            foreach(UMLWidget* w,  m_WidgetList) {
                if (w != newWidget) {
                    createAutoAttributeAssociations(w);

                    if (o->baseType() == UMLObject::ot_Entity)
                        createAutoConstraintAssociations(w);
                }
            }
            break;
        default:
            break;
    }
    resizeSceneToItems();
}

/**
 * Slot called when an object is removed.
 * @param o   removed UML object
 */
void UMLScene::slotObjectRemoved(UMLObject * o)
{
    m_bPaste = false;
    Uml::ID::Type id = o->id();

    foreach(UMLWidget* obj, m_WidgetList) {
        if (obj->id() != id)
            continue;
        removeWidget(obj);
        break;
    }
}

/**
 * Override standard method.
 */
void UMLScene::dragEnterEvent(QGraphicsSceneDragDropEvent *e)
{
    UMLDragData::LvTypeAndID_List tidList;
    if (!UMLDragData::getClip3TypeAndID(e->mimeData(), tidList)) {
        return;
    }
    UMLDragData::LvTypeAndID_It tidIt(tidList);
    UMLDragData::LvTypeAndID * tid;
    if (!tidIt.hasNext()) {
        DEBUG(DBG_SRC) << "UMLDragData::getClip3TypeAndID returned empty list";
        return;
    }
    tid = tidIt.next();
    UMLListViewItem::ListViewType lvtype = tid->type;
    Uml::ID::Type id = tid->id;

    DiagramType::Enum diagramType = type();

    UMLObject* temp = 0;
    //if dragging diagram - might be a drag-to-note
    if (Model_Utils::typeIsDiagram(lvtype)) {
        e->accept();
        return;
    }
    //can't drag anything onto state/activity diagrams
    if (diagramType == DiagramType::State || diagramType == DiagramType::Activity) {
        e->ignore();
        return;
    }
    //make sure can find UMLObject
    if (!(temp = m_doc->findObjectById(id))) {
        DEBUG(DBG_SRC) << "object " << Uml::ID::toString(id) << " not found";
        e->ignore();
        return;
    }
    //make sure dragging item onto correct diagram
    // concept - class, seq, coll diagram
    // actor, usecase - usecase diagram
    UMLObject::ObjectType ot = temp->baseType();
    bool bAccept = true;
    switch (diagramType) {
    case DiagramType::UseCase:
        if ((widgetOnDiagram(id) && ot == UMLObject::ot_Actor) ||
            (ot != UMLObject::ot_Actor && ot != UMLObject::ot_UseCase))
            bAccept = false;
        break;
    case DiagramType::Class:
        if (widgetOnDiagram(id) ||
            (ot != UMLObject::ot_Class &&
             ot != UMLObject::ot_Package &&
             ot != UMLObject::ot_Interface &&
             ot != UMLObject::ot_Enum &&
             ot != UMLObject::ot_Datatype)) {
            bAccept = false;
        }
        break;
    case DiagramType::Sequence:
    case DiagramType::Collaboration:
        if (ot != UMLObject::ot_Class &&
            ot != UMLObject::ot_Interface &&
            ot != UMLObject::ot_Actor)
            bAccept = false;
        break;
    case DiagramType::Deployment:
        if (widgetOnDiagram(id))
            bAccept = false;
        else if (ot != UMLObject::ot_Interface &&
                 ot != UMLObject::ot_Package &&
                 ot != UMLObject::ot_Component &&
                 ot != UMLObject::ot_Class &&
                 ot != UMLObject::ot_Node)
            bAccept = false;
        else if (ot == UMLObject::ot_Package &&
                 temp->stereotype() != "subsystem")
            bAccept = false;
        break;
    case DiagramType::Component:
        if (widgetOnDiagram(id) ||
            (ot != UMLObject::ot_Interface &&
             ot != UMLObject::ot_Package &&
             ot != UMLObject::ot_Component &&
             ot != UMLObject::ot_Artifact &&
             ot != UMLObject::ot_Class))
            bAccept = false;
        if (ot == UMLObject::ot_Class && !temp->isAbstract())
            bAccept = false;
        break;
    case DiagramType::EntityRelationship:
        if (ot != UMLObject::ot_Entity && ot != UMLObject::ot_Category)
            bAccept = false;
        break;
    default:
        break;
    }
    if (bAccept) {
        e->accept();
    } else {
        e->ignore();
    }
}

/**
 * Override standard method.
 */
void UMLScene::dragMoveEvent(QGraphicsSceneDragDropEvent* e)
{
    e->accept();
}

/**
 * Override standard method.
 */
void UMLScene::dropEvent(QGraphicsSceneDragDropEvent *e)
{
    UMLDragData::LvTypeAndID_List tidList;
    if (!UMLDragData::getClip3TypeAndID(e->mimeData(), tidList)) {
        return;
    }
    UMLDragData::LvTypeAndID_It tidIt(tidList);
    UMLDragData::LvTypeAndID * tid;
    if (!tidIt.hasNext()) {
        DEBUG(DBG_SRC) << "UMLDragData::getClip3TypeAndID returned empty list";
        return;
    }
    tid = tidIt.next();
    UMLListViewItem::ListViewType lvtype = tid->type;
    Uml::ID::Type id = tid->id;

    if (Model_Utils::typeIsDiagram(lvtype)) {
        bool breakFlag = false;
        UMLWidget* w = 0;
        foreach(w,  m_WidgetList) {
            if (w->baseType() == WidgetBase::wt_Note && w->onWidget(e->scenePos())) {
                breakFlag = true;
                break;
            }
        }
        if (breakFlag) {
            NoteWidget *note = static_cast<NoteWidget*>(w);
            note->setDiagramLink(id);
        }
        return;
    }
    UMLObject* o = m_doc->findObjectById(id);
    if (!o) {
        DEBUG(DBG_SRC) << "object id=" << Uml::ID::toString(id) << " not found";
        return;
    }
    m_bCreateObject = true;
    m_Pos = e->scenePos();

    slotObjectCreated(o);

    m_doc->setModified(true);
}

/**
 * Overrides the standard operation.
 * Calls the same method in the current tool bar state.
 */
void UMLScene::mouseMoveEvent(QGraphicsSceneMouseEvent* ome)
{
    m_pToolBarState->mouseMove(ome);
}

/**
 * Override standard method.
 * Calls the same method in the current tool bar state.
 */
void UMLScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    m_pToolBarState->mousePress(event);

    //TODO should be managed by widgets when are selected. Right now also has some
    //problems, such as clicking on a widget, and clicking to move that widget shows
    //documentation of the diagram instead of keeping the widget documentation.
    //When should diagram documentation be shown? When clicking on an empty
    //space in the diagram with arrow tool?
    UMLWidget* widget = widgetAt(event->scenePos());
    if (widget) {
        DEBUG(DBG_SRC) << "widget = " << widget->name() << " / type = " << widget->baseTypeStr();
        showDocumentation(widget, true);
        event->accept();
    }
    else {
        AssociationWidget* association = associationAt(event->scenePos());
        if (association) {
            DEBUG(DBG_SRC) << "association widget = " << association->name() << " / type = " << association->baseTypeStr();
            // the following is done in AssociationWidget::setSelected()
            // showDocumentation(association, true);
            // event->accept();
        }
        //:TODO: else if (clicking on other elements with documentation) {
        //:TODO: showDocumentation(umlObject, true);
        else {
            // clicking on an empty space in the diagram with arrow tool
            showDocumentation(true);
            event->accept();
        }
    }
}

/**
 * Override standard method.
 * Calls the same method in the current tool bar state.
 */
void UMLScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    m_pToolBarState->mouseDoubleClick(event);

    if (!onItem(event->scenePos())) {
        // show properties dialog of the scene
        if (m_view->showPropDialog() == true) {
            m_doc->setModified();
        }
    }
    event->accept();
}

/**
 * Overrides the standard operation.
 * Calls the same method in the current tool bar state.
 */
void UMLScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* ome)
{
    m_pToolBarState->mouseRelease(ome);
}

/**
 * Determine whether on a sequence diagram we have clicked on a line
 * of an Object.
 *
 * @return The widget thats line was clicked on.
 *  Returns 0 if no line was clicked on.
 */
ObjectWidget * UMLScene::onWidgetLine(const UMLScenePoint &point) const
{
    foreach(UMLWidget* obj, m_WidgetList) {
        ObjectWidget *ow = dynamic_cast<ObjectWidget*>(obj);
        if (ow == NULL)
            continue;
        SeqLineWidget *pLine = ow->sequentialLine();
        if (pLine == NULL) {
            uError() << "SeqLineWidget of " << ow->name()
            << " (id=" << Uml::ID::toString(ow->localID()) << ") is NULL";
            continue;
        }
        if (pLine->onWidget(point))
            return ow;
    }
    return 0;
}

/**
 * Determine whether on a sequence diagram we have clicked on
 * the destruction box of an Object.
 *
 * @return The widget thats destruction box was clicked on.
 *  Returns 0 if no destruction box was clicked on.
 */
ObjectWidget * UMLScene::onWidgetDestructionBox(const UMLScenePoint &point) const
{
    foreach(UMLWidget* obj,  m_WidgetList) {
        ObjectWidget *ow = dynamic_cast<ObjectWidget*>(obj);
        if (ow == NULL)
            continue;
        SeqLineWidget *pLine = ow->sequentialLine();
        if (pLine == NULL) {
            uError() << "SeqLineWidget of " << ow->name()
                     << " (id=" << Uml::ID::toString(ow->localID()) << ") is NULL";
            continue;
        }
        if (pLine->onDestructionBox(point))
            return ow;
    }
    return 0;
}

/**
 * Return pointer to the first selected widget (for multi-selection)
 */
UMLWidget* UMLScene::getFirstMultiSelectedWidget() const
{
    if (m_selectedList.size() == 0)
        return 0;
    return m_selectedList.first();
}

/**
 * Tests the given point against all widgets and returns the
 * widget for which the point is within its bounding rectangle.
 * In case of multiple matches, returns the smallest widget.
 * Returns NULL if the point is not inside any widget.
 * TODO: What about using QGraphicsScene::items(...)?
 */
UMLWidget* UMLScene::widgetAt(const UMLScenePoint& p)
{
    qreal relativeSize = 99990.0;  // start with an arbitrary large number
    UMLWidget  *retWid = 0;
    foreach(UMLWidget* wid, widgetList()) {
        const int s = wid->onWidget(p);
        if (!s)
            continue;
        if (s < relativeSize) {
            relativeSize = s;
            retWid = wid;
        }
    }
    return retWid;
}

/**
 * Tests the given point against all associations and returns the
 * association widget for which the point is on the line.
 * Returns NULL if the point is not inside any association.
 * CHECK: This is the same method as in ToolBarState.
 */
AssociationWidget* UMLScene::associationAt(const UMLScenePoint& p)
{
    foreach (AssociationWidget* association, associationList()) {
        if (association->onAssociation(p)) {
            return association;
        }
    }
    return 0;
}

/**
 * Tests the given point against all associations and returns the
 * association widget for which the point is on the line.
 * Returns NULL if the point is not inside any association.
 */
MessageWidget* UMLScene::messageAt(const UMLScenePoint& p)
{
    foreach(MessageWidget *message, messageList()) {
        if (message->onWidget(p)) {
            return message;
        }
    }
    return 0;
}

/**
 * Sees if a message is relevant to the given widget.  If it does delete it.
 * @param w The widget to check messages against.
 */
void UMLScene::checkMessages(ObjectWidget * w)
{
    if (type() != DiagramType::Sequence) {
        return;
    }

    MessageWidgetListIt it(m_MessageList);
    foreach(MessageWidget *obj, m_MessageList) {
        if (! obj->hasObjectWidget(w)) {
            continue;
        }
        //make sure message doesn't have any associations
        removeAssociations(obj);
        obj->cleanup();
        //make sure not in selected list
        m_selectedList.removeAll(obj);
        m_MessageList.removeAll(obj);
        obj->deleteLater();
    }
}

/**
 * Returns whether a widget is already on the diagram.
 *
 * @param id The id of the widget to check for.
 *
 * @return Returns true if the widget is already on the diagram, false if not.
 */
bool UMLScene::widgetOnDiagram(Uml::ID::Type id)
{
    foreach(UMLWidget *obj, m_WidgetList) {
        if (id == obj->id())
            return true;
    }

    foreach(UMLWidget *obj, m_MessageList) {
        if (id == obj->id())
            return true;
    }

    return false;
}

/**
 * Finds a widget with the given ID.
 * Search both our UMLWidget AND MessageWidget lists.
 * @param id The ID of the widget to find.
 *
 * @return Returns the widget found, returns 0 if no widget found.
 */
UMLWidget * UMLScene::findWidget(Uml::ID::Type id)
{
    foreach(UMLWidget* obj, m_WidgetList) {
        // object widgets are special..the widget id is held by 'localId' attribute (crappy!)
        if (obj->baseType() == WidgetBase::wt_Object) {
            if (static_cast<ObjectWidget *>(obj)->localID() == id)
                return obj;
        } else if (obj->id() == id) {
            return obj;
        }
    }

    foreach(UMLWidget* obj, m_MessageList) {
        if (obj->id() == id)
            return obj;
    }

    return 0;
}

/**
 * Finds an association widget with the given ID.
 *
 * @param id The ID of the widget to find.
 *
 * @return Returns the widget found, returns 0 if no widget found.
 */
AssociationWidget * UMLScene::findAssocWidget(Uml::ID::Type id)
{
    foreach(AssociationWidget* obj, m_AssociationList) {
        UMLAssociation* umlassoc = obj->association();
        if (umlassoc && umlassoc->id() == id) {
            return obj;
        }
    }
    return 0;
}

/**
 * Finds an association widget with the given widgets and the given role B name.
 * Considers the following association types:
 *  at_Association, at_UniAssociation, at_Composition, at_Aggregation
 * This is used for seeking an attribute association.
 *
 * @param pWidgetA  Pointer to the UMLWidget of role A.
 * @param pWidgetB  Pointer to the UMLWidget of role B.
 * @param roleNameB Name at the B side of the association (the attribute name)
 *
 * @return Returns the widget found, returns 0 if no widget found.
 */
AssociationWidget * UMLScene::findAssocWidget(UMLWidget *pWidgetA,
                                              UMLWidget *pWidgetB, const QString& roleNameB)
{
    foreach(AssociationWidget* assoc, m_AssociationList) {
        const Uml::AssociationType::Enum testType = assoc->associationType();
        if (testType != Uml::AssociationType::Association &&
                testType != Uml::AssociationType::UniAssociation &&
                testType != Uml::AssociationType::Composition &&
                testType != Uml::AssociationType::Aggregation &&
                testType != Uml::AssociationType::Relationship) {
            continue;
        }

        if (pWidgetA->id() == assoc->widgetIDForRole(Uml::RoleType::A) &&
                pWidgetB->id() == assoc->widgetIDForRole(Uml::RoleType::B) &&
                assoc->roleName(Uml::RoleType::B) == roleNameB) {
            return assoc;
        }
    }
    return 0;
}

/**
 * Finds an association widget with the given type and widgets.
 *
 * @param at  The AssociationType of the widget to find.
 * @param pWidgetA Pointer to the UMLWidget of role A.
 * @param pWidgetB Pointer to the UMLWidget of role B.
 *
 * @return Returns the widget found, returns 0 if no widget found.
 */
AssociationWidget * UMLScene::findAssocWidget(AssociationType::Enum at,
                                              UMLWidget *pWidgetA, UMLWidget *pWidgetB)
{
    foreach(AssociationWidget* assoc, m_AssociationList) {
        Uml::AssociationType::Enum testType = assoc->associationType();
        if (testType != at) {
            continue;
        }

        if (pWidgetA->id() == assoc->widgetIDForRole(Uml::RoleType::A) &&
                pWidgetB->id() == assoc->widgetIDForRole(Uml::RoleType::B)) {
            return assoc;
        }
    }
    return 0;
}

/**
 * Remove a widget from view.
 *
 * @param o  The widget to remove.
 */
void UMLScene::removeWidget(UMLWidget * o)
{
    if (!o)
        return;

    emit sigWidgetRemoved(o);

    removeAssociations(o);

    WidgetBase::WidgetType t = o->baseType();
    if (type() == DiagramType::Sequence && t == WidgetBase::wt_Object) {
        checkMessages(static_cast<ObjectWidget*>(o));
    }

    o->cleanup();
    m_selectedList.removeAll(o);
    disconnect(this, SIGNAL(sigClearAllSelected()), o, SLOT(slotClearAllSelected()));
    disconnect(this, SIGNAL(sigFillColorChanged(Uml::ID::Type)), o, SLOT(slotFillColorChanged(Uml::ID::Type)));
    disconnect(this, SIGNAL(sigLineColorChanged(Uml::ID::Type)), o, SLOT(slotLineColorChanged(Uml::ID::Type)));
    disconnect(this, SIGNAL(sigTextColorChanged(Uml::ID::Type)), o, SLOT(slotTextColorChanged(Uml::ID::Type)));
    if (t == WidgetBase::wt_Message) {
        m_MessageList.removeAll(static_cast<MessageWidget*>(o));
    } else
        m_WidgetList.removeAll(o);
    o->deleteLater();
    m_doc->setModified(true);
}

/**
 * Returns whether to use the fill/background color
 */
bool UMLScene::useFillColor() const
{
    return m_Options.uiState.useFillColor;
}

/**
 * Sets whether to use the fill/background color
 */
void UMLScene::setUseFillColor(bool ufc)
{
    m_Options.uiState.useFillColor = ufc;
}

/**
 * Gets the smallest area to print.
 *
 * @return Returns the smallest area to print.
 */
QRectF UMLScene::diagramRect()
{
    return itemsBoundingRect();
}

/**
 * Check if at the given point is a widget or an association widget.
 * @param atPos   the mouse position on the scene
 * @return true if there is a widget or an association line
 */
bool UMLScene::onItem(const UMLScenePoint& atPos)
{
    UMLWidget* widget = widgetAt(atPos);
    if (widget) {
        DEBUG(DBG_SRC) << "uml widget = " << widget->name() << " / type = " << widget->baseTypeStr();
        return true;
    }

    AssociationWidget* association = associationAt(atPos);
    if (association) {
        DEBUG(DBG_SRC) << "association = " << association->name() << " / type = " << association->baseTypeStr();
        return true;
    }

    MessageWidget* message = messageAt(atPos);
    if (message) {
        DEBUG(DBG_SRC) << "message = " << message->name() << " / type = " << message->baseTypeStr();
        return true;
    }

    DEBUG(DBG_SRC) << "Not on any item!";
    return false;
}

/**
 * Sets a widget to a selected state and adds it to a list of selected widgets.
 * This method also sets the state of the cut and copy menu entries.
 *
 * @param w The widget to set to selected.
 * @param me The mouse event containing the information about the selection.
 */
void UMLScene::setSelected(UMLWidget *w, QGraphicsSceneMouseEvent *me)
{
    Q_UNUSED(me);
    //only add if wasn't in list
    if (!m_selectedList.removeAll(w))
        m_selectedList.append(w);
    int count = m_selectedList.count();
    //only call once - if we select more, no need to keep clearing  window

    // if count == 1, widget will update the doc window with their data when selected
    if (count == 2)
        updateDocumentation(true);  //clear doc window

    // selection changed, we have to make sure the copy and paste items
    // are correctly enabled/disabled
    UMLApp::app()->slotCopyChanged();
}

/**
 * Returns a list of selected widgets.
 */
UMLWidgetList UMLScene::selectedWidgets() const
{
    return m_selectedList;
}

/**
 *  Clear the selected widgets list.
 */
void UMLScene::clearSelected()
{
    m_selectedList.clear();
    emit sigClearAllSelected();
    //m_doc->enableCutCopy(false);
}

/**
 * Move all the selected widgets by a relative X and Y offset.
 * TODO: Only used in UMLApp::handleCursorKeyReleaseEvent
 *
 * @param dX The distance to move horizontally.
 * @param dY The distance to move vertically.
 */
void UMLScene::moveSelectedBy(UMLSceneValue dX, UMLSceneValue dY)
{
    // DEBUG(DBG_SRC) << "********** m_selectedList count=" << m_selectedList.count();
    foreach(UMLWidget *w, selectedWidgets()) {
        w->moveByLocal(dX, dY);
    }
}

/**
 * Set the useFillColor variable to all selected widgets
 *
 * @param useFC The state to set the widget to.
 */
void UMLScene::selectionUseFillColor(bool useFC)
{
    foreach(UMLWidget* widget, selectedWidgets()) {
        widget->setUseFillColor(useFC);
    }
}

/**
 * Set the font for all the currently selected items.
 */
void UMLScene::selectionSetFont(const QFont &font)
{
    foreach(UMLWidget* temp, selectedWidgets()) {
        temp->setFont(font);
    }
}

/**
 * Set the line color for all the currently selected items.
 */
void UMLScene::selectionSetLineColor(const QColor &color)
{
    UMLApp::app()->beginMacro(i18n("Change Line Color"));
    foreach(UMLWidget *temp, selectedWidgets()) {
        temp->setLineColor(color);
        temp->setUsesDiagramLineColor(false);
    }
    AssociationWidgetList assoclist = selectedAssocs();
    foreach(AssociationWidget *aw, assoclist) {
        aw->setLineColor(color);
        aw->setUsesDiagramLineColor(false);
    }
    UMLApp::app()->endMacro();
}

/**
 * Set the line width for all the currently selected items.
 */
void UMLScene::selectionSetLineWidth(uint width)
{
    foreach(UMLWidget* temp, selectedWidgets()) {
        temp->setLineWidth(width);
        temp->setUsesDiagramLineWidth(false);
    }
    AssociationWidgetList assoclist = selectedAssocs();
    foreach(AssociationWidget *aw, assoclist) {
        aw->setLineWidth(width);
        aw->setUsesDiagramLineWidth(false);
    }
}

/**
 * Set the fill color for all the currently selected items.
 */
void UMLScene::selectionSetFillColor(const QColor &color)
{
    UMLApp::app()->beginMacro(i18n("Change Fill Color"));

    foreach(UMLWidget* widget, selectedWidgets()) {
        widget->setFillColor(color);
        widget->setUsesDiagramFillColor(false);
    }
    UMLApp::app()->endMacro();
}

/**
 * Toggles the show setting sel of all selected items.
 */
void UMLScene::selectionToggleShow(int sel)
{
    // loop through all selected items
    foreach(UMLWidget *temp, selectedWidgets()) {
        WidgetBase::WidgetType type = temp->baseType();
        ClassifierWidget *cw = dynamic_cast<ClassifierWidget*>(temp);

        // toggle the show setting sel
        switch (sel) {
            // some setting are only available for class, some for interface and some
            // for both
        case ListPopupMenu::mt_Show_Attributes_Selection:
            if (type == WidgetBase::wt_Class)
                cw->toggleVisualProperty(ClassifierWidget::ShowAttributes);
            break;
        case ListPopupMenu::mt_Show_Operations_Selection:
            if (cw)
                cw->toggleVisualProperty(ClassifierWidget::ShowOperations);
            break;
        case ListPopupMenu::mt_Visibility_Selection:
            if (cw)
                cw->toggleVisualProperty(ClassifierWidget::ShowVisibility);
            break;
        case ListPopupMenu::mt_DrawAsCircle_Selection:
            if (type == WidgetBase::wt_Interface)
                cw->toggleVisualProperty(ClassifierWidget::DrawAsCircle);
            break;
        case ListPopupMenu::mt_Show_Operation_Signature_Selection:
            if (cw)
                cw->toggleVisualProperty(ClassifierWidget::ShowOperationSignature);
            break;
        case ListPopupMenu::mt_Show_Attribute_Signature_Selection:
            if (type == WidgetBase::wt_Class)
                cw->toggleVisualProperty(ClassifierWidget::ShowAttributeSignature);
            break;
        case ListPopupMenu::mt_Show_Packages_Selection:
            if (cw)
                cw->toggleVisualProperty(ClassifierWidget::ShowPackage);
            break;
        case ListPopupMenu::mt_Show_Stereotypes_Selection:
            if (type == WidgetBase::wt_Class)
                cw->toggleVisualProperty(ClassifierWidget::ShowStereotype);
            break;
        case ListPopupMenu::mt_Show_Public_Only_Selection:
            if (cw)
                cw->toggleVisualProperty(ClassifierWidget::ShowPublicOnly);
            break;
        default:
            break;
        } // switch (sel)
    }
}

/**
 * Delete the selected widgets list and the widgets in it.
 */
void UMLScene::deleteSelection()
{
    //  Don't delete text widget that are connect to associations as these will
    //  be cleaned up by the associations.

    foreach(UMLWidget* widget, selectedWidgets()) {
        if (widget->baseType() == WidgetBase::wt_Text &&
                static_cast<FloatingTextWidget*>(widget)->textRole() != Uml::TextRole::Floating) {
            m_selectedList.removeAt(m_selectedList.indexOf(widget));
            widget->hide();
        } else {
            removeWidget(widget);
        }
    }

    // Delete any selected associations.
    foreach(AssociationWidget* assocwidget, m_AssociationList) {
        if (assocwidget->isSelected()) {
            removeAssoc(assocwidget);
        }
    }

    // we also have to remove selected messages from sequence diagrams

    // loop through all messages and check the selection state
    foreach(MessageWidget* cur_msgWgt, m_MessageList) {
        if (cur_msgWgt->isSelected()) {
            removeWidget(cur_msgWgt);  // Remove message - it is selected.
        }
    }

    // sometimes we miss one widget, so call this function again to remove it as well
    if (m_selectedList.count() != 0)
        deleteSelection();

    //make sure list empty - it should be anyway, just a check.
    m_selectedList.clear();
}

/**
 * Selects all widgets
 */
void UMLScene::selectAll()
{
    selectWidgets(0, 0, width(), height());
}

/**
 * Return a unique ID for the diagram.  Used by the @ref ObjectWidget class.
 *
 * @return Return a unique ID for the diagram.
 */
Uml::ID::Type UMLScene::localID()
{
    m_nLocalID = UniqueID::gen();
    return m_nLocalID;
}

/**
 * Returns true if this diagram resides in an externalized folder.
 * CHECK: It is probably cleaner to move this to the UMLListViewItem.
 */
bool UMLScene::isSavedInSeparateFile()
{
    if (optionState().generalState.tabdiagrams) {
        // Umbrello currently does not support external folders
        // when tabbed diagrams are enabled.
        return false;
    }
    const QString msgPrefix("UMLScene::isSavedInSeparateFile(" + name() + "): ");
    UMLListView *listView = UMLApp::app()->listView();
    UMLListViewItem *lvItem = listView->findItem(m_nID);
    if (lvItem == NULL) {
        uError() << msgPrefix
                 << "listView->findUMLObject(this) returns false";
        return false;
    }
    UMLListViewItem *parentItem = dynamic_cast<UMLListViewItem*>(lvItem->parent());
    if (parentItem == NULL) {
        uError() << msgPrefix
                 << "parent item in listview is not a UMLListViewItem (?)";
        return false;
    }
    const UMLListViewItem::ListViewType lvt = parentItem->type();
    if (! Model_Utils::typeIsFolder(lvt))
        return false;
    UMLFolder *modelFolder = dynamic_cast<UMLFolder*>(parentItem->umlObject());
    if (modelFolder == NULL) {
        uError() << msgPrefix
                 << "parent model object is not a UMLFolder (?)";
        return false;
    }
    QString folderFile = modelFolder->folderFile();
    return !folderFile.isEmpty();
}

UMLSceneItemList UMLScene::collisions(const UMLScenePoint &p, int delta)
{
    QPointF a = p-QPointF(delta, delta);
    QPointF b = p+QPointF(delta, delta);
    QList<QGraphicsItem *> list = items(QRectF(a, b));
    return list;
}

/**
 * Calls setSelected on the given UMLWidget and enters
 * it into the m_selectedList while making sure it is
 * there only once.
 */
void UMLScene::makeSelected(UMLWidget* uw)
{
    if (uw) {
        uw->setSelected(true);
        m_selectedList.removeAll(uw);  // make sure not in there
        m_selectedList.append(uw);
    }
}

/**
 * Selects all the widgets of the given association widget.
 */
void UMLScene::selectWidgetsOfAssoc(AssociationWidget * a)
{
    if (a) {
        a->setSelected(true);
        //select the two widgets
        makeSelected(a->widgetForRole(Uml::RoleType::A));
        makeSelected(a->widgetForRole(Uml::RoleType::B));
        //select all the text
        makeSelected(a->multiplicityWidget(Uml::RoleType::A));
        makeSelected(a->multiplicityWidget(Uml::RoleType::B));
        makeSelected(a->roleWidget(Uml::RoleType::A));
        makeSelected(a->roleWidget(Uml::RoleType::B));
        makeSelected(a->changeabilityWidget(Uml::RoleType::A));
        makeSelected(a->changeabilityWidget(Uml::RoleType::B));
    }
}

/**
 * Selects all the widgets within an internally kept rectangle.
 */
void UMLScene::selectWidgets(UMLSceneValue px, UMLSceneValue py, UMLSceneValue qx, UMLSceneValue qy)
{
    clearSelected();

    QRectF  rect;
    if (px <= qx) {
        rect.setLeft(px);
        rect.setRight(qx);
    } else {
        rect.setLeft(qx);
        rect.setRight(px);
    }
    if (py <= qy) {
        rect.setTop(py);
        rect.setBottom(qy);
    } else {
        rect.setTop(qy);
        rect.setBottom(py);
    }

    foreach(UMLWidget* temp, m_WidgetList) {
        int x = temp->x();
        int y = temp->y();
        int w = temp->width();
        int h = temp->height();
        QRectF  rect2(x, y, w, h);

        //see if any part of widget is in the rectangle
        if (!rect.intersects(rect2))
            continue;
        //if it is text that is part of an association then select the association
        //and the objects that are connected to it.
        if (temp->baseType() == WidgetBase::wt_Text) {
            FloatingTextWidget *ft = static_cast<FloatingTextWidget*>(temp);
            Uml::TextRole::Enum t = ft->textRole();
            LinkWidget *lw = ft->link();
            MessageWidget * mw = dynamic_cast<MessageWidget*>(lw);
            if (mw) {
                makeSelected(mw);
                makeSelected(mw->objectWidget(Uml::RoleType::A));
                makeSelected(mw->objectWidget(Uml::RoleType::B));
            } else if (t != Uml::TextRole::Floating) {
                AssociationWidget * a = dynamic_cast<AssociationWidget*>(lw);
                if (a)
                    selectWidgetsOfAssoc(a);
            }
        } else if (temp->baseType() == WidgetBase::wt_Message) {
            MessageWidget *mw = static_cast<MessageWidget*>(temp);
            makeSelected(mw->objectWidget(Uml::RoleType::A));
            makeSelected(mw->objectWidget(Uml::RoleType::B));
        }
        if (temp->isVisible()) {
            makeSelected(temp);
        }
    }
    selectAssociations(true);

    //now do the same for the messagewidgets

    foreach(MessageWidget *w, m_MessageList) {
        if (w->objectWidget(Uml::RoleType::A)->isSelected() &&
                w->objectWidget(Uml::RoleType::B)->isSelected()) {
            makeSelected(w);
        }//end if
    }//end foreach
}

/**
 * Selects all the widgets from a list.
 */
void UMLScene::selectWidgets(UMLWidgetList &widgets)
{
    foreach (UMLWidget* widget, widgets)
        makeSelected(widget);
}

/**
 * Returns the PNG picture of the paste operation.
 * @param diagram the class to store PNG picture of the paste operation.
 * @param rect the area of the diagram to copy
 */
void  UMLScene::getDiagram(QPixmap &diagram, const QRectF &rect)
{
    DEBUG(DBG_SRC) << "rect=" << rect << ", pixmap=" << diagram.rect();
    QPainter painter(&diagram);
    painter.fillRect(0, 0, rect.width(), rect.height(), Qt::white);
    getDiagram(painter, rect);
}

/**
 * Paint diagram to the paint device
 * @param source the area of the diagram to copy
 * @param target the rect where to paint into
 */
void  UMLScene::getDiagram(QPainter &painter, const QRectF &source, const QRectF &target)
{
    DEBUG(DBG_SRC) << "painter=" << painter.window() << ", source=" << source << ", target=" << target;
    //TODO unselecting and selecting later doesn't work now as the selection is
    //cleared in UMLSceneImageExporter. Check if the anything else than the
    //following is needed and, if it works, remove the clearSelected in
    //UMLSceneImageExporter and UMLSceneImageExporterModel

    UMLWidgetList selected = selectedWidgets();
    foreach(UMLWidget* widget, selected) {
        widget->setSelected(false);
    }
    AssociationWidgetList selectedAssociationsList = selectedAssocs();

    foreach(AssociationWidget* association, selectedAssociationsList) {
        association->setSelected(false);
    }

    // we don't want to get the grid
    bool showSnapGrid = isSnapGridVisible();
    setSnapGridVisible(false);

    const int sourceMargin = 1;
    QRectF alignedSource(source);
    alignedSource.adjust(-sourceMargin, -sourceMargin, sourceMargin, sourceMargin);

    uDebug() << "TODO: Check if this render method is identical to cavnas()->drawArea()";
    // [PORT]
    render(&painter, target, alignedSource, Qt::KeepAspectRatio);

    setSnapGridVisible(showSnapGrid);

    //select again
    foreach(UMLWidget* widget, selected) {
        widget->setSelected(true);
    }
    foreach(AssociationWidget* association, selectedAssociationsList) {
        association->setSelected(true);
    }
}

/**
 * Returns the imageExporter used to export the view.
 *
 * @return The imageExporter used to export the view.
 */
UMLViewImageExporter* UMLScene::getImageExporter()
{
    return m_pImageExporter;
}

/**
 * makes this view the active view by asking the document to show us
 */
void UMLScene::slotActivate()
{
    m_doc->changeCurrentView(ID());
}

/**
 * Returns a List of all the UMLObjects(Use Cases, Concepts and Actors) in the View
 */
UMLObjectList UMLScene::umlObjects()
{
    UMLObjectList list;
    foreach(UMLWidget* w,  m_WidgetList) {

        switch (w->baseType()) { //use switch for easy future expansion
        case WidgetBase::wt_Actor:
        case WidgetBase::wt_Class:
        case WidgetBase::wt_Interface:
        case WidgetBase::wt_Package:
        case WidgetBase::wt_Component:
        case WidgetBase::wt_Node:
        case WidgetBase::wt_Artifact:
        case WidgetBase::wt_UseCase:
        case WidgetBase::wt_Object:
            list.append(w->umlObject());
            break;
        default:
            break;
        }
    }
    return list;
}

/**
 * Activate all the objects and associations after a load from the clipboard
 */
void UMLScene::activate()
{
    //Activate Regular widgets then activate  messages
    foreach(UMLWidget* obj, m_WidgetList) {
        //If this UMLWidget is already activated or is a MessageWidget then skip it
        if (obj->isActivated() || obj->baseType() == WidgetBase::wt_Message) {
            continue;
        }

       if (obj->activate()) {
           obj->setVisible(true);
       } else {
           m_WidgetList.removeAll(obj);
           delete obj;
       }
    }//end foreach

    //Activate Message widgets
    foreach(UMLWidget* obj, m_MessageList) {
        //If this MessageWidget is already activated then skip it
        if (obj->isActivated())
            continue;

        obj->activate(m_doc->changeLog());
        obj->setVisible(true);

    }//end foreach

    // Activate all association widgets

    foreach(AssociationWidget* aw, m_AssociationList) {
        if (aw->activate()) {
            if (m_PastePoint.x() != 0) {
                int x = m_PastePoint.x() - m_Pos.x();
                int y = m_PastePoint.y() - m_Pos.y();
                aw->moveEntireAssoc(x, y);
            }
        } else {
            m_AssociationList.removeAll(aw);
            delete aw;
        }
    }
}

/**
 * Return the amount of widgets selected.
 *
 * @param filterText  When true, do NOT count floating text widgets that
 *                    belong to other widgets (i.e. only count TextRole::Floating.)
 *                    Default: Count all widgets.
 * @return  Number of widgets selected.
 */
int UMLScene::selectedCount(bool filterText) const
{
    if (!filterText)
        return selectedWidgets().count();
    int counter = 0;
    foreach(UMLWidget* temp, selectedWidgets()) {
        if (temp->baseType() == WidgetBase::wt_Text) {
            const FloatingTextWidget *ft = static_cast<const FloatingTextWidget*>(temp);
            if (ft->textRole() == TextRole::Floating)
                counter++;
        } else {
            counter++;
        }
    }
    return counter;
}

/**
 * Fills the List with all the selected widgets from the diagram
 * The list can be filled with all the selected widgets, or be filtered to prevent
 * text widgets other than tr_Floating to be append.
 *
 * @param filterText Don't append the text, unless their role is tr_Floating
 * @return           The UMLWidgetList to fill.
 */
UMLWidgetList UMLScene::selectedWidgetsExt(bool filterText /*= true*/)
{
    UMLWidgetList widgetList;

    foreach(UMLWidget* widgt, selectedWidgets()) {
        if (filterText && widgt->baseType() == WidgetBase::wt_Text) {
            const FloatingTextWidget *ft = static_cast<const FloatingTextWidget*>(widgt);
            if (ft->textRole() == Uml::TextRole::Floating)
                widgetList.append(widgt);
        } else {
            widgetList.append(widgt);
        }
    }
    return widgetList;
}

/**
 * Returns a list with all the selected associations from the diagram
 */
AssociationWidgetList UMLScene::selectedAssocs()
{
    AssociationWidgetList assocWidgetList;

    foreach(AssociationWidget* assocwidget, m_AssociationList) {
        if (assocwidget->isSelected())
            assocWidgetList.append(assocwidget);
    }
    return assocWidgetList;
}

/**
 * Adds a widget to the view from the given data.
 * Use this method when pasting.
 */
bool UMLScene::addWidget(UMLWidget * pWidget, bool isPasteOperation)
{
    if (!pWidget) {
        return false;
    }
    WidgetBase::WidgetType type = pWidget->baseType();
    if (isPasteOperation) {
        if (type == WidgetBase::wt_Message)
            m_MessageList.append(static_cast<MessageWidget*>(pWidget));
        else
            m_WidgetList.append(pWidget);
        return true;
    }
    if (!isPasteOperation && findWidget(pWidget->id())) {
        uError() << "Not adding (id=" << Uml::ID::toString(pWidget->id())
                 << "/type=" << pWidget->baseTypeStr() << "/name=" << pWidget->name()
                 << ") because it is already there";
        return false;
    }
    IDChangeLog * log = m_doc->changeLog();
    if (isPasteOperation && (!log || !m_pIDChangesLog)) {
        uError() << " Cannot addWidget to view in paste op because a log is not open";
        return false;
    }
    int wX = pWidget->x();
    int wY = pWidget->y();
    bool xIsOutOfRange = (wX < sceneRect().left() || wX > sceneRect().right());
    bool yIsOutOfRange = (wY < sceneRect().top() || wY > sceneRect().bottom());
    if (xIsOutOfRange || yIsOutOfRange) {
        QString name = pWidget->name();
        if (name.isEmpty()) {
            FloatingTextWidget *ft = dynamic_cast<FloatingTextWidget*>(pWidget);
            if (ft)
                name = ft->displayText();
        }
        DEBUG(DBG_SRC) << name << " type=" << pWidget->baseTypeStr() << ": position ("
                       << wX << "," << wY << ") is out of range";
        if (xIsOutOfRange) {
            pWidget->setX(0);
            wX = 0;
        }
        if (yIsOutOfRange) {
            pWidget->setY(0);
            wY = 0;
        }
    }
    if (wX < m_Pos.x())
        m_Pos.setX(wX);
    if (wY < m_Pos.y())
        m_Pos.setY(wY);

    //see if we need a new id to match object
    switch (type) {

    case WidgetBase::wt_Class:
    case WidgetBase::wt_Package:
    case WidgetBase::wt_Component:
    case WidgetBase::wt_Node:
    case WidgetBase::wt_Artifact:
    case WidgetBase::wt_Interface:
    case WidgetBase::wt_Enum:
    case WidgetBase::wt_Entity:
    case WidgetBase::wt_Datatype:
    case WidgetBase::wt_Actor:
    case WidgetBase::wt_UseCase:
    case WidgetBase::wt_Category: {
        Uml::ID::Type id = pWidget->id();
        Uml::ID::Type newID = log ? log->findNewID(id) : Uml::ID::None;
        if (newID == Uml::ID::None) {   // happens after a cut
            if (id == Uml::ID::None)
                return false;
            newID = id; //don't stop paste
        } else
            pWidget->setID(newID);
        UMLObject * pObject = m_doc->findObjectById(newID);
        if (!pObject) {
            DEBUG(DBG_SRC) << "addWidget: Can not find UMLObject for id "
                           << Uml::ID::toString(newID);
            return false;
        }
        pWidget->setUMLObject(pObject);
        //make sure it doesn't already exist.
        if (findWidget(newID)) {
            DEBUG(DBG_SRC) << "Not adding (id=" << Uml::ID::toString(pWidget->id())
                           << "/type=" << pWidget->baseTypeStr()
                           << "/name=" << pWidget->name()
                           << ") because it is already there";
            delete pWidget; // Not nice but if _we_ don't do it nobody else will
            return true;//don't stop paste just because widget found.
        }
        m_WidgetList.append(pWidget);
    }
    break;

    case WidgetBase::wt_Message:
    case WidgetBase::wt_Note:
    case WidgetBase::wt_Box:
    case WidgetBase::wt_Text:
    case WidgetBase::wt_State:
    case WidgetBase::wt_Activity:
    case WidgetBase::wt_ObjectNode: {
        Uml::ID::Type newID = m_doc->assignNewID(pWidget->id());
        pWidget->setID(newID);
        if (type != WidgetBase::wt_Message) {
            m_WidgetList.append(pWidget);
            return true;
        }
        // CHECK
        // Handling of WidgetBase::wt_Message:
        MessageWidget *pMessage = static_cast<MessageWidget *>(pWidget);
        if (pMessage == NULL) {
            DEBUG(DBG_SRC) << "pMessage is NULL";
            return false;
        }
        ObjectWidget *objWidgetA = pMessage->objectWidget(Uml::RoleType::A);
        ObjectWidget *objWidgetB = pMessage->objectWidget(Uml::RoleType::B);
        Uml::ID::Type waID = objWidgetA->localID();
        Uml::ID::Type wbID = objWidgetB->localID();
        Uml::ID::Type newWAID = m_pIDChangesLog->findNewID(waID);
        Uml::ID::Type newWBID = m_pIDChangesLog->findNewID(wbID);
        if (newWAID == Uml::ID::None || newWBID == Uml::ID::None) {
            DEBUG(DBG_SRC) << "Error with ids : " << Uml::ID::toString(newWAID)
                           << " " << Uml::ID::toString(newWBID);
            return false;
        }
        // Assumption here is that the A/B objectwidgets and the textwidget
        // are pristine in the sense that we may freely change their local IDs.
        objWidgetA->setLocalID(newWAID);
        objWidgetB->setLocalID(newWBID);
        FloatingTextWidget *ft = pMessage->floatingTextWidget();
        if (ft == NULL) {
            DEBUG(DBG_SRC) << "FloatingTextWidget of Message is NULL";
        }
        else if (ft->id() == Uml::ID::None) {
            ft->setID(UniqueID::gen());
        }
        else {
            Uml::ID::Type newTextID = m_doc->assignNewID(ft->id());
            ft->setID(newTextID);
        }
        m_MessageList.append(pMessage);
    }
    break;

    case WidgetBase::wt_Object: {
        ObjectWidget* pObjectWidget = static_cast<ObjectWidget*>(pWidget);
        if (pObjectWidget == NULL) {
            DEBUG(DBG_SRC) << "pObjectWidget is NULL";
            return false;
        }
        Uml::ID::Type nNewLocalID = localID();
        Uml::ID::Type nOldLocalID = pObjectWidget->localID();
        m_pIDChangesLog->addIDChange(nOldLocalID, nNewLocalID);
        pObjectWidget->setLocalID(nNewLocalID);
        UMLObject *pObject = m_doc->findObjectById(pWidget->id());
        if (!pObject) {
            DEBUG(DBG_SRC) << "Cannot find UMLObject";
            return false;
        }
        pWidget->setUMLObject(pObject);
        m_WidgetList.append(pWidget);
    }
    break;

    case WidgetBase::wt_Precondition: {
        ObjectWidget* pObjectWidget = static_cast<ObjectWidget*>(pWidget);
        if (pObjectWidget == NULL) {
            DEBUG(DBG_SRC) << "pObjectWidget is NULL";
            return false;
        }
        Uml::ID::Type newID = log->findNewID(pWidget->id());
        if (newID == Uml::ID::None) {
            return false;
        }
        pObjectWidget->setID(newID);
        Uml::ID::Type nNewLocalID = localID();
        Uml::ID::Type nOldLocalID = pObjectWidget->localID();
        m_pIDChangesLog->addIDChange(nOldLocalID, nNewLocalID);
        pObjectWidget->setLocalID(nNewLocalID);
        UMLObject *pObject = m_doc->findObjectById(newID);
        if (!pObject) {
            DEBUG(DBG_SRC) << "Cannot find UMLObject";
            return false;
        }
        pWidget->setUMLObject(pObject);
        m_WidgetList.append(pWidget);
    }
    break;

    case WidgetBase::wt_Pin:
    case WidgetBase::wt_CombinedFragment:
    case WidgetBase::wt_Signal: {
        ObjectWidget* pObjectWidget = static_cast<ObjectWidget*>(pWidget);
        if (pObjectWidget == NULL) {
            DEBUG(DBG_SRC) << "pObjectWidget is NULL";
            return false;
        }
        Uml::ID::Type newID = log->findNewID(pWidget->id());
        if (newID == Uml::ID::None) {
            return false;
        }
        pObjectWidget->setID(newID);
        Uml::ID::Type nNewLocalID = localID();
        Uml::ID::Type nOldLocalID = pObjectWidget->localID();
        m_pIDChangesLog->addIDChange(nOldLocalID, nNewLocalID);
        pObjectWidget->setLocalID(nNewLocalID);
        UMLObject *pObject = m_doc->findObjectById(newID);
        if (!pObject) {
            DEBUG(DBG_SRC) << "Cannot find UMLObject";
            return false;
        }
        pWidget->setUMLObject(pObject);
        m_WidgetList.append(pWidget);
    }
    break;

    default:
        DEBUG(DBG_SRC) << "Trying to add an invalid widget type";
        return false;
        break;
    }

    return true;
}

/**
 * Adds an association to the view from the given data.
 * Use this method when pasting.
 */
bool UMLScene::addAssociation(AssociationWidget* pAssoc, bool isPasteOperation)
{
    if (!pAssoc) {
        return false;
    }
    const Uml::AssociationType::Enum assocType = pAssoc->associationType();

    if (isPasteOperation) {
        IDChangeLog * log = m_doc->changeLog();

        if (!log) {
            return false;
        }

        Uml::ID::Type ida = Uml::ID::None, idb = Uml::ID::None;
        if (type() == DiagramType::Collaboration || type() == DiagramType::Sequence) {
            //check local log first
            ida = m_pIDChangesLog->findNewID(pAssoc->widgetIDForRole(Uml::RoleType::A));
            idb = m_pIDChangesLog->findNewID(pAssoc->widgetIDForRole(Uml::RoleType::B));
            //if either is still not found and assoc type is anchor
            //we are probably linking to a notewidet - else an error
            if (ida == Uml::ID::None && assocType == Uml::AssociationType::Anchor)
                ida = log->findNewID(pAssoc->widgetIDForRole(Uml::RoleType::A));
            if (idb == Uml::ID::None && assocType == Uml::AssociationType::Anchor)
                idb = log->findNewID(pAssoc->widgetIDForRole(Uml::RoleType::B));
        } else {
            Uml::ID::Type oldIdA = pAssoc->widgetIDForRole(Uml::RoleType::A);
            Uml::ID::Type oldIdB = pAssoc->widgetIDForRole(Uml::RoleType::B);
            ida = log->findNewID(oldIdA);
            if (ida == Uml::ID::None) {  // happens after a cut
                if (oldIdA == Uml::ID::None) {
                    return false;
                }
                ida = oldIdA;
            }
            idb = log->findNewID(oldIdB);
            if (idb == Uml::ID::None) {  // happens after a cut
                if (oldIdB == Uml::ID::None) {
                    return false;
                }
                idb = oldIdB;
            }
        }
        if (ida == Uml::ID::None || idb == Uml::ID::None) {
            return false;
        }
        // cant do this anymore.. may cause problem for pasting
        //      pAssoc->setWidgetID(ida, A);
        //      pAssoc->setWidgetID(idb, B);
        pAssoc->setWidgetForRole(findWidget(ida), Uml::RoleType::A);
        pAssoc->setWidgetForRole(findWidget(idb), Uml::RoleType::B);
    }

    UMLWidget * pWidgetA = findWidget(pAssoc->widgetIDForRole(Uml::RoleType::A));
    UMLWidget * pWidgetB = findWidget(pAssoc->widgetIDForRole(Uml::RoleType::B));
    //make sure valid widget ids
    if (!pWidgetA || !pWidgetB) {
        return false;
    }

    //make sure there isn't already the same assoc

    foreach(AssociationWidget* assocwidget, m_AssociationList) {
        if (*pAssoc == *assocwidget)
            // this is nuts. Paste operation wants to know if 'true'
            // for duplicate, but loadFromXMI needs 'false' value
            return (isPasteOperation ? true : false);
    }

    m_AssociationList.append(pAssoc);

    FloatingTextWidget *ft[5] = { pAssoc->nameWidget(),
                                  pAssoc->roleWidget(Uml::RoleType::A),
                                  pAssoc->roleWidget(Uml::RoleType::B),
                                  pAssoc->multiplicityWidget(Uml::RoleType::A),
                                  pAssoc->multiplicityWidget(Uml::RoleType::B)
    };
    for (int i = 0; i < 5; i++) {
        FloatingTextWidget *flotxt = ft[i];
        if (flotxt) {
            flotxt->updateGeometry();
            addWidget(flotxt);
        }
    }

    return true;
}

/**
 * Activate the view after a load a new file
 */
void UMLScene::activateAfterLoad(bool bUseLog)
{
    if (m_isActivated) {
        return;
    }
    if (bUseLog) {
        beginPartialWidgetPaste();
    }

    //now activate them all
    activate();

    if (bUseLog) {
        endPartialWidgetPaste();
    }
    m_view->centerOn(0, 0);
    m_isActivated = true;
}

void UMLScene::beginPartialWidgetPaste()
{
    delete m_pIDChangesLog;
    m_pIDChangesLog = 0;

    m_pIDChangesLog = new IDChangeLog();
    m_bPaste = true;
}

void UMLScene::endPartialWidgetPaste()
{
    delete m_pIDChangesLog;
    m_pIDChangesLog = 0;

    m_bPaste = false;
}

/**
 * Removes a AssociationWidget from a diagram
 * Physically deletes the AssociationWidget passed in.
 *
 * @param pAssoc  Pointer to the AssociationWidget.
 */
void UMLScene::removeAssoc(AssociationWidget* pAssoc)
{
    if (!pAssoc)
        return;

    emit sigAssociationRemoved(pAssoc);

    pAssoc->cleanup();
    m_AssociationList.removeAll(pAssoc);
    pAssoc->deleteLater();
    m_doc->setModified();
}

/**
 * Removes an AssociationWidget from the association list
 * and removes the corresponding UMLAssociation from the current UMLDoc.
 */
void UMLScene::removeAssocInViewAndDoc(AssociationWidget* a)
{
    // For umbrello 1.2, UMLAssociations can only be removed in two ways:
    // 1. Right click on the assocwidget in the view and select Delete
    // 2. Go to the Class Properties page, select Associations, right click
    //    on the association and select Delete
    if (!a)
        return;
    if (a->associationType() == Uml::AssociationType::Containment) {
        UMLObject *objToBeMoved = a->widgetForRole(Uml::RoleType::B)->umlObject();
        if (objToBeMoved != NULL) {
            UMLListView *lv = UMLApp::app()->listView();
            lv->moveObject(objToBeMoved->id(),
                           Model_Utils::convert_OT_LVT(objToBeMoved),
                           lv->theLogicalView());
            // UMLListView::moveObject() will delete the containment
            // AssociationWidget via UMLScene::updateContainment().
        } else {
            DEBUG(DBG_SRC) << "removeAssocInViewAndDoc(containment): "
                           << "objB is NULL";
        }
    } else {
        // Remove assoc in doc.
        m_doc->removeAssociation(a->association());
        // Remove assoc in view.
        removeAssoc(a);
    }
}

/**
 * Removes all the associations related to Widget.
 *
 * @param widget  Pointer to the widget to remove.
 */
void UMLScene::removeAssociations(UMLWidget* widget)
{
    foreach(AssociationWidget* assocwidget, m_AssociationList) {
        if (assocwidget->containsAsEndpoint(widget)) {
            removeAssoc(assocwidget);
        }
    }
}

/**
 * Sets each association as selected if the widgets it associates are selected
 *
 * @param bSelect  True to select, false for unselect
 */
void UMLScene::selectAssociations(bool bSelect)
{
    foreach(AssociationWidget* assocwidget, m_AssociationList) {
        UMLWidget *widA = assocwidget->widgetForRole(Uml::RoleType::A);
        UMLWidget *widB = assocwidget->widgetForRole(Uml::RoleType::B);
        if (bSelect &&
                widA && widA->isSelected() &&
                widB && widB->isSelected()) {
            assocwidget->setSelected(true);
        } else {
            assocwidget->setSelected(false);
        }
    }
}

/**
 * Fills Associations with all the associations that includes a widget related to object
 */
void UMLScene::getWidgetAssocs(UMLObject* Obj, AssociationWidgetList & Associations)
{
    if (! Obj)
        return;

    foreach(AssociationWidget* assocwidget, m_AssociationList) {
        if (assocwidget->widgetForRole(Uml::RoleType::A)->umlObject() == Obj ||
            assocwidget->widgetForRole(Uml::RoleType::B)->umlObject() == Obj)
            Associations.append(assocwidget);
    }

}

/**
 * Removes All the associations of the diagram
 */
void UMLScene::removeAllAssociations()
{
    //Remove All association widgets
    foreach(AssociationWidget* assocwidget, m_AssociationList) {
        removeAssoc(assocwidget);
    }

    qDeleteAll(m_AssociationList);
    m_AssociationList.clear();
}

/**
 * Removes All the widgets of the diagram
 */
void UMLScene::removeAllWidgets()
{
    // Remove widgets.
    foreach(UMLWidget* temp, m_WidgetList) {
        // I had to take this condition back in, else umbrello
        // crashes on exit. Still to be analyzed.  --okellogg
        if (!(temp->baseType() == WidgetBase::wt_Text &&
              ((FloatingTextWidget *)temp)->textRole() != TextRole::Floating)) {
            removeWidget(temp);
        }
    }

    qDeleteAll(m_WidgetList);
    m_WidgetList.clear();
}

/**
 * Calls the same method in the DocWindow.
 */
void UMLScene::showDocumentation(bool overwrite)
{
    UMLApp::app()->docWindow()->showDocumentation(this, overwrite);
}

/**
 * Calls the same method in the DocWindow.
 */
void UMLScene::showDocumentation(UMLObject* object, bool overwrite)
{
    UMLApp::app()->docWindow()->showDocumentation(object, overwrite);
}

/**
 * Calls the same method in the DocWindow.
 */
void UMLScene::showDocumentation(UMLWidget* widget, bool overwrite)
{
    UMLApp::app()->docWindow()->showDocumentation(widget, overwrite);
}

/**
 * Calls the same method in the DocWindow.
 */
void UMLScene::showDocumentation(AssociationWidget* widget, bool overwrite)
{
    UMLApp::app()->docWindow()->showDocumentation(widget, overwrite);
}

/**
 * Calls the same method in the DocWindow.
 */
void UMLScene::updateDocumentation(bool clear)
{
    UMLApp::app()->docWindow()->updateDocumentation(clear);
}

/**
 * Refreshes containment association, i.e. removes possible old
 * containment and adds new containment association if applicable.
 *
 * @param self  Pointer to the contained object for which
 *   the association to the containing object is
 *   recomputed.
 */
void UMLScene::updateContainment(UMLCanvasObject *self)
{
    if (self == NULL)
        return;
    // See if the object has a widget representation in this view.
    // While we're at it, also see if the new parent has a widget here.
    UMLWidget *selfWidget = NULL, *newParentWidget = NULL;
    UMLPackage *newParent = self->umlPackage();
    foreach(UMLWidget* w, m_WidgetList) {
        UMLObject *o = w->umlObject();
        if (o == self)
            selfWidget = w;
        else if (newParent != NULL && o == newParent)
            newParentWidget = w;
    }
    if (selfWidget == NULL)
        return;
    // Remove possibly obsoleted containment association.
    foreach(AssociationWidget* a, m_AssociationList) {
        if (a->associationType() != Uml::AssociationType::Containment)
            continue;
        // Container is at role A, containee at B.
        // We only look at association for which we are B.
        UMLWidget *wB = a->widgetForRole(Uml::RoleType::B);
        UMLObject *roleBObj = wB->umlObject();
        if (roleBObj != self)
            continue;
        UMLWidget *wA = a->widgetForRole(Uml::RoleType::A);
        UMLObject *roleAObj = wA->umlObject();
        if (roleAObj == newParent) {
            // Wow, all done. Great!
            return;
        }
        removeAssoc(a);  // AutoDelete is true
        // It's okay to break out because there can only be a single
        // containing object.
        break;
    }
    if (newParentWidget == NULL)
        return;
    // Create the new containment association.
    AssociationWidget *a = AssociationWidget::create
                             (this, newParentWidget,
                              Uml::AssociationType::Containment, selfWidget);
    m_AssociationList.append(a);
}

/**
 * Creates automatically any Associations that the given @ref UMLWidget
 * may have on any diagram.  This method is used when you just add the UMLWidget
 * to a diagram.
 */
void UMLScene::createAutoAssociations(UMLWidget * widget)
{
    if (widget == NULL ||
        (m_Type != Uml::DiagramType::Class &&
         m_Type != Uml::DiagramType::Component &&
         m_Type != Uml::DiagramType::Deployment
         && m_Type != Uml::DiagramType::EntityRelationship))
        return;
    // Recipe:
    // If this widget has an underlying UMLCanvasObject then
    //   for each of the UMLCanvasObject's UMLAssociations
    //     if umlassoc's "other" role has a widget representation on this view then
    //       if the AssocWidget does not already exist then
    //         if the assoc type is permitted in the current diagram type then
    //           create the AssocWidget
    //         end if
    //       end if
    //     end if
    //   end loop
    //   Do createAutoAttributeAssociations()
    //   if this object is capable of containing nested objects then
    //     for each of the object's containedObjects
    //       if the containedObject has a widget representation on this view then
    //         if the containedWidget is not physically located inside this widget
    //           create the containment AssocWidget
    //         end if
    //       end if
    //     end loop
    //   end if
    //   if the UMLCanvasObject has a parentPackage then
    //     if the parentPackage has a widget representation on this view then
    //       create the containment AssocWidget
    //     end if
    //   end if
    // end if
    UMLObject *tmpUmlObj = widget->umlObject();
    if (tmpUmlObj == NULL)
        return;
    UMLCanvasObject *umlObj = dynamic_cast<UMLCanvasObject*>(tmpUmlObj);
    if (umlObj == NULL)
        return;
    const UMLAssociationList& umlAssocs = umlObj->getAssociations();

    Uml::ID::Type myID = umlObj->id();
    foreach(UMLAssociation* assoc, umlAssocs) {
        UMLCanvasObject *other = NULL;
        UMLObject *roleAObj = assoc->getObject(Uml::RoleType::A);
        if (roleAObj == NULL) {
            DEBUG(DBG_SRC) << "roleA object is NULL at UMLAssoc "
                           << Uml::ID::toString(assoc->id());
            continue;
        }
        UMLObject *roleBObj = assoc->getObject(Uml::RoleType::B);
        if (roleBObj == NULL) {
            DEBUG(DBG_SRC) << "roleB object is NULL at UMLAssoc "
                           << Uml::ID::toString(assoc->id());
            continue;
        }
        if (roleAObj->id() == myID) {
            other = static_cast<UMLCanvasObject*>(roleBObj);
        } else if (roleBObj->id() == myID) {
            other = static_cast<UMLCanvasObject*>(roleAObj);
        } else {
            DEBUG(DBG_SRC) << "Can not find own object "
                           << Uml::ID::toString(myID) << " in UMLAssoc "
                           << Uml::ID::toString(assoc->id());
            continue;
        }
        // Now that we have determined the "other" UMLObject, seek it in
        // this view's UMLWidgets.
        Uml::ID::Type otherID = other->id();

        bool breakFlag = false;
        UMLWidget* pOtherWidget = 0;
        foreach(pOtherWidget,  m_WidgetList) {
            if (pOtherWidget->id() == otherID) {
                breakFlag = true;
                break;
            }
        }
        if (!breakFlag)
            continue;
        // Both objects are represented in this view:
        // Assign widget roles as indicated by the UMLAssociation.
        UMLWidget *widgetA, *widgetB;
        if (myID == roleAObj->id()) {
            widgetA = widget;
            widgetB = pOtherWidget;
        } else {
            widgetA = pOtherWidget;
            widgetB = widget;
        }
        // Check that the assocwidget does not already exist.
        Uml::AssociationType::Enum assocType = assoc->getAssocType();
        AssociationWidget * assocwidget = findAssocWidget(assocType, widgetA, widgetB);
        if (assocwidget) {
            assocwidget->calculateEndingPoints();  // recompute assoc lines
            continue;
        }
        // Check that the assoc is allowed.
        if (!AssocRules::allowAssociation(assocType, widgetA, widgetB)) {
            DEBUG(DBG_SRC) << "not transferring assoc "
                           << "of type " << assocType;
            continue;
        }

        // Create the AssociationWidget.
        assocwidget = AssociationWidget::create(this);
        assocwidget->setWidgetForRole(widgetA, Uml::RoleType::A);
        assocwidget->setWidgetForRole(widgetB, Uml::RoleType::B);
        assocwidget->setAssociationType(assocType);
        assocwidget->setUMLObject(assoc);
        // Call calculateEndingPoints() before setting the FloatingTexts
        // because their positions are computed according to the
        // assocwidget line positions.
        assocwidget->calculateEndingPoints();
        assocwidget->syncToModel();
        assocwidget->setActivated(true);
        if (! addAssociation(assocwidget))
            delete assocwidget;
    }

    createAutoAttributeAssociations(widget);

    if (m_Type == Uml::DiagramType::EntityRelationship) {
        createAutoConstraintAssociations(widget);
    }

    // if this object is capable of containing nested objects then
    UMLObject::ObjectType t = umlObj->baseType();
    if (t == UMLObject::ot_Package || t == UMLObject::ot_Class ||
        t == UMLObject::ot_Interface || t == UMLObject::ot_Component) {
        // for each of the object's containedObjects
        UMLPackage *umlPkg = static_cast<UMLPackage*>(umlObj);
        UMLObjectList lst = umlPkg->containedObjects();
        foreach(UMLObject* obj,  lst) {
            // if the containedObject has a widget representation on this view then
            Uml::ID::Type id = obj->id();
            foreach(UMLWidget *w, m_WidgetList) {
                if (w->id() != id)
                    continue;
                // if the containedWidget is not physically located inside this widget
                if (widget->rect().contains(w->rect()))
                    continue;
                // create the containment AssocWidget
                AssociationWidget *a = AssociationWidget::create(this, widget,
                        Uml::AssociationType::Containment, w);
                a->calculateEndingPoints();
                a->setActivated(true);
                if (! addAssociation(a))
                    delete a;
            }
        }
    }
    // if the UMLCanvasObject has a parentPackage then
    UMLPackage *parent = umlObj->umlPackage();
    if (parent == NULL)
        return;
    // if the parentPackage has a widget representation on this view then
    Uml::ID::Type pkgID = parent->id();

    bool breakFlag = false;
    UMLWidget* pWidget = 0;
    foreach(pWidget, m_WidgetList) {
        if (pWidget->id() == pkgID) {
            breakFlag = true;
            break;
        }
    }
    if (!breakFlag || pWidget->rect().contains(widget->rect()))
        return;
    // create the containment AssocWidget
    AssociationWidget *a = AssociationWidget::create(this, pWidget, Uml::AssociationType::Containment, widget);
    if (! addAssociation(a))
        delete a;
}

/**
 * If the m_Type of the given widget is WidgetBase::wt_Class then
 * iterate through the class' attributes and create an
 * association to each attribute type widget that is present
 * on the current diagram.
 */
void UMLScene::createAutoAttributeAssociations(UMLWidget *widget)
{
    if (widget == NULL || m_Type != Uml::DiagramType::Class || !m_Options.classState.showAttribAssocs)
        return;

    // Pseudocode:
    //   if the underlying model object is really a UMLClassifier then
    //     for each of the UMLClassifier's UMLAttributes
    //       if the attribute type has a widget representation on this view then
    //         if the AssocWidget does not already exist then
    //           if the current diagram type permits compositions then
    //             create a composition AssocWidget
    //           end if
    //         end if
    //       end if
    //       if the attribute type is a Datatype then
    //         if the Datatype is a reference (pointer) type then
    //           if the referenced type has a widget representation on this view then
    //             if the AssocWidget does not already exist then
    //               if the current diagram type permits aggregations then
    //                 create an aggregation AssocWidget from the ClassifierWidget to the
    //                                                 widget of the referenced type
    //               end if
    //             end if
    //           end if
    //         end if
    //       end if
    //     end loop
    //   end if
    //
    // Implementation:
    UMLObject *tmpUmlObj = widget->umlObject();
    if (tmpUmlObj == NULL)
        return;
    // if the underlying model object is really a UMLClassifier then
    if (tmpUmlObj->baseType() == UMLObject::ot_Datatype) {
        UMLClassifier *dt = static_cast<UMLClassifier*>(tmpUmlObj);
        while (dt->originType() != NULL) {
            tmpUmlObj = dt->originType();
            if (tmpUmlObj->baseType() != UMLObject::ot_Datatype)
                break;
            dt = static_cast<UMLClassifier*>(tmpUmlObj);
        }
    }
    if (tmpUmlObj->baseType() != UMLObject::ot_Class)
        return;
    UMLClassifier * klass = static_cast<UMLClassifier*>(tmpUmlObj);
    // for each of the UMLClassifier's UMLAttributes
    UMLAttributeList attrList = klass->getAttributeList();
    foreach(UMLAttribute* attr, attrList) {
        createAutoAttributeAssociation(attr->getType(), attr, widget);
        /*
         * The following code from attachment 19935 of http://bugs.kde.org/140669
         * creates Aggregation/Composition to the template parameters.
         * The current solution uses Dependency instead, see handling of template
         * instantiation at Import_Utils::createUMLObject().
        UMLClassifierList templateList = attr->getTemplateParams();
        for (UMLClassifierListIt it(templateList); it.current(); ++it) {
            createAutoAttributeAssociation(it, attr, widget);
        }
         */
    }
}

/**
 * Create an association with the attribute attr associated with the UMLWidget
 * widget if the UMLClassifier type is present on the current diagram.
 */
void UMLScene::createAutoAttributeAssociation(UMLClassifier *type, UMLAttribute *attr,
                                              UMLWidget *widget /*, UMLClassifier * klass*/)
{
    if (type == NULL) {
        // DEBUG(DBG_SRC) << klass->getName() << ": type is NULL for "
        //                << "attribute " << attr->getName();
        return;
    }
    Uml::AssociationType::Enum assocType = Uml::AssociationType::Composition;
    UMLWidget *w = findWidget(type->id());
    // if the attribute type has a widget representation on this view
    if (w) {
        AssociationWidget *a = findAssocWidget(widget, w, attr->name());
        if (a == NULL &&
            // if the current diagram type permits compositions
            AssocRules::allowAssociation(assocType, widget, w)) {
            // Create a composition AssocWidget, or, if the attribute type is
            // stereotyped <<CORBAInterface>>, create a UniAssociation widget.
            if (type->stereotype() == "CORBAInterface")
                assocType = Uml::AssociationType::UniAssociation;
            a = AssociationWidget::create(this, widget, assocType, w, attr);
            a->setVisibility(attr->visibility(), Uml::RoleType::B);
            /*
            if (assocType == Uml::AssociationType::Aggregation || assocType == Uml::AssociationType::UniAssociation)
            a->setMulti("0..1", Uml::RoleType::B);
            */
            a->setRoleName(attr->name(), Uml::RoleType::B);
            a->setActivated(true);
            if (! addAssociation(a))
                delete a;
        }
    }
    // if the attribute type is a Datatype then
    if (type->baseType() == UMLObject::ot_Datatype) {
        UMLClassifier *dt = static_cast<UMLClassifier*>(type);
        // if the Datatype is a reference (pointer) type
        if (dt->isReference()) {
            //Uml::AssociationType::Enum assocType = Uml::AssociationType::Composition;
            UMLClassifier *c = dt->originType();
            UMLWidget *w = c ? findWidget(c->id()) : 0;
            // if the referenced type has a widget representation on this view
            if (w) {
                AssociationWidget *a = findAssocWidget(widget, w, attr->name());
                if (a == NULL &&
                        // if the current diagram type permits aggregations
                        AssocRules::allowAssociation(Uml::AssociationType::Aggregation, widget, w)) {
                    // create an aggregation AssocWidget from the ClassifierWidget
                    // to the widget of the referenced type
                    a = AssociationWidget::create (this, widget,
                                                   Uml::AssociationType::Aggregation, w, attr);
                    a->setVisibility(attr->visibility(), Uml::RoleType::B);
                    //a->setChangeability(true, Uml::RoleType::B);
                    a->setMultiplicity("0..1", Uml::RoleType::B);
                    a->setRoleName(attr->name(), Uml::RoleType::B);
                    a->setActivated(true);
                    if (! addAssociation(a))
                        delete a;
                }
            }
        }
    }
}

void UMLScene::createAutoConstraintAssociations(UMLWidget *widget)
{
    if (widget == NULL || m_Type != Uml::DiagramType::EntityRelationship)
        return;

    // Pseudocode:
    //   if the underlying model object is really a UMLEntity then
    //     for each of the UMLEntity's UMLForeignKeyConstraint's
    //       if the attribute type has a widget representation on this view then
    //         if the AssocWidget does not already exist then
    //           if the current diagram type permits relationships then
    //             create a relationship AssocWidget
    //           end if
    //         end if
    //       end if

    UMLObject *tmpUmlObj = widget->umlObject();
    if (tmpUmlObj == NULL)
        return;
    // check if the underlying model object is really a UMLEntity
    UMLCanvasObject *umlObj = dynamic_cast<UMLCanvasObject*>(tmpUmlObj);
    if (umlObj == NULL)
        return;
    // finished checking whether this widget has a UMLCanvas Object

    if (tmpUmlObj->baseType() != UMLObject::ot_Entity)
        return;
    UMLEntity *entity = static_cast<UMLEntity*>(tmpUmlObj);

    // for each of the UMLEntity's UMLForeignKeyConstraints
    UMLClassifierListItemList constrList = entity->getFilteredList(UMLObject::ot_ForeignKeyConstraint);

    foreach(UMLClassifierListItem* cli, constrList) {
        UMLEntityConstraint *eConstr = static_cast<UMLEntityConstraint*>(cli);

        UMLForeignKeyConstraint* fkc = static_cast<UMLForeignKeyConstraint*>(eConstr);
        if (fkc == NULL) {
            return;
        }

        UMLEntity* refEntity = fkc->getReferencedEntity();
        if (refEntity == NULL) {
            return;
        }

        createAutoConstraintAssociation(refEntity, fkc, widget);
    }
}

void UMLScene::createAutoConstraintAssociation(UMLEntity* refEntity, UMLForeignKeyConstraint* fkConstraint, UMLWidget* widget)
{
    if (refEntity == NULL) {
        return;
    }

    Uml::AssociationType::Enum assocType = Uml::AssociationType::Relationship;
    UMLWidget *w = findWidget(refEntity->id());
    AssociationWidget *aw = NULL;

    if (w) {
        aw = findAssocWidget(widget, w, fkConstraint->name());
        if (aw == NULL &&
            // if the current diagram type permits relationships
            AssocRules::allowAssociation(assocType, widget, w)) {

            // for foreign key contstraint, we need to create the association type Uml::AssociationType::Relationship.
            // The referenced entity is the "1" part (Role A) and the entity holding the relationship is the "many" part. (Role B)
            AssociationWidget *a = AssociationWidget::create(this, w, assocType, widget);
            a->setUMLObject(fkConstraint);
            //a->setVisibility(attr->getVisibility(), Uml::RoleType::B);
            a->setRoleName(fkConstraint->name(), Uml::RoleType::B);
            a->setActivated(true);
            if (! addAssociation(a))
                delete a;
        }
    }

}

/**
 * Find the maximum bounding rectangle of FloatingTextWidget widgets.
 * Auxiliary to copyAsImage().
 *
 * @param ft Pointer to the FloatingTextWidget widget to consider.
 * @param px  X coordinate of lower left corner. This value will be
 *            updated if the X coordinate of the lower left corner
 *            of ft is smaller than the px value passed in.
 * @param py  Y coordinate of lower left corner. This value will be
 *            updated if the Y coordinate of the lower left corner
 *            of ft is smaller than the py value passed in.
 * @param qx  X coordinate of upper right corner. This value will be
 *            updated if the X coordinate of the upper right corner
 *            of ft is larger than the qx value passed in.
 * @param qy  Y coordinate of upper right corner. This value will be
 *            updated if the Y coordinate of the upper right corner
 *            of ft is larger than the qy value passed in.
 */
void UMLScene::findMaxBoundingRectangle(const FloatingTextWidget* ft, UMLSceneValue& px, UMLSceneValue& py, UMLSceneValue& qx, UMLSceneValue& qy)
{
    if (ft == NULL || !ft->isVisible())
        return;

    UMLSceneValue x = ft->x();
    UMLSceneValue y = ft->y();
    UMLSceneValue x1 = x + ft->width() - 1;
    UMLSceneValue y1 = y + ft->height() - 1;

    if (px == -1 || x < px)
        px = x;
    if (py == -1 || y < py)
        py = y;
    if (qx == -1 || x1 > qx)
        qx = x1;
    if (qy == -1 || y1 > qy)
        qy = y1;
}

/**
 * Returns the PNG picture of the paste operation.
 */
void UMLScene::copyAsImage(QPixmap*& pix)
{
    //get the smallest rect holding the diagram
    QRectF rect = diagramRect();
    QPixmap diagram(rect.width(), rect.height());

    //only draw what is selected
    m_bDrawSelectedOnly = true;
    selectAssociations(true);
    getDiagram(diagram, rect);

    //now get the selection cut
    UMLSceneValue px = -1, py = -1, qx = -1, qy = -1;

    //first get the smallest rect holding the widgets
    foreach(UMLWidget* temp, m_selectedList) {
        UMLSceneValue x = temp->x();
        UMLSceneValue y = temp->y();
        UMLSceneValue x1 = x + temp->width() - 1;
        UMLSceneValue y1 = y + temp->height() - 1;
        if (px == -1 || x < px) {
            px = x;
        }
        if (py == -1 || y < py) {
            py = y;
        }
        if (qx == -1 || x1 > qx) {
            qx = x1;
        }
        if (qy == -1 || y1 > qy) {
            qy = y1;
        }
    }

    //also take into account any text lines in assocs or messages

    //get each type of associations
    //This needs to be reimplemented to increase the rectangle
    //if a part of any association is not included
    foreach(AssociationWidget *a, m_AssociationList) {
        if (! a->isSelected())
            continue;
        const FloatingTextWidget* multiA = a->multiplicityWidget(Uml::RoleType::A);
        const FloatingTextWidget* multiB = a->multiplicityWidget(Uml::RoleType::B);
        const FloatingTextWidget* roleA = a->roleWidget(Uml::RoleType::A);
        const FloatingTextWidget* roleB = a->roleWidget(Uml::RoleType::B);
        const FloatingTextWidget* changeA = a->changeabilityWidget(Uml::RoleType::A);
        const FloatingTextWidget* changeB = a->changeabilityWidget(Uml::RoleType::B);
        findMaxBoundingRectangle(multiA, px, py, qx, qy);
        findMaxBoundingRectangle(multiB, px, py, qx, qy);
        findMaxBoundingRectangle(roleA, px, py, qx, qy);
        findMaxBoundingRectangle(roleB, px, py, qx, qy);
        findMaxBoundingRectangle(changeA, px, py, qx, qy);
        findMaxBoundingRectangle(changeB, px, py, qx, qy);
    }//end foreach

    QRectF imageRect;  //area with respect to diagramRect()
    //i.e. all widgets on the scene.  Was previously with
    //respect to whole scene

    imageRect.setLeft(px - rect.left());
    imageRect.setTop(py - rect.top());
    imageRect.setRight(qx - rect.left());
    imageRect.setBottom(qy - rect.top());

    pix = new QPixmap(imageRect.width(), imageRect.height());
    QPainter output(pix);
    output.drawPixmap(QPoint(0, 0), diagram, imageRect);
    m_bDrawSelectedOnly = false;
}

/**
 * Reset the toolbar.
 */
void UMLScene::resetToolbar()
{
    emit sigResetToolBar();
}

/**
 * Event handler for context menu events.
 */
void UMLScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
{
    if (onItem(contextMenuEvent->scenePos())) {
        // forward the event to the item
        QGraphicsScene::contextMenuEvent(contextMenuEvent);
    }
    else {
        // set the position for the eventually created widget
        setPos(contextMenuEvent->scenePos());
        setMenu(contextMenuEvent->screenPos());
        contextMenuEvent->accept();
    }
}

/**
 * Sets the popup menu to use when clicking on a diagram background
 * (rather than a widget or listView).
 */
void UMLScene::setMenu(const QPoint& pos)
{
    ListPopupMenu::MenuType menu = ListPopupMenu::mt_Undefined;
    switch (type()) {
    case DiagramType::Class:
        menu = ListPopupMenu::mt_On_Class_Diagram;
        break;

    case DiagramType::UseCase:
        menu = ListPopupMenu::mt_On_UseCase_Diagram;
        break;

    case DiagramType::Sequence:
        menu = ListPopupMenu::mt_On_Sequence_Diagram;
        break;

    case DiagramType::Collaboration:
        menu = ListPopupMenu::mt_On_Collaboration_Diagram;
        break;

    case DiagramType::State:
        menu = ListPopupMenu::mt_On_State_Diagram;
        break;

    case DiagramType::Activity:
        menu = ListPopupMenu::mt_On_Activity_Diagram;
        break;

    case DiagramType::Component:
        menu = ListPopupMenu::mt_On_Component_Diagram;
        break;

    case DiagramType::Deployment:
        menu = ListPopupMenu::mt_On_Deployment_Diagram;
        break;

    case DiagramType::EntityRelationship:
        menu = ListPopupMenu::mt_On_EntityRelationship_Diagram;
        break;

    default:
        uWarning() << "unknown diagram type " << type();
        menu = ListPopupMenu::mt_Undefined;
        break;
    }//end switch
    if (menu != ListPopupMenu::mt_Undefined) {
        // DEBUG(DBG_SRC) << "create popup for MenuType " << ListPopupMenu::toString(menu);
        ListPopupMenu* popup = new ListPopupMenu(activeView(), menu, activeView());
        QAction *triggered = popup->exec(pos);
        slotMenuSelection(triggered);
    }
}

/**
 * Returns the status on whether in a paste state.
 *
 * @return Returns the status on whether in a paste state.
 */
bool UMLScene::getPaste() const
{
    return m_bPaste;
}

/**
 * Sets the status on whether in a paste state.
 */
void UMLScene::setPaste(bool paste)
{
    m_bPaste = paste;
}

/**
 * When a menu selection has been made on the menu
 * that this view created, this method gets called.
 */
void UMLScene::slotMenuSelection(QAction* action)
{
    ListPopupMenu::MenuType sel = ListPopupMenu::typeFromAction(action);
    switch (sel) {
    case ListPopupMenu::mt_Undo:
        UMLApp::app()->undo();
        break;

    case ListPopupMenu::mt_Redo:
        UMLApp::app()->redo();
        break;

    case ListPopupMenu::mt_Clear:
        clearDiagram();
        break;

    case ListPopupMenu::mt_Export_Image:
        m_pImageExporter->exportView();
        break;

    case ListPopupMenu::mt_Apply_Layout:
    case ListPopupMenu::mt_Apply_Layout1:
    case ListPopupMenu::mt_Apply_Layout2:
    case ListPopupMenu::mt_Apply_Layout3:
    case ListPopupMenu::mt_Apply_Layout4:
    case ListPopupMenu::mt_Apply_Layout5:
    case ListPopupMenu::mt_Apply_Layout6:
    case ListPopupMenu::mt_Apply_Layout7:
    case ListPopupMenu::mt_Apply_Layout8:
    case ListPopupMenu::mt_Apply_Layout9:
        {
            QVariant value = ListPopupMenu::dataFromAction(ListPopupMenu::dt_ApplyLayout, action);
            applyLayout(value.toString());
        }
        break;

    case ListPopupMenu::mt_FloatText:
        {
            FloatingTextWidget* ft = new FloatingTextWidget(this);
            ft->showChangeTextDialog();
            //if no text entered delete
            if (!FloatingTextWidget::isTextValid(ft->text())) {
                delete ft;
            } else {
                ft->setID(UniqueID::gen());
                setupNewWidget(ft);
            }
        }
        break;

    case ListPopupMenu::mt_UseCase:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_UseCase);
        break;

    case ListPopupMenu::mt_Actor:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Actor);
        break;

    case ListPopupMenu::mt_Class:
    case ListPopupMenu::mt_Object:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Class);
        break;

    case ListPopupMenu::mt_Package:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Package);
        break;

    case ListPopupMenu::mt_Component:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Component);
        break;

    case ListPopupMenu::mt_Node:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Node);
        break;

    case ListPopupMenu::mt_Artifact:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Artifact);
        break;

    case ListPopupMenu::mt_Interface:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Interface);
        break;

    case ListPopupMenu::mt_Enum:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Enum);
        break;

    case ListPopupMenu::mt_Entity:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Entity);
        break;

    case ListPopupMenu::mt_Category:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Category);
        break;

    case ListPopupMenu::mt_Datatype:
        m_bCreateObject = true;
        Object_Factory::createUMLObject(UMLObject::ot_Datatype);
        break;

    case ListPopupMenu::mt_Cut:
        //FIXME make this work for diagram's right click menu
        if (selectedWidgets().count() &&
            UMLApp::app()->editCutCopy(true)) {
            deleteSelection();
            m_doc->setModified(true);
        }
        break;

    case ListPopupMenu::mt_Copy:
        //FIXME make this work for diagram's right click menu
        selectedWidgets().count() && UMLApp::app()->editCutCopy(true);
        break;

    case ListPopupMenu::mt_Paste:
        m_PastePoint = m_Pos;
        m_Pos.setX(2000);
        m_Pos.setY(2000);
        UMLApp::app()->slotEditPaste();

        m_PastePoint.setX(0);
        m_PastePoint.setY(0);
        break;

    case ListPopupMenu::mt_Initial_State:
        {
            StateWidget* state = new StateWidget(this, StateWidget::Initial);
            setupNewWidget(state);
        }
        break;

    case ListPopupMenu::mt_End_State:
        {
            StateWidget* state = new StateWidget(this, StateWidget::End);
            setupNewWidget(state);
        }
        break;

    case ListPopupMenu::mt_Junction:
        {
            StateWidget* state = new StateWidget(this, StateWidget::Junction);
            setupNewWidget(state);
        }
        break;

    case ListPopupMenu::mt_DeepHistory:
        {
            StateWidget* state = new StateWidget(this, StateWidget::DeepHistory);
            setupNewWidget(state);
        }
        break;

    case ListPopupMenu::mt_ShallowHistory:
        {
            StateWidget* state = new StateWidget(this, StateWidget::ShallowHistory);
            setupNewWidget(state);
        }
        break;

    case ListPopupMenu::mt_Choice:
        {
            StateWidget* state = new StateWidget(this, StateWidget::Choice);
            setupNewWidget(state);
        }
        break;

    case ListPopupMenu::mt_StateFork:
        {
            StateWidget* state = new StateWidget(this, StateWidget::Fork);
            setupNewWidget(state);
        }
        break;

    case ListPopupMenu::mt_StateJoin:
        {
            StateWidget* state = new StateWidget(this, StateWidget::Join);
            setupNewWidget(state);
        }
        break;

    case ListPopupMenu::mt_State:
        {
            bool ok = false;
            QString name = KInputDialog::getText(i18n("Enter State Name"),
                                                 i18n("Enter the name of the new state:"),
                                                 i18n("new state"), &ok, UMLApp::app());
            if (ok) {
                StateWidget* state = new StateWidget(this);
                state->setName(name);
                setupNewWidget(state);
            }
        }
        break;

    case ListPopupMenu::mt_Initial_Activity:
        {
            ActivityWidget* activity = new ActivityWidget(this, ActivityWidget::Initial);
            setupNewWidget(activity);
        }
        break;

    case ListPopupMenu::mt_End_Activity:
        {
            ActivityWidget* activity = new ActivityWidget(this, ActivityWidget::End);
            setupNewWidget(activity);
        }
        break;

    case ListPopupMenu::mt_Branch:
        {
            ActivityWidget* activity = new ActivityWidget(this, ActivityWidget::Branch);
            setupNewWidget(activity);
        }
        break;

    case ListPopupMenu::mt_Activity:
        {
            bool ok = false;
            QString name = KInputDialog::getText(i18n("Enter Activity Name"),
                                                 i18n("Enter the name of the new activity:"),
                                                 i18n("new activity"), &ok, UMLApp::app());
            if (ok) {
                ActivityWidget* activity = new ActivityWidget(this, ActivityWidget::Normal);
                activity->setName(name);
                setupNewWidget(activity);
            }
        }
        break;

    case ListPopupMenu::mt_SnapToGrid:
        toggleSnapToGrid();
        m_doc->setModified();
        break;

    case ListPopupMenu::mt_ShowSnapGrid:
        toggleShowGrid();
        m_doc->setModified();
        break;

    case ListPopupMenu::mt_ShowDocumentationIndicator:
        setShowDocumentationIndicator(!isShowDocumentationIndicator());
        update();
        break;

    case ListPopupMenu::mt_Properties:
        if (m_view->showPropDialog() == true)
            m_doc->setModified();
        break;

    case ListPopupMenu::mt_Delete:
        m_doc->removeDiagram(ID());
        break;

    case ListPopupMenu::mt_Rename:
        {
            bool ok = false;
            QString newName = KInputDialog::getText(i18n("Enter Diagram Name"),
                                                    i18n("Enter the new name of the diagram:"),
                                                    name(), &ok, UMLApp::app());
            if (ok) {
                setName(newName);
                m_doc->signalDiagramRenamed(activeView());
            }
        }
        break;

    default:
        uWarning() << "unknown ListPopupMenu::MenuType " << ListPopupMenu::toString(sel);
        break;
    }
}

/**
 * Connects to the signal that @ref UMLApp emits when a cut operation
 * is successful.
 * If the view or a child started the operation the flag m_bStartedCut will
 * be set and we can carry out any operation that is needed, like deleting the selected
 * widgets for the cut operation.
 */
void UMLScene::slotCutSuccessful()
{
    if (m_bStartedCut) {
        deleteSelection();
        m_bStartedCut = false;
    }
}

/**
 * Called by menu when to show the instance of the view.
 */
void UMLScene::slotShowView()
{
    m_doc->changeCurrentView(ID());
}

/**
 * Returns the offset point at which to place the paste from clipboard.
 * Just add the amount to your co-ords.
 * Only call this straight after the event, the value won't stay valid.
 * Should only be called by Assoc widgets at the moment. no one else needs it.
 */
UMLScenePoint UMLScene::getPastePoint()
{
    UMLScenePoint point = m_PastePoint;
    point.setX(point.x() - m_Pos.x());
    point.setY(point.y() - m_Pos.y());
    return point;
}

/**
 * Reset the paste point.
 */
void UMLScene::resetPastePoint()
{
    m_PastePoint = m_Pos;
}

/**
 * Called by the view or any of its children when they start a cut
 * operation.
 */
void UMLScene::setStartedCut()
{
    m_bStartedCut = true;
}

/**
 * Returns the font to use
 */
QFont UMLScene::font() const
{
    return m_Options.uiState.font;
}

/**
 * Sets the font for the view and optionally all the widgets on the view.
 */
void UMLScene::setFont(QFont font, bool changeAllWidgets /* = false */)
{
    m_Options.uiState.font = font;
    if (!changeAllWidgets)
        return;
    foreach(UMLWidget* w, m_WidgetList) {
        w->setFont(font);
    }
}

/**
 * Sets some options for all the @ref ClassifierWidget on the view.
 */
void UMLScene::setClassWidgetOptions(ClassOptionsPage * page)
{
    foreach(UMLWidget* pWidget, m_WidgetList) {
        WidgetBase::WidgetType wt = pWidget->baseType();
        if (wt == WidgetBase::wt_Class || wt == WidgetBase::wt_Interface) {
            page->setWidget(static_cast<ClassifierWidget *>(pWidget));
            page->apply();
        }
    }
}

/**
 * Call before copying/cutting selected widgets.  This will make sure
 * any associations/message selected will make sure both the widgets
 * widgets they are connected to are selected.
 */
void UMLScene::checkSelections()
{
    UMLWidget * pWA = 0, * pWB = 0;
    //check messages
    foreach(UMLWidget *pTemp, selectedWidgets()) {
        if (pTemp->baseType() == WidgetBase::wt_Message && pTemp->isSelected()) {
            MessageWidget * pMessage = static_cast<MessageWidget *>(pTemp);
            pWA = pMessage->objectWidget(Uml::RoleType::A);
            pWB = pMessage->objectWidget(Uml::RoleType::B);
            if (!pWA->isSelected()) {
                pWA->setSelectedFlag(true);
                m_selectedList.append(pWA);
            }
            if (!pWB->isSelected()) {
                pWB->setSelectedFlag(true);
                m_selectedList.append(pWB);
            }
        }//end if
    }//end for
    //check Associations

    foreach(AssociationWidget *pAssoc, m_AssociationList) {
        if (pAssoc->isSelected()) {
            pWA = pAssoc->widgetForRole(Uml::RoleType::A);
            pWB = pAssoc->widgetForRole(Uml::RoleType::B);
            if (!pWA->isSelected()) {
                pWA->setSelectedFlag(true);
                m_selectedList.append(pWA);
            }
            if (!pWB->isSelected()) {
                pWB->setSelectedFlag(true);
                m_selectedList.append(pWB);
            }
        }//end if
    }//end foreach
}

/**
 * This function checks if the currently selected items have all the same
 * type (class, interface, ...). If true, the selection is unique and true
 * will be returned.
 * If there are no items selected, the function will return always true.
 */
bool UMLScene::checkUniqueSelection()
{
    // if there are no selected items, we return true
    if (selectedWidgets().isEmpty())
        return true;

    // get the first item and its base type
    UMLWidget * pTemp = (UMLWidget *) selectedWidgets().first();
    WidgetBase::WidgetType tmpType = pTemp->baseType();

    // check all selected items, if they have the same BaseType
    foreach(pTemp, selectedWidgets()) {
        if (pTemp->baseType() != tmpType) {
            return false; // the base types are different, the list is not unique
        }
    } // for (through all selected items)

    return true; // selected items are unique
}

/**
 * Asks for confirmation and clears everything on the diagram.
 * Called from menus.
 */
void UMLScene::clearDiagram()
{
    if (KMessageBox::Continue == KMessageBox::warningContinueCancel(activeView(),
                                     i18n("You are about to delete the entire diagram.\nAre you sure?"),
                                     i18n("Delete Diagram?"),
                                     KGuiItem(i18n("&Delete"), "edit-delete"))) {
        removeAllWidgets();
    }
}

/**
 * Apply an automatic layout.
 */
void UMLScene::applyLayout(const QString &variant)
{
    DEBUG(DBG_SRC) << "layout = " << variant;
    LayoutGenerator r;
    r.generate(this, variant);
    r.apply(this);
    UMLApp::app()->slotZoomFit();
}

/**
 * Changes snap to grid boolean.
 * Called from menus.
 */
void UMLScene::toggleSnapToGrid()
{
    setSnapToGrid(!snapToGrid());
}

/**
 * Changes snap to grid for component size boolean.
 * Called from menus.
 */
void UMLScene::toggleSnapComponentSizeToGrid()
{
    setSnapComponentSizeToGrid(!snapComponentSizeToGrid());
}

/**
 * Changes show grid boolean.
 * Called from menus.
 */
void UMLScene::toggleShowGrid()
{
    setSnapGridVisible(!isSnapGridVisible());
}

/**
 * Return whether to use snap to grid.
 */
bool UMLScene::snapToGrid() const
{
    return m_bUseSnapToGrid;
}

/**
 *  Sets whether to snap to grid.
 */
void UMLScene::setSnapToGrid(bool bSnap)
{
    m_bUseSnapToGrid = bSnap;
    emit sigSnapToGridToggled(snapToGrid());
}

/**
 * Return whether to use snap to grid for component size.
 */
bool UMLScene::snapComponentSizeToGrid() const
{
    return m_bUseSnapComponentSizeToGrid;
}

/**
 * Sets whether to snap to grid for component size.
 */
void UMLScene::setSnapComponentSizeToGrid(bool bSnap)
{
    m_bUseSnapComponentSizeToGrid = bSnap;
    updateComponentSizes();
    emit sigSnapComponentSizeToGridToggled(snapComponentSizeToGrid());
}

/**
 * Returns the x grid size.
 */
int UMLScene::snapX() const
{
    return m_layoutGrid->gridSpacingX();
}

/**
 * Returns the y grid size.
 */
int UMLScene::snapY() const
{
    return m_layoutGrid->gridSpacingY();
}

/**
 * Sets the grid size in x and y.
 */
void UMLScene::setSnapSpacing(int x, int y)
{
    m_layoutGrid->setGridSpacing(x, y);
}

/**
 * Returns the input coordinate with possible grid-snap applied.
 */
UMLSceneValue UMLScene::snappedX(UMLSceneValue _x)
{
    int x = (int)_x;
    if (snapToGrid()) {
        int gridX = snapX();
        int modX = x % gridX;
        x -= modX;
        if (modX >= gridX / 2)
            x += gridX;
    }
    return x;
}

/**
 * Returns the input coordinate with possible grid-snap applied.
 */
UMLSceneValue UMLScene::snappedY(UMLSceneValue _y)
{
    int y = (int)_y;
    if (snapToGrid()) {
        int gridY = snapY();
        int modY = y % gridY;
        y -= modY;
        if (modY >= gridY / 2)
            y += gridY;
    }
    return y;
}

/**
 *  Returns whether to show snap grid or not.
 */
bool UMLScene::isSnapGridVisible() const
{
    return m_layoutGrid->isVisible();
}

/**
 * Sets whether to show snap grid.
 */
void UMLScene::setSnapGridVisible(bool bShow)
{
    m_layoutGrid->setVisible(bShow);
    emit sigShowGridToggled(bShow);
}

/**
 *  Returns whether to show documentation indicator.
 */
bool UMLScene::isShowDocumentationIndicator() const
{
    return m_showDocumentationIndicator;
}

/**
 *  sets whether to show documentation indicator.
 */
void UMLScene::setShowDocumentationIndicator(bool bShow)
{
    m_showDocumentationIndicator = bShow;
}

/**
 * Returns whether to show operation signatures.
 */
bool UMLScene::showOpSig() const
{
    return m_Options.classState.showOpSig;
}

/**
 * Sets whether to show operation signatures.
 */
void UMLScene::setShowOpSig(bool bShowOpSig)
{
    m_Options.classState.showOpSig = bShowOpSig;
}

/**
 * Changes the zoom to the currently set level (now loaded from file)
 * Called from UMLApp::slotUpdateViews()
 */
void UMLScene::fileLoaded()
{
    m_view->setZoom(m_view->zoom());
    resizeSceneToItems();
}

/**
 * Sets the size of the scene to just fit on all the items
 */
void UMLScene::resizeSceneToItems()
{
    // let QGraphicsScene handle scene size by itself
    setSceneRect(QRectF());
}

/**
 * Updates the size of all components in this view.
 */
void UMLScene::updateComponentSizes()
{
    // update sizes of all components
    foreach(UMLWidget *obj, m_WidgetList) {
        obj->updateGeometry();
    }
}

/**
 * Force the widget font metrics to be updated next time
 * the widgets are drawn.
 * This is necessary because the widget size might depend on the
 * font metrics and the font metrics might change for different
 * QPainter, i.e. font metrics for Display font and Printer font are
 * usually different.
 * Call this when you change the QPainter.
 */
void UMLScene::forceUpdateWidgetFontMetrics(QPainter * painter)
{
    foreach(UMLWidget *obj, m_WidgetList) {
        obj->forceUpdateFontMetrics(painter);
    }
}

/**
 * Overrides standard method from QGraphicsScene drawing the background.
 */
void UMLScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    m_layoutGrid->paint(painter, rect);
}

/**
 * Creates the "diagram" tag and fills it with the contents of the diagram.
 */
void UMLScene::saveToXMI(QDomDocument & qDoc, QDomElement & qElement)
{
    resizeSceneToItems();
    QDomElement viewElement = qDoc.createElement("diagram");
    viewElement.setAttribute("xmi.id", Uml::ID::toString(m_nID));
    viewElement.setAttribute("name", name());
    viewElement.setAttribute("type", m_Type);
    viewElement.setAttribute("documentation", m_Documentation);
    //option state
    Settings::saveToXMI(viewElement, m_Options);
    //misc
    viewElement.setAttribute("localid", Uml::ID::toString(m_nLocalID));
    viewElement.setAttribute("showgrid", m_layoutGrid->isVisible());
    viewElement.setAttribute("snapgrid", m_bUseSnapToGrid);
    viewElement.setAttribute("snapcsgrid", m_bUseSnapComponentSizeToGrid);
    viewElement.setAttribute("snapx", m_layoutGrid->gridSpacingX());
    viewElement.setAttribute("snapy", m_layoutGrid->gridSpacingY());
    // FIXME: move to UMLView
    viewElement.setAttribute("zoom", activeView()->zoom());
    viewElement.setAttribute("canvasheight", height());
    viewElement.setAttribute("canvaswidth", width());
    viewElement.setAttribute("isopen", isOpen());

    //now save all the widgets
    QDomElement widgetElement = qDoc.createElement("widgets");
    foreach(UMLWidget *widget, m_WidgetList) {
        // Having an exception is bad I know, but gotta work with
        // system we are given.
        // We DON'T want to record any text widgets which are belonging
        // to associations as they are recorded later in the "associations"
        // section when each owning association is dumped. -b.t.
        if ((widget->baseType() != WidgetBase::wt_Text &&
             widget->baseType() != WidgetBase::wt_FloatingDashLine) ||
             static_cast<FloatingTextWidget*>(widget)->link() == NULL)
            widget->saveToXMI(qDoc, widgetElement);
    }
    viewElement.appendChild(widgetElement);
    //now save the message widgets
    QDomElement messageElement = qDoc.createElement("messages");
    foreach(UMLWidget* widget, m_MessageList) {
        widget->saveToXMI(qDoc, messageElement);
    }
    viewElement.appendChild(messageElement);
    //now save the associations
    QDomElement assocElement = qDoc.createElement("associations");
    if (m_AssociationList.count()) {
        // We guard against (m_AssociationList.count() == 0) because
        // this code could be reached as follows:
        //  ^  UMLScene::saveToXMI()
        //  ^  UMLDoc::saveToXMI()
        //  ^  UMLDoc::addToUndoStack()
        //  ^  UMLDoc::setModified()
        //  ^  UMLDoc::createDiagram()
        //  ^  UMLDoc::newDocument()
        //  ^  UMLApp::newDocument()
        //  ^  main()
        //
        AssociationWidget * assoc = 0;
        foreach(assoc, m_AssociationList) {
            assoc->saveToXMI(qDoc, assocElement);
        }
    }
    viewElement.appendChild(assocElement);
    qElement.appendChild(viewElement);
}

/**
 * Loads the "diagram" tag.
 */
bool UMLScene::loadFromXMI(QDomElement & qElement)
{
    QString id = qElement.attribute("xmi.id", "-1");
    m_nID = Uml::ID::fromString(id);
    if (m_nID == Uml::ID::None)
        return false;
    setName(qElement.attribute("name", ""));
    QString type = qElement.attribute("type", "0");
    m_Documentation = qElement.attribute("documentation", "");
    QString localid = qElement.attribute("localid", "0");
    // option state
    Settings::loadFromXMI(qElement, m_Options);
    setBackgroundBrush(m_Options.uiState.backgroundColor);
    setGridDotColor(m_Options.uiState.gridDotColor);
    //misc
    QString showgrid = qElement.attribute("showgrid", "0");
    m_layoutGrid->setVisible((bool)showgrid.toInt());

    QString snapgrid = qElement.attribute("snapgrid", "0");
    m_bUseSnapToGrid = (bool)snapgrid.toInt();

    QString snapcsgrid = qElement.attribute("snapcsgrid", "0");
    m_bUseSnapComponentSizeToGrid = (bool)snapcsgrid.toInt();

    QString snapx = qElement.attribute("snapx", "10");
    QString snapy = qElement.attribute("snapy", "10");
    m_layoutGrid->setGridSpacing(snapx.toInt(), snapy.toInt());

    QString zoom = qElement.attribute("zoom", "100");
    activeView()->setZoom(zoom.toInt());
    resizeSceneToItems();

    QString isOpen = qElement.attribute("isopen", "1");
    m_isOpen = (bool)isOpen.toInt();

    int nType = type.toInt();
    if (nType == -1 || nType >= 400) {
        // Pre 1.5.5 numeric values
        // Values of "type" were changed in 1.5.5 to merge with Settings::Diagram
        switch (nType) {
        case 400:
            m_Type = Uml::DiagramType::UseCase;
            break;
        case 401:
            m_Type = Uml::DiagramType::Collaboration;
            break;
        case 402:
            m_Type = Uml::DiagramType::Class;
            break;
        case 403:
            m_Type = Uml::DiagramType::Sequence;
            break;
        case 404:
            m_Type = Uml::DiagramType::State;
            break;
        case 405:
            m_Type = Uml::DiagramType::Activity;
            break;
        case 406:
            m_Type = Uml::DiagramType::Component;
            break;
        case 407:
            m_Type = Uml::DiagramType::Deployment;
            break;
        case 408:
            m_Type = Uml::DiagramType::EntityRelationship;
            break;
        default:
            m_Type = Uml::DiagramType::Undefined;
            break;
        }
    } else {
        m_Type = Uml::DiagramType::fromInt(nType);
    }
    m_nLocalID = Uml::ID::fromString(localid);

    QDomNode node = qElement.firstChild();
    bool widgetsLoaded = false, messagesLoaded = false, associationsLoaded = false;
    while (!node.isNull()) {
        QDomElement element = node.toElement();
        if (!element.isNull()) {
            if (element.tagName() == "widgets")
                widgetsLoaded = loadWidgetsFromXMI(element);
            else if (element.tagName() == "messages")
                messagesLoaded = loadMessagesFromXMI(element);
            else if (element.tagName() == "associations")
                associationsLoaded = loadAssociationsFromXMI(element);
        }
        node = node.nextSibling();
    }

    if (!widgetsLoaded) {
        uWarning() << "failed UMLScene load on widgets";
        return false;
    }
    if (!messagesLoaded) {
        uWarning() << "failed UMLScene load on messages";
        return false;
    }
    if (!associationsLoaded) {
        uWarning() << "failed UMLScene load on associations";
        return false;
    }
    return true;
}

bool UMLScene::loadWidgetsFromXMI(QDomElement & qElement)
{
    UMLWidget* widget = 0;
    QDomNode node = qElement.firstChild();
    QDomElement widgetElement = node.toElement();
    while (!widgetElement.isNull()) {
        widget = loadWidgetFromXMI(widgetElement);
        if (widget) {
            m_WidgetList.append(widget);
            widget->clipSize();
            // In the interest of best-effort loading, in case of a
            // (widget == NULL) we still go on.
            // The individual widget's loadFromXMI method should
            // already have generated an error message to tell the
            // user that something went wrong.
        }
        node = widgetElement.nextSibling();
        widgetElement = node.toElement();
    }

    return true;
}

/**
 * Loads a "widget" element from XMI, used by loadFromXMI() and the clipboard.
 */
UMLWidget* UMLScene::loadWidgetFromXMI(QDomElement& widgetElement)
{
    if (!m_doc) {
        uWarning() << "m_doc is NULL";
        return 0L;
    }

    QString tag  = widgetElement.tagName();
    QString idstr  = widgetElement.attribute("xmi.id", "-1");
    UMLWidget* widget = Widget_Factory::makeWidgetFromXMI(tag, idstr, this);

    if (widget == NULL)
        return NULL;
    if (!widget->loadFromXMI(widgetElement)) {
        widget->cleanup();
        delete widget;
        return 0;
    }
    return widget;
}

bool UMLScene::loadMessagesFromXMI(QDomElement & qElement)
{
    MessageWidget * message = 0;
    QDomNode node = qElement.firstChild();
    QDomElement messageElement = node.toElement();
    while (!messageElement.isNull()) {
        QString tag = messageElement.tagName();
        DEBUG(DBG_SRC) << "tag = " << tag;
        if (tag == "messagewidget" ||
            tag == "UML:MessageWidget") {   // for bkwd compatibility
            message = new MessageWidget(this, SequenceMessage::Asynchronous,
                                        Uml::ID::Reserved);
            if (!message->loadFromXMI(messageElement)) {
                delete message;
                return false;
            }
            m_MessageList.append(message);
            FloatingTextWidget *ft = message->floatingTextWidget();
            if (ft)
                m_WidgetList.append(ft);
            else if (message->sequenceMessageType() != SequenceMessage::Creation)
                DEBUG(DBG_SRC) << "floating text is NULL for message " << Uml::ID::toString(message->id());
        }
        node = messageElement.nextSibling();
        messageElement = node.toElement();
    }
    return true;
}

bool UMLScene::loadAssociationsFromXMI(QDomElement & qElement)
{
    QDomNode node = qElement.firstChild();
    QDomElement assocElement = node.toElement();
    int countr = 0;
    while (!assocElement.isNull()) {
        QString tag = assocElement.tagName();
        if (tag == "assocwidget" ||
            tag == "UML:AssocWidget") {  // for bkwd compatibility
            countr++;
            AssociationWidget *assoc = AssociationWidget::create(this);
            if (!assoc->loadFromXMI(assocElement)) {
                uError() << "could not loadFromXMI association widget:"
                         << assoc << ", bad XMI file? Deleting from UMLScene.";
                delete assoc;
                /* return false;
                   Returning false here is a little harsh when the
                   rest of the diagram might load okay.
                 */
            } else {
                assoc->clipSize();
                if (!addAssociation(assoc, false)) {
                    uError() << "Could not addAssociation(" << assoc << ") to UMLScene, deleting.";
                    delete assoc;
                    //return false; // soften error.. may not be that bad
                }
            }
        }
        node = assocElement.nextSibling();
        assocElement = node.toElement();
    }
    return true;
}

/**
 * Add an object to the application, and update the view.
 */
void UMLScene::addObject(UMLObject *object)
{
    m_bCreateObject = true;
    if (m_doc->addUMLObject(object))
        m_doc->signalUMLObjectCreated(object);  // m_bCreateObject is reset by slotObjectCreated()
    else
        m_bCreateObject = false;
}

bool UMLScene::loadUisDiagramPresentation(QDomElement & qElement)
{
    for (QDomNode node = qElement.firstChild(); !node.isNull(); node = node.nextSibling()) {
        QDomElement elem = node.toElement();
        QString tag = elem.tagName();
        if (! UMLDoc::tagEq(tag, "Presentation")) {
            uError() << "ignoring unknown UisDiagramPresentation tag " << tag;
            continue;
        }
        QDomNode n = elem.firstChild();
        QDomElement e = n.toElement();
        QString idStr;
        int x = 0, y = 0, w = 0, h = 0;
        while (!e.isNull()) {
            tag = e.tagName();
            DEBUG(DBG_SRC) << "Presentation: tag = " << tag;
            if (UMLDoc::tagEq(tag, "Presentation.geometry")) {
                QDomNode gnode = e.firstChild();
                QDomElement gelem = gnode.toElement();
                QString csv = gelem.text();
                QStringList dim = csv.split(',');
                x = dim[0].toInt();
                y = dim[1].toInt();
                w = dim[2].toInt();
                h = dim[3].toInt();
            } else if (UMLDoc::tagEq(tag, "Presentation.style")) {
                // TBD
            } else if (UMLDoc::tagEq(tag, "Presentation.model")) {
                QDomNode mnode = e.firstChild();
                QDomElement melem = mnode.toElement();
                idStr = melem.attribute("xmi.idref", "");
            } else {
                DEBUG(DBG_SRC) << "ignoring tag " << tag;
            }
            n = n.nextSibling();
            e = n.toElement();
        }
        Uml::ID::Type id = Uml::ID::fromString(idStr);
        UMLObject *o = m_doc->findObjectById(id);
        if (o == NULL) {
            uError() << "Cannot find object for id " << idStr;
        } else {
            UMLObject::ObjectType ot = o->baseType();
            DEBUG(DBG_SRC) << "Create widget for model object of type " << UMLObject::toString(ot);
            UMLWidget *widget = NULL;
            switch (ot) {
            case UMLObject::ot_Class:
                widget = new ClassifierWidget(this, static_cast<UMLClassifier*>(o));
                break;
            case UMLObject::ot_Association: {
                UMLAssociation *umla = static_cast<UMLAssociation*>(o);
                Uml::AssociationType::Enum at = umla->getAssocType();
                UMLObject* objA = umla->getObject(Uml::RoleType::A);
                UMLObject* objB = umla->getObject(Uml::RoleType::B);
                if (objA == NULL || objB == NULL) {
                    uError() << "intern err 1";
                    return false;
                }
                UMLWidget *wA = findWidget(objA->id());
                UMLWidget *wB = findWidget(objB->id());
                if (wA != NULL && wB != NULL) {
                    AssociationWidget *aw =
                        AssociationWidget::create(this, wA, at, wB, umla);
                    aw->syncToModel();
                    m_AssociationList.append(aw);
                } else {
                    uError() << "cannot create assocwidget from (" ; //<< wA << ", " << wB << ")";
                }
                break;
            }
            case UMLObject::ot_Role: {
                //UMLRole *robj = static_cast<UMLRole*>(o);
                //UMLAssociation *umla = robj->getParentAssociation();
                // @todo properly display role names.
                //       For now, in order to get the role names displayed
                //       simply delete the participating diagram objects
                //       and drag them from the list view to the diagram.
                break;
            }
            default:
                uError() << "Cannot create widget of type " << ot;
            }
            if (widget) {
                DEBUG(DBG_SRC) << "Widget: x=" << x << ", y=" << y
                               << ", w=" << w << ", h=" << h;
                widget->setX(x);
                widget->setY(y);
                widget->setSize(w, h);
                m_WidgetList.append(widget);
            }
        }
    }
    return true;
}

/**
 * Loads the "UISDiagram" tag of Unisys.IntegratePlus.2 generated files.
 */
bool UMLScene::loadUISDiagram(QDomElement & qElement)
{
    QString idStr = qElement.attribute("xmi.id", "");
    if (idStr.isEmpty())
        return false;
    m_nID = Uml::ID::fromString(idStr);
    UMLListViewItem *ulvi = NULL;
    for (QDomNode node = qElement.firstChild(); !node.isNull(); node = node.nextSibling()) {
        if (node.isComment())
            continue;
        QDomElement elem = node.toElement();
        QString tag = elem.tagName();
        if (tag == "uisDiagramName") {
            setName(elem.text());
            if (ulvi)
                ulvi->setText(name());
        } else if (tag == "uisDiagramStyle") {
            QString diagramStyle = elem.text();
            if (diagramStyle != "ClassDiagram") {
                uError() << "diagram style " << diagramStyle << " is not yet implemented";
                continue;
            }
            m_doc->setMainViewID(m_nID);
            m_Type = Uml::DiagramType::Class;
            UMLListView *lv = UMLApp::app()->listView();
            ulvi = new UMLListViewItem(lv->theLogicalView(), name(),
                                       UMLListViewItem::lvt_Class_Diagram, m_nID);
        } else if (tag == "uisDiagramPresentation") {
            loadUisDiagramPresentation(elem);
        } else if (tag != "uisToolName") {
            DEBUG(DBG_SRC) << "ignoring tag " << tag;
        }
    }
    return true;
}

/**
 * Left Alignment
 */
void UMLScene::alignLeft()
{
    UMLWidgetList widgetList = selectedWidgetsExt();
    if (widgetList.isEmpty())
        return;

    qreal smallestX = WidgetList_Utils::getSmallestX(widgetList);

    foreach(UMLWidget *widget, widgetList) {
        widget->setX(smallestX);
        widget->adjustAssocs(widget->x(), widget->y());
    }
    //TODO: Push stored cmds to stack.
}

/**
 * Right Alignment
 */
void UMLScene::alignRight()
{
    UMLWidgetList widgetList = selectedWidgetsExt();
    if (widgetList.isEmpty())
        return;
    qreal biggestX = WidgetList_Utils::getBiggestX(widgetList);

    foreach(UMLWidget *widget, widgetList) {
        widget->setX(biggestX - widget->width());
        widget->adjustAssocs(widget->x(), widget->y());
    }
    //TODO: Push stored cmds to stack.
}

/**
 * Top Alignment
 */
void UMLScene::alignTop()
{
    UMLWidgetList widgetList = selectedWidgetsExt();
    if (widgetList.isEmpty())
        return;

    qreal smallestY = WidgetList_Utils::getSmallestY(widgetList);

    foreach(UMLWidget *widget, widgetList) {
        widget->setY(smallestY);
        widget->adjustAssocs(widget->x(), widget->y());
    }
    //TODO: Push stored cmds to stack.
}

/**
 * Bottom Alignment
 */
void UMLScene::alignBottom()
{
    UMLWidgetList widgetList = selectedWidgetsExt();
    if (widgetList.isEmpty())
        return;
    qreal biggestY = WidgetList_Utils::getBiggestY(widgetList);

    foreach(UMLWidget *widget, widgetList) {
        widget->setY(biggestY - widget->height());
        widget->adjustAssocs(widget->x(), widget->y());
    }
    //TODO: Push stored cmds to stack.
}

/**
 * Vertical Middle Alignment
 */
void UMLScene::alignVerticalMiddle()
{
    UMLWidgetList widgetList = selectedWidgetsExt();
    if (widgetList.isEmpty())
        return;

    qreal smallestY = WidgetList_Utils::getSmallestY(widgetList);
    qreal biggestY = WidgetList_Utils::getBiggestY(widgetList);
    qreal middle = int((biggestY - smallestY) / 2) + smallestY;

    foreach(UMLWidget *widget, widgetList) {
        widget->setY(middle - int(widget->height() / 2));
        widget->adjustAssocs(widget->x(), widget->y());
    }
    //TODO: Push stored cmds to stack.
}

/**
 * Horizontal Middle Alignment
 */
void UMLScene::alignHorizontalMiddle()
{
    UMLWidgetList widgetList = selectedWidgetsExt();
    if (widgetList.isEmpty())
        return;

    qreal smallestX = WidgetList_Utils::getSmallestX(widgetList);
    qreal biggestX = WidgetList_Utils::getBiggestX(widgetList);
    qreal middle = int((biggestX - smallestX) / 2) + smallestX;

    foreach(UMLWidget *widget, widgetList) {
        widget->setX(middle - int(widget->width() / 2));
        widget->adjustAssocs(widget->x(), widget->y());
    }
    //TODO: Push stored cmds to stack.
}

/**
 * Vertical Distribute Alignment
 */
void UMLScene::alignVerticalDistribute()
{
    UMLWidgetList widgetList = selectedWidgetsExt();
    if (widgetList.isEmpty())
        return;

    qreal smallestY = WidgetList_Utils::getSmallestY(widgetList);
    qreal biggestY = WidgetList_Utils::getBiggestY(widgetList);
    qreal heightsSum = WidgetList_Utils::getHeightsSum(widgetList);
    qreal distance = int(((biggestY - smallestY) - heightsSum) / (widgetList.count() - 1.0) + 0.5);

    qSort(widgetList.begin(), widgetList.end(), Widget_Utils::hasSmallerY);

    int i = 1;
    UMLWidget* widgetPrev = NULL;
    foreach(UMLWidget *widget, widgetList) {
        if (i == 1) {
            widgetPrev = widget;
        } else {
            widget->setY(widgetPrev->y() + widgetPrev->height() + distance);
            widget->adjustAssocs(widget->x(), widget->y());
            widgetPrev = widget;
        }
        i++;
    }
    //TODO: Push stored cmds to stack.
}

/**
 * Horizontal Distribute Alignment
 */
void UMLScene::alignHorizontalDistribute()
{
    UMLWidgetList widgetList = selectedWidgetsExt();
    if (widgetList.isEmpty())
        return;

    qreal smallestX = WidgetList_Utils::getSmallestX(widgetList);
    qreal biggestX = WidgetList_Utils::getBiggestX(widgetList);
    qreal widthsSum = WidgetList_Utils::getWidthsSum(widgetList);
    qreal distance = int(((biggestX - smallestX) - widthsSum) / (widgetList.count() - 1.0) + 0.5);

    qSort(widgetList.begin(), widgetList.end(), Widget_Utils::hasSmallerX);

    int i = 1;
    UMLWidget* widgetPrev = NULL;
    foreach(UMLWidget *widget,  widgetList) {
        if (i == 1) {
            widgetPrev = widget;
        } else {
            widget->setX(widgetPrev->x() + widgetPrev->width() + distance);
            widget->adjustAssocs(widget->x(), widget->y());
            widgetPrev = widget;
        }
        i++;
    }
    //TODO: Push stored cmds to stack.
}

/**
 * Overloading operator for debugging output.
 */
QDebug operator<<(QDebug dbg, UMLScene *item)
{
    dbg.nospace() << "UMLScene: " << item->name()
                  << " / type=" << DiagramType::toString(item->type())
                  << " / id=" << Uml::ID::toString(item->ID())
                  << " / isOpen=" << item->isOpen();
    return dbg.space();
}
