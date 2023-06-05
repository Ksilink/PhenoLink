#include "processprogresswidget.h"
#include "ui_processprogresswidget.h"

ProcessProgressWidget::ProcessProgressWidget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ProcessProgressWidget)
{
  ui->setupUi(this);
}

ProcessProgressWidget::~ProcessProgressWidget()
{
  delete ui;
}
