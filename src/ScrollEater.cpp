#include "ScrollEater.h"
#include <QWidget>
#include <QCoreApplication>

ScrollEater::ScrollEater(QObject* parent) : QObject(parent) {}

bool ScrollEater::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        QWidget *widget = qobject_cast<QWidget*>(obj);

        if (widget && !widget->hasFocus()) {
            event->ignore();

            if (widget->parentWidget()) {
                QCoreApplication::sendEvent(widget->parentWidget(), event);
            }
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}