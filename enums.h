#ifndef ENUMS_H
#define ENUMS_H

#include <QObject>

constexpr int SERVER_PORT = 1967;

enum class MessageType {
    Info,
    Warning,
    Critical
};
Q_DECLARE_METATYPE(MessageType)

enum Type {
    SenderName,//string //кто отправил сообщение
    SenderUid,//string  //кто отправил сообщение
    ReceiverName,//string //кто получает сообщение
    ReceiverUid,//string  //кто получает сообщение
    DataType,//string
    UserName, //string //имя нового игрока или отключившегося игрока
    UserUid, //string  //uid нового игрока или отключившегося игрока
    Success, //bool
    Reason, //string
    Users,//list

    //ниже - те типы сообщений, про которые сервер не обязан знать
    Text, //string //текст сообщения в чат
    Ready, //bool //игрок готов или не готов начать игру
    InvitorFirst, //bool // пригласивший в игру делает первый ход
    Row, //int //номер ряда
    Column, //int //номер столбца
    CellType, //int // попал = 1 / не попал = 0 / убил = 2
    Size, //int размер корабля
    Orientation, //int ориентация корабля

    Unknown = 65535
};

enum ConnectionState {
    WaitingForGreeting,
    ReadingGreeting,
    ProcessingGreeting,
    ReadyForUse
};

using DataList = QMap<int, QVariant>;

#endif // ENUMS_H
