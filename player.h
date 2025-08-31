#ifndef PLAYER_H
#define PLAYER_H

#include <QString>

struct Player{
    enum class Status{
        offline,
        online,
        inGame
    };
    QString name;
    Status status = Status::offline;
    int totalPlayed = 0;
    int won = 0;
    QString uid;
};

#endif // PLAYER_H
