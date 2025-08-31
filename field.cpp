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
            QRadialGradient alphaGradient(event->pos(), 100);
            alphaGradient.setColorAt(1, QColor("#007dcc"));
            alphaGradient.setColorAt(0.25, QColor("#30007dcc"));
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
    :QGraphicsView(parent),
    paletteWidth{cellSize*4 + cellMargin*3 + fieldMargin*2},
    totalWidth{10*cellSize + 9*cellMargin + 2*fieldMargin + (palette ? paletteWidth : 0)},
    totalHeight{10*cellSize + 9*cellMargin + 2*fieldMargin},
    havePalette{palette}
{
    scene = new QGraphicsScene(parent);
    scene->setSceneRect(QRect(1, 1, totalWidth-2, totalHeight-2));
    scene->addItem(grid());
    if(palette) {
        // this->palette = scene->addRect(QRectF(totalHeight+1, -1, paletteWidth+1, totalHeight+2),
        //                                Qt::NoPen , QBrush(QColor(0x555555)));
    }
    else {
        QRadialGradient g({0.0,0.0}, 1.5, {0.0, 0.0});
        g.setColorAt(0, QColor("#007dcc"));
        g.setColorAt(1, QColor("#0065a7"));
        opacityRect = new OpacityRect(QRectF(1,1,totalWidth-2, totalWidth-2),
                                      Qt::NoPen, g);
        scene->addItem(opacityRect);
    }

    this->setScene(scene);
    // this->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    // this->setRenderHint(QPainter::Antialiasing, false);
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
        el->setPos(totalHeight+fieldMargin, cellSize*i+(cellMargin*i)+ fieldMargin);
        i++;
        connect(el, &Ship::draggingBegin, this, [this](Ship *ship){
            delete outline;
            outline = nullptr;
            outline = new Ship(ship->size, this);
            outline->setOrientation(ship->getOrientation(), false);
            outline->setOutline(true);
            startPos = ship->pos().toPoint();
            outline->setZValue(5);
            // outline->setPen(QColor(0xeeeeee));
            // outline->setBrush(Qt::NoBrush);
            outline->setVisible(false);
            this->scene->addItem(outline);
            outline->setPos(startPos);
        });
        connect(el, &Ship::dragging, this, [this](const QPoint &pos){
            QPoint finalPos = getFinalPos(pos);
            // if (!valid(finalPos.y(), finalPos.x())) return;
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

void Field::disableField(bool isReady)
{
    this->setDisabled(isReady);
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
        if(val == 2) {
            shipKilled(ship);
            emit shipKilled(ship->row, ship->col, ship->size, ship->getOrientation());
            killedShips++;
            if(killedShips == 10) emit gameOver();
        }
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
        auto l = scene->addPixmap(QPixmap(":/images/miss.png"));
        l->setPos(fieldMargin + col*(cellSize + cellMargin),
                  fieldMargin + row*(cellSize + cellMargin));
        l->setZValue(30);
    }
    else {
        auto l = scene->addPixmap(QPixmap(":/images/hit.png"));
        l->setPos(fieldMargin + col*(cellSize + cellMargin),
                  fieldMargin + row*(cellSize + cellMargin));
        l->setZValue(30);
    }
}

void Field::showKilledShipArea(int row, int col, int size, Qt::Orientation o)
{
    if(o == Qt::Horizontal){
        if(row > 0 && col > 0){
            showHit(0, row - 1, col - 1);
        }
        if(row < 9 && col > 0){
            showHit(0, row + 1, col - 1);
        }
        if(row > 0 &&  col +  size <= 9){
            showHit(0,  row - 1,  col +  size);
        }
        if(row < 9 &&  col +  size <= 9){
            showHit(0,  row + 1,  col +  size);
        }

        if( col > 0){
            showHit(0,  row,  col - 1);
        }
        if( col +  size <= 9){
            showHit(0,  row,  col +  size);
        }
        for(int i =  col; i <  col+ size; i++){
            if( row > 0){
                showHit(0,  row - 1, i);
            }
            if( row < 9){
                showHit(0,  row + 1, i);
            }
        }
    }
    else {
        if( row > 0 &&  col > 0){
            showHit(0,  row - 1,  col - 1);
        }
        if( row +  size <= 9 &&  col > 0){
            showHit(0,  row +  size,  col - 1);
        }
        if( row > 0 &&  col < 9){
            showHit(0,  row - 1,  col + 1);
        }
        if( row +  size <= 9 &&  col < 9){
            showHit(0,  row +  size,  col + 1);
        }


        if( row > 0){
            showHit(0, row - 1, col);
        }
        if( row +  size <= 9){
            showHit(0,  row + size, col);
        }
        for(int i =  row; i < row+ size; i++){
            if( col > 0){
                showHit(0, i,  col - 1);
            }
            if( col < 9){
                showHit(0, i,  col + 1);
            }
        }
    }
}

bool Field::validFinalPos(Ship *exception) const
{
    // for(auto &ship : ships){
    //     if(exception == ship) continue;
    //     if(ship->pos().x() > totalHeight) continue;
    //     if(ship->sceneBoundingRect().adjusted(-cellSize, -cellSize, cellSize, cellSize).intersects(outline->sceneBoundingRect())) return false;
    // }
    // return true;

    // check the outline position
    if (!outline->positionValid()) return false;

    for(auto &ship : ships) {
        if(exception == ship) continue;
        if (outline->intersects(ship)) return false;
    }
    return true;
}

QGraphicsItem *Field::grid()
{
    QGraphicsItemGroup *grid = new QGraphicsItemGroup();
    QColor lineColor(qRgba(160,160,200,120));

    for (int i=0; i <= 10; ++i) {
        auto line = new QGraphicsLineItem(double(fieldMargin) - double(cellMargin)/2,
                                          -double(cellMargin)/2 + fieldMargin + i*(cellSize + cellMargin),
                                          totalHeight - double(fieldMargin) + double(cellMargin)/2,
                                          -double(cellMargin)/2 + fieldMargin + i*(cellSize + cellMargin),
                                          grid);
        line->setPen(QPen(lineColor, cellMargin/2));
    }
    for (int i=0; i <= 10; ++i) {
        auto line = new QGraphicsLineItem(-double(cellMargin)/2 + fieldMargin + i*(cellSize + cellMargin),
                                          double(fieldMargin) - double(cellMargin)/2,
                                          -double(cellMargin)/2 + fieldMargin + i*(cellSize + cellMargin),
                                          totalHeight - double(fieldMargin) + double(cellMargin)/2,
                                          grid);
        line->setPen(QPen(lineColor, cellMargin/2));
    }
    return grid;
}

void Field::shipKilled(Ship *ship)
{
    showKilledShipArea(ship->row, ship->col, ship->size, ship->getOrientation());
}

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
