#pragma once

#include <QFrame>
#include <QLabel>
#include <QJsonObject>
#include <QGraphicsOpacityEffect>
#include <QVariantAnimation>
#include <QPaintEvent>

class AnimatedIconWidget;

class MemberWidget : public QFrame {
    Q_OBJECT
public:
    explicit MemberWidget(const QJsonObject& data, QWidget* parent = nullptr);

    void updateData(const QJsonObject& data);
    void setAvatar(const QPixmap& pixmap);
    void animateIn();
    void animateOut();

    QString memberId;
    QString avatarUrl;

protected:
    void paintEvent(QPaintEvent* event) override;
private:
    void animateStateChange(bool muted, bool deafened, bool video);
    void updateStatusIcons(bool muted, bool deafened, bool video);
    void updateLabelColors();
    QColor interpolateColor(const QColor& start, const QColor& end, qreal progress);

    QLabel* avatarLabel;
    QLabel* nameLabel;
    QLabel* liveLabel;
    AnimatedIconWidget* micIcon;
    AnimatedIconWidget* deafenIcon;
    AnimatedIconWidget* videoIcon;
    QGraphicsOpacityEffect* opacityEffect;

    bool isFirstUpdate = true;
    bool currentMuted = false;
    bool currentDeafened = false;
    bool currentSpeaking = false;
    bool currentStreaming = false;
    bool currentVideo = false;
    qreal speakingIntensity = 0.0;
};