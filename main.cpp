#include "mathexpreval.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MathExprEval expr;
    int result = expr.evaluateExpression("1+6*2");
    Q_ASSERT(result == 13);

    MainWindow w;
    w.show();

    return a.exec();
}
