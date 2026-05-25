#ifndef ANIMATEDICONWIDGET_H
#define ANIMATEDICONWIDGET_H

#include <QWidget>
#include <QSvgRenderer>
#include <QPropertyAnimation>

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

#endif