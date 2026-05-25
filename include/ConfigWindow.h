#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QSlider>
#include <QSettings>
#include <QPoint>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include "BindButton.h"

class ConfigWindow : public QWidget {
    Q_OBJECT
public:
    explicit ConfigWindow(QWidget* parent = nullptr);

    signals:
        void toggleEditMode(bool editing);
    void hideChannelNameChanged(bool hidden);
    void requireVcForHotkeysChanged(bool require);
    void connectionSettingsChanged(const QString& host, int port);
    void opacityChanged(qreal opacity);
    void configVisibleChanged(bool visible);

    void muteKeyChanged(const QString& sequence);
    void deafenKeyChanged(const QString& sequence);
    void configKeyChanged(const QString& sequence);
    void overlayToggleKeyChanged(const QString& sequence);
    void toggleOverlayVisibilityRequested();

    void tunnelUpdated(const QString& host, int port);

public slots:
    void fetchGistTunnel(bool force = false);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    QPushButton* btnEditMode;
    QCheckBox* chkHideName;
    QCheckBox* chkRequireVc;

    QLineEdit* hostInput;
    QSpinBox* portInput;
    QPushButton* btnApplyNetwork;

    QSlider* opacitySlider;

    BindButton* bindMute;
    BindButton* bindDeafen;
    BindButton* bindConfig;
    BindButton* bindToggleOverlay;
    QPushButton* btnToggleOverlay;

    QLineEdit* gistIdInput;
    QLineEdit* patInput;
    QPushButton* btnLoadGist;
    QCheckBox* chkAutoRefresh;

    QTimer* gistTimer;
    QNetworkAccessManager net;
    QString lastTunnel;

    bool isEditing = false;
    QPoint dragPosition;
};

#endif