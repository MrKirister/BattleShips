#include "gameoverdialog.h"

GameOverDialog::GameOverDialog(bool rivalWon, QWidget *parent) :
    QDialog{parent}
{
    message = new QLabel(this);
    //TODO : сделать Message как надо
    //TODO : сделать кнопку вернуться в меню
    //TODO : сделать кнопку реванш с соперникой
    //TODO : сделать кнопку свернуть окно
}
