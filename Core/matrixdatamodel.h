#ifndef MATRIXDATAMODEL_H
#define MATRIXDATAMODEL_H

class DllCoreExport MatrixDataModel
{
public:
  static const unsigned Count;

    enum Group { Field, Stack, Time, Channel };

    Q_DECLARE_FLAGS(Groups, Group);

    MatrixDataModel(int colCount);

    QVector<QVector<double> >& operator()();

    bool isActive(int x, int y);

    bool isActive(int r);


    QList<QString> getFeaturesNames();
    // Returns the 2D position name
    QString rowName(int r);
    QPoint rowPos(int r);


    void setTag(int r, QString t)
    {
        _tags[r] = t;
      //  qDebug() << r << t;
    }

    QString getTag(int r) { return _tags[r]; }

#define P2(X) (X)*(X)

    int rowCount() { return P2(Count); }

    int columnCount() { return _colNames.size(); }
    // Returns the field/stack/time/channel/feature name associated to a column data
    QString columnName(int c);

    //
    QVector<QVector<double> > getColumns(QString feature);
    QVector<QVector<double> > getColumnsGroup(Group g, QString groupValue, QString feature);
    QVector<QVector<double> > getColumnsGroups(Groups g, QVector<QString> groupValues, QStringList features);


    void addData(QString XP, int field, int stackZ, int time, int chan, int x, int y, double data);
    double data(int x, int y, int col = 0);

    int pointToPos(QPoint p);

protected:
    QMap<QString, int>  _colNames; // handle the ordering of columns by their adapted names
    QSet<QString>       _xps; // Handle the naming of data by the feature name
    QSet<int>           _pos;
    QMap<int, QString>  _tags;
    QVector<QVector<double> > _dataset;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MatrixDataModel::Groups);

#endif // MATRIXDATAMODEL_H
