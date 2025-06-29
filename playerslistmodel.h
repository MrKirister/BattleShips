#ifndef PLAYERSLISTMODEL_H
#define PLAYERSLISTMODEL_H

#include <QAbstractTableModel>
#include <QFont>
#include "player.h"

class PlayersListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PlayersListModel(QObject *parent = nullptr);


    // QAbstractItemModel interface
signals:
    void usersChanged(const QString &username, bool joined);
public:
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    void userJoined(const QString &username, const QString &uid);
    void userLeft(const QString &username, const QString &uid);
    void updatePlayers(const QStringList &list);
    QString getUid(int row) const;
    QVector<Player> players;
    QFont f;
};

#endif // PLAYERSLISTMODEL_H
