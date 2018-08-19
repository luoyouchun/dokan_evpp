#include "vmServer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    vmServer w;
    w.show();
    return a.exec();
}
