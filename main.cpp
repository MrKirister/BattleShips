#include "widget.h"
#include <QApplication>
#include <QSettings>

//TODO: Нужно написать клиента который будет общатся с сервером
//TODO: Нужно написать отрисовку игры на экране. (Корабли) (Field)
//TODO: Придумать интерфейс игры. Куда разместить кнопки и поле
//TODO: Предусмотреть возможность смотреть статистику. (W/L)

int main(int argc, char *argv[])
{
    qputenv("QT_USE_PHYSICAL_DPI", "1");
    QApplication a(argc, argv);
    a.setApplicationName("BattleShips");
    a.setOrganizationName("BattleShips");
    Widget w;
    w.show();
    return a.exec();
}
