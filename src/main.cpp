#include <QApplication>
#include "AppController.h"

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("DeskMute");
    QCoreApplication::setOrganizationDomain("deskmute.local");
    QCoreApplication::setApplicationName("VoiceOverlay");

    QApplication app(argc, argv);

    AppController controller;
    controller.start();

    return app.exec();
}