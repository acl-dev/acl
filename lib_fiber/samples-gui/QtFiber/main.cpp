#include "stdafx.h"
#include "mainwindow.h"
#include <QPushButton>
#include <QApplication>
#include <QTimer>

class fiber_dummy : public acl::fiber {
public:
    fiber_dummy() {}

protected:
    void run() override {
        qDebug() << "fiber_dummy started!";
        delete this;
    }

    ~fiber_dummy() = default;
};

static void startupCallback()
{
    acl::fiber::init(acl::FIBER_EVENT_T_WMSG, true);
    acl::fiber* fb = new fiber_dummy;
    fb->start();
}

static void app_run(int argc, char **argv)
{
    QApplication app(argc, argv);

    MainWindow w;
    w.resize(800, 800);
    w.show();

    QTimer::singleShot(0, startupCallback);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &w, &MainWindow::onAboutToQuit);

    app.exec();
}

int main(int argc, char *argv[])
{
    acl::acl_cpp_init();
    app_run(argc, argv);
    return 0;
}
