#ifndef CHILDWINDOWS_H
#define CHILDWINDOWS_H

#include <QWidget>
#include <QPushButton>

class ChildWindows : public QWidget
{
    Q_OBJECT
public:
    explicit ChildWindows(QWidget *parent = nullptr);
    ~ChildWindows() = default;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onCloseButtonClicked();

private:
    QPushButton *closeButton;
};

#endif // CHILDWINDOWS_H
