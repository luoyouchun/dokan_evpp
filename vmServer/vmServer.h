#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_vmServer.h"

class vmServer : public QMainWindow
{
    Q_OBJECT

public:
    vmServer(QWidget *parent = Q_NULLPTR);

private:
    Ui::vmServerClass ui;
};
