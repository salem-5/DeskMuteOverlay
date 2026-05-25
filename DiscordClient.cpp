#include "DiscordClient.h"
#include <QJsonDocument>
#include <QTimer>

DiscordClient::DiscordClient(QObject* parent) : QObject(parent), socket(new QTcpSocket(this)) {
    connect(socket, &QTcpSocket::readyRead, this, &DiscordClient::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &DiscordClient::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit disconnected();
    });
}

void DiscordClient::setConnectionDetails(const QString& host, int port) {
    currentHost = host;
    currentPort = port;
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    } else {
        connectToServer();
    }
}

void DiscordClient::connectToServer() {
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        socket->connectToHost(currentHost, currentPort);
    }
}

void DiscordClient::sendAction(const QString& action) {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(action.toUtf8() + "\n");
    }
}

void DiscordClient::onReadyRead() {
    buffer.append(socket->readAll());
    while (buffer.contains('\n')) {
        int newlineIdx = buffer.indexOf('\n');
        QByteArray packet = buffer.left(newlineIdx).trimmed();
        buffer.remove(0, newlineIdx + 1);

        if (!packet.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(packet);
            if (doc.isObject()) emit stateReceived(doc.object());
        }
    }
}

void DiscordClient::onDisconnected() {
    emit disconnected();
    QTimer::singleShot(2000, this, &DiscordClient::connectToServer);
}