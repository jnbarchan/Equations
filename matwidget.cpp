#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QLayout>
#include <QMimeData>
#include <QPropertyAnimation>

#include "diewidget.h"
#include "matwidget.h"

MatWidget::MatWidget(QWidget *parent)
    : QGroupBox{parent}
{
    _model = nullptr;
    _diceList = nullptr;
    setAcceptDrops(true);
}

DieWidget *MatWidget::findDieWidgetForFace(char face) const
{
    QList<DieWidget *> diceWidgets = findChildren<DieWidget *>();
    for (DieWidget *dw : diceWidgets)
        if (dw->die().face == face)
            return dw;
    return nullptr;
}

void MatWidget::moveDieHere(DieWidget *dw)
{
    const MatWidget *source = qobject_cast<MatWidget *>(dw->parentWidget());

    if (QWidget *oldParent = dw->parentWidget())
        if (QLayout *oldLayout = oldParent->layout())
            oldLayout->removeWidget(dw);
    dw->setParent(this);
    layout()->addWidget(dw);
    dw->show();

    Die die{dw->die()};
    source->diceList()->removeOne(die);
    this->diceList()->append(die);

    emit diceChanged();
}

void MatWidget::emulateDragDrop(DieWidget *dieWidget, QWidget *ancestor)
{
    QPoint startPos = dieWidget->mapTo(ancestor, QPoint(0, 0));
    QPoint endPos = this->mapTo(ancestor, this->rect().center());

    QPixmap pixmap = dieWidget->grab();

    QLabel *movingDie = new QLabel(ancestor);
    movingDie->setPixmap(pixmap);
    movingDie->setAttribute(Qt::WA_TransparentForMouseEvents);
    movingDie->setStyleSheet("background: transparent;");
    movingDie->setGeometry(startPos.x(), startPos.y(), pixmap.width(), pixmap.height());

    movingDie->show();
    movingDie->raise();

    QPropertyAnimation *animation = new QPropertyAnimation(movingDie, "pos");

    animation->setDuration(1000);
    animation->setStartValue(startPos);
    animation->setEndValue(endPos);

    connect(animation, &QPropertyAnimation::finished,
            this, [this, dieWidget, movingDie, animation]()
            {
                // Perform the actual transfer.
                moveDieHere(dieWidget);

                movingDie->deleteLater();
                animation->deleteLater();
            });

    animation->start();
}

void MatWidget::dragEnterEvent(QDragEnterEvent *event) /*override*/
{
    DieWidget *dw = qobject_cast<DieWidget *>(event->source());
    if (!(dw && event->mimeData()->hasFormat(DieWidget::mimetype)))
        return;

    const MatWidget *source = qobject_cast<MatWidget *>(dw->parentWidget());
    if (!(source && dropAllowed && dropAllowed(source, this)))
        return;

    event->acceptProposedAction();
}

void MatWidget::dropEvent(QDropEvent *event) /*override*/
{
    DieWidget *dw = qobject_cast<DieWidget *>(event->source());
    if (!(dw && event->mimeData()->hasFormat(DieWidget::mimetype)))
        return;

    const MatWidget *source = qobject_cast<MatWidget *>(dw->parentWidget());
    if (!(source && dropAllowed && dropAllowed(source, this)))
        return;

    moveDieHere(dw);
    emit diceDropped();

    event->setDropAction(Qt::MoveAction);
    event->accept();
}
