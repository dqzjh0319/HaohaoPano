#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("MosaicsDemo");

    //QHBoxLayout baselayout = new QHBoxLayout;

    w.show();
    
    return a.exec();
}
