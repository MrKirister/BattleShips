#ifndef FIELD_H
#define FIELD_H

#include <QGraphicsView>
#include <QGraphicsEffect>

class Ship;

class Field : public QGraphicsView
{
    Q_OBJECT
public:
    Field(bool palette, QWidget* parent = nullptr);
    const int cellSize = 67;
    const int fieldMargin = 6;
    const int cellMargin = 2;
    const int paletteWidth;
    void createShips();
    // QJsonArray getShips();
    void disableField(bool isReady);
    void hidePalette();
    void placeRandomly();
    void checkMyCell(int row, int col);
    bool validFinalPos(Ship* exception) const;
    QPoint getFinalPos(const QPoint &pos) const;
    bool valid(int row, int col) const;
    void showHit(int val, int row, int col);
    void hideDisabledRect();
private:
    bool havePalette = false;
    QGraphicsItem *opacityRect {};
    QVector<Ship*> ships;
    QGraphicsScene *scene;
    Ship *outline = nullptr;
    QPoint startPos;
    QGraphicsRectItem* selectedCell = nullptr;
    QGraphicsItem *palette = nullptr;
    QGraphicsItem *disabledRect = nullptr;
    QGraphicsItem *grid();
signals:
    void shipsPlaced(bool placed);
    void checkRivalCell(int row, int col);
    void hit(int val, int row, int col);
    // QWidget interface
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
};

#endif // FIELD_H
