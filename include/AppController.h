#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QString>
#include <QKeySequence>
#include "DiscordClient.h"
#include "OverlayWindow.h"
#include "ConfigWindow.h"

class QHotkey;

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController();

    void start();

private slots:
    void showConfig();
    void handleMouseButton(int btn);

private:
    void loadStyle();
    void setupConnections();
    void setupHotkeys();
    void setupMouseHook();
    QKeySequence getValidSeq(const QString& seq);

    DiscordClient client;
    OverlayWindow overlay;
    ConfigWindow config;

    QString muteSeq;
    QString deafenSeq;
    QString configSeq;
    QString toggleSeq;

    QHotkey* shortcutMute;
    QHotkey* shortcutDeafen;
    QHotkey* shortcutConfig;
    QHotkey* shortcutToggle;
};

#endif