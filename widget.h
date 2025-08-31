#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMessageBox>
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
class QVariantAnimation;

enum class InfoType {
    Info,
    OK,
    Warning,
    Critical
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
private slots:
    void loginToServer();
    void iAmReady();
    void rivalIsReady(bool rivalState, const QString &uid);
private:
    int totalWidth = 0;
    int totalHeight = 0;
    int totalFieldWidth = 0;
    int totalFieldHeight = 0;
    int margin = 20;
    int span = 10;
    int buttonFontSize = 36;
    int labelFontSize = 40;
    int infoLabelFontSize = 30;
    int editFontSize = 40;
    int widgetHeight = 55;
    int buttonMargin = 50;
    QString labelErrorColor = "#e90000";
    QString labelInfoColor = "#eeeeee";
    QString labelOKColor = "#26fe07";
    QString labelWarningColor = "#ffcf3f";

    Field* myBoard;
    Field* rivalBoard;
    Client* client;
    QTreeView* playersList;
    PlayersListModel* playersModel;
    PlayerFilter* filterModel;
    ChatModel* chatModel;
    QListView* chatView;
    QWidget* chat;
    QPushButton* inviteButton;
    QLineEdit* filterEdit;
    QPushButton* connectButton;
    QPushButton* readyButton;
    QLineEdit* usernameEdit;
    QLabel* whoLabel;
    QLabel* infoLabel;
    QLabel* myStatus;
    QLabel* rivalStatus;
    QLabel* gameStatus;
    QLabel *welcomeLabel;
    QPushButton *randomPlacement;

    QString username;
    QString uid;
    QString rivalName;
    QString rivalUid;
    QVariantAnimation *infoAnimation{};
    //QPushButton* playSolo;
    bool isReady = false;
    bool rivalReady = false;
    bool invitor = false;
    bool meFirst = false;
    int currentScreen = 1;
    void initWidgets();
    void showSecondScreen();
    void showThirdScreen();
    void showFourthScreen();
    void checkGameReadiness();
    void showInfo(InfoType type, const QString &message);
    void showGameOverScreen(bool rivalWon);
};
#endif // WIDGET_H
