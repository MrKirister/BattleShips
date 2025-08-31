#include "widget.h"
#include "chat.h"
#include "field.h"
#include "playerslistmodel.h"
#include "gameoverdialog.h"
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
#include "qevent.h"
#include <QFontDatabase>
#include <QStyledItemDelegate>
#include <QApplication>

#include "enums.h"

class PlayerDelegate : public QStyledItemDelegate
{
    QLabel *name;
    QLabel *status;
    QLabel *stats;
    QWidget *w;
public:
    PlayerDelegate() {
        w = new QWidget(nullptr);
        w->setAutoFillBackground(false);
        name = new QLabel(w);
        name->setAutoFillBackground(false);
        name->setStyleSheet("color: #eeeeee; font-size: 24pt");
        status = new QLabel(w);
        stats = new QLabel(w);
        stats->setStyleSheet("color: #eeeeee; font-size: 16pt");

        auto l = new QGridLayout;
        l->addWidget(name, 0, 0, Qt::AlignLeft);
        l->addWidget(status, 0, 1, Qt::AlignRight);
        l->addWidget(stats, 1, 0, 1, 2, Qt::AlignLeft);
        w->setLayout(l);
    }
    void setStatus(Player::Status status) const
    {
        switch (status) {
        case Player::Status::offline: {
            this->status->setText(" offline ");
            this->status->setStyleSheet("color: #666666; font-size: 16pt;"
                                        "border-radius: 5px; border: none;"
                                        "background-color: #c0888888");
            break;
        }
        case Player::Status::online: {
            this->status->setText(" online ");
            this->status->setStyleSheet("color: #19cd00; font-size: 16pt;"
                                        "border-radius: 5px; border: none;"
                                        "background-color: #c0888888");
            break;
        }
        case Player::Status::inGame: {
            this->status->setText(" in game ");
            this->status->setStyleSheet("color: #fe0707; font-size: 16pt;"
                                        "border-radius: 5px; border: none;"
                                        "background-color: #c0888888");
            break;
        }
        }
    }
    ~PlayerDelegate() {
        delete w;
    }

public:
    const PlayersListModel *getModel(const QModelIndex &index) const
    {
        if (auto m = dynamic_cast<const PlayersListModel *>(index.model()))
            return m;

        if (auto m = dynamic_cast<const QSortFilterProxyModel *>(index.model()))
            return dynamic_cast<const PlayersListModel *>(m->sourceModel());
        return nullptr;
    }
    int getRow(const QModelIndex &index) const {
        if (auto m = dynamic_cast<const QSortFilterProxyModel *>(index.model()))
            return m->mapToSource(index).row();
        return index.row();
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (!painter) return;

        painter->save();

        if (index.isValid()) {
            if (!index.parent().isValid()) {
                int column = index.column();
                Q_ASSERT(column == 0);
                int row = getRow(index);

                QString name = index.data().toString();

                auto model = getModel(index);
                if (!model) {
                    QStyledItemDelegate::paint(painter,option,index);
                    painter->restore();
                    return;
                }

                Player::Status status = model->players[row].status;
                int played = model->players[row].totalPlayed;
                int won = model->players[row].won;

                this->name->setText(name);
                setStatus(status);

                this->stats->setText(QString("Played %1 games, won %2").arg(played).arg(won));

                if (name.isEmpty()) {
                    QStyledItemDelegate::paint(painter,option,index);
                    painter->restore();
                    return;
                }

                // Copy the style option and fill in from the current index
                QStyleOptionViewItem optionV4 = option;
                initStyleOption(&optionV4, index);

                // Get the current application style
                QStyle *style = QApplication::style();

                // Paint the background
                optionV4.text = QString();
                style->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter);

                // Paint the widget
                QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &optionV4);
                painter->translate(textRect.topLeft());
                painter->setClipRect(textRect.translated(-textRect.topLeft()));
                w->render(painter, {}, {}, QWidget::DrawChildren);
            }
            else QStyledItemDelegate::paint(painter,option,index);
        }
        else QStyledItemDelegate::paint(painter,option,index);

        painter->restore();
    }
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (!index.isValid()) return option.rect.size();
        auto size = w->sizeHint();
        size.setWidth(400);
        return size;
    }
};

class PlayersList : public QTreeView {
public:
    PlayersList(QAbstractItemModel *model, QWidget *parent = nullptr)
        : QTreeView{parent}, m_model{model}
    {

    }


    // QWidget interface
protected:
    virtual void paintEvent(QPaintEvent *event) override
    {
        QTreeView::paintEvent(event);
        // Если модель пустая, выводит надпись
        if (m_model && m_model->rowCount() == 0) {
            QPainter painter(this->viewport());
            QFont f = this->font();
            f.setPointSize(18);
            painter.setFont(f);
            painter.setPen(QColor(0xeeeeee));
            QTextOption textOption(Qt::AlignCenter);
            textOption.setWrapMode(QTextOption::WordWrap);
            painter.drawText(event->rect(), "No one is online", textOption);
        }
        event->accept();
    }
private:
    QAbstractItemModel *m_model{};
};

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("Widget {background-color: #3c3c3c}"
                  "QPushButton {"
                  "background-image: none; "
                  "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
                  "             stop: 0 #007cc1, stop: 0.2 #0065a7, stop: 0.8 #0065a7, stop: 1 #00588e); "
                  "             border-radius: 25px; "
                  "             color: #eeeeee; font-size: 36px}"
                  "QPushButton:!enabled {color: #aaaaaa}"
                  "QLineEdit {background-color: #c0888888; background-image: none; "
                  "           border-radius: 5px; border: 1px solid #777777; "
                  "           color: #eeeeee; font-size: 40px;"
                  "           selection-background-color: #777777; selection-color: #eeeeee"
                  "}"
                  "QLabel {font-size: 40px; color: #eeeeee; background-image: none;}"
                  "QLabel#infoLabel {font-size: 30px}"
                  "QLabel#welcomeLabel {font-size: 40pt}"
                  "QListView {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
                  "           stop: 0 #c0888888, stop: 1 #c0004d7a); "
                  "           background-image: none; "
                  "           border: 1px solid #777777; "
                  "           color: #eeeeee; border-radius: 5px; "
                  "           font-size: 18px; show-decoration-selected: 1}"
                  "QListView::item:hover {background-color: #0077c1}"
                  "QTreeView {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
                  "           stop: 0 #c0888888, stop: 1 #c0004d7a); border: 1px solid #777777; "
                  "           background-image: none; "
                  "           border-radius: 5px; color: #eeeeee; font-size: 18px}"
                  "Field {"
                  // "background-color: #0065a7; "
                  "background-color: qradialgradient(cx: 0, cy: 0, radius: 1.5, fx: 0, fy: 0, "
                  "           stop: 0 #c0007dcc, stop: 1 #c00065a7); "
                  "border: 1px solid #777777}"
                  );

    client = new Client(this);
    initWidgets();


    // соединяемся с сервером. Если успешно, client посылает сигнал connected
    //client->connectToServer(QHostAddress("5.61.37.57"), SERVER_PORT);
    client->connectToServer(QHostAddress("127.0.0.1"), SERVER_PORT);

    connect(client, &Client::connected, this, [this]() {
        showInfo(InfoType::Info, "Connected to server");
    });
    connect(client, &Client::loggedIn, this, &Widget::showSecondScreen);
    connect(client, &Client::loginError, this, [this](const QString &reason){
        showInfo(InfoType::Critical, reason);
        connectButton->setEnabled(true);
    });

    //connect(&client, &Client::messageReceived, this, &ChatWindow::messageReceived); - 2. Это для сообщений а у нас уэе обрабатывается newmove
    // connect(client, &Client::disconnected, this, [this](){
    //     QMessageBox::warning(this, tr("Disconnected"), tr("The host terminated the connection"));
    // });

    connect(client, &Client::error, this, [this](const QString& error){
        showInfo(InfoType::Critical, error);
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
        showThirdScreen();
        invitor = true;
    });
    connect(client, &Client::playerReady, this, &Widget::rivalIsReady);
    connect(myBoard, &Field::shipsPlaced, readyButton, &QPushButton::setEnabled);
    connect(client, &Client::gameStarted, this, [this](bool invitorFirst){
        meFirst = invitor ? invitorFirst : !invitorFirst;
        rivalBoard->setEnabled(meFirst);
        readyButton->setDisabled(true);

        if (meFirst) gameStatus->setText("Your turn");
        else gameStatus->setText(QString("%1's turn").arg(rivalName));
    });
    connect(rivalBoard, &Field::checkRivalCell, this, [this](int row, int col){
        client->checkRivalCell(row, col, rivalUid);
    });
    connect(client, &Client::checkMyCell, myBoard, &Field::checkMyCell);
    connect(myBoard, &Field::hit, client, [this](int val, int row, int col){
        client->cellChecked(val, rivalUid, row, col);
        if(val == 0){
            rivalBoard->setEnabled(true);
            gameStatus->setText(QString("Your turn"));
        }
        else
        {
            rivalBoard->setEnabled(false);
            gameStatus->setText(QString("%1's turn").arg(rivalName));
        }
    });
    connect(client, &Client::cellCheckedResult, this, [this](int val, int row, int col){
        rivalBoard->showHit(val, row, col);
        if(val == 0)
        {
            rivalBoard->setEnabled(false);
            gameStatus->setText(QString("%1's turn").arg(rivalName));
        }
        else
        {
            rivalBoard->setEnabled(true);
            gameStatus->setText(QString("Your turn"));
        }
    });
    connect(myBoard, qOverload<int, int, int, Qt::Orientation>(&Field::shipKilled),
            this, [this](int row, int col, int size, Qt::Orientation o){
                client->shipKilled(row, col, size, o, rivalUid);
            });
    connect(client, &Client::shipKilledResult, rivalBoard, &Field::showKilledShipArea);
    connect(myBoard, &Field::gameOver, this, [this](){
        client->gameOver(rivalUid);
        //showGameOverScreen(true); // true - rival won, false - rival lost
    });
    connect(client, qOverload<>(&Client::gameOver), this, [this](){
        //showGameOverScreen(false); // false - rival won, true - rival lost
    });

    setFixedSize({totalWidth, totalHeight});
}

Widget::~Widget() {}

void Widget::loginToServer() // SLOT
{
    username = usernameEdit->text().simplified();
    if (username.isEmpty()) {
        showInfo(InfoType::Warning, "Username is empty");
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

void Widget::iAmReady()
{
    isReady = !isReady;
    isReady ? readyButton->setText("Not ready") : readyButton->setText("Ready");
    client->readyPressed(isReady, rivalUid);
    myBoard->disableField(isReady);
    myStatus->setText(isReady ? QString("%1 (ready)").arg(username) : QString("%1 (not ready)").arg(username));

    checkGameReadiness();
}

void Widget::rivalIsReady(bool rivalState, const QString &uid)
{
    if(rivalUid != uid) return;
    rivalReady = rivalState;
    if(rivalReady){
        rivalStatus->setText(QString("%1 (ready)").arg(rivalName));
    }
    else {
        rivalStatus->setText(QString("%1 (not ready)").arg(rivalName));
    }
    checkGameReadiness();
}

void Widget::initWidgets()
{
    totalHeight = margin + widgetHeight + span + Field::getBaseHeight() + span + widgetHeight
                  + margin + widgetHeight + margin;
    totalWidth = margin + Field::getBaseWidth() + span + Field::getBaseWidth()
                 + margin + 400 + margin;

    auto backgroundLabel = new QLabel(this);
    backgroundLabel->setFixedSize(totalWidth, totalHeight);
    backgroundLabel->setPixmap(QPixmap(":/images/background.jpg"));
    backgroundLabel->setScaledContents(true);

    // screen 1

    QFontDatabase::addApplicationFont(":/images/STENCIL.TTF");

    welcomeLabel = new QLabel("Battleships", this);
    welcomeLabel->setObjectName("welcomeLabel");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLabel->setProperty("screen1Geometry", QRect(margin, margin,
                                                       totalWidth - 2*margin,
                                                       150));
    welcomeLabel->setProperty("screen2Geometry", QRect(margin,
                                                       -150 - margin,
                                                       totalWidth - 2*margin,
                                                       150));
    welcomeLabel->setProperty("screen3Geometry", welcomeLabel->property("screen2Geometry"));
    welcomeLabel->setProperty("screen4Geometry", welcomeLabel->property("screen2Geometry"));
    welcomeLabel->setGeometry(welcomeLabel->property("screen1Geometry").toRect());
    auto wf = welcomeLabel->font();
    wf.setFamily("Stencil");
    welcomeLabel->setFont(wf);

    connectButton = new QPushButton("Login", this);
    auto f = connectButton->font();
    f.setPointSize(buttonFontSize);
    connectButton->setFont(f);
    auto buttonRect = connectButton->fontMetrics().boundingRect(connectButton->text());
    connectButton->resize({buttonRect.width() + buttonMargin, widgetHeight});
    connectButton->setProperty("screen1Geometry", QRect((totalWidth - connectButton->width())/2,
                                                        totalHeight*2/3-widgetHeight,
                                                        connectButton->width(),
                                                        connectButton->height()));
    connectButton->setProperty("screen2Geometry", QRect(totalWidth/2, totalHeight / 2, 0, 0));
    connectButton->setProperty("screen3Geometry", connectButton->property("screen2Geometry"));
    connectButton->setProperty("screen4Geometry", connectButton->property("screen2Geometry"));
    connectButton->setGeometry(connectButton->property("screen1Geometry").toRect());
    connect(connectButton, &QPushButton::clicked, this, &Widget::loginToServer);

    QSettings settings;

    usernameEdit = new QLineEdit(settings.value("username").toString(), this);
    f.setPointSize(editFontSize);
    usernameEdit->setFont(f);
    usernameEdit->setFixedSize({totalWidth / 3, widgetHeight});
    usernameEdit->setAlignment(Qt::AlignCenter);
    usernameEdit->setProperty("screen1Geometry", QRect((totalWidth - usernameEdit->width())/2,
                              totalHeight*2/3-widgetHeight-margin-widgetHeight,
                              usernameEdit->width(),
                              usernameEdit->height()));
    usernameEdit->setProperty("screen2Geometry", QRect((totalWidth - usernameEdit->width())/2,
                                                     -widgetHeight-margin, // up
                                                     usernameEdit->width(),
                                                     usernameEdit->height()));
    usernameEdit->setProperty("screen3Geometry", usernameEdit->property("screen2Geometry"));
    usernameEdit->setProperty("screen4Geometry", usernameEdit->property("screen2Geometry"));
    usernameEdit->setGeometry(usernameEdit->property("screen1Geometry").toRect());

    infoLabel = new QLabel(this);
    infoLabel->setObjectName("infoLabel");
    infoLabel->setAlignment(Qt::AlignCenter);
    f.setPointSize(infoLabelFontSize);
    infoLabel->setFont(f);
    infoLabel->setGeometry(margin,
                           totalHeight-margin-widgetHeight,
                           totalWidth-2*margin,
                           widgetHeight);
    showInfo(InfoType::Info, "Connecting to server...");

    whoLabel = new QLabel("Who are you?", this);
    whoLabel->setAlignment(Qt::AlignCenter);
    f.setPointSize(labelFontSize);
    whoLabel->setFont(f);
    whoLabel->setProperty("screen1Geometry", QRect(margin,
                                                   totalHeight*2/3-widgetHeight-margin-widgetHeight-margin-widgetHeight,
                                                   totalWidth - 2*margin,
                                                   widgetHeight));
    whoLabel->setProperty("screen2Geometry", QRect(margin,
                                                   -totalHeight - margin,
                                                   totalWidth - 2*margin,
                                                   widgetHeight));
    whoLabel->setProperty("screen3Geometry", whoLabel->property("screen2Geometry"));
    whoLabel->setProperty("screen4Geometry", whoLabel->property("screen2Geometry"));
    whoLabel->setGeometry(whoLabel->property("screen1Geometry").toRect());

    // screen 2

    playersModel = new PlayersListModel(this);
    filterModel = new PlayerFilter(this);
    playersList = new QTreeView(this);
    playersList->setHeaderHidden(true);
    playersList->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    playersList->setRootIsDecorated(false);
    playersList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    playersList->setItemDelegate(new PlayerDelegate);
    filterModel->setSourceModel(playersModel);
    playersList->setModel(filterModel);
    connect(playersList, &QTreeView::clicked, this, [this](const QModelIndex &index){
        if(index.isValid()){
            auto a = filterModel->mapToSource(index);
            auto state = playersModel->players[a.row()].status;
            inviteButton->setEnabled(state == Player::Status::online);
        }
    });
    playersList->setProperty("screen1Geometry", QRect(-450, margin + widgetHeight,
                                                      400, Field::getBaseHeight()));
    playersList->setProperty("screen2Geometry", QRect(margin, margin + widgetHeight + span,
                                                      400, Field::getBaseHeight()));
    playersList->setProperty("screen3Geometry", playersList->property("screen1Geometry"));
    playersList->setProperty("screen4Geometry", playersList->property("screen1Geometry"));
    playersList->setGeometry(playersList->property("screen1Geometry").toRect());

    filterEdit = new QLineEdit(this);
    f.setPixelSize(editFontSize);
    filterEdit->setFont(f);
    connect(filterEdit, &QLineEdit::textEdited, filterModel, &PlayerFilter::filter);

    inviteButton = new QPushButton("Invite", this);
    f = inviteButton->font();
    f.setPointSize(buttonFontSize);
    inviteButton->setFont(f);
    buttonRect = inviteButton->fontMetrics().boundingRect(inviteButton->text());
    inviteButton->setFixedSize({buttonRect.width() + buttonMargin, widgetHeight});
    connect(inviteButton, &QPushButton::clicked, this, [this](){
        auto model = playersList->selectionModel();
        if(model->hasSelection()){
            auto row = model->selectedIndexes()[0].row();
            auto uid = playersModel->getUid(row);
            client->invite(uid);
        }
    });
    inviteButton->setEnabled(false);
    inviteButton->setProperty("screen1Geometry", QRect(margin + 400 - inviteButton->width(),
                                                       totalHeight + 20,
                                                       inviteButton->width(),
                                                       inviteButton->height()));
    inviteButton->setProperty("screen2Geometry", QRect(margin + 400 - inviteButton->width(),
                                                       totalHeight - 2*margin - 2*widgetHeight,
                                                       inviteButton->width(),
                                                       inviteButton->height()));
    inviteButton->setProperty("screen3Geometry", inviteButton->property("screen1Geometry"));
    inviteButton->setProperty("screen4Geometry", inviteButton->property("screen1Geometry"));
    inviteButton->setGeometry(inviteButton->property("screen1Geometry").toRect());
    filterEdit->setProperty("screen1Geometry", QRect(margin,
                                                     totalHeight + 20,
                                                     400 - inviteButton->width() - span,
                                                     widgetHeight));
    filterEdit->setProperty("screen2Geometry", QRect(margin,
                                                   totalHeight - 2*margin - 2*widgetHeight,
                                                   400 - inviteButton->width() - span,
                                                   widgetHeight));
    filterEdit->setProperty("screen3Geometry", filterEdit->property("screen1Geometry"));
    filterEdit->setProperty("screen4Geometry", filterEdit->property("screen1Geometry"));
    filterEdit->setGeometry(filterEdit->property("screen1Geometry").toRect());

    chat = new QWidget(this);
    chat->setFont(f);
    chat->setProperty("screen1Geometry", QRect(totalWidth + 10,
                                               margin + widgetHeight + span,
                                               400,
                                               Field::getBaseHeight()));
    chat->setProperty("screen2Geometry", QRect(totalWidth - margin - 400,
                                             margin + widgetHeight, 400, Field::getBaseHeight()));
    chat->setProperty("screen3Geometry", chat->property("screen2Geometry"));
    chat->setProperty("screen4Geometry", chat->property("screen2Geometry"));
    chat->setGeometry(chat->property("screen1Geometry").toRect());

    chatModel = new ChatModel(this);
    chatView = new QListView(this);
    chatView->setModel(chatModel);
    auto l = new QGridLayout();
    l->addWidget(chatView);
    chat->setLayout(l);

    myStatus = new QLabel("Players", this);
    myStatus->setProperty("screen1Geometry", QRect(margin, -widgetHeight - margin,
                                                   Field::getBaseWidth(), widgetHeight));
    myStatus->setProperty("screen2Geometry", QRect(margin, margin,
                                                   Field::getBaseWidth(), widgetHeight));
    myStatus->setProperty("screen3Geometry", myStatus->property("screen2Geometry"));
    myStatus->setProperty("screen4Geometry", myStatus->property("screen2Geometry"));
    myStatus->setGeometry(myStatus->property("screen1Geometry").toRect());
    f = myStatus->font();
    f.setPointSize(labelFontSize);
    myStatus->setFont(f);

    // screen 3

    myBoard = new Field(true, this);
    myBoard->setProperty("screen1Geometry", QRect(-myBoard->totalWidth,
                                                  margin + widgetHeight + span,
                                                  myBoard->totalWidth,
                                                  myBoard->totalHeight));
    myBoard->setProperty("screen2Geometry", myBoard->property("screen1Geometry"));
    myBoard->setProperty("screen3Geometry", QRect(margin,
                                                  margin + widgetHeight + span,
                                                  myBoard->totalWidth,
                                                  myBoard->totalHeight));
    myBoard->setProperty("screen4Geometry", QRect(margin,
                                                  margin + widgetHeight + span,
                                                  Field::getBaseWidth(),
                                                  Field::getBaseHeight()));
    myBoard->setGeometry(myBoard->property("screen1Geometry").toRect());
    myBoard->createShips();
    myBoard->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    myBoard->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // myBoard->setStyleSheet("border: none; background-color: #3c3c3c");

    readyButton = new QPushButton("Ready", this);
    f.setPixelSize(buttonFontSize);
    readyButton->setFont(f);
    buttonRect = readyButton->fontMetrics().boundingRect("Not ready");
    readyButton->resize({buttonRect.width() + buttonMargin, widgetHeight});
    readyButton->setProperty("screen1Geometry", QRect(margin,
                                                      totalHeight + 10,
                                                      readyButton->width(),
                                                      widgetHeight));
    readyButton->setProperty("screen2Geometry", readyButton->property("screen1Geometry"));
    readyButton->setProperty("screen3Geometry", QRect(margin,
                                                      margin + widgetHeight + span + Field::getBaseHeight() + span,
                                                      readyButton->width(),
                                                      widgetHeight));
    readyButton->setProperty("screen4Geometry", readyButton->property("screen1Geometry"));
    readyButton->setGeometry(readyButton->property("screen1Geometry").toRect());
    readyButton->setEnabled(false);
    connect(readyButton, &QPushButton::clicked, this, &Widget::iAmReady);

    randomPlacement = new QPushButton("Random", this);
    f.setPixelSize(buttonFontSize);
    randomPlacement->setFont(f);
    buttonRect = randomPlacement->fontMetrics().boundingRect(randomPlacement->text());
    randomPlacement->resize({buttonRect.width() + buttonMargin, widgetHeight});
    randomPlacement->setProperty("screen1Geometry", QRect(margin + Field::getBaseWidth() - randomPlacement->width(),
                                                          totalHeight + 10,
                                                          randomPlacement->width(),
                                                          widgetHeight));
    randomPlacement->setProperty("screen2Geometry", randomPlacement->property("screen1Geometry"));
    randomPlacement->setProperty("screen3Geometry", QRect(margin + Field::getBaseWidth() - randomPlacement->width(),
                                                        margin + widgetHeight + span + Field::getBaseHeight() + span,
                                                        randomPlacement->width(),
                                                        widgetHeight));
    randomPlacement->setProperty("screen4Geometry", randomPlacement->property("screen1Geometry"));
    randomPlacement->setGeometry(randomPlacement->property("screen1Geometry").toRect());
    connect(randomPlacement, &QPushButton::clicked, myBoard, &Field::placeRandomly);

    // screen 4

    rivalBoard = new Field(false, this);
    rivalBoard->setProperty("screen1Geometry", QRect(totalWidth+10,
                                                     margin + widgetHeight + span,
                                                     rivalBoard->totalWidth,
                                                     rivalBoard->totalHeight));
    rivalBoard->setProperty("screen2Geometry", rivalBoard->property("screen1Geometry"));
    rivalBoard->setProperty("screen3Geometry", rivalBoard->property("screen1Geometry"));
    rivalBoard->setProperty("screen4Geometry", QRect(margin + Field::getBaseWidth() + span,
                                                     margin + widgetHeight + span,
                                                     rivalBoard->totalWidth,
                                                     rivalBoard->totalHeight));
    rivalBoard->setGeometry(rivalBoard->property("screen1Geometry").toRect());
    rivalBoard->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rivalBoard->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    rivalStatus = new QLabel("Rival unknown", this);
    rivalStatus->setAlignment(Qt::AlignRight);
    rivalStatus->setProperty("screen1Geometry", QRect(margin + Field::getBaseWidth() + span,
                                                      -margin - widgetHeight,
                                                      Field::getBaseWidth(),
                                                      widgetHeight));
    rivalStatus->setProperty("screen2Geometry", rivalStatus->property("screen1Geometry"));
    rivalStatus->setProperty("screen3Geometry", QRect(margin + Field::getBaseWidth() + span,
                                                      margin,
                                                      Field::getBaseWidth(),
                                                      widgetHeight));
    rivalStatus->setProperty("screen4Geometry", rivalStatus->property("screen3Geometry"));
    rivalStatus->setGeometry(rivalStatus->property("screen1Geometry").toRect());
    rivalStatus->setFont(f);

    gameStatus = new QLabel("Your turn", this);
    gameStatus->setProperty("screen1Geometry", QRect(margin,
                                                     -100,
                                                     2*Field::getBaseWidth() + span,
                                                     widgetHeight));
    gameStatus->setProperty("screen2Geometry", gameStatus->property("screen1Geometry"));
    gameStatus->setProperty("screen3Geometry", gameStatus->property("screen1Geometry"));
    gameStatus->setProperty("screen4Geometry", QRect(margin,
                                                     margin,
                                                     2*Field::getBaseWidth() + span,
                                                     widgetHeight));
    gameStatus->setGeometry(gameStatus->property("screen1Geometry").toRect());
    gameStatus->setAlignment(Qt::AlignCenter);
    gameStatus->setStyleSheet("color : #ffd65c");
    f = gameStatus->font();
    f.setPointSize(labelFontSize);
    gameStatus->setFont(f);
}

void Widget::checkGameReadiness()
{
    if (!isReady || !rivalReady) return;

    showFourthScreen();

    randomPlacement->setEnabled(false);
    readyButton->setEnabled(false);
    rivalBoard->setEnabled(meFirst);
    myStatus->setText(username);
    rivalStatus->setText(rivalName);

    if (invitor) client->startGame(rivalUid);
}

void Widget::showInfo(InfoType type, const QString &message)
{
    infoLabel->show();
    infoLabel->setText(message);
    QColor color;
    switch (type) {
        case InfoType::Info:
            color = labelInfoColor; break;
        case InfoType::OK:
            color = labelOKColor; break;
        case InfoType::Warning:
            color = labelWarningColor; break;
        case InfoType::Critical:
            color = labelErrorColor; break;
    }

    if (!infoAnimation)
        infoAnimation = new QVariantAnimation(this);
    else infoAnimation->stop();

    infoAnimation->setStartValue(255);
    infoAnimation->setEndValue(0);
    infoAnimation->setDuration(2000);
    infoAnimation->setEasingCurve(QEasingCurve::InExpo);
    connect(infoAnimation, &QVariantAnimation::valueChanged, [this, color](const QVariant &val){
        infoLabel->setStyleSheet(QString("color: rgba(%1, %2, %3, %4)")
                                     .arg(color.red())
                                     .arg(color.green())
                                     .arg(color.blue())
                                     .arg(val.toInt()));
    });
    connect(infoAnimation, &QPropertyAnimation::finished, infoLabel, &QLabel::hide);
    infoAnimation->start(QAbstractAnimation::KeepWhenStopped);
}

//TODO: В конце игры должны показываться все корабли на полях
void Widget::showGameOverScreen(bool rivalWon)
{
    GameOverDialog *dialog = new GameOverDialog(rivalWon, this);
    dialog->show();
    // if(rivalWon){

        // }
        // else {

        // }
}

void Widget::showSecondScreen()
{
    //TODO: если что это тест
    chatModel->newMessage(QString("%1 : Hello everyone!").arg(username));
    showInfo(InfoType::OK, "Login successful");

    QList<QWidget*> toHide = {welcomeLabel, whoLabel, connectButton, usernameEdit, myBoard, rivalBoard,
                               readyButton, myStatus, rivalStatus, gameStatus, randomPlacement};
    QList<QWidget*> toShow = {playersList, filterEdit, inviteButton, chat};

    auto allAnimations = new QSequentialAnimationGroup(this);

    auto animationGroup1 = new QParallelAnimationGroup(this);
    auto animationGroup2 = new QParallelAnimationGroup(this);

    for (auto hide: toHide) {
        auto animation = new QPropertyAnimation(hide, "geometry", this);
        animation->setStartValue(hide->geometry());
        animation->setEndValue(hide->property("screen2Geometry"));
        animation->setDuration(1000);
        animation->setEasingCurve(QEasingCurve::InBack);
        animationGroup1->addAnimation(animation);
    }

    for (auto show: toShow) {
        auto animation = new QPropertyAnimation(show, "geometry", this);
        animation->setStartValue(show->geometry());
        animation->setEndValue(show->property("screen2Geometry"));
        animation->setDuration(1000);
        animation->setEasingCurve(QEasingCurve::OutQuad);
        animationGroup2->addAnimation(animation);
    }

    allAnimations->addAnimation(animationGroup1);
    allAnimations->addAnimation(animationGroup2);
    allAnimations->start(QAbstractAnimation::DeleteWhenStopped);
}

void Widget::showThirdScreen()
{
    rivalStatus->setText(QString("%1 (not ready)").arg(rivalName));
    myStatus->setText(QString("%1 (not ready)").arg(username));

    QList<QWidget*> toHide = {welcomeLabel, whoLabel, connectButton, usernameEdit, rivalBoard,
                               gameStatus, playersList, filterEdit, inviteButton, chat};
    QList<QWidget*> toShow = {myBoard, readyButton, randomPlacement, myStatus, rivalStatus};

    auto allAnimations = new QSequentialAnimationGroup(this);

    auto animationGroup1 = new QParallelAnimationGroup(this);
    auto animationGroup2 = new QParallelAnimationGroup(this);

    for (auto hide: toHide) {
        auto animation = new QPropertyAnimation(hide, "geometry", this);
        animation->setStartValue(hide->geometry());
        animation->setEndValue(hide->property("screen3Geometry"));
        animation->setDuration(1000);
        animation->setEasingCurve(QEasingCurve::InBack);
        animationGroup1->addAnimation(animation);
    }

    for (auto show: toShow) {
        auto animation = new QPropertyAnimation(show, "geometry", this);
        animation->setStartValue(show->geometry());
        animation->setEndValue(show->property("screen3Geometry"));
        animation->setDuration(1000);
        animation->setEasingCurve(QEasingCurve::OutQuad);
        animationGroup2->addAnimation(animation);
    }

    allAnimations->addAnimation(animationGroup1);
    allAnimations->addAnimation(animationGroup2);
    allAnimations->start(QAbstractAnimation::DeleteWhenStopped);
}

void Widget::showFourthScreen()
{
    myBoard->hidePalette();

    QList<QWidget*> toHide = {welcomeLabel, whoLabel, connectButton, usernameEdit,
                               readyButton, randomPlacement, myStatus, rivalStatus,
                               playersList, filterEdit, inviteButton, chat};
    QList<QWidget*> toShow = {myBoard, gameStatus, rivalBoard};

    auto allAnimations = new QSequentialAnimationGroup(this);

    auto animationGroup1 = new QParallelAnimationGroup(this);
    auto animationGroup2 = new QParallelAnimationGroup(this);

    for (auto hide: toHide) {
        auto animation = new QPropertyAnimation(hide, "geometry", this);
        animation->setStartValue(hide->geometry());
        animation->setEndValue(hide->property("screen4Geometry"));
        animation->setDuration(1000);
        animation->setEasingCurve(QEasingCurve::InBack);
        animationGroup1->addAnimation(animation);
    }

    for (auto show: toShow) {
        auto animation = new QPropertyAnimation(show, "geometry", this);
        animation->setStartValue(show->geometry());
        animation->setEndValue(show->property("screen4Geometry"));
        animation->setDuration(1000);
        animation->setEasingCurve(QEasingCurve::OutQuad);
        animationGroup2->addAnimation(animation);
    }

    allAnimations->addAnimation(animationGroup1);
    allAnimations->addAnimation(animationGroup2);
    allAnimations->start(QAbstractAnimation::DeleteWhenStopped);
}
