#ifndef RAWPROCMAINWIN_H
#define RAWPROCMAINWIN_H

#include <QMainWindow>

namespace Ui {
  class RawProcMainWin;
}

class RawProcMainWin : public QMainWindow
{
  Q_OBJECT

public:
  explicit RawProcMainWin(QWidget *parent = 0);
  ~RawProcMainWin();

protected:
  void getValue(QWidget *wid, QJsonObject &obj);

  virtual   void timerEvent(QTimerEvent *event);

protected slots:
  void processFinished();
  void processStarted(QString hash);

private slots:
  void on_comboBox_currentTextChanged(const QString &arg1);

  void on_comboBox_2_currentTextChanged(const QString &arg1);
  void startProcess();

private:
  Ui::RawProcMainWin *ui;
  QString hash;
  int timerId;
};

#endif // RAWPROCMAINWIN_H
