#include "panel.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Set app style
    QFile file(":/res/default.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet(file.readAll());
    a.setStyleSheet(styleSheet);

    Panel w;
    w.show();
    return a.exec();
}
