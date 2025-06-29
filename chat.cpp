#include "chat.h"
#include <QApplication>

ChatModel::ChatModel(QObject *parent)
    : QStringListModel{parent}
{
    f = qApp->font();
    f.setPixelSize(32);
}

void ChatModel::newMessage(QString message)
{
    beginResetModel();
    messages.push_back(message);
    endResetModel();
}


int ChatModel::rowCount(const QModelIndex &parent) const
{
    return messages.size();
}

int ChatModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto row = index.row();
    auto column = index.column();

    auto &message = messages[row];
    if(role == Qt::FontRole) return f;
    if(role == Qt::DisplayRole && column == 0) return message;

    return QVariant();
}


QVariant ChatModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

Qt::ItemFlags ChatModel::flags(const QModelIndex &index) const
{
    return QStringListModel::flags(index);
}
