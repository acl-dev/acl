#include "inputdialog.h"

InputDialog::InputDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("input: ");
    setGeometry(200, 200, 300, 200);

    QVBoxLayout *layout = new QVBoxLayout(this);

    lineEdit = new QLineEdit(this);
    lineEdit->setPlaceholderText("输入内容...");
    layout->addWidget(lineEdit);

    button = new QPushButton("确定", this);
    layout->addWidget(button);

    connect(button, &QPushButton::clicked, this, &InputDialog::onAccept);
}

void InputDialog::onAccept()
{
    emit dialogAccepted(lineEdit->text());
    accept();
}
