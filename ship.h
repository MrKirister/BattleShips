#ifndef SHIP_H
#define SHIP_H

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPoint>

class Field;

class Ship : public QObject, public QGraphicsPixmapItem
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
    void setOutline(bool outline);

    bool setPosition(int row, int col);
    QPoint getPosition() const; // позиция корабля в сетке row,col
    bool isPositioned() const;
    bool positionValid() const;
    bool intersects(Ship *ship) const;

    int checkHit(int row, int col);
private:
    static QString getFileName(int size, Qt::Orientation orientation);
    Qt::Orientation orientation = Qt::Horizontal;
    Field* field;
    QPoint distance;
    QVector<QPair<int, int>> hits;
    bool outline = false;

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
