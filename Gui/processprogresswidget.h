#ifndef PROCESSPROGRESSWIDGET_H
#define PROCESSPROGRESSWIDGET_H

#include <QWidget>

namespace Ui {
  class ProcessProgressWidget;
}

class ProcessProgressWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ProcessProgressWidget(QWidget *parent = 0);
  ~ProcessProgressWidget();

private:
  Ui::ProcessProgressWidget *ui;
};

#endif // PROCESSPROGRESSWIDGET_H
