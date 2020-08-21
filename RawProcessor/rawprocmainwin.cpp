#include "rawprocmainwin.h"
#include "ui_rawprocmainwin.h"

#include <Core/checkoutprocess.h>
#include <Gui/mainwindow.h>

#include <QtSql>

#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>

#include <Gui/ctkWidgets/ctkDoubleRangeSlider.h>
#include <Gui/ctkWidgets/ctkCollapsibleGroupBox.h>
#include <Gui/ctkWidgets/ctkRangeSlider.h>
#include <Gui/ctkWidgets/ctkPopupWidget.h>
#include <Gui/ctkWidgets/ctkDoubleSlider.h>
#include <Gui/ctkWidgets/ctkCheckableHeaderView.h>
#include <Gui/ctkWidgets/ctkColorPickerButton.h>


RawProcMainWin::RawProcMainWin(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::RawProcMainWin)
{
  ui->setupUi(this);

  CheckoutProcess& handler = CheckoutProcess::handler();

//  QSqlQuery q;

//  if (!q.exec("select distinct process_tag from Processes"))
//       qDebug() << q.lastQuery() << q.lastError();

//  QStringList l;

//  while (q.next()) {
//      QString process = q.value(0).toString();
//         QJsonObject obj;
//         handler.getParameters(process, obj);
//        if (!obj.isEmpty())
//          l << process;
//     }
//    ui->comboBox->addItems(l);
//ui->comboBox

    ui->textBrowser->hide();
}

RawProcMainWin::~RawProcMainWin()
{
  delete ui;
}

void RawProcMainWin::on_comboBox_currentTextChanged(const QString &arg1)
{
  QSqlQuery q;

  if (! q.exec(QString("select process_json from Processes where process_tag == '%1' order by lastload").arg(arg1)))
     qDebug() << q.lastQuery() << q.lastError();

  ui->comboBox_2->clear();
  while (q.next()) {
      QByteArray process = q.value(0).toByteArray();
      QJsonObject obj = QJsonDocument::fromJson(process).object();
      QJsonArray arr = obj["Parameters"].toArray();
      QString p;
      for (int i = 0; i < arr.size(); ++i)
        {
          QJsonObject ob = arr.at(i).toObject();
          if (ob.contains("Value"))
            p += QString("%1: %2; ").arg(
                  ob["Tag"].toString()).arg(ob["Value"].isString() ?
                      ob["Value"].toString()
                  :
                      QString("%1").arg(ob["Value"].toDouble()));
        }
      ui->comboBox_2->addItem(p, process);
     }


}

void RawProcMainWin::on_comboBox_2_currentTextChanged(const QString &arg1)
{
  QByteArray jdata = ui->comboBox_2->currentData().toByteArray();
  QJsonObject obj = QJsonDocument::fromJson(jdata).object();
  // Build the Gui Accordingly

  QString process = obj["Path"].toString();

  QFormLayout* lay =  dynamic_cast<QFormLayout*>(ui->scrollAreaWidgetContents->layout());
  if (!lay)
    {
      qDebug() << "Error layout not in correct format";
      return;
    }

  QList<QWidget*> list = ui->scrollAreaWidgetContents->findChildren<QWidget*>();
  foreach(QWidget* wid, list)
    {
      wid->hide();
      wid->close();
      ui->scrollAreaWidgetContents->layout()->removeWidget(wid);
    }


  QStringList l = process.split('/');
  //  qDebug() << l;

  QLabel * lb = new QLabel(l.back());
  lb->setObjectName(process);
  lb->setToolTip(obj["Comment"].toString());
  lay->addRow(lb);

  // FIXME: Properly handle the "Position" of parameter
  // FIXME: Properly handle other data types

  QJsonArray params = obj["Parameters"].toArray();
  for (int i = 0; i < params.size(); ++i )
    {
      QJsonObject par = params[i].toObject();
      QWidget* wid = 0;

//      wid = widgetFromJSON(par);

      if (par.contains("ImageType"))
        {
          // Display Image Accordingly
          //
        }

      if (wid)
        {
          wid->setAttribute( Qt::WA_DeleteOnClose, true);
          wid->setObjectName(par["Tag"].toString());
          lay->addRow(par["Tag"].toString(), wid);
          lay->labelForField(wid)->setToolTip(par["Comment"].toString());
        }

    }
  QPushButton* button = new QPushButton("Start");
  connect(button, SIGNAL(pressed()), this, SLOT(startProcess()));
  lay->addRow(button);

}


void RawProcMainWin::getValue(QWidget* wid, QJsonObject& obj)
{
  {
    QSlider* s = dynamic_cast<QSlider*>(wid);
    if (s) obj["Value"] = s->value();
  }

  {
    QSpinBox* s = dynamic_cast<QSpinBox*>(wid);
    if (s) obj["Value"] = s->value();
  }

  {
    ctkDoubleSlider* s = dynamic_cast<ctkDoubleSlider*>(wid);
    if (s) obj["Value"] =  s->value();
  }

  {
    QDoubleSpinBox* s = dynamic_cast<QDoubleSpinBox*>(wid);
    if (s) obj["Value"] =  s->value();
  }

  {
    QComboBox* s = dynamic_cast<QComboBox*>(wid);
    if (s) obj["Value"] =  s->currentText();
  }
}


void RawProcMainWin::startProcess()
{
  QPushButton* s = qobject_cast<QPushButton*>(sender());
  if (s) s->setDisabled(true);

  QByteArray jdata = ui->comboBox_2->currentData().toByteArray();
  QJsonObject obj = QJsonDocument::fromJson(jdata).object();
  QJsonArray params = obj["Parameters"].toArray();
  for (int i = 0; i < params.size(); ++i )
  {
      QJsonObject par = params[i].toObject();

      QString tag = par["Tag"].toString();
      QWidget* wid = ui->scrollAreaWidgetContents->findChild<QWidget*>(tag);

      if (!wid) continue;

      getValue(wid, par);
      params.replace(i, par);
  }

  obj["Parameters"] = params;

  QJsonArray procArray; procArray.append(obj);
  QString process = obj["Path"].toString();
  qDebug() << procArray;
  CheckoutProcess& handler = CheckoutProcess::handler();
  handler.startProcessServer(process, procArray);


  connect(&CheckoutProcess::handler(), SIGNAL(processStarted(QString)),
          this, SLOT(processStarted(QString)));

  timerId = startTimer(500);
  ui->textBrowser->show();
}

void RawProcMainWin::timerEvent(QTimerEvent *event)
{
  event->accept();
  QJsonObject ob = CheckoutProcess::handler().refreshProcessStatus(hash);

  if (ob["State"].toString() == "Finished")
    killTimer(timerId);

  QJsonDocument doc(ob);
  ui->textBrowser->setText(doc.toJson());

}

void RawProcMainWin::processStarted(QString hash)
{
  this->hash = hash;
}

void RawProcMainWin::processFinished()
{

  QList<QPushButton*> list = ui->scrollAreaWidgetContents->findChildren<QPushButton*>();
  foreach (QPushButton* b, list)
    b->setEnabled(true);
}
