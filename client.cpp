#include "client.h"

#include <QtNetwork>

static const int TransferTimeout = 30 * 1000;
static const int PongTimeout = 60 * 1000;
static const int PingInterval = 5 * 1000;

#include <QTcpSocket>
#include <QDataStream>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

//TODO: Все методы сделать с таймером

Client::Client(QObject *parent)
    : QObject(parent)
    , m_clientSocket(new QTcpSocket(this))
    , m_loggedIn(false)
{
    timer.setInterval(500);
    // Forward the connected and disconnected signals

    connect(m_clientSocket, &QTcpSocket::connected, this, &Client::connected);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &Client::disconnected);
    // connect readyRead() to the slot that will take care of reading the data in
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    // Forward the error signal, QOverload is necessary as error() is overloaded, see the Qt docs
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    connect(m_clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &ChatClient::error);
#else
    connect(m_clientSocket, &QAbstractSocket::errorOccurred, this, &Client::onError);
#endif
    // Reset the m_loggedIn variable when we disconnec. Since the operation is trivial we use a lambda instead of creating another slot
    connect(m_clientSocket, &QTcpSocket::disconnected, this, [this]()->void{m_loggedIn = false;});
}

void Client::invite(const QString &uid, const QString &username, const QString &senderUid)
{
    args.clear();
    args.push_back(uid);
    args.push_back(username);
    args.push_back(senderUid);
    timer.disconnect();
    connect(&timer, &QTimer::timeout, this, [this](){
        if (args[0].isEmpty())
            return; // We don't send empty messages
        // create a QDataStream operating on the socket
        QDataStream clientStream(m_clientSocket);
        // set the version so that programs compiled with different versions of Qt can agree on how to serialise
        clientStream.setVersion(QDataStream::Qt_5_7);
        // Create the JSON we want to send
        QJsonObject message;
        message[QStringLiteral("type")] = QStringLiteral("invite");
        message[QStringLiteral("sender")] = args[1];
        message[QStringLiteral("senderUid")] = args[2];
        message[QStringLiteral("receiver")] = args[0];
        // send the JSON using QDataStream
        clientStream << QJsonDocument(message).toJson();
    });
    timer.start();
}

void Client::login(const QString &userName, const QString &uid)
{
    args.clear();
    args.push_back(uid);
    args.push_back(userName);
    timer.disconnect();
    connect(&timer, &QTimer::timeout, this, [this](){
        //qDebug() << username << this->uid;
        timer.start();
        if (m_clientSocket->state() == QAbstractSocket::ConnectedState) { // if the client is connected
            // create a QDataStream operating on the socket
            QDataStream clientStream(m_clientSocket);
            // set the version so that programs compiled with different versions of Qt can agree on how to serialise
            clientStream.setVersion(QDataStream::Qt_5_7);
            // Create the JSON we want to send
            QJsonObject message;
            message[QStringLiteral("type")] = QStringLiteral("login");
            message[QStringLiteral("uid")] = args[0];
            message[QStringLiteral("username")] = args[1];
            // send the JSON using QDataStream
            clientStream << QJsonDocument(message).toJson(QJsonDocument::Compact);
        }
        else qDebug() << "no connection to server";
    });
    timer.start();
}

void Client::sendMessage(const QString &text)
{
    if (text.isEmpty())
        return; // We don't send empty messages
    // create a QDataStream operating on the socket
    QDataStream clientStream(m_clientSocket);
    // set the version so that programs compiled with different versions of Qt can agree on how to serialise
    clientStream.setVersion(QDataStream::Qt_5_7);
    // Create the JSON we want to send
    QJsonObject message;
    message[QStringLiteral("type")] = QStringLiteral("message");
    message[QStringLiteral("text")] = text;
    // send the JSON using QDataStream
    clientStream << QJsonDocument(message).toJson();
}

void Client::disconnectFromHost()
{
    m_clientSocket->disconnectFromHost();
}

void Client::acceptGame(const QString &uid, const QString &username, const QString &senderUid)
{
    args.clear();
    args.push_back(uid);
    args.push_back(username);
    args.push_back(senderUid);
    timer.disconnect();
    connect(&timer, &QTimer::timeout, this, [this](){
        if (args[0].isEmpty() || args[1].isEmpty() || args[2].isEmpty())
            return; // We don't send empty messages
        // create a QDataStream operating on the socket
        QDataStream clientStream(m_clientSocket);
        // set the version so that programs compiled with different versions of Qt can agree on how to serialise
        clientStream.setVersion(QDataStream::Qt_5_7);
        // Create the JSON we want to send
        QJsonObject message;
        message[QStringLiteral("type")] = QStringLiteral("accepted");
        message[QStringLiteral("sender")] = args[1];
        message[QStringLiteral("senderUid")] = args[2];
        message[QStringLiteral("receiver")] = args[0];
        // send the JSON using QDataStream
        clientStream << QJsonDocument(message).toJson();
    });
    timer.start();
}

void Client::declineGame(const QString &uid, const QString &username, const QString &senderUid)
{
    args.clear();
    args.push_back(uid);
    args.push_back(username);
    args.push_back(senderUid);
    timer.disconnect();
    connect(&timer, &QTimer::timeout, this, [this](){
        if (args[0].isEmpty() || args[1].isEmpty() || args[2].isEmpty())
            return; // We don't send empty messages
        // create a QDataStream operating on the socket
        QDataStream clientStream(m_clientSocket);
        // set the version so that programs compiled with different versions of Qt can agree on how to serialise
        clientStream.setVersion(QDataStream::Qt_5_7);
        // Create the JSON we want to send
        QJsonObject message;
        message[QStringLiteral("type")] = QStringLiteral("declined");
        message[QStringLiteral("sender")] = args[1];
        message[QStringLiteral("senderUid")] = args[2];
        message[QStringLiteral("receiver")] = args[0];
        // send the JSON using QDataStream
        clientStream << QJsonDocument(message).toJson();
    });
    timer.start();
}

void Client::jsonReceived(const QJsonObject &docObj)
{
    //TODO: не забыть остановить таймер во всех кейсах

    qDebug() << docObj;
    const auto typeVal = docObj.value(QLatin1String("type")).toString();
    if (typeVal == QLatin1String("login") && !m_loggedIn) {
        timer.stop();
        const QJsonValue resultVal = docObj.value(QLatin1String("success"));
        if (resultVal.isNull() || !resultVal.isBool())
            return;
        const bool loginSuccess = resultVal.toBool();
        if (loginSuccess) {
            emit loggedIn();
        }
        else {
            emit loginError(docObj.value(QLatin1String("reason")).toString());
        }
        const auto usernameVal = docObj.value(QLatin1String("users")).toArray();
        QStringList list;
        std::transform(usernameVal.begin(), usernameVal.end(), std::back_inserter(list), [](QJsonValue a){
            return a.toString();
        });
        emit userList(list);
    } else if (typeVal == QLatin1String("message")) { //It's a chat message
        // we extract the text field containing the chat text
        const auto textVal = docObj.value(QLatin1String("text")).toString();
        // we extract the sender field containing the username of the sender
        const auto senderVal = docObj.value(QLatin1String("sender")).toString();
        if (!textVal.isEmpty() && !senderVal.isEmpty())
            // we notify a new message was received via the messageReceived signal
            emit messageReceived(senderVal, textVal);
    } else if (typeVal == QLatin1String("newuser")) { // A user joined the chat
        // we extract the username of the new user
        const auto usernameVal = docObj.value(QLatin1String("username")).toString();
        const auto uidVal = docObj.value(QLatin1String("uid")).toString();
        if (!usernameVal.isEmpty() && !uidVal.isEmpty())
            // we notify of the new user via the userJoined signal
            emit userJoined(usernameVal, uidVal);
    } else if (typeVal == QLatin1String("userdisconnected")) { // A user left the chat
        // we extract the username of the new user
        const auto usernameVal = docObj.value(QLatin1String("username")).toString();
        const auto uidVal = docObj.value(QLatin1String("uid")).toString();
        if (!usernameVal.isEmpty() && !uidVal.isEmpty())
            // we notify of the new user via the userJoined signal
            emit userLeft(usernameVal, uidVal);
    } else if (typeVal == QLatin1String("invite")){
        timer.stop();
        const auto usernameVal = docObj.value(QLatin1String("sender")).toString();
        const auto uidVal = docObj.value(QLatin1String("senderUid")).toString();
        if(!uidVal.isEmpty() && !usernameVal.isEmpty()) emit invited(usernameVal, uidVal);
        // message[QStringLiteral("type")] = QStringLiteral("invite");
        // message[QStringLiteral("sender")] = username;
        // message[QStringLiteral("senderUid")] = senderUid;
        // message[QStringLiteral("receiver")] = uid;
    }
    else if (typeVal == QLatin1String("accepted")){
        timer.stop();
        const auto usernameVal = docObj.value(QLatin1String("sender")).toString();
        const auto uidVal = docObj.value(QLatin1String("senderUid")).toString();
        if(!uidVal.isEmpty() && !usernameVal.isEmpty()) emit gameAccepted(usernameVal, uidVal);
        // message[QStringLiteral("type")] = QStringLiteral("invite");
        // message[QStringLiteral("sender")] = username;
        // message[QStringLiteral("senderUid")] = senderUid;
        // message[QStringLiteral("receiver")] = uid;
    }
    else if (typeVal == QLatin1String("declined")){
        timer.stop();
        const auto usernameVal = docObj.value(QLatin1String("sender")).toString();
        const auto uidVal = docObj.value(QLatin1String("senderUid")).toString();
        if(!uidVal.isEmpty() && !usernameVal.isEmpty()) emit gameDeclined(usernameVal, uidVal);
        // message[QStringLiteral("type")] = QStringLiteral("invite");
        // message[QStringLiteral("sender")] = username;
        // message[QStringLiteral("senderUid")] = senderUid;
        // message[QStringLiteral("receiver")] = uid;
    }/*
    else if (typeVal == QLatin1String("newMove")){
        const auto move = docObj.value("move").toObject();
        if (!move.isEmpty()) {
            auto player = move.value("player").toInt();
            auto cells = move.value("cells").toArray();
            QVector<QPair<int, int>> data;
            for (const auto &cell: cells) {
                auto c = cell.toArray();
                data << QPair<int, int>{c.at(0).toInt(), c.at(1).toInt()};
            }
            emit newMove(data, player);
        }
    }
    else if (typeVal == "gameOver"){
        const int player = docObj.value("player").toInt();
        emit gameOver(player);
    }*/
    else {
        // a message with no type was received so we just ignore it
    }
}

void Client::connectToServer(const QHostAddress &address, quint16 port)
{
    m_clientSocket->connectToHost(address, port);
}

void Client::onReadyRead()
{
    // prepare a container to hold the UTF-8 encoded JSON we receive from the socket
    QByteArray jsonData;
    // create a QDataStream operating on the socket
    QDataStream socketStream(m_clientSocket);
    // set the version so that programs compiled with different versions of Qt can agree on how to serialise
    socketStream.setVersion(QDataStream::Qt_5_7);
    // start an infinite loop
    for (;;) {
        // we start a transaction so we can revert to the previous state in case we try to read more data than is available on the socket
        socketStream.startTransaction();
        // we try to read the JSON data
        socketStream >> jsonData;
        if (socketStream.commitTransaction()) {
            // we successfully read some data
            // we now need to make sure it's in fact a valid JSON
            QJsonParseError parseError;
            // we try to create a json document with the data we received
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                // if the data was indeed valid JSON
                if (jsonDoc.isObject()) // and is a JSON object
                    jsonReceived(jsonDoc.object()); // parse the JSON
            }
            // loop and try to read more JSONs if they are available
        } else {
            // the read failed, the socket goes automatically back to the state it was in before the transaction started
            // we just exit the loop and wait for more data to become available
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
        //QMessageBox::critical(this, tr("Error"), tr("The host refused the connection"));
        message = tr("The host refused the connection");
        break;
    case QAbstractSocket::ProxyConnectionRefusedError:
        //QMessageBox::critical(this, tr("Error"), tr("The proxy refused the connection"));
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
    default:
        Q_UNREACHABLE();
    }
    emit this->error(message);
}

// void Client::sendGame(const QVector<Cell> &data, int size, int colorCount)
// {
//     QJsonArray cells;
//     for (const auto &cell: data) {
//         int owner = cell.getOwner();
//         QJsonObject c;
//         c.insert("owner", owner);
//         c.insert("color", cell.getColor());
//         cells.append(c);
//     }
//     QJsonObject json;
//     json.insert("cells", cells);
//     json.insert("size", size);
//     json.insert("colors", colorCount);
//     QDataStream clientStream(m_clientSocket);
//     clientStream.setVersion(QDataStream::Qt_5_7);
//     QJsonObject message;
//     message[QStringLiteral("type")] = QStringLiteral("newGame");
//     message[QStringLiteral("game")] = json;
//     clientStream << QJsonDocument(message).toJson();
// }

// bool Client::sendNewMove(const QVector<QPair<int, int> > &captured, int player)
// {
//     QJsonArray cells;
//     for (const auto &cell: captured) {
//         QJsonArray c;
//         c.append(cell.first);
//         c.append(cell.second);
//         cells.append(c);
//     }
//     QJsonObject json;
//     json.insert("cells", cells);
//     json.insert("player", player);
//     QDataStream clientStream(m_clientSocket);
//     clientStream.setVersion(QDataStream::Qt_5_7);
//     QJsonObject message;
//     message[QStringLiteral("type")] = QStringLiteral("newMove");
//     message[QStringLiteral("move")] = json;
//     clientStream << QJsonDocument(message).toJson();
// }

// void Client::sendGameOver(int player)
// {
//     QDataStream clientStream(m_clientSocket);
//     clientStream.setVersion(QDataStream::Qt_5_7);
//     QJsonObject message;
//     message[QStringLiteral("type")] = QStringLiteral("gameOver");
//     message[QStringLiteral("player")] = player;
//     clientStream << QJsonDocument(message).toJson();
// }
