#include "MemberWidget.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <qmath.h>

// Constants for SVGs
const QByteArray SVG_MIC_ON = R"(<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M12 14c1.66 0 3-1.34 3-3V5c0-1.66-1.34-3-3-3S9 3.34 9 5v6c0 1.66 1.34 3 3 3z" fill="#B5BAC1"/><path d="M17 11c0 2.76-2.24 5-5 5s-5-2.24-5-5H5c0 3.53 2.61 6.43 6 6.92V21h2v-3.08c3.39-.49 6-3.39 6-6.92h-2z" fill="#B5BAC1"/></svg>)";
const QByteArray SVG_MIC_OFF = R"(<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M12 14c1.66 0 3-1.34 3-3V5c0-1.66-1.34-3-3-3S9 3.34 9 5v6c0 1.66 1.34 3 3 3z" fill="#F23F43"/><path d="M17.73 11.3c-.18-.45-.69-.67-1.14-.49-.45.18-.67.69-.49 1.14C15.65 13.04 13.92 14 12 14s-3.65-.96-4.1-2.05c-.18-.45-.69-.67-1.14-.49-.45.18-.67.69-.49 1.14C6.91 14.15 9.24 15.77 12 15.96V19H9c-.55 0-1 .45-1 1s.45 1 1 1h6c-.55 0-1-.45-1-1s.45-1-1-1h-3v-3.04c2.76-.19 5.09-1.81 5.73-4.66z" fill="#F23F43"/><line x1="4.5" y1="4.5" x2="19.5" y2="19.5" stroke="#F23F43" stroke-width="2" stroke-linecap="round"/></svg>)";
const QByteArray SVG_HEADSET_ON = R"(<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path fill-rule="evenodd" clip-rule="evenodd" d="M12 2C6.48 2 2 6.48 2 12v7c0 1.1.9 2 2 2h3c1.1 0 2-.9 2-2v-4c0-1.1-.9-2-2-2H4v-1c0-4.41 3.59-8 8-8s8 3.41 8 8v1h-3c-1.1 0-2 .9-2 2v4c0 1.1.9 2 2 2h3c1.1 0 2-.9 2-2v-7c0-5.52-4.48-10-10-10z" fill="#B5BAC1"/></svg>)";
const QByteArray SVG_HEADSET_OFF = R"(<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path fill-rule="evenodd" clip-rule="evenodd" d="M12 2C6.48 2 2 6.48 2 12v7c0 1.1.9 2 2 2h3c1.1 0 2-.9 2-2v-4c0-1.1-.9-2-2-2H4v-1c0-4.41 3.59-8 8-8s8 3.41 8 8v1h-3c-1.1 0-2 .9-2 2v4c0 1.1.9 2 2 2h3c1.1 0 2-.9 2-2v-7c0-5.52-4.48-10-10-10z" fill="#F23F43"/><line x1="4.5" y1="4.5" x2="19.5" y2="19.5" stroke="#F23F43" stroke-width="2" stroke-linecap="round"/></svg>)";

// --- AnimatedIconWidget Implementation ---
AnimatedIconWidget::AnimatedIconWidget(const QByteArray& svgOn, const QByteArray& svgOff, QWidget* parent)
    : QWidget(parent), rendererOn(svgOn), rendererOff(svgOff) {
    anim = new QPropertyAnimation(this, "progress", this);
    anim->setDuration(300);
    anim->setEasingCurve(QEasingCurve::InOutBack);
}

void AnimatedIconWidget::setProgress(qreal p) {
    m_progress = p;
    update();
}

void AnimatedIconWidget::setState(bool isOff, bool animated) {
    qreal target = isOff ? 1.0 : 0.0;
    if (m_progress == target) return;
    if (animated) {
        anim->stop();
        anim->setStartValue(m_progress);
        anim->setEndValue(target);
        anim->start();
    } else {
        setProgress(target);
    }
}

void AnimatedIconWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    qreal opOn = qBound(0.0, 1.0 - m_progress, 1.0);
    qreal opOff = qBound(0.0, m_progress, 1.0);
    qreal scale = 1.0 - (0.3 * (1.0 - qAbs(qBound(0.0, m_progress, 1.0) - 0.5) * 2.0));
    painter.translate(width() / 2.0, height() / 2.0);
    painter.scale(scale, scale);
    painter.translate(-width() / 2.0, -height() / 2.0);
    if (opOn > 0.0) { painter.setOpacity(opOn); rendererOn.render(&painter, rect()); }
    if (opOff > 0.0) { painter.setOpacity(opOff); rendererOff.render(&painter, rect()); }
}

// --- MemberWidget Implementation ---
MemberWidget::MemberWidget(const QJsonObject& data, QWidget* parent) : QFrame(parent) {
    memberId = data["id"].toString();
    setStyleSheet("MemberWidget { background-color: rgba(0, 0, 0, 0.6); border-radius: 16px; }");
    opacityEffect = new QGraphicsOpacityEffect(this);
    opacityEffect->setOpacity(1.0);
    setGraphicsEffect(opacityEffect);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 8, 12, 8);
    layout->setSpacing(12);

    avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(32, 32);
    layout->addWidget(avatarLabel);

    nameLabel = new QLabel(this);
    nameLabel->setStyleSheet("color: #FFFFFF; font-size: 14px; font-weight: bold;");
    layout->addWidget(nameLabel);
    layout->addStretch();

    micIcon = new AnimatedIconWidget(SVG_MIC_ON, SVG_MIC_OFF, this);
    micIcon->setFixedSize(20, 20);
    layout->addWidget(micIcon);

    deafenIcon = new AnimatedIconWidget(SVG_HEADSET_ON, SVG_HEADSET_OFF, this);
    deafenIcon->setFixedSize(20, 20);
    layout->addWidget(deafenIcon);

    updateData(data);
}

void MemberWidget::updateData(const QJsonObject& data) {
    if (data.contains("displayName")) nameLabel->setText(data["displayName"].toString());
    if (data.contains("avatarUrl")) avatarUrl = data["avatarUrl"].toString();

    bool muted = data["isMuted"].toBool() || data["isServerMuted"].toBool();
    bool deafened = data["isDeafened"].toBool() || data["isServerDeafened"].toBool();

    if (isFirstUpdate) {
        currentMuted = muted;
        currentDeafened = deafened;
        isFirstUpdate = false;
        updateStatusIcons(muted, deafened);
        nameLabel->setStyleSheet(QString("color: rgba(255, 255, 255, %1); font-size: 14px; font-weight: bold;").arg((muted || deafened) ? "0.6" : "1.0"));
    } else {
        if (muted != currentMuted || deafened != currentDeafened) {
            animateStateChange(muted, deafened);
            currentMuted = muted;
            currentDeafened = deafened;
        }
    }
}

void MemberWidget::animateStateChange(bool muted, bool deafened) {
    updateStatusIcons(muted, deafened);
    auto* colorAnim = new QVariantAnimation(this);
    colorAnim->setDuration(300);
    colorAnim->setStartValue((currentMuted || currentDeafened) ? 0.6 : 1.0);
    colorAnim->setEndValue((muted || deafened) ? 0.6 : 1.0);
    connect(colorAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val){
        nameLabel->setStyleSheet(QString("color: rgba(255, 255, 255, %1); font-size: 14px; font-weight: bold;").arg(val.toDouble()));
    });
    colorAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MemberWidget::updateStatusIcons(bool muted, bool deafened) {
    micIcon->setState(muted || deafened, !isFirstUpdate);
    deafenIcon->setState(deafened, !isFirstUpdate);
}

void MemberWidget::setAvatar(const QPixmap& pixmap) {
    QPixmap target(32, 32);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, 32, 32);
    painter.setClipPath(path);
    if (pixmap.isNull()) {
        painter.fillRect(0, 0, 32, 32, QColor("#5865F2"));
        painter.setPen(QColor("#FFFFFF"));
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        QString firstLetter = nameLabel->text().isEmpty() ? "?" : nameLabel->text().left(1).toUpper();
        painter.drawText(QRect(0, 0, 32, 32), Qt::AlignCenter, firstLetter);
    } else {
        painter.drawPixmap(0, 0, pixmap.scaled(32, 32, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
    avatarLabel->setPixmap(target);
}

// Animate functions kept as required
void MemberWidget::animateIn() {
    setMinimumHeight(0);
    setMaximumHeight(0);
    opacityEffect->setOpacity(0.0);

    auto* heightAnim = new QPropertyAnimation(this, "maximumHeight");
    heightAnim->setDuration(350);
    heightAnim->setStartValue(0);
    heightAnim->setEndValue(48);
    heightAnim->setEasingCurve(QEasingCurve::OutBack);

    auto* minHeightAnim = new QPropertyAnimation(this, "minimumHeight");
    minHeightAnim->setDuration(350);
    minHeightAnim->setStartValue(0);
    minHeightAnim->setEndValue(48);
    minHeightAnim->setEasingCurve(QEasingCurve::OutBack);

    auto* opacityAnim = new QPropertyAnimation(opacityEffect, "opacity");
    opacityAnim->setDuration(300);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);

    heightAnim->start(QAbstractAnimation::DeleteWhenStopped);
    minHeightAnim->start(QAbstractAnimation::DeleteWhenStopped);
    opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MemberWidget::animateOut() {
    auto* heightAnim = new QPropertyAnimation(this, "maximumHeight");
    heightAnim->setDuration(250);
    heightAnim->setStartValue(height());
    heightAnim->setEndValue(0);
    heightAnim->setEasingCurve(QEasingCurve::InBack);

    auto* minHeightAnim = new QPropertyAnimation(this, "minimumHeight");
    minHeightAnim->setDuration(250);
    minHeightAnim->setStartValue(height());
    minHeightAnim->setEndValue(0);
    minHeightAnim->setEasingCurve(QEasingCurve::InBack);

    auto* opacityAnim = new QPropertyAnimation(opacityEffect, "opacity");
    opacityAnim->setDuration(200);
    opacityAnim->setStartValue(opacityEffect->opacity());
    opacityAnim->setEndValue(0.0);

    connect(heightAnim, &QPropertyAnimation::finished, this, &QObject::deleteLater);

    heightAnim->start(QAbstractAnimation::DeleteWhenStopped);
    minHeightAnim->start(QAbstractAnimation::DeleteWhenStopped);
    opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
}