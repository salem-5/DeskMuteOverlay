#ifndef BINDBUTTON_H
#define BINDBUTTON_H

#include <QPushButton>
#include <QString>
#include <QKeyEvent>
#include <QMouseEvent>

class BindButton : public QPushButton {
    Q_OBJECT
public:
    explicit BindButton(const QString& currentBind, QWidget* parent = nullptr);
    QString currentBindStr;

    signals:
        void bindChanged(const QString& newBind);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
};

#endif