#include "widget.h"
#include "chat.h"
#include "playerslistmodel.h"
#include "client.h"
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


Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    this->setFixedSize({1920, 1080});
    initWidgets();
    client = new Client(this);
    auto connectLambda = [this](){
        client->login(username, uid);
    };
    connect(client, &Client::connected, this, connectLambda);
    connect(client, &Client::loggedIn, this, &Widget::showSecondScreen);
    connect(client, &Client::loginError, this, [this, connectLambda](const QString &reason){
        infoLabel->setText("Username is already in use");
        infoLabel->setStyleSheet({"color: red"});
        connectButton->setEnabled(true);
        client->disconnectFromHost();
    });
    //connect(&client, &Client::messageReceived, this, &ChatWindow::messageReceived); - 2. Это для сообщений а у нас уэе обрабатывается newmove
    connect(client, &Client::disconnected, this, [this](){
        QMessageBox::warning(this, tr("Disconnected"), tr("The host terminated the connection"));
    });//&ChatWindow::disconnectedFromServer

    connect(client, &Client::error, this, [this](const QString& error){
        QMessageBox::critical(this, QStringLiteral("Error"), error);
    });//&ChatWindow::error
    connect(client, &Client::userJoined, this, [this](const QString &username, const QString &uid){
        playersModel->userJoined(username, uid);
    });
    connect(client, &Client::userLeft, this, [this](const QString &username, const QString &uid){
        playersModel->userLeft(username, uid);
    });
    connect(client, &Client::userList, this, [this](const QStringList &list){
        playersModel->updatePlayers(list);
    });
    connect(playersModel, &PlayersListModel::usersChanged, chatModel, [this](const QString &username, bool joined){
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
        if(q.exec()){
            if(q.clickedButton() == a) {
                showThirdScreen(); //TODO: написать функцию(все готово в предыдущей)
                client->acceptGame(uid, this->username, this->uid);
                rivalName = username;
                rivalUid = uid;
                qDebug() << "rival name : " << rivalName;
            }
            else {
                client->declineGame(uid, this->username, this->uid);
                rivalName.clear();
                rivalUid.clear();
            }
        }
    });
    connect(client, &Client::gameDeclined, this, [this](const QString &username, const QString &uid){
        rivalName.clear();
        rivalUid.clear();
        QMessageBox::warning(this, "Invitation declined", QString("%1 declined your game invitation").arg(username));
    });
    connect(client, &Client::gameAccepted, this, [this](const QString &username, const QString &uid){
        rivalName = username;
        qDebug() << "rival name : " << rivalName;
        rivalUid = uid;
        showThirdScreen();
    });
}

Widget::~Widget() {}

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
    connect(connectButton, &QPushButton::clicked, this, [this](){

        username = usernameEdit->text().simplified();
        if(username.isEmpty()) {
            infoLabel->setText("Username is empty");
            infoLabel->setStyleSheet({"color: red"});
            return;
        }

        QSettings settings;
        if(settings.contains("uid")) uid = settings.value("uid").toString();
        else {
            uid = QUuid::createUuid().toString();
            settings.setValue("uid", uid);
        }
        settings.setValue("username", username);

        client->connectToServer(QHostAddress("5.61.37.57"), 1967);
        //TODO: Проверить повторяется ли username
        //TODO: Переместить вызов в прием сигнала от клиента(сервера)
        connectButton->setEnabled(false);

    });

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
            qDebug() << uid;
            client->invite(uid, username, this->uid);
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

    Mfield = new QWidget(this);
    Mfield->setGeometry(25, -800, 700, 700);
    Mfield->setStyleSheet("border : 2px solid white; border-radius: 10%; background-color: #250018");

    Mfield2 = new QWidget(this);
    Mfield2->setGeometry(750, 2200, 700, 700);
    Mfield2->setStyleSheet("border : 2px solid white; border-radius: 10%; background-color: #250018");
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

    auto f1 = Mfield->geometry().topLeft();
    auto fA = new QPropertyAnimation(Mfield, "pos", this);
    fA->setStartValue(f1);
    fA->setEndValue(f1 + QPoint(0, 1000));
    fA->setDuration(1000);
    fA->setEasingCurve(QEasingCurve::OutQuad);

    auto f2 = Mfield2->geometry().topLeft();
    auto fA2 = new QPropertyAnimation(Mfield2, "pos", this);
    fA2->setStartValue(f2);
    fA2->setEndValue(f2 + QPoint(0, -2000));
    fA2->setDuration(1000);
    fA2->setEasingCurve(QEasingCurve::OutQuad);

    animationGroup1->addAnimation(fA);
    animationGroup1->addAnimation(fA2);

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
