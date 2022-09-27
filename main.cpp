#include "uppercomputer_hk.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    UpperComputer_HK w;
    w.show();
    return a.exec();
}
