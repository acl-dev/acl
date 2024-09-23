#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class InputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InputDialog(QWidget *parentr);

signals:
    void dialogAccepted(const QString &text);

private slots:
    void onAccept();

private:
    QTextEdit *options;
    QPushButton *confirm;
};

#endif // INPUTDIALOG_H
