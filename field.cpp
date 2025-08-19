#include "field.h"
#include <QJsonObject>
#include "ship.h"
#include <QGraphicsOpacityEffect>
#include <QGraphicsSceneHoverEvent>
#include <QRandomGenerator>
#include <QMouseEvent>

class OpacityRect : public QGraphicsRectItem {
public:
    OpacityRect(const QRectF &rect, const QPen &pen, const QBrush &brush)
        : QGraphicsRectItem(rect)
    {
        setPen(pen);
        setBrush(brush);
        setAcceptHoverEvents(true);
    }

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override
    {
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect;
        effect->setOpacity(1);
        setGraphicsEffect(effect);
        event->accept();
    }
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override
    {
        if (auto effect = dynamic_cast<QGraphicsOpacityEffect *>(graphicsEffect())) {
            QRadialGradient alphaGradient(event->pos(), 325);
            alphaGradient.setColorAt(0.25, QColor(0xE3E3F3));
            alphaGradient.setColorAt(0.1, Qt::transparent);
            effect->setOpacityMask(alphaGradient);
        }
        event->accept();
    }
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override
    {
        setGraphicsEffect(nullptr);
        event->accept();
    }
};



Field::Field(bool palette, QWidget *parent)
    :QGraphicsView(parent), paletteWidth{cellSize*4 + cellMargin*3 + fieldMargin*2}, havePalette{palette}
{
    scene = new QGraphicsScene(parent);
    if(palette){
        scene->setSceneRect(QRect(1, 1, 700+paletteWidth-2, 700-2));
        this->palette = scene->addRect(QRect(701, -1, paletteWidth+1, 702).toRectF(),
                                       QPen("black") , QBrush(QColor(212, 211, 177)));
    }
    else {
        scene->setSceneRect(QRect(1, 1, 700-2, 700-2)); // ! TODO: учесть
        scene->addItem(grid());

        opacityRect = new OpacityRect(QRectF(1,1,698, 698), Qt::NoPen, QColor(0x518487)); // ! TODO: учесть
        scene->addItem(opacityRect);
    }
    this->setScene(scene);
    this->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    this->setRenderHint(QPainter::Antialiasing, false);
}

void Field::createShips()
{
    ships.push_back(new Ship(4, this));
    ships.push_back(new Ship(3, this));
    ships.push_back(new Ship(3, this));
    ships.push_back(new Ship(2, this));
    ships.push_back(new Ship(2, this));
    ships.push_back(new Ship(2, this));
    ships.push_back(new Ship(1, this));
    ships.push_back(new Ship(1, this));
    ships.push_back(new Ship(1, this));
    ships.push_back(new Ship(1, this));
    int i = 0;
    for(auto &el : ships) {
        scene->addItem(el);
        el->setPos(702+fieldMargin, cellSize*i+(cellMargin*i)+ fieldMargin);
        i++;
        connect(el, &Ship::draggingBegin, this, [this](Ship *ship){ //TODO: учесть все connect
            delete outline;
            outline = nullptr;
            outline = new Ship(ship->size, this);
            outline->setOrientation(ship->getOrientation(), false);
            startPos = ship->pos().toPoint();
            outline->setZValue(5);
            outline->setPen(QColor(128, 128, 128));
            outline->setBrush(Qt::NoBrush);
            outline->setVisible(false);
            this->scene->addItem(outline);
            outline->setPos(startPos);
        });
        connect(el, &Ship::dragging, this, [this](const QPoint &pos){
            QPoint finalPos = getFinalPos(pos);
            if (!valid(finalPos.y(), finalPos.x())) return;
            outline->setPosition(finalPos.y(), finalPos.x());
            outline->setVisible(true);
        });
        connect(el, &Ship::draggingEnd, this, [this, el]() {
            outline->setVisible(false);
            if (validFinalPos(el))
                el->setPosition(outline->getPosition().x(), outline->getPosition().y());
            else el->setPos(startPos);
            bool r = std::all_of(ships.begin(), ships.end(), [](Ship *ship){
                return ship->isPositioned();
            });
            emit shipsPlaced(r);
        });
        connect(el, &Ship::rotationRequested, this, [this, el](){
            if (outline->setOrientation(outline->getOrientation() == Qt::Horizontal
                                            ? Qt::Vertical
                                            : Qt::Horizontal), true)
                el->setOrientation(outline->getOrientation(), false);
        });
    }
}

// QJsonArray Field::getShips()
// {
//     QJsonArray a;
//     for(auto ship: ships){
//         QJsonObject o;
//         o["orientation"] = ship->getOrientation();
//         o["size"] = ship->size;
//         o["position"] = QVariant::fromValue(ship->pos()).toJsonValue();
//         a.push_back(o);
//     }
//     return a;
// }

void Field::disableField(bool isReady)
{
    this->setDisabled(isReady);
    if(isReady){
        if(!disabledRect){
            disabledRect = scene->addRect(QRect(0, 0, 700, 704), QPen(), QBrush(QColor(80, 80, 80, 150)));
            disabledRect->setZValue(20);
        }
        disabledRect->show();
    }
    else disabledRect->hide();
}

void Field::hidePalette()
{
    scene->removeItem(palette);
    delete palette;
}

void Field::placeRandomly()
{
    int i = 0;
    int col;
    int row;
    int o;
    bool flag = false;
    while(i < 10){
        flag = false;
        // o == 1 - horizontal
        // o == 0 - vertical
        o = QRandomGenerator::global()->bounded(0, 2);
        if (o) {
            col = QRandomGenerator::global()->bounded(0, 11-ships[i]->size);
            row = QRandomGenerator::global()->bounded(0, 10);
            ships[i]->setOrientation(Qt::Horizontal);
        }
        else {
            col = QRandomGenerator::global()->bounded(0, 10);
            row = QRandomGenerator::global()->bounded(0, 11-ships[i]->size);
            ships[i]->setOrientation(Qt::Vertical);
        }
        ships[i]->setPosition(row, col);
        for(auto & ship: ships) {
            if(ship == ships[i]) continue;
            if(ships[i]->sceneBoundingRect().adjusted(-cellSize, -cellSize, cellSize, cellSize)
                    .intersects(ship->sceneBoundingRect())) {
                flag = true;
                break;
            }
        }
        if (flag) continue;
        emit shipsPlaced(true);
        i++;
    }
}

void Field::checkMyCell(int row, int col)
{
    int val;
    for(auto &ship : ships){
        val = ship->checkHit(row, col);
        if(val != 0) break;
    }
    showHit(val, row, col);
    emit hit(val, row, col);
}

QPoint Field::getFinalPos(const QPoint &pos) const
{
    QPoint result = pos;

    result -= QPoint(fieldMargin, fieldMargin);
    result.setX(qRound(double(result.x()) / (cellSize + cellMargin)));
    result.setY(qRound(double(result.y()) / (cellSize + cellMargin)));
    return result;
}

bool Field::valid(int row, int col) const
{
    if (outline->getOrientation() == Qt::Horizontal) {
        if (col < 0 || col > 10-outline->size) return false;
        if (row < 0 || row > 9) return false;
    }
    else {
        if (row < 0 || row > 10-outline->size) return false;
        if (col < 0 || col > 9) return false;
    }
    return true;
}

void Field::showHit(int val, int row, int col)
{

    if(val == 0){
        auto a = scene->addEllipse(QRect(fieldMargin + col*(cellSize + cellMargin) + cellMargin,
                                         fieldMargin + row*(cellSize + cellMargin) + cellMargin,
                                         cellSize - 3, cellSize - 3));
        a->setPen(QPen(Qt::black, 3));
    }
    else {
        auto l1 = scene->addLine(QLineF(fieldMargin + col*(cellSize + cellMargin), //TODO: сделать икс меньше
                                        fieldMargin + row*(cellSize + cellMargin),
                                        fieldMargin + (col+1)*(cellSize + cellMargin) - cellMargin,
                                        fieldMargin + (row+1)*(cellSize + cellMargin) - cellMargin));
        auto l2 = scene->addLine(QLineF(fieldMargin + col*(cellSize + cellMargin),
                                      fieldMargin + (row+1)*(cellSize + cellMargin) - cellMargin,
                                      fieldMargin + (col+1)*(cellSize + cellMargin) - cellMargin,
                                        fieldMargin + row*(cellSize + cellMargin)));
        l1->setZValue(30);
        l2->setZValue(30);
        l1->setPen(QPen(Qt::black, 3));
        l2->setPen(QPen(Qt::black, 3));

    }
    //TODO: При убийстве коробля надо зачеркивать клетки около него
}

void Field::hideDisabledRect()
{
    delete disabledRect;
}

bool Field::validFinalPos(Ship *exception) const
{
    for(auto &ship : ships){
        if(exception == ship) continue;
        if(ship->pos().x() > 700) continue;
        if(ship->sceneBoundingRect().adjusted(-cellSize, -cellSize, cellSize, cellSize).intersects(outline->sceneBoundingRect())) return false;
    }
    return true;
}

QGraphicsItem *Field::grid()
{
    QGraphicsItemGroup *grid = new QGraphicsItemGroup();

    for (int i=0; i <= 10; ++i) {
        auto line = new QGraphicsLineItem(double(fieldMargin) - double(cellMargin)/2,
                                          -double(cellMargin)/2 + fieldMargin + i*(cellSize + cellMargin),
                                          700.0 - double(fieldMargin) + double(cellMargin)/2,
                                          -double(cellMargin)/2 + fieldMargin + i*(cellSize + cellMargin),
                                          grid);
        line->setPen(QPen(QColor(0xe3e3d3).lighter(100), cellMargin));
    }
    for (int i=0; i <= 10; ++i) {
        auto line = new QGraphicsLineItem(-double(cellMargin)/2 + fieldMargin + i*(cellSize + cellMargin),
                                          double(fieldMargin) - double(cellMargin)/2,
                                          -double(cellMargin)/2 + fieldMargin + i*(cellSize + cellMargin),
                                          700.0 - double(fieldMargin) + double(cellMargin)/2,
                                          grid);
        line->setPen(QPen(QColor(0xe3e3d3).lighter(100), cellMargin));
    }
    return grid;
}

//67*10 + 2*9 18 670+18=688 12/2
//67 - клетка


void Field::mousePressEvent(QMouseEvent *event)
{
    if(havePalette){
        QGraphicsView::mousePressEvent(event);
        return;
    }
    auto pos = event->pos();
    int col = (pos.x() - fieldMargin )/ (cellSize+cellMargin);
    int row = (pos.y() - fieldMargin )/ (cellSize+cellMargin);
    if(selectedCell && selectedCell->contains(pos)){
        emit checkRivalCell(row, col);
    }
    else {
        delete selectedCell;
        selectedCell = new QGraphicsRectItem;
        selectedCell->setRect(QRect(fieldMargin + (cellSize + cellMargin) * col,
                                    fieldMargin + (cellSize + cellMargin) * row,
                                    cellSize, cellSize));
        //selectedCell->setBrush(QColor(colors[size-1]));
        selectedCell->setPen(QPen(QColor(0xe3e3d3).lighter(95), 2));
        selectedCell->setZValue(25);
        scene->addItem(selectedCell);
    }
}
