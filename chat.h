#ifndef CHAT_H
#define CHAT_H

#include <QStringListModel>
#include <QStack>
#include <QFont>

class ChatModel : public QStringListModel
{
    Q_OBJECT
public:
    explicit ChatModel(QObject *parent = nullptr);


    // QAbstractItemModel interface
public:
    void newMessage(QString message);
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
private:
    QStack<QString> messages;
    QFont f;
};

#endif // CHAT_H
