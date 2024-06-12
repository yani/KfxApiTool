#include "kfx_api_tool.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    KfxApiTool w;
    w.show();
    return a.exec();
}
