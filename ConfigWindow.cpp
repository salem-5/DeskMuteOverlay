#include "ConfigWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QShowEvent>
#include <QHideEvent>

ConfigWindow::ConfigWindow(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(320, 520);

    QWidget* container = new QWidget(this);
    container->setStyleSheet("QWidget { background-color: #111214; color: #F2F3F5; border-radius: 16px; } "
                             "QCheckBox { font-size: 13px; font-weight: 500; padding: 2px; } "
                             "QLabel { font-size: 11px; font-weight: bold; color: #80848E; } "
                             "QLineEdit, QSpinBox, QKeySequenceEdit { background-color: #1E1F22; border: none; border-radius: 8px; padding: 8px; color: #F2F3F5; font-weight: bold; } "
                             "QPushButton { background-color: #5865F2; border: none; padding: 10px; border-radius: 8px; font-weight: bold; } "
                             "QPushButton:hover { background-color: #4752C4; } "
                             "QSlider::groove:horizontal { border-radius: 4px; height: 8px; background: #1E1F22; } "
                             "QSlider::handle:horizontal { background: #5865F2; width: 16px; margin: -4px 0; border-radius: 8px; }");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(container);

    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(20, 20, 20, 20);

    auto* headerLayout = new QHBoxLayout();
    QLabel* title = new QLabel("Overlay Settings", this);
    title->setStyleSheet("font-size: 15px; color: #FFFFFF; background: transparent;");

    QPushButton* closeBtn = new QPushButton("✕", this);
    closeBtn->setFixedSize(26, 26);
    closeBtn->setStyleSheet("QPushButton { background-color: transparent; color: #80848E; font-size: 16px; padding: 0px; } "
                            "QPushButton:hover { color: #F23F43; }");
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::hide);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    layout->addLayout(headerLayout);
    layout->addSpacing(5);

    QSettings settings;

    auto* bindsLayout = new QHBoxLayout();

    auto* col1 = new QVBoxLayout();
    col1->addWidget(new QLabel("MUTE BIND", this));
    bindMute = new QKeySequenceEdit(QKeySequence(settings.value("bindMute", "Ctrl+M").toString()), this);
    connect(bindMute, &QKeySequenceEdit::keySequenceChanged, this, [this](const QKeySequence& ks) {
        QSettings().setValue("bindMute", ks.toString());
        emit muteKeyChanged(ks.toString());
    });
    col1->addWidget(bindMute);

    auto* col2 = new QVBoxLayout();
    col2->addWidget(new QLabel("DEAFEN BIND", this));
    bindDeafen = new QKeySequenceEdit(QKeySequence(settings.value("bindDeafen", "Ctrl+D").toString()), this);
    connect(bindDeafen, &QKeySequenceEdit::keySequenceChanged, this, [this](const QKeySequence& ks) {
        QSettings().setValue("bindDeafen", ks.toString());
        emit deafenKeyChanged(ks.toString());
    });
    col2->addWidget(bindDeafen);

    bindsLayout->addLayout(col1);
    bindsLayout->addLayout(col2);
    layout->addLayout(bindsLayout);

    layout->addWidget(new QLabel("OPEN SETTINGS BIND", this));
    bindConfig = new QKeySequenceEdit(QKeySequence(settings.value("bindConfig", "Ctrl+Shift+O").toString()), this);
    connect(bindConfig, &QKeySequenceEdit::keySequenceChanged, this, [this](const QKeySequence& ks) {
        QSettings().setValue("bindConfig", ks.toString());
        emit configKeyChanged(ks.toString());
    });
    layout->addWidget(bindConfig);

    layout->addSpacing(5);

    layout->addWidget(new QLabel("TCP HOST", this));
    hostInput = new QLineEdit(settings.value("host", "127.0.0.1").toString(), this);
    layout->addWidget(hostInput);

    layout->addWidget(new QLabel("TCP PORT", this));
    portInput = new QSpinBox(this);
    portInput->setRange(1, 65535);
    portInput->setButtonSymbols(QAbstractSpinBox::NoButtons);
    portInput->setValue(settings.value("port", 3845).toInt());
    layout->addWidget(portInput);

    btnApplyNetwork = new QPushButton("Apply Connection", this);
    btnApplyNetwork->setStyleSheet("background-color: #2B2D31;");
    connect(btnApplyNetwork, &QPushButton::clicked, this, [this]() {
        QSettings().setValue("host", hostInput->text());
        QSettings().setValue("port", portInput->value());
        emit connectionSettingsChanged(hostInput->text(), portInput->value());
    });
    layout->addWidget(btnApplyNetwork);

    layout->addSpacing(10);

    layout->addWidget(new QLabel("OVERLAY OPACITY", this));
    opacitySlider = new QSlider(Qt::Horizontal, this);
    opacitySlider->setRange(20, 100);
    opacitySlider->setValue(settings.value("opacity", 90).toInt());
    connect(opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        QSettings().setValue("opacity", value);
        emit opacityChanged(value / 100.0);
    });
    layout->addWidget(opacitySlider);

    layout->addSpacing(10);

    chkHideName = new QCheckBox("Hide Voice Channel Name", this);
    chkHideName->setChecked(settings.value("hideName", false).toBool());
    connect(chkHideName, &QCheckBox::toggled, this, [this](bool checked) {
        QSettings().setValue("hideName", checked);
        emit hideChannelNameChanged(checked);
    });
    layout->addWidget(chkHideName);

    chkRequireVc = new QCheckBox("Disable Hotkeys if not in VC", this);
    chkRequireVc->setChecked(settings.value("requireVc", true).toBool());
    connect(chkRequireVc, &QCheckBox::toggled, this, [this](bool checked) {
        QSettings().setValue("requireVc", checked);
        emit requireVcForHotkeysChanged(checked);
    });
    layout->addWidget(chkRequireVc);

    layout->addStretch();

    btnEditMode = new QPushButton("Unlock Overlay Position", this);
    layout->addWidget(btnEditMode);

    connect(btnEditMode, &QPushButton::clicked, this, [this]() {
        isEditing = !isEditing;
        btnEditMode->setText(isEditing ? "Lock Overlay Position" : "Unlock Overlay Position");
        btnEditMode->setStyleSheet(isEditing ? "background-color: #F23F43;" : "background-color: #5865F2;");
        emit toggleEditMode(isEditing);
    });

    emit hideChannelNameChanged(chkHideName->isChecked());
    emit requireVcForHotkeysChanged(chkRequireVc->isChecked());
    emit opacityChanged(opacitySlider->value() / 100.0);
}

void ConfigWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}
void ConfigWindow::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}
void ConfigWindow::showEvent(QShowEvent* event) {
    emit configVisibleChanged(true);
    QWidget::showEvent(event);
}
void ConfigWindow::hideEvent(QHideEvent* event) {
    if (isEditing) {
        isEditing = false;
        btnEditMode->setText("Unlock Overlay Position");
        btnEditMode->setStyleSheet("background-color: #5865F2;");
        emit toggleEditMode(false);
    }
    emit configVisibleChanged(false);
    QWidget::hideEvent(event);
}