#include <QApplication>
#include <QHotkey>
#include <QSettings>
#include <functional>
#include <QFile>
#include <QLabel>
#include <QProcess>

#include "../include/DiscordClient.h"
#include "../include/OverlayWindow.h"
#include "../include/ConfigWindow.h"

#ifdef Q_OS_WIN
#include <windows.h>
HHOOK mouseHook = nullptr;
std::function<void(int)> mouseCallback;

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_XBUTTONDOWN) {
        MSLLHOOKSTRUCT* hookStruct = (MSLLHOOKSTRUCT*)lParam;
        int button = HIWORD(hookStruct->mouseData);
        if (button == 1 && mouseCallback) mouseCallback(4);
        else if (button == 2 && mouseCallback) mouseCallback(5);
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}
#endif

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("DeskMute");
    QCoreApplication::setOrganizationDomain("deskmute.local");
    QCoreApplication::setApplicationName("VoiceOverlay");

    QApplication app(argc, argv);

    DiscordClient client;
    OverlayWindow overlay;
    ConfigWindow config;

    QSettings settings;
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    } else {
        qDebug() << "Warning: Failed to load style.qss from resources!";
    }
    QString gistId = settings.value("gistId", "").toString();
    QString githubPat = settings.value("githubPat", "").toString();

    if (gistId.isEmpty() || githubPat.isEmpty()) {
        client.setConnectionDetails(settings.value("host", "127.0.0.1").toString(), settings.value("port", 3845).toInt());
    }

    overlay.setOverlayOpacity(settings.value("opacity", 90).toInt() / 100.0);
    overlay.setHideChannelName(settings.value("hideName", false).toBool());
    overlay.setRequireVcForHotkeys(settings.value("requireVc", true).toBool());

    QObject::connect(&client, &DiscordClient::stateReceived, &overlay, &OverlayWindow::updateState);
    QObject::connect(&client, &DiscordClient::disconnected, &overlay, &OverlayWindow::showDisconnected);

    QObject::connect(&config, &ConfigWindow::connectionSettingsChanged, [&](const QString& host, int port) {
        client.setConnectionDetails(host, port);
        if (auto* lbl = config.findChild<QLabel*>("lblLiveStatus")) {
            lbl->setText("CONNECTING...");
            lbl->setStyleSheet("color: #FEE75C; font-weight: bold;");
        }
    });

    QObject::connect(&client, &DiscordClient::stateReceived, &config, [&config]() {
        if (auto* lbl = config.findChild<QLabel*>("lblLiveStatus")) {
            lbl->setText("CONNECTED");
            lbl->setStyleSheet("color: #4BB543; font-weight: bold;");
        }
    });
    QObject::connect(&client, &DiscordClient::disconnected, &config, [&config]() {
        if (auto* lbl = config.findChild<QLabel*>("lblLiveStatus")) {
            lbl->setText("DISCONNECTED");
            lbl->setStyleSheet("color: #FF4B4B; font-weight: bold;");
        }
    });

    QObject::connect(&config, &ConfigWindow::toggleEditMode, &overlay, &OverlayWindow::setEditMode);
    QObject::connect(&config, &ConfigWindow::hideChannelNameChanged, &overlay, &OverlayWindow::setHideChannelName);
    QObject::connect(&config, &ConfigWindow::requireVcForHotkeysChanged, &overlay, &OverlayWindow::setRequireVcForHotkeys);

    QObject::connect(&config, &ConfigWindow::configVisibleChanged, &overlay, &OverlayWindow::setConfigActive);
    QObject::connect(&config, &ConfigWindow::opacityChanged, &overlay, &OverlayWindow::setOverlayOpacity);
    QObject::connect(&config, &ConfigWindow::toggleOverlayVisibilityRequested, &overlay, &OverlayWindow::toggleVisibility);
    QObject::connect(&overlay, &OverlayWindow::reloadGistRequested, &config, [&config]() {
        config.fetchGistTunnel(true);
    });

    auto showConfig = [&config]() {
        if (config.isVisible()) {
            config.hide();
        } else {
            config.show();
            config.raise();
            config.activateWindow();
        }
    };
    if (QCoreApplication::arguments().contains("--config")) showConfig();
    QObject::connect(&overlay, &OverlayWindow::openSettingsRequested, showConfig);

    QString muteSeq = settings.value("bindMute", "Ctrl+M").toString();
    QString deafenSeq = settings.value("bindDeafen", "Ctrl+D").toString();
    QString configSeq = settings.value("bindConfig", "Ctrl+Shift+O").toString();
    QString toggleSeq = settings.value("bindToggle", "Ctrl+Shift+T").toString();

    auto getValidSeq = [](const QString& seq) {
        return seq.startsWith("Mouse") ? QKeySequence() : QKeySequence(seq);
    };

    QHotkey* shortcutMute = new QHotkey(getValidSeq(muteSeq), true, &app);
    QObject::connect(shortcutMute, &QHotkey::activated, [&client, &overlay]() {
        if (overlay.canUseHotkeys()) client.sendAction("mute");
    });

    QHotkey* shortcutDeafen = new QHotkey(getValidSeq(deafenSeq), true, &app);
    QObject::connect(shortcutDeafen, &QHotkey::activated, [&client, &overlay]() {
        if (overlay.canUseHotkeys()) client.sendAction("deafen");
    });

    QHotkey* shortcutConfig = new QHotkey(getValidSeq(configSeq), true, &app);
    QObject::connect(shortcutConfig, &QHotkey::activated, showConfig);

    QHotkey* shortcutToggle = new QHotkey(getValidSeq(toggleSeq), true, &app);
    QObject::connect(shortcutToggle, &QHotkey::activated, &overlay, &OverlayWindow::toggleVisibility);

    QObject::connect(&config, &ConfigWindow::muteKeyChanged, [&](const QString& seq) {
        muteSeq = seq;
        shortcutMute->setShortcut(getValidSeq(seq), true);
    });
    QObject::connect(&config, &ConfigWindow::deafenKeyChanged, [&](const QString& seq) {
        deafenSeq = seq;
        shortcutDeafen->setShortcut(getValidSeq(seq), true);
    });
    QObject::connect(&config, &ConfigWindow::configKeyChanged, [&](const QString& seq) {
        configSeq = seq;
        shortcutConfig->setShortcut(getValidSeq(seq), true);
    });
    QObject::connect(&config, &ConfigWindow::overlayToggleKeyChanged, [&](const QString& seq) {
        toggleSeq = seq;
        shortcutToggle->setShortcut(getValidSeq(seq), true);
    });

#ifdef Q_OS_WIN
    mouseCallback = [&](int btn) {
        QString mStr = QString("Mouse %1").arg(btn);
        if (muteSeq == mStr && overlay.canUseHotkeys()) client.sendAction("mute");
        if (deafenSeq == mStr && overlay.canUseHotkeys()) client.sendAction("deafen");
        if (configSeq == mStr) showConfig();
        if (toggleSeq == mStr) overlay.toggleVisibility();
    };
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        if (mouseHook) UnhookWindowsHookEx(mouseHook);
    });
#endif
    overlay.show();
    return app.exec();
}