#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

#include "diewidget.h"

DieWidget::DieWidget(Die die) :
    _die(die)
{
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Raised);
    setLineWidth(2);
    setAlignment(Qt::AlignCenter);
    setFixedSize(25, 25);

    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, die.colour);
    setPalette(palette);

    setVisibleFace(die.face);
}

void DieWidget::mousePressEvent(QMouseEvent *event) /*override*/
{
    if (event->button() == Qt::LeftButton)
        dragStartPosition = event->position().toPoint();

    QLabel::mousePressEvent(event);
}

void DieWidget::mouseMoveEvent(QMouseEvent *event) /*override*/
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->position().toPoint() - dragStartPosition).manhattanLength() < QApplication::startDragDistance())
        return;

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(mimetype, QByteArrayLiteral("die"));
    drag->setMimeData(mimeData);

    drag->setPixmap(grab());
    drag->setHotSpot(event->position().toPoint());

    drag->exec(Qt::MoveAction);
}

void DieWidget::setVisibleFace(char face)
{
    QChar visibleFace{face};
    if (face == '+')
        visibleFace = QChar(0x002B);    // addition symbol
    else if (face == '-')
        visibleFace = QChar(0x2212);    // subtraction symbol
    else if (face == '*')
        visibleFace = QChar(0x00D7);    // multiplication symbol
    else if (face == '/')
        visibleFace = QChar(0x00F7);    // division symbol
    else if (face == '~')
        visibleFace = QChar(0x221A);    // square root symbol
    setText(visibleFace);
}
