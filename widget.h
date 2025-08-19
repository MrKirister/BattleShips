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
class Field;
class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
private:
    Field* myBoard;
    Field* rivalBoard;
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
    QPushButton* ready;
    QLineEdit* usernameEdit;
    QLabel* infoLabel;
    QString username;
    QString uid;
    QString rivalName;
    QString rivalUid;
    QLabel* myStatus;
    QLabel* rivalStatus;
    QLabel* gameStatus;
    QPushButton *randomPlacement;
    //QPushButton* playSolo;
    bool isReady = false;
    bool rivalReady = false;
    bool invitor = false;
    bool meFirst = false;
    void initWidgets();
    void showSecondScreen();
    void showThirdScreen();
    void showFourthScreen();
};
#endif // WIDGET_H
