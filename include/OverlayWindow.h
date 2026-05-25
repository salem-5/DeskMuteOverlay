#ifndef OVERLAYWINDOW_H
#define OVERLAYWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QLabel>
#include <QMap>
#include <QNetworkAccessManager>
#include <QSystemTrayIcon>
#include <QSizeGrip>
#include "MemberWidget.h"

class OverlayWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit OverlayWindow(QWidget* parent = nullptr);
    bool canUseHotkeys() const;

    signals:
        void openSettingsRequested();
    void reloadGistRequested();

public slots:
    void updateState(const QJsonObject& state);
    void showDisconnected();
    void setEditMode(bool enabled);
    void setHideChannelName(bool hide);
    void setRequireVcForHotkeys(bool require);
    void setConfigActive(bool active);
    void setOverlayOpacity(qreal opacity);
    void toggleVisibility();
    void showConnecting();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void loadAvatar(const QString& memberId, const QString& url);
    void setupTrayIcon();
    void updateContainerStyle();

    QVBoxLayout* listLayout;
    QLabel* channelTitle;
    QMap<QString, MemberWidget*> activeWidgets;
    QMap<QString, QPixmap> avatarCache;
    QNetworkAccessManager* networkManager;
    QSystemTrayIcon* trayIcon;
    QSizeGrip* sizeGrip;
    QAction* toggleVisibilityAction;

    bool isEditMode = false;
    bool isConfigActive = false;
    bool hideChannelName = false;
    bool requireVcForHotkeys = true;
    bool currentlyInVc = false;
    QPoint dragPosition;
};

#endif