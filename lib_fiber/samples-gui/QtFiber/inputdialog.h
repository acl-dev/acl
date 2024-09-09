#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class InputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InputDialog(QWidget *parent = nullptr);

signals:
    void dialogAccepted(const QString &text);

private slots:
    void onAccept();

private:
    QLineEdit *lineEdit;
    QPushButton *button;
};

#endif // INPUTDIALOG_H
