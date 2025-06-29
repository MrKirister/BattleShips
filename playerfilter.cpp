#include "playerfilter.h"


PlayerFilter::PlayerFilter(QObject *parent)
    : QSortFilterProxyModel{parent}
{}

void PlayerFilter::filter(const QString &word)
{
    beginResetModel();
    this->filterBy = word;
    endResetModel();
}


bool PlayerFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if(sourceModel()->rowCount() == 0) return true;
    auto index = sourceModel()->index(source_row, 0, source_parent);
    if(!index.isValid()) return false;
    auto uid = sourceModel()->data(index).toString();
    return uid.contains(filterBy, Qt::CaseInsensitive);
}
