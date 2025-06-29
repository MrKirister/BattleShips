#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "playerfilter.h"

class QLabel;
class QPushButton;
class QLineEdit;
class PlayersListModel;
class QTreeView;
class Client;
class ChatModel;
class QListView;
class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
private:
    QWidget* Mfield;
    QWidget* Mfield2;
    Client* client;
    QTreeView* playersList;
    PlayersListModel* playersModel;
    PlayerFilter* filterModel;
    ChatModel* chatModel;
    QListView* chatView;
    QWidget* chat;
    QPushButton* play;
    QLineEdit* filter;
    QLabel* titleLabel;
    QPushButton* connectButton;
    QLineEdit* usernameEdit;
    QLabel* infoLabel;
    QString username;
    QString uid;
    QString rivalName;
    QString rivalUid;
    void initWidgets();
    void showSecondScreen();
    void showThirdScreen();
};
#endif // WIDGET_H
