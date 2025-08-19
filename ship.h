#ifndef SHIP_H
#define SHIP_H

#include <QGraphicsRectItem>
#include <QObject>
#include <QPoint>

class Field;
class Ship : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    explicit Ship(int size, Field* field);
    int size;
    int row{-1}; // начальное положение, соответствует кораблю на палитре
    int col{-1}; // начальное положение, соответствует кораблю на палитре
    //bool rotate();

    Qt::Orientation getOrientation() const;
    bool setOrientation(Qt::Orientation o, bool check = true);

    bool setPosition(int row, int col);
    QPoint getPosition() const; // позиция корабля в сетке row,col
    bool isPositioned() const;

    int checkHit(int row, int col);
private:
    Qt::Orientation orientation = Qt::Horizontal;
    Field* field;
    QPoint distance;
    QVector<QPair<int, int>> hits;
    // QGraphicsItem interface
protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;

signals:
    void draggingBegin(Ship *ship);
    void dragging(const QPoint &pos);
    void draggingEnd();
    void rotationRequested();
};

#endif // SHIP_H
