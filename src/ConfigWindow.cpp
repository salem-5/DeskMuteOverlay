#include "ConfigWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QShowEvent>
#include <QHideEvent>
#include <QKeySequence>
#include <QScrollArea>
#include <QStyle>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>

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

void ConfigWindow::fetchGistTunnel(bool force) {
    QString id = gistIdInput->text();
    QString pat = patInput->text();

    if (id.isEmpty() || pat.isEmpty())
        return;

    QNetworkRequest req(QUrl("https://api.github.com/gists/" + id));
    req.setRawHeader("Authorization", ("Bearer " + pat).toUtf8());
    req.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply* reply = net.get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, force]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        auto files = doc.object()["files"].toObject();
        if (files.isEmpty()) return;
        auto file = files.begin().value().toObject();

        QString content = file["content"].toString().trimmed();
        content = content.replace("tcp://", "");

        if (content.isEmpty() || (!force && content == lastTunnel))
            return;

        lastTunnel = content;

        QStringList parts = content.split(":");
        if (parts.size() != 2)
            return;

        QString host = parts[0];
        int port = parts[1].toInt();

        hostInput->setText(host);
        portInput->setValue(port);

        QSettings().setValue("host", host);
        QSettings().setValue("port", port);

        emit connectionSettingsChanged(host, port);
    });
}

ConfigWindow::ConfigWindow(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(380, 660);

    QWidget* container = new QWidget(this);
    container->setObjectName("mainContainer");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(container);

    auto* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(20, 20, 20, 20);

    auto* headerLayout = new QHBoxLayout();
    QLabel* title = new QLabel("Overlay Settings", this);
    title->setObjectName("configTitle");

    QPushButton* closeBtn = new QPushButton("✕", this);
    closeBtn->setObjectName("configCloseBtn");
    closeBtn->setFixedSize(26, 26);
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::hide);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    containerLayout->addLayout(headerLayout);
    containerLayout->addSpacing(5);

    QScrollArea* scrollArea = new QScrollArea(container);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName("scrollContent");

    auto* layout = new QVBoxLayout(scrollContent);
    layout->setContentsMargins(0, 0, 8, 0);

    QSettings settings;

    auto* bindsLayout = new QHBoxLayout();

    auto* col1 = new QVBoxLayout();
    col1->addWidget(new QLabel("MUTE BIND", this));
    bindMute = new BindButton(settings.value("bindMute", "Ctrl+M").toString(), this);
    connect(bindMute, &BindButton::bindChanged, this, [this](const QString& ks) {
        QSettings().setValue("bindMute", ks);
        emit muteKeyChanged(ks);
    });
    col1->addWidget(bindMute);

    auto* col2 = new QVBoxLayout();
    col2->addWidget(new QLabel("DEAFEN BIND", this));
    bindDeafen = new BindButton(settings.value("bindDeafen", "Ctrl+D").toString(), this);
    connect(bindDeafen, &BindButton::bindChanged, this, [this](const QString& ks) {
        QSettings().setValue("bindDeafen", ks);
        emit deafenKeyChanged(ks);
    });
    col2->addWidget(bindDeafen);

    bindsLayout->addLayout(col1);
    bindsLayout->addLayout(col2);
    layout->addLayout(bindsLayout);

    auto* secondaryBindsLayout = new QHBoxLayout();

    auto* col3 = new QVBoxLayout();
    col3->addWidget(new QLabel("OPEN SETTINGS BIND", this));
    bindConfig = new BindButton(settings.value("bindConfig", "Ctrl+Shift+O").toString(), this);
    connect(bindConfig, &BindButton::bindChanged, this, [this](const QString& ks) {
        QSettings().setValue("bindConfig", ks);
        emit configKeyChanged(ks);
    });
    col3->addWidget(bindConfig);

    auto* col4 = new QVBoxLayout();
    col4->addWidget(new QLabel("TOGGLE OVERLAY BIND", this));
    bindToggleOverlay = new BindButton(settings.value("bindToggle", "Ctrl+Shift+T").toString(), this);
    connect(bindToggleOverlay, &BindButton::bindChanged, this, [this](const QString& ks) {
        QSettings().setValue("bindToggle", ks);
        emit overlayToggleKeyChanged(ks);
    });
    col4->addWidget(bindToggleOverlay);

    secondaryBindsLayout->addLayout(col3);
    secondaryBindsLayout->addLayout(col4);
    layout->addLayout(secondaryBindsLayout);

    layout->addSpacing(5);
    btnToggleOverlay = new QPushButton("Toggle Overlay Visibility", this);
    btnToggleOverlay->setObjectName("secondaryBtn");
    connect(btnToggleOverlay, &QPushButton::clicked, this, &ConfigWindow::toggleOverlayVisibilityRequested);
    layout->addWidget(btnToggleOverlay);

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
    btnApplyNetwork->setObjectName("secondaryBtn");
    connect(btnApplyNetwork, &QPushButton::clicked, this, [this]() {
        QSettings().setValue("host", hostInput->text());
        QSettings().setValue("port", portInput->value());
        emit connectionSettingsChanged(hostInput->text(), portInput->value());
    });
    layout->addWidget(btnApplyNetwork);

    layout->addSpacing(10);
    layout->addWidget(new QLabel("GITHUB GIST ID", this));
    gistIdInput = new QLineEdit(settings.value("gistId", "").toString(), this);
    connect(gistIdInput, &QLineEdit::textChanged, this, [](const QString& text) {
        QSettings().setValue("gistId", text);
    });
    layout->addWidget(gistIdInput);

    layout->addWidget(new QLabel("GITHUB PAT", this));
    patInput = new QLineEdit(settings.value("githubPat", "").toString(), this);
    patInput->setEchoMode(QLineEdit::Password);
    connect(patInput, &QLineEdit::textChanged, this, [](const QString& text) {
        QSettings().setValue("githubPat", text);
    });
    layout->addWidget(patInput);

    btnLoadGist = new QPushButton("Load IP:PORT from Gist", this);
    layout->addWidget(btnLoadGist);

    chkAutoRefresh = new QCheckBox("Auto refresh every 5 minutes", this);
    chkAutoRefresh->setChecked(settings.value("autoGist", false).toBool());
    connect(chkAutoRefresh, &QCheckBox::toggled, this, [](bool checked) {
        QSettings().setValue("autoGist", checked);
    });
    layout->addWidget(chkAutoRefresh);

    gistTimer = new QTimer(this);
    gistTimer->setInterval(5 * 60 * 1000);
    gistTimer->start();

    connect(btnLoadGist, &QPushButton::clicked, this, [this]() {
        fetchGistTunnel(true);
    });

    connect(gistTimer, &QTimer::timeout, this, [this]() {
        if (chkAutoRefresh->isChecked())
            fetchGistTunnel(false);
    });

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
    btnEditMode->setObjectName("editModeBtn");
    layout->addWidget(btnEditMode);

    connect(btnEditMode, &QPushButton::clicked, this, [this]() {
        isEditing = !isEditing;
        btnEditMode->setText(isEditing ? "Lock Overlay Position" : "Unlock Overlay Position");
        btnEditMode->setProperty("editing", isEditing);
        btnEditMode->style()->unpolish(btnEditMode);
        btnEditMode->style()->polish(btnEditMode);
        emit toggleEditMode(isEditing);
    });

    scrollArea->setWidget(scrollContent);
    containerLayout->addWidget(scrollArea);

    emit hideChannelNameChanged(chkHideName->isChecked());
    emit requireVcForHotkeysChanged(chkRequireVc->isChecked());
    emit opacityChanged(opacitySlider->value() / 100.0);

    if (!gistIdInput->text().isEmpty() && !patInput->text().isEmpty()) {
        fetchGistTunnel(false);
    }
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
        btnEditMode->setProperty("editing", false);
        btnEditMode->style()->unpolish(btnEditMode);
        btnEditMode->style()->polish(btnEditMode);
        emit toggleEditMode(false);
    }
    emit configVisibleChanged(false);
    QWidget::hideEvent(event);
}