#include <QApplication>
#include <QHotkey>
#include <QSettings>
#include "DiscordClient.h"
#include "OverlayWindow.h"
#include "ConfigWindow.h"

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("DeskMute");
    QCoreApplication::setOrganizationDomain("deskmute.local");
    QCoreApplication::setApplicationName("VoiceOverlay");

    QApplication app(argc, argv);

    DiscordClient client;
    OverlayWindow overlay;
    ConfigWindow config;

    QSettings settings;
    client.setConnectionDetails(settings.value("host", "127.0.0.1").toString(), settings.value("port", 3845).toInt());

    overlay.setOverlayOpacity(settings.value("opacity", 90).toInt() / 100.0);
    overlay.setHideChannelName(settings.value("hideName", false).toBool());
    overlay.setRequireVcForHotkeys(settings.value("requireVc", true).toBool());

    QObject::connect(&client, &DiscordClient::stateReceived, &overlay, &OverlayWindow::updateState);
    QObject::connect(&client, &DiscordClient::disconnected, &overlay, &OverlayWindow::showDisconnected);
    QObject::connect(&config, &ConfigWindow::connectionSettingsChanged, &client, &DiscordClient::setConnectionDetails);

    QObject::connect(&config, &ConfigWindow::toggleEditMode, &overlay, &OverlayWindow::setEditMode);
    QObject::connect(&config, &ConfigWindow::hideChannelNameChanged, &overlay, &OverlayWindow::setHideChannelName);
    QObject::connect(&config, &ConfigWindow::requireVcForHotkeysChanged, &overlay, &OverlayWindow::setRequireVcForHotkeys);

    QObject::connect(&config, &ConfigWindow::configVisibleChanged, &overlay, &OverlayWindow::setConfigActive);
    QObject::connect(&config, &ConfigWindow::opacityChanged, &overlay, &OverlayWindow::setOverlayOpacity);

    auto showConfig = [&config]() {
        if (config.isVisible()) {
            config.hide();
        } else {
            config.show();
            config.raise();
            config.activateWindow();
        }
    };
    QObject::connect(&overlay, &OverlayWindow::openSettingsRequested, showConfig);

    QHotkey* shortcutMute = new QHotkey(QKeySequence(settings.value("bindMute", "Ctrl+M").toString()), true, &app);
    QObject::connect(shortcutMute, &QHotkey::activated, [&client, &overlay]() {
        if (overlay.canUseHotkeys()) client.sendAction("mute");
    });

    QHotkey* shortcutDeafen = new QHotkey(QKeySequence(settings.value("bindDeafen", "Ctrl+D").toString()), true, &app);
    QObject::connect(shortcutDeafen, &QHotkey::activated, [&client, &overlay]() {
        if (overlay.canUseHotkeys()) client.sendAction("deafen");
    });

    QHotkey* shortcutConfig = new QHotkey(QKeySequence(settings.value("bindConfig", "Ctrl+Shift+D").toString()), true, &app);
    QObject::connect(shortcutConfig, &QHotkey::activated, showConfig);

    QObject::connect(&config, &ConfigWindow::muteKeyChanged, [shortcutMute](const QString& seq) {
        shortcutMute->setShortcut(QKeySequence(seq), true);
    });
    QObject::connect(&config, &ConfigWindow::deafenKeyChanged, [shortcutDeafen](const QString& seq) {
        shortcutDeafen->setShortcut(QKeySequence(seq), true);
    });
    QObject::connect(&config, &ConfigWindow::configKeyChanged, [shortcutConfig](const QString& seq) {
        shortcutConfig->setShortcut(QKeySequence(seq), true);
    });

    overlay.show();

    return app.exec();
}