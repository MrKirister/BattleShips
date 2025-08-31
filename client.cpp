#include "client.h"

#include <QTcpSocket>
#include <QDataStream>

#include <QRandomGenerator>

Client::Client(QObject *parent)
    : QObject(parent)
    , m_socket(this)
    , m_reader(&m_socket)
    , m_writer(&m_socket)
{
    connect(&m_socket, &QTcpSocket::connected, this, [this](){
        if (!m_socket.isOpen()) {
            qDebug() << "Error: socket is not open";
        }
        if (!m_writer.device()) {
            qDebug() << "No writer device set";
            m_writer.setDevice(&m_socket);
        }
        if (!m_reader.device()) {
            qDebug() << "No reader device set";
            m_reader.setDevice(&m_socket);
        }

        emit connected();
    });
    connect(this, &Client::dataReceived, this, &Client::parseData);
    connect(&m_socket, &QTcpSocket::disconnected, this, &Client::disconnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    connect(m_clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &ChatClient::error);
#else
    connect(&m_socket, &QAbstractSocket::errorOccurred, this, &Client::onError);
#endif
    connect(&m_socket, &QTcpSocket::disconnected, this, [this](){m_started = false;});
}

Client::~Client()
{
    if (m_writeOpened && m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_writer.endArray();
        m_socket.waitForBytesWritten(2000);
    }
}

void Client::disconnectFromHost()
{
    m_writer.endArray();
    m_socket.waitForBytesWritten(2000);
    m_socket.disconnectFromHost();
    m_writeOpened = false;
}

void Client::sendData()
{
    // qDebug() << "Sending" << m_args;

    if (m_args.isEmpty()) return;
    if (!m_writeOpened) {
        // qDebug() << "starting the main array";
        m_writer.startArray();
        m_writeOpened = true;
    }

    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        // qDebug() << "Connected to socket. Starting sending data";
        m_writer.startMap(m_args.size());
        // qDebug() << "started map with data";
        for (auto i = m_args.cbegin(); i!=m_args.cend(); ++i) {
            m_writer.append(i.key());
            // qDebug() << "appended key" << i.key();
            switch (i.value().type()) {
                case QVariant::Bool: m_writer.append(i.value().toBool()); break;
                case QVariant::Int: m_writer.append(i.value().toInt()); break;
                case QVariant::UInt: m_writer.append(i.value().toUInt()); break;
                case QVariant::LongLong: m_writer.append(i.value().toLongLong()); break;
                case QVariant::ULongLong: m_writer.append(i.value().toULongLong()); break;
                case QVariant::Double: m_writer.append(i.value().toDouble()); break;
                case QVariant::Char: m_writer.append(i.value().toString()); break;
                case QVariant::Map: { // QMap<QString, QString>
                    auto map = i.value().toMap();
                    m_writer.startMap(map.size());
                    for (auto j = map.cbegin(); j != map.cend(); ++j) {
                        m_writer.append(j.key());
                        m_writer.append(j.value().toString());
                    }
                    m_writer.endMap();
                    break;
                }
                case QVariant::List: { // QList<QVariant>
                    auto list = i.value().toList();
                    m_writer.startArray(list.size());
                    for (auto j = 0; j < list.size(); ++j) {
                        m_writer.append(list.at(j).toString());
                    }
                    m_writer.endArray();
                    break;
                }
                case QVariant::String: m_writer.append(i.value().toString()); break;
                case QVariant::StringList: {
                    auto list = i.value().toStringList();
                    m_writer.startArray(list.size());
                    for (auto j = 0; j < list.size(); ++j) {
                        m_writer.append(list.at(j));
                    }
                    m_writer.endArray();
                    break;
                }
                case QVariant::ByteArray: m_writer.append(i.value().toByteArray()); break;
                default:
                    break;
            }
            // qDebug() << "appended value"<<i.value();
        }
        m_writer.endMap();
        // qDebug() << "closed map with m_args";
    }
    // else qDebug() << "no connection to server";
}

void Client::login(const QString &userName, const QString &uid)
{
    m_args.clear();

    m_args.insert(DataType, QLatin1String("login"));
    m_args.insert(UserUid, uid);
    m_args.insert(UserName, userName);

    sendData();
}

void Client::sendMessage(const QString &text)
{
    if (text.isEmpty()) return;

    m_args.clear();

    m_args.insert(DataType, QLatin1String("message"));
    m_args.insert(Text, text);
    m_args.insert(ReceiverUid, QLatin1String("all"));

    sendData();
}

void Client::invite(const QString &uid)
{
    m_args.clear();
    m_args.insert(DataType, QLatin1String("invite"));
    m_args.insert(ReceiverUid, uid);
    sendData();
}

void Client::acceptGame(const QString &uid)
{
    m_args.clear();
    m_args.insert(DataType, QLatin1String("accepted"));
    m_args.insert(ReceiverUid, uid);
    sendData();
}

void Client::declineGame(const QString &uid)
{
    m_args.clear();
    m_args.insert(DataType, QLatin1String("declined"));
    m_args.insert(ReceiverUid, uid);
    sendData();
}

void Client::readyPressed(bool isReady, const QString &uid)
{
    m_args.clear();
    m_args.insert(DataType, QLatin1String("ready"));
    m_args.insert(ReceiverUid, uid);
    m_args.insert(Ready, isReady);
    sendData();
}

void Client::startGame(const QString &uid)
{
    bool invitorFirst = QRandomGenerator::global()->bounded(0, 2);
    m_args.clear();
    m_args.insert(DataType, QLatin1String("game"));
    m_args.insert(ReceiverUid, uid);
    m_args.insert(InvitorFirst, invitorFirst);
    sendData();

    emit gameStarted(invitorFirst);
}

void Client::checkRivalCell(int row, int col, const QString &uid)
{
    m_args.clear();
    m_args.insert(DataType, QLatin1String("move"));
    m_args.insert(ReceiverUid, uid);
    m_args.insert(Row, row);
    m_args.insert(Column, col);
    sendData();
}

void Client::cellChecked(int val, const QString &uid, int row, int col)
{
    m_args.clear();
    m_args.insert(DataType, QLatin1String("cell"));
    m_args.insert(ReceiverUid, uid);
    m_args.insert(CellType, val);
    m_args.insert(Row, row);
    m_args.insert(Column, col);
    sendData();
}

void Client::shipKilled(int row, int col, int size, Qt::Orientation o, const QString &uid)
{
    m_args.clear();
    m_args.insert(DataType, QLatin1String("killed"));
    m_args.insert(ReceiverUid, uid);
    m_args.insert(Size, size);
    m_args.insert(Row, row);
    m_args.insert(Column, col);
    m_args.insert(Orientation, o);
    sendData();
}

void Client::gameOver(const QString &uid)
{
    m_args.clear();
    m_args.insert(DataType, QLatin1String("gameOver"));
    m_args.insert(ReceiverUid, uid);
    sendData();
}

void Client::parseData(const DataList &data)
{
    const auto type = data.value(DataType).toString();
    if (type == QLatin1String("login")) {
        const auto resultVal = data.value(Success).toBool();
        if (resultVal) {
            emit loggedIn();
        }
        else {
            emit loginError(data.value(Reason).toString());
        }
        const auto users = data.value(Users).toStringList();
        emit userList(users);
    }
    else if (type == QLatin1String("message")) {
        const auto textVal = data.value(Text).toString();
        const auto senderVal = data.value(SenderName).toString();
        if (!textVal.isEmpty() && !senderVal.isEmpty())
            emit messageReceived(senderVal, textVal);
    }
    else if (type == QLatin1String("newuser")) { // A user joined the game
        const auto username = data.value(UserName).toString();
        const auto uid = data.value(UserUid).toString();
        if (!username.isEmpty() && !uid.isEmpty())
            emit userJoined(username, uid);
    }
    else if (type == QLatin1String("userdisconnected")) { // A user left the game
        const auto username = data.value(UserName).toString();
        const auto uid = data.value(UserUid).toString();
        if (!username.isEmpty() && !uid.isEmpty())
            emit userLeft(username, uid);
    }
    // An invite was received from sender
    else if (type == QLatin1String("invite")) {
        const auto username = data.value(SenderName).toString();
        const auto uid = data.value(SenderUid).toString();
        if(!uid.isEmpty() && !username.isEmpty())
            emit invited(username, uid);
    }
    // Sender accepted out invite
    else if (type == QLatin1String("accepted")) {
        const auto username = data.value(SenderName).toString();
        const auto uid = data.value(SenderUid).toString();
        if(!uid.isEmpty() && !username.isEmpty())
            emit gameAccepted(username, uid);
    }
    // Sender declined our invite
    else if (type == QLatin1String("declined")) {
        const auto username = data.value(SenderName).toString();
        const auto uid = data.value(SenderUid).toString();
        if(!uid.isEmpty() && !username.isEmpty())
            emit gameDeclined(username, uid);
    }
    else if (type == QLatin1String("ready")) {
        const auto ready = data.value(Ready).toBool();
        const auto uid = data.value(SenderUid).toString();
        if(!uid.isEmpty())
            emit playerReady(ready, uid);
        //playerReady - готовность противника
    }
    else if (type == QLatin1String("game")){
        const auto invitorFirst = data.value(InvitorFirst).toBool();
        emit gameStarted(invitorFirst);
    }
    else if (type == QLatin1String("move")){
        const auto row = data.value(Row).toInt();
        const auto col = data.value(Column).toInt();
        emit checkMyCell(row, col);
    }
    else if (type == QLatin1String("cell")) {
        const auto cellType = data.value(CellType).toInt();
        const auto row = data.value(Row).toInt();
        const auto col = data.value(Column).toInt();
        emit cellCheckedResult(cellType, row, col);
    }
    else if (type == QLatin1String("killed")){
        const auto uid = data.value(SenderUid).toString();
        const auto row = data.value(Row).toInt();
        const auto col = data.value(Column).toInt();
        const auto o = static_cast<Qt::Orientation>(data.value(Orientation).toInt());
        const auto size = data.value(Size).toInt();
        emit shipKilledResult(row, col, size, o);
    }
    else if (type ==QLatin1String("gameOver")){
        const auto uid = data.value(SenderUid).toString();
        emit gameOver();
    }
    else {
        qDebug() << "A message with unknown type:" << type << ", ignore";
        // a message with no type was received so we just ignore it
    }
}

void Client::connectToServer(const QHostAddress &address, quint16 port)
{
    qDebug() << "Connecting socket to server...";
    m_socket.connectToHost(address, port);
}

void Client::onReadyRead()
{
    m_reader.reparse();
    // qDebug() << "last reader error:"<<m_reader.lastError().toString();

    // Протокол:
    // [
    // {Type, Val}
    // ...
    // ]

    while (m_reader.lastError() == QCborError::NoError) {
        if (!m_started) {
            // qDebug() << "not started yet";
            if (!m_reader.isArray()) {
                // qDebug() << "Error: must be an array";
                break; // protocol error
            }
            // qDebug() << "entering the main array";
            m_reader.enterContainer();
            m_started = true;
        }
        else if (m_reader.containerDepth() == 1) {
            // qDebug() << "we are at depth 1";
            if (!m_reader.hasNext()) {
                // qDebug() << "nothing to read, disconnecting...";
                m_reader.leaveContainer();
                // disconnectFromHost();
                return;
            }

            if (!m_reader.isMap() || !m_reader.isLengthKnown()) {
                // qDebug() << "Error: must be a map";
                break; // protocol error
            }
            // qDebug() << "we are at a map. Receiving message";
            m_leftToRead = m_reader.length();
            // qDebug() << "message size is"<<m_leftToRead;
            m_reader.enterContainer();
            m_receivedData.clear();
        }
        else if (m_lastMessageType == Unknown) {
            // qDebug() << "reading message type";
            if (!m_reader.isInteger()) {
                // qDebug() << "Error: message type must be an integer";
                break; // protocol error
            }
            m_lastMessageType = Type(m_reader.toInteger());
            // qDebug() << "message type"<<m_lastMessageType;
            m_reader.next();
        }
        else {
            // qDebug() << "reading payload";
            switch (m_reader.type()) {
                case QCborStreamReader::UnsignedInteger:
                case QCborStreamReader::NegativeInteger: {
                    m_receivedData.insert(m_lastMessageType, m_reader.toInteger());
                    m_reader.next();
                    break;
                }
                case QCborStreamReader::Float:
                case QCborStreamReader::Double: {
                    m_receivedData.insert(m_lastMessageType, m_reader.toDouble());
                    m_reader.next();
                    break;
                }
                case QCborStreamReader::ByteString: {
                    m_receivedData.insert(m_lastMessageType, handleByteArray());
                    break;
                }
                case QCborStreamReader::TextString: {
                    m_receivedData.insert(m_lastMessageType, handleString());
                    break;
                }
                case QCborStreamReader::Array: {
                    m_receivedData.insert(m_lastMessageType, handleArray());
                    break;
                }
                case QCborStreamReader::Map: {
                    m_receivedData.insert(m_lastMessageType, handleMap());
                    break;
                }
                case QCborStreamReader::SimpleType: { // treat as bool
                    m_receivedData.insert(m_lastMessageType, m_reader.toBool());
                    m_reader.next();
                    break;
                }
                default: {
                    qDebug() << "Error: unknown message payload";
                    m_reader.next(); // skip unknown value
                    break;
                }
            }
            m_leftToRead--;
            // qDebug() << "read so far:"<<m_receivedData;
            // qDebug() << "left to read:"<<m_leftToRead;

            if (m_leftToRead == 0) {
                m_reader.leaveContainer();
                // qDebug() << "The total message data:"<<m_receivedData;
                emit dataReceived(m_receivedData);
            }
            m_lastMessageType = Unknown;
        }
    }

    if (m_reader.lastError() != QCborError::NoError) {
        qDebug() << "Invalid message:" << m_reader.lastError().toString();
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

QByteArray Client::handleByteArray()
{
    QByteArray result;
    auto r = m_reader.readByteArray();
    while (r.status == QCborStreamReader::Ok) {
        result += r.data;
        r = m_reader.readByteArray();
    }

    if (r.status == QCborStreamReader::Error)
        result.clear();
    return result;
}

QString Client::handleString()
{
    QString result;
    auto r = m_reader.readString();
    while (r.status == QCborStreamReader::Ok) {
        result += r.data;
        r = m_reader.readString();
    }

    if (r.status == QCborStreamReader::Error)
        result.clear();

    return result;
}

QVariantList Client::handleArray()
{
    QVariantList result;

    if (m_reader.isLengthKnown())
        result.reserve(m_reader.length());

    m_reader.enterContainer();
    while (m_reader.lastError() == QCborError::NoError && m_reader.hasNext())
        result.append(handleString());

    if (m_reader.lastError() == QCborError::NoError)
        m_reader.leaveContainer();

    return result;
}

QVariantMap Client::handleMap()
{
    QVariantMap result;

    m_reader.enterContainer();
    while (m_reader.lastError() == QCborError::NoError && m_reader.hasNext()) {
        QString key = handleString();
        result.insert(key, handleString());
    }

    if (m_reader.lastError() == QCborError::NoError)
        m_reader.leaveContainer();

    return result;
}
