#ifndef GAMEOVERDIALOG_H
#define GAMEOVERDIALOG_H

#include <QLabel>
#include <QDialog>

class GameOverDialog : public QDialog
{
    Q_OBJECT
public:
    GameOverDialog(bool rivalWon, QWidget *parent = nullptr);
private:
    QLabel* message;
};

#endif // GAMEOVERDIALOG_H
