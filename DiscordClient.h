#ifndef DISCORDCLIENT_H
#define DISCORDCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QByteArray>

class DiscordClient : public QObject {
    Q_OBJECT
public:
    explicit DiscordClient(QObject* parent = nullptr);
    void connectToServer();
    void sendAction(const QString& action);

public slots:
    void setConnectionDetails(const QString& host, int port);

    signals:
        void stateReceived(const QJsonObject& state);
    void disconnected();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* socket;
    QByteArray buffer;

    QString currentHost = "127.0.0.1";
    int currentPort = 3845;
};

#endif