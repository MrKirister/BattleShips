#ifndef CONNECTION_H
#define CONNECTION_H

#include <QHostAddress>
#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QObject>

#include "enums.h"

#include <QCborStreamReader>
#include <QCborStreamWriter>

class Client : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Client)
public:
    explicit Client(QObject *parent = nullptr);
    ~Client();
public slots:
    void connectToServer(const QHostAddress &address, quint16 port);
    void login(const QString &userName, const QString &uid);
    void sendMessage(const QString &text);
    void disconnectFromHost();
    void invite(const QString &uid);
    void acceptGame(const QString &uid);
    void declineGame(const QString &uid);
    void readyPressed(bool isReady, const QString &uid);
    void startGame(bool invitorFirst, const QString &uid);
    void checkRivalCell(int row, int col, const QString &uid);
    void cellChecked(int val, const QString &uid, int row, int col);
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
    void playerReady(bool isReady, const QString &uid);
    void gameStarted(bool invitorFirst);
    void checkMyCell(int row, int col);
    void cellCheckedResult(int val, int row, int col);
    //void newGame(const QVector<Cell> &data, int size, int colorCount);
    //void newMove(const QVector<QPair<int, int>> &captured, int player);
private:
    QTcpSocket m_clientSocket;
    QCborStreamReader m_reader;
    QCborStreamWriter m_writer;
    bool m_started {false};
    bool m_writeOpened{false};
    DataList m_args;
    Type m_lastMessageType {Type::Unknown};
    DataList m_receivedData;
    int m_leftToRead{0};

    void dataReceived(const DataList &doc);
    void sendData();

    QByteArray handleByteArray();
    QString handleString();
    QVariantList handleArray();
    QVariantMap handleMap();
};

#endif
