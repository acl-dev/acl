#include "stdafx.h"
#include "mainwindow.h"
#include <QPushButton>
#include <QApplication>
#include <QTimer>

//#define USE_FIBER_INIT
#ifdef USE_FIBER_INIT

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

#endif

static void startupCallback()
{
    qDebug() << "Begin schedule gui fiber!";
#ifdef USE_FIBER_INIT
    acl::fiber::init(acl::FIBER_EVENT_T_WMSG, true);
    acl::fiber* fb = new fiber_backend;
    fb->start();
#else
   acl::fiber::schedule_gui(); // Won't return until schedule finished.
#endif
    qDebug() << "schedule gui fiber end!";
}

static void app_run(int argc, char **argv)
{
    QApplication app(argc, argv);

    MainWindow w;
    //w.resize(1600, 800);
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
