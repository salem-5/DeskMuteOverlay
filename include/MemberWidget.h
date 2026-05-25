#ifndef MEMBERWIDGET_H
#define MEMBERWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QJsonObject>
#include <QPixmap>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSvgRenderer>

class AnimatedIconWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit AnimatedIconWidget(const QByteArray& svgOn, const QByteArray& svgOff, QWidget* parent = nullptr);

    qreal progress() const { return m_progress; }
    void setProgress(qreal p);
    void setState(bool isOff, bool animated = true);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QSvgRenderer rendererOn;
    QSvgRenderer rendererOff;
    QPropertyAnimation* anim;
    qreal m_progress = 0.0;
};

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

private:
    void updateStatusIcons(bool muted, bool deafened);
    void animateStateChange(bool muted, bool deafened);
    void redrawAvatar();

    QLabel* avatarLabel;
    QLabel* nameLabel;

    AnimatedIconWidget* micIcon;
    AnimatedIconWidget* deafenIcon;

    QGraphicsOpacityEffect* opacityEffect;

    QPixmap m_cachedAvatar;
    bool isFirstUpdate = true;
    bool currentMuted = false;
    bool currentDeafened = false;
    bool currentSpeaking = false;
};

#endif