#ifndef DIEWIDGET_H
#define DIEWIDGET_H

#include <QLabel>

#include "dice.h"

class DieWidget : public QLabel
{
    Q_OBJECT
public:
    DieWidget(Die die);

    Die die() const { return _die; }

    static constexpr char mimetype[] = "application/x-die";

private:
    Die _die;

    void setVisibleFace(char face);

protected:
    QPoint dragStartPosition;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // DIEWIDGET_H
