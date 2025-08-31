#include "playerslistmodel.h"
#include <QApplication>
#include <QColor>
#include <QSettings>

PlayersListModel::PlayersListModel(QObject *parent)
    : QAbstractTableModel{parent}
{

}


int PlayersListModel::rowCount(const QModelIndex &parent) const
{
    return players.size();
}

int PlayersListModel::columnCount(const QModelIndex &parent) const
{
    //nick, status, gamesplayed maybe
    return 1;
}

QVariant PlayersListModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    auto row = index.row();
    auto column = index.column();
    Q_ASSERT(column == 0);

    auto &player = players[row];

    switch(role) {
    case Qt::DisplayRole: {
        return player.name;
    }
    }
    return QVariant();
}

QVariant PlayersListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

Qt::ItemFlags PlayersListModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index);

}

void PlayersListModel::userJoined(const QString &username, const QString &uid)
{
    beginResetModel();
    auto it = std::find_if(players.begin(), players.end(), [uid](const Player &player){
        return player.uid == uid;
    });
    if(it != players.end()) (*it).status = Player::Status::online;
    else {
        QSettings settings;
        Player a;
        a.name = username;
        a.status = Player::Status::online;
        a.uid = uid;
        QVariantMap users = settings.value("players").toMap();
        a.totalPlayed = users[uid].toPoint().x() + users[uid].toPoint().y();
        a.won = users[uid].toPoint().x();
        players.push_back(a);
    }
    emit usersChanged(username, true);
    endResetModel();
}

void PlayersListModel::userLeft(const QString &username, const QString &uid)
{
    beginResetModel();
    auto it = std::find_if(players.begin(), players.end(), [uid](const Player &player){
        return player.uid == uid;
    });
    if(it != players.end()) (*it).status = Player::Status::offline;
    emit usersChanged(username, false);
    endResetModel();
}

void PlayersListModel::updatePlayers(const QStringList &list)
{
    beginResetModel();
    players.clear();
    QSettings settings;
    QVariantMap users = settings.value("players").toMap();
    // key : UID , Value : W/L (int/int)
    for(auto &player : list){
        Player a;
        a.name = player.section('\n', 0, 0);
        a.status = Player::Status::online;
        a.uid = player.section('\n', 1, 1);
        a.totalPlayed = users[a.uid].toPoint().x() + users[a.uid].toPoint().y();
        a.won = users[a.uid].toPoint().x();
        players.push_back(a);
    }
    endResetModel();
}

QString PlayersListModel::getUid(int row) const
{
    return players[row].uid;
}
