#include "stdafx.h"
#include "inputdialog.h"

InputDialog::InputDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Http Options: ");
    setGeometry(200, 200, 600, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);

    options = new QTextEdit(this);
    options->setPlaceholderText("输入内容...");
    layout->addWidget(options);

    confirm = new QPushButton("确定", this);
    layout->addWidget(confirm);

    connect(confirm, &QPushButton::clicked, this, &InputDialog::onAccept);
}

void InputDialog::onAccept()
{
    emit dialogAccepted(options->toPlainText());
    accept();
}
