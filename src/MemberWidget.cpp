#include "MemberWidget.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QRadialGradient>
#include <QPalette>
#include <QFile>
#include "AnimatedIconWidget.h"

static QByteArray loadSvgData(const QString& path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        return file.readAll();
    }
    return QByteArray();
}

MemberWidget::MemberWidget(const QJsonObject& data, QWidget* parent) : QFrame(parent) {
    memberId = data["id"].toString();

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
    nameLabel->setObjectName("nameLabel");
    layout->addWidget(nameLabel);
    layout->addStretch();

    liveLabel = new QLabel("LIVE", this);
    liveLabel->setObjectName("liveLabel");
    liveLabel->setVisible(false);
    layout->addWidget(liveLabel);

    micIcon = new AnimatedIconWidget(loadSvgData(":/icons/mic_on.svg"), loadSvgData(":/icons/mic_off.svg"), this);
    micIcon->setFixedSize(20, 20);
    layout->addWidget(micIcon);

    deafenIcon = new AnimatedIconWidget(loadSvgData(":/icons/headset_on.svg"), loadSvgData(":/icons/headset_off.svg"), this);
    deafenIcon->setFixedSize(20, 20);
    layout->addWidget(deafenIcon);

    updateData(data);
}

void MemberWidget::updateData(const QJsonObject& data) {
    if (data.contains("displayName")) nameLabel->setText(data["displayName"].toString());
    if (data.contains("avatarUrl")) avatarUrl = data["avatarUrl"].toString();

    bool muted = data["isMuted"].toBool() || data["isServerMuted"].toBool();
    bool deafened = data["isDeafened"].toBool() || data["isServerDeafened"].toBool();
    bool speaking = data["isSpeaking"].toBool();
    bool streaming = data["isStreaming"].toBool();

    if (isFirstUpdate) {
        currentMuted = muted;
        currentDeafened = deafened;
        currentSpeaking = speaking;
        currentStreaming = streaming;

        liveLabel->setVisible(streaming);

        speakingIntensity = speaking ? 1.0 : 0.0;
        isFirstUpdate = false;
        updateStatusIcons(muted, deafened);
        updateLabelColors();
    } else {
        if (streaming != currentStreaming) {
            currentStreaming = streaming;
            liveLabel->setVisible(streaming);
        }
        if (muted != currentMuted || deafened != currentDeafened) {
            currentMuted = muted;
            currentDeafened = deafened;
            animateStateChange(muted, deafened);
        }
        if (speaking != currentSpeaking) {
            currentSpeaking = speaking;
            auto* anim = new QVariantAnimation(this);
            anim->setDuration(200);
            anim->setStartValue(speakingIntensity);
            anim->setEndValue(speaking ? 1.0 : 0.0);
            connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
                speakingIntensity = value.toReal();
                updateLabelColors();
                update();
            });
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
}

QColor MemberWidget::interpolateColor(const QColor& start, const QColor& end, qreal progress) {
    return QColor(
        start.red() + (end.red() - start.red()) * progress,
        start.green() + (end.green() - start.green()) * progress,
        start.blue() + (end.blue() - start.blue()) * progress,
        start.alpha() + (end.alpha() - start.alpha()) * progress
    );
}

void MemberWidget::updateLabelColors() {
    QColor normalColor(255, 255, 255, (currentMuted || currentDeafened) ? 153 : 255);
    QColor speakingColor(149, 250, 142, 255);

    QPalette pal = nameLabel->palette();
    pal.setColor(QPalette::WindowText, interpolateColor(normalColor, speakingColor, speakingIntensity));
    nameLabel->setPalette(pal);
}

void MemberWidget::animateStateChange(bool muted, bool deafened) {
    updateStatusIcons(muted, deafened);
    if (!currentSpeaking) {
        updateLabelColors();
    }
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

void MemberWidget::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);

    if (speakingIntensity > 0.0) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath clipPath;
        clipPath.addRoundedRect(rect(), 16, 16);
        painter.setClipPath(clipPath);

        QPointF center(avatarLabel->pos().x() + avatarLabel->width() / 2.0, avatarLabel->pos().y() + avatarLabel->height() / 2.0);

        QRadialGradient glowGrad(center, 80.0);
        glowGrad.setColorAt(0.0, QColor(120, 255, 205, int(70 * speakingIntensity)));
        glowGrad.setColorAt(0.5, QColor(120, 255, 205, int(20 * speakingIntensity)));
        glowGrad.setColorAt(1.0, Qt::transparent);

        painter.setPen(Qt::NoPen);
        painter.setBrush(glowGrad);
        painter.drawEllipse(center, 80.0, 80.0);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(120, 255, 205, int(100 * speakingIntensity)), 1.5));
        painter.drawEllipse(center, 17.0, 17.0);
    }
}

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