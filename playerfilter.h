#ifndef PLAYERFILTER_H
#define PLAYERFILTER_H

#include "player.h"
#include <QSortFilterProxyModel>

class PlayerFilter : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PlayerFilter(QObject *parent = nullptr);
    void filter(const QString &word);
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    QString filterBy;
};

#endif // PLAYERFILTER_H
