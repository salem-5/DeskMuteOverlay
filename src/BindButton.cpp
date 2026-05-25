#include "BindButton.h"
#include <QKeySequence>

BindButton::BindButton(const QString& currentBind, QWidget* parent) : QPushButton(currentBind, parent) {
    currentBindStr = currentBind;
    setCheckable(true);
    setFocusPolicy(Qt::StrongFocus);

    connect(this, &QPushButton::toggled, this, [this](bool checked) {
        setText(checked ? "Listening (Press Key or Mouse 4/5)..." : currentBindStr);
    });
}

void BindButton::keyPressEvent(QKeyEvent* event) {
    if (!isChecked()) {
        QPushButton::keyPressEvent(event);
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        setChecked(false);
        return;
    }
    int key = event->key();

    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta) {
        return;
    }

    QKeySequence seq(event->modifiers() | key);
    currentBindStr = seq.toString();
    setChecked(false);
    emit bindChanged(currentBindStr);
}

void BindButton::mousePressEvent(QMouseEvent* event) {
    if (!isChecked()) {
        QPushButton::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::BackButton || event->button() == Qt::ExtraButton1) {
        currentBindStr = "Mouse 4";
        setChecked(false);
        emit bindChanged(currentBindStr);
    } else if (event->button() == Qt::ForwardButton || event->button() == Qt::ExtraButton2) {
        currentBindStr = "Mouse 5";
        setChecked(false);
        emit bindChanged(currentBindStr);
    } else if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        setChecked(false);
    }
}