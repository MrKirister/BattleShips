#include "ship.h"
#include "field.h"
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

Ship::Ship(int size, Field *field)
    : QGraphicsRectItem{}, size{size}, field{field}
{
    static uint colors[4] {0xa20018, 0x1fa146, 0x406dcc, 0xa349a4};
    this->setRect(QRect(0, 0, field->cellMargin*(size-1)+field->cellSize*size, field->cellSize));
    this->setBrush(QColor(colors[size-1]));
    this->setPen(QPen(Qt::black, 0));
    this->setZValue(10);
}

Qt::Orientation Ship::getOrientation() const
{
    return orientation;
}

bool Ship::setOrientation(Qt::Orientation o, bool check)
{
    if ((row == -1 || col == -1) && check)
        return false; // корабль в палитре не поворачиваем

    if (o == Qt::Horizontal) {
        if (col + size > 10 && check) return false;
        setRect(QRect(0, 0, field->cellMargin*(size-1)+field->cellSize*size,
                      field->cellSize));
    }
    else { // vertical
        if (row + size > 10 && check) return false;
        setRect(QRect(0, 0, field->cellSize,
                      field->cellMargin*(size-1)+field->cellSize*size));
    }
    orientation = o;
    return true;
}

bool Ship::setPosition(int row, int col)
{
    if (orientation == Qt::Horizontal) {
        if (row < 0 || row > 9) return false;
        if (col < 0 || col > 10-size) return false;
    }
    else { // vertical
        if (row < 0 || row > 10-size) return false;
        if (col < 0 || col > 9) return false;
    }
    this->row = row;
    this->col = col;
    setPos(QPoint(field->fieldMargin + col*(field->cellSize + field->cellMargin),
                  field->fieldMargin + row*(field->cellSize + field->cellMargin)));
    return true;
}

QPoint Ship::getPosition() const
{
    return QPoint(row, col);
}

bool Ship::isPositioned() const
{
    return (row != -1 && col != -1);
}

int Ship::checkHit(int row, int col)
{
    if(orientation == Qt::Horizontal){
        if(this->row != row) return 0;
        if(col < this->col || col  >= this->col + this->size) return 0;

    }
    else {
        if(this->col != col) return 0;
        if(row < this->row || row  >= this->row + this->size) return 0;
    }

    if(!hits.contains({row, col})) hits.push_back({row, col});  //damaged - 1
    if(hits.size() == size) return 2; // killed - 2
    return 1;
}

void Ship::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() != Qt::LeftButton){
        event->ignore();
        return;
    }
    distance = this->pos().toPoint() - event->scenePos().toPoint();
    emit draggingBegin(this);
    this->setFlag(QGraphicsItem::ItemIsFocusable, true);
    this->setFocus(Qt::MouseFocusReason);
    event->accept();
}

void Ship::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    this->setPos(distance + event->scenePos().toPoint());
    emit dragging(distance + event->scenePos().toPoint());
}

void Ship::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    emit draggingEnd();
    this->setFlag(QGraphicsItem::ItemIsFocusable, false);
    this->clearFocus();
    event->accept();
}

void Ship::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
}

void Ship::keyPressEvent(QKeyEvent *event)
{
    if(event->key() != Qt::Key_R){
        event->ignore();
        return;
    }
    emit rotationRequested();
    event->accept();
}
