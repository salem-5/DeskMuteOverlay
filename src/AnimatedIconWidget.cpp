#include "AnimatedIconWidget.h"
#include <QPainter>

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