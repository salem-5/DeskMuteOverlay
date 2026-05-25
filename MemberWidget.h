#ifndef MEMBERWIDGET_H
#define MEMBERWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QSvgWidget>
#include <QJsonObject>
#include <QPixmap>
#include <QGraphicsOpacityEffect>

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

    QLabel* avatarLabel;
    QLabel* nameLabel;
    QSvgWidget* micIcon;
    QSvgWidget* deafenIcon;
    QGraphicsOpacityEffect* opacityEffect;

    bool isFirstUpdate = true;
    bool currentMuted = false;
    bool currentDeafened = false;
};

#endif // MEMBERWIDGET_H