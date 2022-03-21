
#include "wellplatemodel.h"

#include "Core/checkoutdataloaderplugininterface.h"

#include <QJsonObject>
#include <QJsonArray>
#include <opencv2/highgui/highgui.hpp>


const unsigned MatrixDataModel::Count = 'Z'-'A'+1;

MatrixDataModel::MatrixDataModel(int colCount)
{
  _dataset.resize(colCount);
}

QVector<QVector<double> > &MatrixDataModel::operator()() { return _dataset; }

bool MatrixDataModel::isActive(int x, int y)
{
  return _pos.contains(x+y*(Count));
}

int MatrixDataModel::pointToPos(QPoint p)
{
 return p.x()+p.y()*(Count);
}

QString MatrixDataModel::rowName(int r)
{
  int x = r % (Count);
  int y = r / (Count);

  return QString("%1%2").arg(QChar('A'+x)).arg(y+1);
}

QPoint MatrixDataModel::rowPos(int r)
{
  int x = r % (Count);
  int y = r / (Count);
  return QPoint(x,y);
}

QString MatrixDataModel::columnName(int c)
{
  for (QMap<QString, int>::const_iterator it = _colNames.cbegin(), e = _colNames.cend(); it != e; ++it)
    if (it.value() == c) return it.key();
  return QString("-");
}

QVector<QVector<double> > MatrixDataModel::getColumns(QString feature)
{
  QVector<QVector<double> > res;
  foreach (QString s, _colNames.keys())
    {
      if (s.endsWith(feature))
        res.push_back(_dataset[_colNames[s]]);
    }
  return res;
}

QVector<QVector<double> > MatrixDataModel::getColumnsGroup(MatrixDataModel::Group g, QString groupValue, QString feature)
{
  QRegExp r(QString("f%1s%2t%3dc%4%5").arg(g==Field ? groupValue :  "\\d")
            .arg(g== Stack ? groupValue :  "\\d")
            .arg(g== Time ? groupValue :  "\\d")
            .arg(g== Channel ? groupValue :  "\\d").arg(feature));

  QVector<QVector<double> > res;
  foreach (QString s, _colNames.keys())
    {
      if (r.indexIn(s) != -1)
        res.push_back(_dataset[_colNames[s]]);
    }
  return res;
}
bool MatrixDataModel::isActive(int r)
{
return _pos.contains(r);
}


QList<QString> MatrixDataModel::getFeaturesNames()
{
  return _xps.values();
}

void MatrixDataModel::addData(QString XP, int field, int stackZ, int time, int chan, int x, int y, double data)
{

  QString colname = QString("f%1s%2t%3c%4%5").arg(field).arg(stackZ).arg(time).arg(chan).arg(XP);
  int col = -1;

  _xps.insert(XP);
  if (_colNames.contains(colname)) col = _colNames[colname];
  else {
      col = _colNames.size();
      _colNames[colname] = col;
      if (_dataset.size() <= col)
        qDebug() << "happened here..." << colname << col;
//      Q_ASSERT(_dataset.size() > col);
      _dataset[col].resize(P2(Count));
    }

  _pos.insert(x+y*(Count));

  _dataset[col][x+y*(Count)] = data;
}

double MatrixDataModel::data(int x, int y, int col)
{
  return _dataset[col][x+y*(Count)];
}
