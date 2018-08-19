#include "vmClient.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    vmClient w;
    w.show();
    return a.exec();
}
