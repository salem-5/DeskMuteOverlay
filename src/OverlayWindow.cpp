#include "OverlayWindow.h"
#include <QMouseEvent>
#include <QResizeEvent>
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QPainter>
#include <QMenu>
#include <QDebug>
#include <QApplication>
#include <QStyle>

OverlayWindow::OverlayWindow(QWidget* parent) : QMainWindow(parent), networkManager(new QNetworkAccessManager(this)) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
    setAttribute(Qt::WA_TranslucentBackground);

    QSettings settings;
    move(settings.value("pos", QPoint(20, 20)).toPoint());
    resize(settings.value("size", QSize(320, 450)).toSize());

    QWidget* central = new QWidget(this);
    central->setObjectName("OverlayContainer");
    setCentralWidget(central);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    channelTitle = new QLabel("CONNECTING...", this);
    channelTitle->setObjectName("channelTitle");
    mainLayout->addWidget(channelTitle);

    listLayout = new QVBoxLayout();
    listLayout->setSpacing(4);
    listLayout->addStretch();
    mainLayout->addLayout(listLayout);

    sizeGrip = new QSizeGrip(this);
    sizeGrip->setObjectName("sizeGrip");
    sizeGrip->setFixedSize(16, 16);
    sizeGrip->hide();

    setupTrayIcon();
    updateContainerStyle();
}

void OverlayWindow::resizeEvent(QResizeEvent* event) {
    sizeGrip->move(width() - sizeGrip->width() - 8, height() - sizeGrip->height() - 8);
    QMainWindow::resizeEvent(event);
}

void OverlayWindow::setOverlayOpacity(qreal opacity) {
    setWindowOpacity(opacity);
}

void OverlayWindow::setupTrayIcon() {
    QPixmap trayPix(32, 32);
    trayPix.fill(Qt::transparent);
    QPainter p(&trayPix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#5865F2"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(2, 2, 28, 28);
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(trayPix.rect(), Qt::AlignCenter, "DM");

    trayIcon = new QSystemTrayIcon(QIcon(trayPix), this);

    QMenu* trayMenu = new QMenu(this);
    toggleVisibilityAction = trayMenu->addAction("Hide Overlay", this, &OverlayWindow::toggleVisibility);
    trayMenu->addAction("Reload from Gist", this, [this]() {
        emit reloadGistRequested();
    });
    trayMenu->addSeparator();
    trayMenu->addAction("Settings", this, &OverlayWindow::openSettingsRequested);
    trayMenu->addAction("Quit DeskMute", qApp, &QCoreApplication::quit);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
}

void OverlayWindow::toggleVisibility() {
    if (isVisible()) {
        hide();
        if (toggleVisibilityAction) toggleVisibilityAction->setText("Show Overlay");
    } else {
        show();
        if (toggleVisibilityAction) toggleVisibilityAction->setText("Hide Overlay");
    }
}

bool OverlayWindow::canUseHotkeys() const {
    if (!requireVcForHotkeys) return true;
    return currentlyInVc;
}

void OverlayWindow::setHideChannelName(bool hide) {
    hideChannelName = hide;
    channelTitle->setVisible(!hide);
}

void OverlayWindow::setRequireVcForHotkeys(bool require) { requireVcForHotkeys = require; }

void OverlayWindow::setConfigActive(bool active) {
    isConfigActive = active;
    updateContainerStyle();
}

void OverlayWindow::setEditMode(bool enabled) {
    isEditMode = enabled;

    if (enabled) {
        setWindowFlag(Qt::WindowTransparentForInput, false);
        sizeGrip->show();
    } else {
        setWindowFlag(Qt::WindowTransparentForInput, true);
        sizeGrip->hide();

        QSettings().setValue("pos", pos());
        QSettings().setValue("size", size());
    }

    updateContainerStyle();
    show();
}

void OverlayWindow::updateContainerStyle() {
    if (isEditMode) {
        centralWidget()->setProperty("containerState", "edit");
    } else if (isConfigActive) {
        centralWidget()->setProperty("containerState", "config");
    } else {
        centralWidget()->setProperty("containerState", "normal");
    }
    centralWidget()->style()->unpolish(centralWidget());
    centralWidget()->style()->polish(centralWidget());
}

void OverlayWindow::mousePressEvent(QMouseEvent* event) {
    if (isEditMode && event->button() == Qt::LeftButton) {
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void OverlayWindow::mouseMoveEvent(QMouseEvent* event) {
    if (isEditMode && event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

void OverlayWindow::mouseReleaseEvent(QMouseEvent* event) {
    QMainWindow::mouseReleaseEvent(event);
}

void OverlayWindow::showDisconnected() {
    currentlyInVc = false;
    channelTitle->setText("DISCONNECTED");

    for (auto* widget : activeWidgets.values()) {
        widget->animateOut();
    }
    activeWidgets.clear();
}

void OverlayWindow::updateState(const QJsonObject& state) {
    if (!state.contains("voiceChannel") || state["voiceChannel"].isNull()) {
        if (currentlyInVc) showDisconnected();
        return;
    }

    currentlyInVc = true;
    QJsonObject voice = state["voiceChannel"].toObject();
    channelTitle->setText(voice["name"].toString().toUpper());

    QString selfId = state.contains("self") ? state["self"].toObject()["id"].toString() : "";
    QJsonArray members = voice["members"].toArray();
    QSet<QString> currentMembers;

    for (int i = 0; i < members.size(); ++i) {
        QJsonObject m = members[i].toObject();
        QString id = m["id"].toString();
        currentMembers.insert(id);

        if (activeWidgets.contains(id)) {
            activeWidgets[id]->updateData(m);
        } else {
            MemberWidget* widget = new MemberWidget(m, this);
            listLayout->insertWidget(listLayout->count() - 1, widget);
            activeWidgets.insert(id, widget);
            widget->animateIn();
        }

        QString url = m["avatarUrl"].toString();
        if (!url.isEmpty()) {
            if (avatarCache.contains(url)) {
                activeWidgets[id]->setAvatar(avatarCache[url]);
            } else {
                activeWidgets[id]->setAvatar(QPixmap());
                loadAvatar(id, url);
            }
        } else {
            activeWidgets[id]->setAvatar(QPixmap());
        }
    }

    int expectedIndex = 0;
    if (activeWidgets.contains(selfId)) {
        if (listLayout->indexOf(activeWidgets[selfId]) != expectedIndex) {
            listLayout->removeWidget(activeWidgets[selfId]);
            listLayout->insertWidget(expectedIndex, activeWidgets[selfId]);
        }
        expectedIndex++;
    }

    for (const QString& id : currentMembers) {
        if (id == selfId) continue;
        if (listLayout->indexOf(activeWidgets[id]) != expectedIndex) {
            listLayout->removeWidget(activeWidgets[id]);
            listLayout->insertWidget(expectedIndex, activeWidgets[id]);
        }
        expectedIndex++;
    }

    auto it = activeWidgets.begin();
    while (it != activeWidgets.end()) {
        if (!currentMembers.contains(it.key())) {
            MemberWidget* w = it.value();
            w->animateOut();
            it = activeWidgets.erase(it);
        } else {
            ++it;
        }
    }
}

void OverlayWindow::loadAvatar(const QString& memberId, const QString& url) {
    QString safeUrl = url;
    safeUrl.replace(".webp", ".png");
    QNetworkRequest request((QUrl(safeUrl)));
    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, memberId, url, safeUrl]() {
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pix;
            if (pix.loadFromData(reply->readAll())) {
                avatarCache.insert(url, pix);
                if (activeWidgets.contains(memberId) && activeWidgets[memberId]->avatarUrl == url) {
                    activeWidgets[memberId]->setAvatar(pix);
                }
            } else {
                qDebug() << "Still failing to decode data for:" << safeUrl;
            }
        } else {
            qDebug() << "Network Error:" << reply->errorString();
        }
        reply->deleteLater();
    });
}