#include "client.h"

#include <QTcpSocket>
#include <QDataStream>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

Client::Client(QObject *parent)
    : QObject(parent)
    , m_clientSocket(new QTcpSocket(this))
    , m_loggedIn(false)
{
    timer.setInterval(500);
    connect(&timer, &QTimer::timeout, this, &Client::sendData);
    // Forward the connected and disconnected signals

    connect(m_clientSocket, &QTcpSocket::connected, this, &Client::connected);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &Client::disconnected);
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    connect(m_clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &ChatClient::error);
#else
    connect(m_clientSocket, &QAbstractSocket::errorOccurred, this, &Client::onError);
#endif
    connect(m_clientSocket, &QTcpSocket::disconnected, this, [this]()->void{m_loggedIn = false;});
}

void Client::disconnectFromHost()
{
    m_clientSocket->disconnectFromHost();
}

void Client::sendToServer()
{
    if (args.isEmpty()) return; // сообщение не подготовлено

    if (timer.isActive()) {
        qDebug() << "Previous message was not received. Aborting the previous message";
        timer.stop();
    }

    static int messageID = 0;
    args[QLatin1String("messageID")] = messageID;
    messageID++;

    sendData();
}

void Client::sendData()
{
    if (m_clientSocket->state() == QAbstractSocket::ConnectedState) {
        QDataStream clientStream(m_clientSocket);
        clientStream.setVersion(QDataStream::Qt_5_7);
        QJsonObject message;
        for (auto i = args.cbegin(); i!=args.cend(); ++i) {
            message[i.key()] = i.value();
        }
        clientStream << QJsonDocument(message).toJson(QJsonDocument::Compact);
    }
    else qDebug() << "no connection to server";
    if (!timer.isActive()) timer.start();
}

void Client::login(const QString &userName, const QString &uid)
{
    args.clear();

    args.insert(QLatin1String("type"), QLatin1String("login"));
    args.insert(QLatin1String("uid"), uid);
    args.insert(QLatin1String("username"), userName);

    sendToServer();
}

void Client::sendMessage(const QString &text)
{
    if (text.isEmpty()) return;

    args.clear();

    args.insert(QLatin1String("type"), QLatin1String("message"));
    args.insert(QLatin1String("text"), text);
    args.insert(QLatin1String("receiver"), QLatin1String("all"));

    sendToServer();
}

void Client::invite(const QString &uid)
{
    args.clear();
    args.insert(QLatin1String("type"), QLatin1String("invite"));
    args.insert(QLatin1String("receiver"), uid);
    sendToServer();
}

void Client::acceptGame(const QString &uid)
{
    args.clear();
    args.insert(QLatin1String("type"), QLatin1String("accepted"));
    args.insert(QLatin1String("receiver"), uid);
    sendToServer();
}

void Client::declineGame(const QString &uid)
{
    args.clear();
    args.insert(QLatin1String("type"), QLatin1String("declined"));
    args.insert(QLatin1String("receiver"), uid);
    sendToServer();
}

void Client::jsonReceived(const QJsonObject &docObj)
{
    qDebug() << docObj;

    // Проверяем, не ответное ли это сообщение
    const bool received = docObj.value(QLatin1String("received")).toBool();
    if (received) {
        timer.stop(); // останавливаем таймер, так как нам пришел ответ от
                      // сервера, что сообщение получено
        return;
    }

    const auto type = docObj.value(QLatin1String("type")).toString();
    if (type == QLatin1String("login") && !m_loggedIn) {
        const QJsonValue resultVal = docObj.value(QLatin1String("success"));
        if (resultVal.isNull() || !resultVal.isBool())
            return;
        const bool loginSuccess = resultVal.toBool();
        if (loginSuccess) {
            m_loggedIn = true;
            emit loggedIn();
        }
        else {
            emit loginError(docObj.value(QLatin1String("reason")).toString());
        }
        const auto users = docObj.value(QLatin1String("users")).toArray();
        QStringList list;
        std::transform(users.begin(), users.end(), std::back_inserter(list), [](QJsonValue a){
            return a.toString();
        });
        emit userList(list);
    }
    else if (type == QLatin1String("message")) {
        const auto textVal = docObj.value(QLatin1String("text")).toString();
        const auto senderVal = docObj.value(QLatin1String("sender")).toString();
        if (!textVal.isEmpty() && !senderVal.isEmpty())
            emit messageReceived(senderVal, textVal);
    }
    else if (type == QLatin1String("newuser")) { // A user joined the game
        const auto username = docObj.value(QLatin1String("username")).toString();
        const auto uid = docObj.value(QLatin1String("uid")).toString();
        if (!username.isEmpty() && !uid.isEmpty())
            emit userJoined(username, uid);
    }
    else if (type == QLatin1String("userdisconnected")) { // A user left the game
        const auto username = docObj.value(QLatin1String("username")).toString();
        const auto uid = docObj.value(QLatin1String("uid")).toString();
        if (!username.isEmpty() && !uid.isEmpty())
            emit userLeft(username, uid);
    }
    // An invite was received from sender
    else if (type == QLatin1String("invite")) {
        const auto username = docObj.value(QLatin1String("sender")).toString();
        const auto uid = docObj.value(QLatin1String("senderUid")).toString();
        if(!uid.isEmpty() && !username.isEmpty())
            emit invited(username, uid);
    }
    // Sender accepted out invite
    else if (type == QLatin1String("accepted")) {
        const auto username = docObj.value(QLatin1String("sender")).toString();
        const auto uid = docObj.value(QLatin1String("senderUid")).toString();
        if(!uid.isEmpty() && !username.isEmpty())
            emit gameAccepted(username, uid);
    }
    // Sender declined our invite
    else if (type == QLatin1String("declined")) {
        const auto username = docObj.value(QLatin1String("sender")).toString();
        const auto uid = docObj.value(QLatin1String("senderUid")).toString();
        if(!uid.isEmpty() && !username.isEmpty())
            emit gameDeclined(username, uid);
    }
    else {
        qDebug() << "A message with unknown type:" << type << ", ignore";
        // a message with no type was received so we just ignore it
    }
}

void Client::connectToServer(const QHostAddress &address, quint16 port)
{
    m_clientSocket->connectToHost(address, port);
}

void Client::onReadyRead()
{
    QByteArray jsonData;
    QDataStream socketStream(m_clientSocket);
    socketStream.setVersion(QDataStream::Qt_5_7);
    for (;;) {
        socketStream.startTransaction();
        socketStream >> jsonData;
        if (socketStream.commitTransaction()) {
            qDebug() << "full transaction" << jsonData;
            QJsonParseError parseError;
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (jsonDoc.isObject())
                    jsonReceived(jsonDoc.object());
            }
        } else {
            qDebug() << "partial transaction:" << jsonData;
            break;
        }
    }
}

void Client::onError(QAbstractSocket::SocketError error)
{
    QString message;
    switch (error) {
    case QAbstractSocket::RemoteHostClosedError:
    case QAbstractSocket::ProxyConnectionClosedError:
        return; // handled by disconnectedFromServer
    case QAbstractSocket::ConnectionRefusedError:
        message = tr("The host refused the connection");
        break;
    case QAbstractSocket::ProxyConnectionRefusedError:
        message = tr("The proxy refused the connection");
        break;
    case QAbstractSocket::ProxyNotFoundError:
        message = tr("Could not find the proxy");
        break;
    case QAbstractSocket::HostNotFoundError:
        message = tr("Could not find the server");
        break;
    case QAbstractSocket::SocketAccessError:
        message = tr("You don't have permissions to execute this operation");
        break;
    case QAbstractSocket::SocketResourceError:
        message = tr("Too many connections opened");
        break;
    case QAbstractSocket::SocketTimeoutError:
        message = tr("Operation timed out");
        return;
    case QAbstractSocket::ProxyConnectionTimeoutError:
        message = tr("Proxy timed out");
        break;
    case QAbstractSocket::NetworkError:
        message = tr("Unable to reach the network");
        break;
    case QAbstractSocket::UnknownSocketError:
        message = tr("An unknown error occured");
        break;
    case QAbstractSocket::UnsupportedSocketOperationError:
        message = tr("Operation not supported");
        break;
    case QAbstractSocket::ProxyAuthenticationRequiredError:
        message = tr("Your proxy requires authentication");
        break;
    case QAbstractSocket::ProxyProtocolError:
        message = tr("Proxy comunication failed");
        break;
    case QAbstractSocket::TemporaryError:
    case QAbstractSocket::OperationError:
        message = tr("Operation failed, please try again");
        break;
    default:
        Q_UNREACHABLE();
    }
    emit this->error(message);
}
