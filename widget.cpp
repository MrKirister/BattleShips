#include "widget.h"
#include "chat.h"
#include "field.h"
#include "playerslistmodel.h"
#include "client.h"
#include "ship.h"
#include <QSequentialAnimationGroup>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFontMetrics>
#include <QSettings>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QTreeView>
#include <QHeaderView>
#include <QMessageBox>
#include <QGridLayout>
#include <QListView>
#include <QUuid>
#include <QTimer>
#include <QRandomGenerator>
#include <QJsonArray>

#include "enums.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    this->setFixedSize({1920, 1080});
    client = new Client(this);
    initWidgets();

    // соединяемся с сервером. Если успешно, client посылает сигнал connected
    //client->connectToServer(QHostAddress("5.61.37.57"), SERVER_PORT);
    infoLabel->setText("Connecting to server...");
    infoLabel->setStyleSheet({"color: gray"});
    client->connectToServer(QHostAddress("127.0.0.1"), SERVER_PORT);

    connect(client, &Client::connected, this, [this]() {
        infoLabel->setText("Connected to server");
        infoLabel->setStyleSheet({"color: green"});
    });
    connect(client, &Client::loggedIn, this, &Widget::showSecondScreen);
    connect(client, &Client::loginError, this, [this](const QString &reason){
        infoLabel->setText(reason);
        infoLabel->setStyleSheet({"color: red"});
        connectButton->setEnabled(true);
    });

    //connect(&client, &Client::messageReceived, this, &ChatWindow::messageReceived); - 2. Это для сообщений а у нас уэе обрабатывается newmove
    connect(client, &Client::disconnected, this, [this](){
        QMessageBox::warning(this, tr("Disconnected"), tr("The host terminated the connection"));
    });

    connect(client, &Client::error, this, [this](const QString& error){
        QMessageBox::critical(this, QStringLiteral("Error"), error);
    });
    connect(client, &Client::userJoined, this, [this](const QString &username, const QString &uid){
        playersModel->userJoined(username, uid);
    });
    connect(client, &Client::userLeft, this, [this](const QString &username, const QString &uid){
        playersModel->userLeft(username, uid);
    });
    connect(client, &Client::userList, this, [this](const QStringList &list){
        playersModel->updatePlayers(list);
    });
    connect(playersModel, &PlayersListModel::usersChanged, this, [this](const QString &username, bool joined){
        if(joined){
            chatModel->newMessage(QString("%1 joined").arg(username));
        }
        else chatModel->newMessage(QString("%1 left").arg(username));
    });
    connect(client, &Client::invited, this, [this](const QString &username, const QString &uid){
        QMessageBox q;
        auto a = q.addButton("Accept" , QMessageBox::AcceptRole);
        q.addButton("Decline", QMessageBox::RejectRole);
        q.setText(QString("%1 wants to play").arg(username));
        q.setWindowTitle("Invitation");
        q.exec();
        if(q.clickedButton() == a) {
            rivalName = username;
            rivalUid = uid;
            rivalStatus->setText(QString("%1 is not ready").arg(rivalName));
            rivalStatus->setStyleSheet("color: red");
            showThirdScreen();
            client->acceptGame(uid);
        }
        else {
            client->declineGame(uid);
            rivalName.clear();
            rivalUid.clear();
        }
    });
    connect(client, &Client::gameDeclined, this, [this](const QString &username, const QString &uid){
        rivalName.clear();
        rivalUid.clear();
        QMessageBox::warning(this, "Invitation declined", QString("%1 declined your game invitation").arg(username));
    });
    connect(client, &Client::gameAccepted, this, [this](const QString &username, const QString &uid){
        rivalName = username;
        rivalUid = uid;
        rivalStatus->setText(QString("%1 is not ready").arg(rivalName));
        rivalStatus->setStyleSheet("color: red");
        // myStatus->setText("You are not ready");
        // myStatus->setStyleSheet("color: red");
        showThirdScreen();
        invitor = true;
    });
    connect(client, &Client::playerReady, this, [this](bool rivalState, const QString &uid){
        if(rivalUid != uid) return;
        rivalReady = rivalState;
        if(rivalReady){
            rivalStatus->setStyleSheet("color: green");
            rivalStatus->setText(QString("%1 is ready").arg(rivalName));
        }
        else {
            rivalStatus->setStyleSheet("color: red");
            rivalStatus->setText(QString("%1 is not ready").arg(rivalName));
        }
        if(isReady && rivalReady){
            showFourthScreen();
            randomPlacement->setEnabled(false);
            myStatus->setGeometry(rivalStatus->geometry());
            myStatus->setText(username);
            myStatus->setStyleSheet("color : #7e8d94");
            rivalStatus->setFont(myStatus->font());
            rivalStatus->setGeometry(rivalBoard->geometry().left(),
                                     rivalStatus->geometry().y(), rivalBoard->geometry().width(), rivalStatus->geometry().height());
            rivalStatus->setText(rivalName);
            rivalStatus->setStyleSheet("color : #7e8d94");
            rivalStatus->setAlignment(Qt::AlignRight);
            ready->setEnabled(false);
            myBoard->hideDisabledRect();
        }
    });
    connect(myBoard, &Field::shipsPlaced, ready, &QPushButton::setEnabled);
    connect(client, &Client::gameStarted, this, [this](bool invitorFirst){
        meFirst = !invitorFirst;
        rivalBoard->setEnabled(meFirst);
        ready->setDisabled(true);
        // if(meFirst) myStatus->setText("Your turn");
        // else myStatus->setText(QString("%1's turn").arg(rivalName));

        if(meFirst) gameStatus->setText("Your turn");
        else gameStatus->setText(QString("%1's turn").arg(rivalName));
    });
    connect(rivalBoard, &Field::checkRivalCell, this, [this](int row, int col){
        client->checkRivalCell(row, col, rivalUid);
    });
    connect(client, &Client::checkMyCell, myBoard, &Field::checkMyCell);
    connect(myBoard, &Field::hit, client, [this](int val, int row, int col){
        client->cellChecked(val, rivalUid, row, col);
    });
    connect(client, &Client::cellCheckedResult, rivalBoard, &Field::showHit);
}

Widget::~Widget() {}

void Widget::loginToServer() // SLOT
{
    username = usernameEdit->text().simplified();
    if(username.isEmpty()) {
        infoLabel->setText("Username is empty");
        infoLabel->setStyleSheet({"color: red"});
        return;
    }

    connectButton->setEnabled(false);

    QSettings settings;
    // if(settings.contains("uid")) uid = settings.value("uid").toString();
    // else {
    //     uid = QUuid::createUuid().toString();
    //     settings.setValue("uid", uid);
    // }
    settings.setValue("username", username);
    uid = QUuid::createUuid().toString();

    client->login(username, uid);
}

void Widget::initWidgets()
{
    titleLabel = new QLabel("𝓑𝓪𝓽𝓽𝓵𝓮𝓢𝓱𝓲𝓹𝓼", this);
    auto f = titleLabel->font();
    f.setPointSize(120);
    titleLabel->setFont(f);
    auto titleMetrics = titleLabel->fontMetrics();
    auto titleRect = titleMetrics.boundingRect(titleLabel->text());
    titleRect.adjust(-2, 0, 0, 0);
    titleLabel->setGeometry(titleRect.translated(1920/2 - 1073/2, 230));

    connectButton = new QPushButton("Connect", this);
    f.setPointSize(84);
    connectButton->setFont(f);
    auto buttonMetrics = connectButton->fontMetrics();
    auto buttonRect = buttonMetrics.boundingRect(connectButton->text());
    connectButton->resize({buttonRect.width() + 25, buttonRect.height()});
    connectButton->setGeometry(connectButton->rect().translated(1920/2 - connectButton->rect().width() / 2, 450));
    connect(connectButton, &QPushButton::clicked, this, &Widget::loginToServer);

    QSettings settings;

    usernameEdit = new QLineEdit(settings.value("username").toString(), this);
    f.setPointSize(72);
    usernameEdit->setFont(f);
    usernameEdit->setFixedSize({900, 135});
    usernameEdit->setAlignment(Qt::AlignCenter);
    usernameEdit->setGeometry(usernameEdit->rect().translated(1920/2 - usernameEdit->rect().width() / 2, 730));

    infoLabel = new QLabel("Enter username", this);
    infoLabel->setAlignment(Qt::AlignCenter);
    f.setPointSize(36);
    infoLabel->setFont(f);
    auto infoMetrics = infoLabel->fontMetrics();
    auto infoRect = infoMetrics.boundingRect("Username is already in use");
    infoRect.adjust(-4, 0, 0, 0);
    infoLabel->setGeometry(infoRect.translated(1920/2 - infoRect.width() / 2, 935));

    playersModel = new PlayersListModel(this);
    filterModel = new PlayerFilter(this);
    playersList = new QTreeView(this);
    playersList->setHeaderHidden(true);
    playersList->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    playersList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    filterModel->setSourceModel(playersModel);
    playersList->setModel(filterModel);
    connect(playersList, &QTreeView::clicked, this, [this](const QModelIndex &index){
        if(index.isValid()){
            auto a = filterModel->mapToSource(index);
            auto state = playersModel->players[a.row()].status;
            play->setEnabled(state == Player::Status::online);
        }
    });

    playersList->setGeometry(QRect(-800, 200, 650, 400));

    filter = new QLineEdit(this);
    filter->setGeometry(QRect(250, 1450, 300, 70));
    f.setPixelSize(48);
    filter->setFont(f);
    connect(filter, &QLineEdit::textEdited, this, [this](const QString &text){
        this->filterModel->filter(text);

    });

    play = new QPushButton("Invite", this);
    play->setGeometry(600, 1450, 200, 70);
    play->setFont(f);
    connect(play, &QPushButton::clicked, this, [this](){
        auto model = playersList->selectionModel();
        if(model->hasSelection()){
            auto row = model->selectedIndexes()[0].row();
            auto uid = playersModel->getUid(row);
            client->invite(uid);
        }
    });

    chat = new QWidget(this);
    chat->setStyleSheet("border : 2px solid white; border-radius: 10%; background-color: #250018");
    chat->setFont(f);
    chat->setGeometry(2075, 200, 425, 600);

    chatModel = new ChatModel(this);

    chatView = new QListView(this);
    chatView->setModel(chatModel);
    auto l = new QGridLayout();
    l->addWidget(chatView);
    chat->setLayout(l);

    myBoard = new Field(true, this);
    myBoard->setGeometry(25, -1800, 700+myBoard->paletteWidth+4, 700+4);
    myBoard->setStyleSheet("border : 2px solid #7C7B6F; border-radius: 5%; background-color: #e3F3E3");
    myBoard->createShips();
    myBoard->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    myBoard->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    rivalBoard = new Field(false, this);
    rivalBoard->setGeometry(750, 2200, 700+4, 700+4);
    rivalBoard->setStyleSheet("border : 2px solid #7C7B6F; border-radius: 5%; background-color: #518487");

    ready = new QPushButton(this);
    ready->setGeometry(-130, 920, 100, 50);
    ready->setText("ready");
    f.setPixelSize(24);
    ready->setFont(f);
    ready->setEnabled(false);
    connect(ready, &QPushButton::clicked, this, [this](){
        isReady = !isReady;
        isReady ? ready->setText("not ready") : ready->setText("ready");
        client->readyPressed(isReady, rivalUid);
        myBoard->disableField(isReady);
        if(isReady){
            myStatus->show();
        }
        else {
            myStatus->hide();
        }
        if(isReady && rivalReady){
            showFourthScreen();

            randomPlacement->setEnabled(false);
            myStatus->setGeometry(rivalStatus->geometry());
            myStatus->setText(username);
            myStatus->setStyleSheet("color : #7e8d94");
            rivalStatus->setFont(myStatus->font());
            rivalStatus->setGeometry(rivalBoard->geometry().left(),
                                     rivalStatus->geometry().top(),
                                     rivalBoard->geometry().width(),
                                     rivalStatus->geometry().height());
            rivalStatus->setText(rivalName);
            rivalStatus->setStyleSheet("color : #7e8d94");
            rivalStatus->setAlignment(Qt::AlignRight);
            ready->setEnabled(false);
            myBoard->hideDisabledRect();

            if(invitor) {
                meFirst = QRandomGenerator::global()->bounded(0, 2);
                client->startGame(meFirst, rivalUid);
                rivalBoard->setEnabled(meFirst);

                if (meFirst) gameStatus->setText("Your turn");
                else gameStatus->setText(QString("%1's turn").arg(rivalName));
            }
        }
    });

    myStatus = new QLabel(this);
    myStatus->setGeometry(150, 500, 500, 100);
    myStatus->setStyleSheet("color: green ");
    myStatus->setText("You are ready");
    myStatus->hide();
    f = myStatus->font();
    f.setPointSize(48);
    myStatus->setFont(f);

    rivalStatus = new QLabel(this);
    rivalStatus->setGeometry(25, -150, 1000, 100);
    rivalStatus->setFont(f);

    randomPlacement = new QPushButton("random", this);
    f.setPixelSize(24);
    randomPlacement->setFont(f);
    randomPlacement->setGeometry(-130, 920, 130, 50);
    connect(randomPlacement, &QPushButton::clicked, myBoard, &Field::placeRandomly);

    gameStatus = new QLabel("Your turn", this);
    gameStatus->setGeometry(0, -200, 1600 , 100);
    gameStatus->setAlignment(Qt::AlignCenter);
    gameStatus->setStyleSheet(" color : #00A2E8");
    f = gameStatus->font();
    f.setPointSize(48);
    gameStatus->setFont(f);
}

void Widget::showSecondScreen()
{

    infoLabel->setText("Login successful");
    infoLabel->setStyleSheet({"color: green"});

    auto allAnimations = new QSequentialAnimationGroup(this);

    auto animationGroup1 = new QParallelAnimationGroup(this);

    auto titlePos = titleLabel->geometry().topLeft();
    auto titleAnimation = new QPropertyAnimation(titleLabel, "pos", this);
    titleAnimation->setStartValue(titlePos);
    titleAnimation->setEndValue(titlePos + QPoint(0, -400));
    titleAnimation->setDuration(1000);
    titleAnimation->setEasingCurve(QEasingCurve::InBack);

    auto connectSize = connectButton->geometry();
    auto connectAnimation = new QPropertyAnimation(connectButton, "geometry", this);
    connectAnimation->setStartValue(connectSize);
    connectAnimation->setEndValue(QRect(this->width()/2, this->height() / 2, 0, 0));
    connectAnimation->setDuration(1000);
    connectAnimation->setEasingCurve(QEasingCurve::InBack);

    auto usernameEditPos = usernameEdit->geometry().topLeft();
    auto usernameAnimation = new QPropertyAnimation(usernameEdit, "pos", this);
    usernameAnimation->setStartValue(usernameEditPos);
    usernameAnimation->setEndValue(usernameEditPos + QPoint(-1410, 0));
    usernameAnimation->setDuration(1000);
    usernameAnimation->setEasingCurve(QEasingCurve::InBack);


    auto infoPos = infoLabel->geometry().topLeft();
    auto infoAnimation = new QPropertyAnimation(infoLabel, "pos", this);
    infoAnimation->setStartValue(infoPos);
    infoAnimation->setEndValue(infoPos + QPoint(1500, 0));
    infoAnimation->setDuration(1000);
    infoAnimation->setEasingCurve(QEasingCurve::InBack);

    animationGroup1->addAnimation(titleAnimation);
    animationGroup1->addAnimation(connectAnimation);
    animationGroup1->addAnimation(usernameAnimation);
    animationGroup1->addAnimation(infoAnimation);

    auto animationGroup2 = new QParallelAnimationGroup(this);

    auto listPos = playersList->geometry().topLeft();
    auto listAnimation = new QPropertyAnimation(playersList, "pos", this);
    listAnimation->setStartValue(listPos);
    listAnimation->setEndValue(listPos + QPoint(1000, 0));
    listAnimation->setDuration(1000);
    listAnimation->setEasingCurve(QEasingCurve::OutQuad);

    auto filterPos = filter->geometry().topLeft();
    auto filterAnimation = new QPropertyAnimation(filter, "pos", this);
    filterAnimation->setStartValue(filterPos);
    filterAnimation->setEndValue(filterPos + QPoint(0, -800));
    filterAnimation->setDuration(1300);
    filterAnimation->setEasingCurve(QEasingCurve::OutQuad);

    auto playPos = play->geometry().topLeft();
    auto playAnimation = new QPropertyAnimation(play, "pos", this);
    playAnimation->setStartValue(playPos);
    playAnimation->setEndValue(playPos + QPoint(0, -800));
    playAnimation->setDuration(1150);
    playAnimation->setEasingCurve(QEasingCurve::OutQuad);

    auto chatPos = chat->geometry().topLeft();
    auto chatAnimation = new QPropertyAnimation(chat, "pos", this);
    chatAnimation->setStartValue(chatPos);
    chatAnimation->setEndValue(chatPos + QPoint(-600, 0));
    chatAnimation->setDuration(1000);
    chatAnimation->setEasingCurve(QEasingCurve::OutQuad);


    animationGroup2->addAnimation(listAnimation);
    animationGroup2->addAnimation(filterAnimation);
    animationGroup2->addAnimation(playAnimation);
    animationGroup2->addAnimation(chatAnimation);



    allAnimations->addAnimation(animationGroup1);
    allAnimations->addAnimation(animationGroup2);

    allAnimations->start(QAbstractAnimation::DeleteWhenStopped);
    //TODO: если что это тест
    chatModel->newMessage(QString("%1 : Hello everyone!").arg(username));


}

void Widget::showThirdScreen()
{
    auto allAnimations = new QSequentialAnimationGroup(this);

    auto animationGroup1 = new QParallelAnimationGroup(this);

    auto f1 = myBoard->geometry().topLeft();
    auto fA = new QPropertyAnimation(myBoard, "pos", this);
    fA->setStartValue(f1);
    fA->setEndValue(f1 + QPoint(0, 2000));
    fA->setDuration(1000);
    fA->setEasingCurve(QEasingCurve::OutQuad);

    auto readyP = ready->geometry().topLeft();
    auto readyA = new QPropertyAnimation(ready, "pos", this);
    readyA->setStartValue(readyP);
    readyA->setEndValue(readyP + QPoint(180, 0));
    readyA->setDuration(1000);
    readyA->setEasingCurve(QEasingCurve::OutQuad);

    auto randomP = randomPlacement->geometry().topLeft();
    auto randomA = new QPropertyAnimation(randomPlacement, "pos", this);
    randomA->setStartValue(randomP);
    randomA->setEndValue(randomP + QPoint(300, 0));
    randomA->setDuration(1000);
    randomA->setEasingCurve(QEasingCurve::OutQuad);


    auto rsPos = rivalStatus->geometry().topLeft();
    auto rsAnimation = new QPropertyAnimation(rivalStatus, "pos", this);
    rsAnimation->setStartValue(rsPos);
    rsAnimation->setEndValue(rsPos + QPoint(0, 250));
    rsAnimation->setDuration(1000);
    rsAnimation->setEasingCurve(QEasingCurve::OutQuad);

    animationGroup1->addAnimation(fA);
    animationGroup1->addAnimation(readyA);
    animationGroup1->addAnimation(randomA);
    animationGroup1->addAnimation(rsAnimation);

    auto animationGroup2 = new QParallelAnimationGroup(this);

    auto listPos = playersList->geometry().topLeft();
    auto listAnimation = new QPropertyAnimation(playersList, "pos", this);
    listAnimation->setStartValue(listPos);
    listAnimation->setEndValue(listPos + QPoint(-1000, 0));
    listAnimation->setDuration(1000);
    listAnimation->setEasingCurve(QEasingCurve::InBack);

    auto filterPos = filter->geometry().topLeft();
    auto filterAnimation = new QPropertyAnimation(filter, "pos", this);
    filterAnimation->setStartValue(filterPos);
    filterAnimation->setEndValue(filterPos + QPoint(0, 800));
    filterAnimation->setDuration(1300);
    filterAnimation->setEasingCurve(QEasingCurve::InBack);

    auto playPos = play->geometry().topLeft();
    auto playAnimation = new QPropertyAnimation(play, "pos", this);
    playAnimation->setStartValue(playPos);
    playAnimation->setEndValue(playPos + QPoint(0, 800));
    playAnimation->setDuration(1150);
    playAnimation->setEasingCurve(QEasingCurve::InBack);

    animationGroup2->addAnimation(listAnimation);
    animationGroup2->addAnimation(filterAnimation);
    animationGroup2->addAnimation(playAnimation);

    allAnimations->addAnimation(animationGroup2);
    allAnimations->addAnimation(animationGroup1);
    allAnimations->start(QAbstractAnimation::DeleteWhenStopped);
}

void Widget::showFourthScreen()
{
    auto r = myBoard->geometry();
    auto a = new QPropertyAnimation(myBoard, "geometry", this);
    a->setStartValue(r);
    a->setEndValue(QRect(r.topLeft().x(), r.topLeft().y() ,r.width() - myBoard->cellSize*4-myBoard->cellMargin*3 - myBoard->fieldMargin*2 - 2, r.height()));
    a->setDuration(1200);
    //a->setEasingCurve()
    a->start(QAbstractAnimation::DeleteWhenStopped);
    myBoard->hidePalette();

    auto f2 = rivalBoard->geometry().topLeft();
    auto fA2 = new QPropertyAnimation(rivalBoard, "pos", this);
    fA2->setStartValue(f2);
    fA2->setEndValue(f2 + QPoint(0, -2000));
    fA2->setDuration(1000);
    fA2->setEasingCurve(QEasingCurve::OutQuad);
    fA2->start(QAbstractAnimation::DeleteWhenStopped);

    auto gs = new QPropertyAnimation(gameStatus, "pos", this);
    auto gsPos = gameStatus->geometry().topLeft();
    gs->setStartValue(gsPos);
    gs->setEndValue(gsPos + QPoint(0, 300));
    gs->setDuration(1000);
    gs->setEasingCurve(QEasingCurve::OutQuad);
    gs->start(QAbstractAnimation::DeleteWhenStopped);
}
