#ifndef FIELD_H
#define FIELD_H

#include <QGraphicsView>

class Ship;
class Field : public QGraphicsView
{
    Q_OBJECT
public:
    Field();
private:
    QVector<Ship*> ships;
};

#endif // FIELD_H
