#include "dashoptions.h"
#include "ui_dashoptions.h"

DashOptions::DashOptions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DashOptions)
{
    ui->setupUi(this);
}

DashOptions::~DashOptions()
{
    delete ui;
}
