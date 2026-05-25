#include "AppController.h"
#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QLabel>
#include <QDebug>
#include <QHotkey>

#ifdef Q_OS_WIN
#include <windows.h>
#include <functional>

static HHOOK mouseHook = nullptr;
static std::function<void(int)> mouseCallbackInstance;

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_XBUTTONDOWN) {
        MSLLHOOKSTRUCT* hookStruct = (MSLLHOOKSTRUCT*)lParam;
        int button = HIWORD(hookStruct->mouseData);
        if (button == 1 && mouseCallbackInstance) mouseCallbackInstance(4);
        else if (button == 2 && mouseCallbackInstance) mouseCallbackInstance(5);
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}
#endif

AppController::AppController(QObject* parent)
    : QObject(parent),
      shortcutMute(nullptr),
      shortcutDeafen(nullptr),
      shortcutConfig(nullptr),
      shortcutToggle(nullptr) {
}

AppController::~AppController() {
#ifdef Q_OS_WIN
    if (mouseHook) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = nullptr;
    }
#endif
}

void AppController::start() {
    loadStyle();

    QSettings settings;
    QString gistId = settings.value("gistId", "").toString();
    QString githubPat = settings.value("githubPat", "").toString();

    if (gistId.isEmpty() || githubPat.isEmpty()) {
        client.setConnectionDetails(settings.value("host", "127.0.0.1").toString(), settings.value("port", 3845).toInt());
    }

    overlay.setOverlayOpacity(settings.value("opacity", 90).toInt() / 100.0);
    overlay.setHideChannelName(settings.value("hideName", false).toBool());
    overlay.setRequireVcForHotkeys(settings.value("requireVc", true).toBool());

    setupConnections();
    setupHotkeys();
    setupMouseHook();

    if (QCoreApplication::arguments().contains("--config")) {
        showConfig();
    }

    overlay.show();
}

void AppController::loadStyle() {
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        qApp->setStyleSheet(styleFile.readAll());
        styleFile.close();
    } else {
        qDebug() << "Warning: Failed to load style.qss from resources!";
    }
}

void AppController::setupConnections() {

    connect(&client, &DiscordClient::stateReceived, &overlay, &OverlayWindow::updateState);
    connect(&client, &DiscordClient::disconnected, &overlay, &OverlayWindow::showDisconnected);

    connect(&config, &ConfigWindow::connectionSettingsChanged, this, [this](const QString& host, int port) {
        client.setConnectionDetails(host, port);
        if (auto* lbl = config.findChild<QLabel*>("lblLiveStatus")) {
            lbl->setText("CONNECTING...");
            lbl->setStyleSheet("color: #FEE75C; font-weight: bold;");
        }
    });

    connect(&client, &DiscordClient::stateReceived, this, [this]() {
        if (auto* lbl = config.findChild<QLabel*>("lblLiveStatus")) {
            lbl->setText("CONNECTED");
            lbl->setStyleSheet("color: #4BB543; font-weight: bold;");
        }
    });
    connect(&client, &DiscordClient::disconnected, this, [this]() {
        if (auto* lbl = config.findChild<QLabel*>("lblLiveStatus")) {
            lbl->setText("DISCONNECTED");
            lbl->setStyleSheet("color: #FF4B4B; font-weight: bold;");
        }
    });

    connect(&config, &ConfigWindow::toggleEditMode, &overlay, &OverlayWindow::setEditMode);
    connect(&config, &ConfigWindow::hideChannelNameChanged, &overlay, &OverlayWindow::setHideChannelName);
    connect(&config, &ConfigWindow::requireVcForHotkeysChanged, &overlay, &OverlayWindow::setRequireVcForHotkeys);
    connect(&config, &ConfigWindow::configVisibleChanged, &overlay, &OverlayWindow::setConfigActive);
    connect(&config, &ConfigWindow::opacityChanged, &overlay, &OverlayWindow::setOverlayOpacity);
    connect(&config, &ConfigWindow::toggleOverlayVisibilityRequested, &overlay, &OverlayWindow::toggleVisibility);

    connect(&overlay, &OverlayWindow::reloadGistRequested, &config, [this]() {
        config.fetchGistTunnel(true);
    });
    connect(&overlay, &OverlayWindow::openSettingsRequested, this, &AppController::showConfig);

    connect(&config, &ConfigWindow::muteKeyChanged, this, [this](const QString& seq) {
        muteSeq = seq;
        shortcutMute->setShortcut(getValidSeq(seq), true);
    });
    connect(&config, &ConfigWindow::deafenKeyChanged, this, [this](const QString& seq) {
        deafenSeq = seq;
        shortcutDeafen->setShortcut(getValidSeq(seq), true);
    });
    connect(&config, &ConfigWindow::configKeyChanged, this, [this](const QString& seq) {
        configSeq = seq;
        shortcutConfig->setShortcut(getValidSeq(seq), true);
    });
    connect(&config, &ConfigWindow::overlayToggleKeyChanged, this, [this](const QString& seq) {
        toggleSeq = seq;
        shortcutToggle->setShortcut(getValidSeq(seq), true);
    });
}

void AppController::setupHotkeys() {
    QSettings settings;
    muteSeq = settings.value("bindMute", "Ctrl+M").toString();
    deafenSeq = settings.value("bindDeafen", "Ctrl+D").toString();
    configSeq = settings.value("bindConfig", "Ctrl+Shift+O").toString();
    toggleSeq = settings.value("bindToggle", "Ctrl+Shift+T").toString();

    shortcutMute = new QHotkey(getValidSeq(muteSeq), true, qApp);
    connect(shortcutMute, &QHotkey::activated, this, [this]() {
        if (overlay.canUseHotkeys()) client.sendAction("mute");
    });

    shortcutDeafen = new QHotkey(getValidSeq(deafenSeq), true, qApp);
    connect(shortcutDeafen, &QHotkey::activated, this, [this]() {
        if (overlay.canUseHotkeys()) client.sendAction("deafen");
    });

    shortcutConfig = new QHotkey(getValidSeq(configSeq), true, qApp);
    connect(shortcutConfig, &QHotkey::activated, this, &AppController::showConfig);

    shortcutToggle = new QHotkey(getValidSeq(toggleSeq), true, qApp);
    connect(shortcutToggle, &QHotkey::activated, &overlay, &OverlayWindow::toggleVisibility);
}

void AppController::setupMouseHook() {
#ifdef Q_OS_WIN
    mouseCallbackInstance = [this](int btn) {
        handleMouseButton(btn);
    };
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
#endif
}

void AppController::handleMouseButton(int btn) {
    QString mStr = QString("Mouse %1").arg(btn);
    if (muteSeq == mStr && overlay.canUseHotkeys()) client.sendAction("mute");
    if (deafenSeq == mStr && overlay.canUseHotkeys()) client.sendAction("deafen");
    if (configSeq == mStr) showConfig();
    if (toggleSeq == mStr) overlay.toggleVisibility();
}

void AppController::showConfig() {
    if (config.isVisible()) {
        config.hide();
    } else {
        config.show();
        config.raise();
        config.activateWindow();
    }
}

QKeySequence AppController::getValidSeq(const QString& seq) {
    return seq.startsWith("Mouse") ? QKeySequence() : QKeySequence(seq);
}