#ifndef DASHOPTIONS_H
#define DASHOPTIONS_H

#include <QWidget>

namespace Ui {
class DashOptions;
}

class DashOptions : public QWidget
{
    Q_OBJECT

public:
    explicit DashOptions(QWidget *parent = nullptr);
    ~DashOptions();

private:
    Ui::DashOptions *ui;
};

#endif // DASHOPTIONS_H
