#ifndef CONNECTION_H
#define CONNECTION_H

#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QString>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>

#include <QObject>
class QHostAddress;
class QJsonDocument;
class Client : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Client)
public:
    explicit Client(QObject *parent = nullptr);
    void invite(const QString &uid, const QString &username, const QString &senderUid);
public slots:
    void connectToServer(const QHostAddress &address, quint16 port);
    void login(const QString &userName, const QString &uid);
    void sendMessage(const QString &text);
    void disconnectFromHost();
    void acceptGame(const QString &uid, const QString &username, const QString &senderUid);
    void declineGame(const QString &uid, const QString &username, const QString &senderUid);
    //void sendGame(const QVector<Cell> &data, int size, int colorCount); TODO: rewrite
    //bool sendNewMove(const QVector<QPair<int, int> > &captured, int player);
    //void sendGameOver(int player);
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
    QTimer timer;
    QStringList args;
};

#endif
