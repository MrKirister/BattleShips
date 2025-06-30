#ifndef CONNECTION_H
#define CONNECTION_H

#include <QHostAddress>
#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QObject>
#include <QJsonValue>

class Client : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Client)
public:
    explicit Client(QObject *parent = nullptr);
public slots:
    void connectToServer(const QHostAddress &address, quint16 port);
    void login(const QString &userName, const QString &uid);
    void sendMessage(const QString &text);
    void disconnectFromHost();
    void invite(const QString &uid);
    void acceptGame(const QString &uid);
    void declineGame(const QString &uid);
private slots:
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
signals:
    //void gameOver(int player);
    void gameDeclined(const QString &username, const QString &uid);
    void gameAccepted(const QString &username, const QString &uid);
    void invited(const QString &username, const QString &uid);
    void userList(const QStringList &list);
    void connected();
    void loggedIn();
    void loginError(const QString &reason);
    void disconnected();
    void messageReceived(const QString &sender, const QString &text);
    void error(const QString &error);
    void userJoined(const QString &username, const QString &uid);
    void userLeft(const QString &username, const QString &uid);
    //void newGame(const QVector<Cell> &data, int size, int colorCount);
    //void newMove(const QVector<QPair<int, int>> &captured, int player);
private:
    QTcpSocket *m_clientSocket;
    bool m_loggedIn;
    void jsonReceived(const QJsonObject &doc);
    void sendToServer();
    void sendData();
    QTimer timer;
    QMap<QString, QJsonValue> args;
};

#endif
