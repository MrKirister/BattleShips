#ifndef SHIP_H
#define SHIP_H

#include <QObject>
#include <QPoint>

class Ship : public QObject
{
    Q_OBJECT
public:
    explicit Ship(QObject *parent = nullptr);

private:
    int size;
    Qt::Orientation orientation;
    QPoint coord;
};

#endif // SHIP_H
