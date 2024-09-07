#include "stdafx.h"
#include <QVBoxLayout>
#include <QDebug>
#include "childwindows.h"

ChildWindows::ChildWindows(QWidget *parent)
: QWidget(parent)
{
    setWindowTitle("The child window");

    QVBoxLayout *layout = new QVBoxLayout(this);
    closeButton = new QPushButton("Close", this);
    layout->addWidget(closeButton);

    closeButton = new QPushButton("Close", this);
    closeButton->setGeometry(QRect(QPoint(50, 50), QSize(100, 30)));
    connect(closeButton, &QPushButton::clicked, this, &ChildWindows::onCloseButtonClicked);

    setFixedSize(300, 200);
}

void ChildWindows::onCloseButtonClicked()
{
    this->close();
}

void ChildWindows::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    qDebug() << "Second window is shown";
}
