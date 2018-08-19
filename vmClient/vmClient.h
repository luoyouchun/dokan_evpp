#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_vmClient.h"

class vmClient : public QMainWindow
{
    Q_OBJECT

public:
    vmClient(QWidget *parent = Q_NULLPTR);

private:
    Ui::vmClientClass ui;
};
