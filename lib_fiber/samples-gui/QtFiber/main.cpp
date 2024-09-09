#include "stdafx.h"
#include "mainwindow.h"
#include <QPushButton>
#include <QApplication>
#include <QTimer>

class fiber_backend : public acl::fiber {
public:
    fiber_backend() {}

protected:
    void run() override {
        qDebug() << "fiber_backend started!";
        while (true) {
            acl::fiber::delay(1000);
        }
        delete this;
    }

    ~fiber_backend() = default;
};

static void startupCallback()
{
    acl::fiber::init(acl::FIBER_EVENT_T_WMSG, true);
    acl::fiber* fb = new fiber_backend;
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
