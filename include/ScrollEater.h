#ifndef SCROLLEATER_H
#define SCROLLEATER_H

#include <QObject>
#include <QEvent>

class ScrollEater : public QObject {
    Q_OBJECT
public:
    explicit ScrollEater(QObject* parent = nullptr);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif