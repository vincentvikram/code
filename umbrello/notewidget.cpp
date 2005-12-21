/*
 *  copyright (C) 2002-2005
 *  Umbrello UML Modeller Authors <uml-devel@ uml.sf.net>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// own header
#include "notewidget.h"
//qt includes
#include <q3pointarray.h>
#include <qpainter.h>
#include <q3textedit.h>
#include <q3frame.h>
//Added by qt3to4:
#include <QMouseEvent>
// kde includes
#include <kdebug.h>
#include <kcursor.h>
#include <kcolordialog.h>
// app includes
#include "dialogs/notedialog.h"
#include "clipboard/umldrag.h"
#include "umldoc.h"
#include "umlview.h"
#include "uml.h"
#include "listpopupmenu.h"

#define NOTEMARGIN 10

NoteWidget::NoteWidget(UMLView * view, Uml::IDType id)
        : UMLWidget(view, id) {
    init();
    setSize(100,80);
#ifdef NOTEWIDGET_EMBED_EDITOR
    // NB: This code is currently deactivated because
    // Zoom does not yet work with the embedded text editor.
    m_pEditor = new Q3TextEdit(view);
    m_pEditor->setFrameStyle(Q3Frame::NoFrame | Q3Frame::Plain);
    m_pEditor->setHScrollBarMode(Q3ScrollView::AlwaysOff);
    m_pEditor->setVScrollBarMode(Q3ScrollView::AlwaysOff);
    m_pEditor->setTextFormat(Qt::RichText);
    m_pEditor->setShown(true);
    setEditorGeometry();
    connect(m_pView, SIGNAL(contentsMoving(int, int)),
            this, SLOT(slotViewScrolled(int, int)));
#endif
}

void NoteWidget::init() {
    UMLWidget::setBaseType(Uml::wt_Note);
    m_DiagramLink = Uml::id_None;
}

NoteWidget::~NoteWidget() {
#ifdef NOTEWIDGET_EMBED_EDITOR
    delete m_pEditor;
#endif
}

void NoteWidget::setDiagramLink(Uml::IDType viewID) {
    UMLDoc *umldoc = UMLApp::app()->getDocument();
    UMLView *view = umldoc->findView(viewID);
    if (view == NULL) {
        kdError() << "NoteWidget::setDiagramLink(" << ID2STR(viewID)
        << "): no view found for this ID." << endl;
        return;
    }
    QString linkText("Diagram: " + view->getName());
#if defined (NOTEWIDGET_EMBED_EDITOR)
    m_pEditor->setUnderline(true);
    m_pEditor->insert(linkText);
    m_pEditor->setUnderline(false);
#else
    setDoc(linkText);
    update();
#endif
    m_DiagramLink = viewID;
}

Uml::IDType NoteWidget::getDiagramLink() const {
    return m_DiagramLink;
}

void NoteWidget::slotViewScrolled(int x, int y) {
    setEditorGeometry(x, y);
}

void NoteWidget::setFont(QFont font) {
    UMLWidget::setFont(font);
#ifdef NOTEWIDGET_EMBED_EDITOR
    m_pEditor->setFont(font);
#endif
}

void NoteWidget::setEditorGeometry(int dx /*=0*/, int dy /*=0*/) {
#if defined (NOTEWIDGET_EMBED_EDITOR)
    const QRect editorGeometry( UMLWidget::getX() - dx + 6,
                                UMLWidget::getY() - dy + 10,
                                UMLWidget::getWidth() - 16,
                                UMLWidget::getHeight() - 16);
    m_pEditor->setGeometry( editorGeometry );
    drawText();
#else
    dx=0; dy=0;   // avoid "unused arg" warnings
#endif
}

void NoteWidget::setX( int x ) {
    UMLWidget::setX(x);
    setEditorGeometry();
}

void NoteWidget::setY( int y ) {
    UMLWidget::setY(y);
    setEditorGeometry();
}

QString NoteWidget::getDoc() const {
#if defined (NOTEWIDGET_EMBED_EDITOR)
    return m_pEditor->text();
#else
    return m_Text;
#endif
}

void NoteWidget::setDoc(const QString &newText) {
#if defined (NOTEWIDGET_EMBED_EDITOR)
    m_pEditor->setText(newText);
#else
    m_Text = newText;
#endif
}

void NoteWidget::draw(QPainter & p, int offsetX, int offsetY) {
    int margin = 10;
    int w = width()-1;

    int h= height()-1;
    Q3PointArray poly(6);
    poly.setPoint(0, offsetX, offsetY);
    poly.setPoint(1, offsetX, offsetY + h);
    poly.setPoint(2, offsetX + w, offsetY + h);
    poly.setPoint(3, offsetX + w, offsetY + margin);
    poly.setPoint(4, offsetX + w - margin, offsetY);
    poly.setPoint(5, offsetX, offsetY);
    UMLWidget::setPen(p);
    if ( UMLWidget::getUseFillColour() ) {
        QBrush brush( UMLWidget::getFillColour() );
        p.setBrush(brush);
        p.drawPolygon(poly);
#if defined (NOTEWIDGET_EMBED_EDITOR)
        m_pEditor->setPaper(brush);
#endif
    } else
        p.drawPolyline(poly);
    p.drawLine(offsetX + w - margin, offsetY, offsetX + w - margin, offsetY + margin);
    p.drawLine(offsetX + w - margin, offsetY + margin, offsetX + w, offsetY + margin);
    if(m_bSelected) {
        drawSelected(&p, offsetX, offsetY);
    }

    drawText(&p, offsetX, offsetY);
}

void NoteWidget::mouseMoveEvent(QMouseEvent *me) {
    UMLWidget::mouseMoveEvent(me);
    setEditorGeometry();
}

QSize NoteWidget::calculateSize() {
    return QSize(50, 50);
}

void NoteWidget::slotMenuSelection(int sel) {
    NoteDialog * dlg = 0;
    UMLDoc *doc = UMLApp::app()->getDocument();
    switch(sel) {
        ///OBSOLETE - remove ListPopupMenu::mt_Link_Docs
        // case ListPopupMenu::mt_Link_Docs:
        //      m_pView->updateNoteWidgets();
        //      doc -> setModified(true);
        //      break;

    case ListPopupMenu::mt_Rename:
        m_pView -> updateDocumentation( false );
        dlg = new NoteDialog( m_pView, this );
        if( dlg -> exec() ) {
            m_pView -> showDocumentation( this, true );
            doc -> setModified(true);
            update();
        }
        delete dlg;
        break;

    default:
        UMLWidget::slotMenuSelection(sel);
        break;
    }
}

void NoteWidget::mouseReleaseEvent( QMouseEvent * me ) {
    UMLWidget::mouseReleaseEvent( me );
    if (m_bResizing) {
        drawText();
        UMLWidget::mouseReleaseEvent(me);
    }
}

void NoteWidget::mouseDoubleClickEvent( QMouseEvent * me ) {
    if( me -> button() != Qt::LeftButton )
        return;
    if (m_DiagramLink == Uml::id_None) {
        slotMenuSelection( ListPopupMenu::mt_Rename );
    } else {
        UMLDoc *umldoc = UMLApp::app()->getDocument();
        umldoc->changeCurrentView(m_DiagramLink);
    }
}

void NoteWidget::drawText(QPainter * p /*=NULL*/, int offsetX /*=0*/, int offsetY /*=0*/) {
#if defined (NOTEWIDGET_EMBED_EDITOR)
    m_pEditor->setText( getDoc() );
    m_pEditor->setShown(true);
    m_pEditor->repaint();
#else
    if (p == NULL)
        return;
    /*
    Implement word wrap for text as follows:
    wrap at width on whole words.
    if word is wider than width then clip word
    if reach height exit and don't print anymore
    start new line on \n character
    */
    p->setPen( Qt::black );
    QFont font = UMLWidget::getFont();
    p->setFont( font );
    const QFontMetrics &fm = getFontMetrics(FT_NORMAL);
    const int fontHeight  = fm.lineSpacing();
    QString text = getDoc();
    if( text.length() == 0 )
        return;
    uint i = 0;
    QString word = "";
    const int margin = fm.width( "W" );
    int textY = fontHeight / 2;
    int textX = margin;
    const int width = this -> width() - margin * 2;
    const int height = this -> height() - fontHeight;
    QChar returnChar('\n');
    while( i <= text.length() ) {
        QChar c = text[ i++ ];
        if( c == returnChar ) {
            if( word.length() > 0 ) {
                int textWidth = fm.width( word );
                if( ( textX + textWidth ) > width )//wrap word
                {
                    textWidth = textWidth < width ? textWidth: width;
                    textX = margin;
                    textY += fontHeight;
                    if( textY > height )
                        return;
                    p->drawText( offsetX + textX, offsetY + textY , textWidth, fontHeight, Qt::AlignLeft, word );
                }//end if
                else
                {
                    if ( textY > height )
                        return;
                    p->drawText( offsetX + textX, offsetY + textY , textWidth, fontHeight, Qt::AlignLeft, word );
                }
            }//end if
            textX = margin;
            textY += fontHeight;
            word = "";
        } else if( c.isSpace() ) {
            if( word.length() > 0 ) {
                int textWidth = fm.width( word );
                if( ( textX + textWidth ) > width )//wrap word
                {
                    textWidth = textWidth < width ? textWidth: width;
                    if( textX != margin )
                        textY += fontHeight;
                    textX = margin;
                    if( textY > height )
                        return;
                    p->drawText( offsetX + textX, offsetY + textY , textWidth, fontHeight, Qt::AlignLeft, word );
                }//end if
                else
                {
                    if ( textY > height )
                        return;
                    p->drawText( offsetX + textX, offsetY + textY , textWidth, fontHeight, Qt::AlignLeft, word );
                }
                textX += textWidth;
            }//end if
            textX += fm.width( " " );
            word = "";
        } else {
            if (c!='\0') word += c;
        }
    }//end while
    if( word.length() > 0 ) {
        const int textWidth = fm.width( word );
        if( ( textWidth + textX ) > width )//wrap word
        {
            textX = margin;
            textY += fontHeight;
            if( textY > height )
                return;
            p->drawText( offsetX + textX, offsetY + textY , textWidth, fontHeight, Qt::AlignLeft, word );
        }//end if
        else
        {
            if ( textY > height )
                return;
            p->drawText( offsetX + textX, offsetY + textY , textWidth, fontHeight, Qt::AlignLeft, word );
        }
    }//end if
#endif
}

void NoteWidget::saveToXMI( QDomDocument & qDoc, QDomElement & qElement ) {
    QDomElement noteElement = qDoc.createElement( "notewidget" );
    UMLWidget::saveToXMI( qDoc, noteElement );
    noteElement.setAttribute( "text", getDoc() );
    if (m_DiagramLink != Uml::id_None)
        noteElement.setAttribute( "diagramlink", ID2STR(m_DiagramLink) );
    qElement.appendChild( noteElement );
}

bool NoteWidget::loadFromXMI( QDomElement & qElement ) {
    if( !UMLWidget::loadFromXMI( qElement ) )
        return false;
    setDoc( qElement.attribute("text", "") );
    QString diagramlink = qElement.attribute("diagramlink", "");
    if (!diagramlink.isEmpty())
        m_DiagramLink = STR2ID(diagramlink);
    return true;
}


#include "notewidget.moc"

