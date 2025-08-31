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
    QGraphicsScene *scene;
    constexpr static double cellSize = 50;
    constexpr static double fieldMargin = 5;
    constexpr static double cellMargin = 1;
    const double paletteWidth;
    const double totalWidth;
    const double totalHeight;
    static int getBaseWidth() {return int(cellSize*10+cellMargin*9+2*fieldMargin);}
    // static int getTotalWidth();
    static int getBaseHeight() {return Field::getBaseWidth();}
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
    void showKilledShipArea(int row, int col, int size, Qt::Orientation o);
private:
    bool havePalette = false;
    QGraphicsItem *opacityRect {};
    QVector<Ship*> ships;

    Ship *outline = nullptr;
    QPoint startPos;
    QGraphicsRectItem* selectedCell = nullptr;
    QGraphicsItem *palette = nullptr;
    QGraphicsItem *grid();
    bool checkedCells[10][10]{};
    int killedShips = 0;
    void shipKilled(Ship* ship);
signals:
    void shipsPlaced(bool placed);
    void checkRivalCell(int row, int col);
    void hit(int val, int row, int col);
    void shipKilled(int row, int col, int size, Qt::Orientation o);
    void gameOver();
    // QWidget interface
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
};

#endif // FIELD_H
