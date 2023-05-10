#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));


    QApplication a(argc, argv);
    a.setApplicationName("sinchro");
    a.setWindowIcon(QIcon(":/img/icon.png"));//a.setWindowIcon(QIcon(":/img/sinchro.png"));
    MainWindow w;
    if(w.result)
        return 0;
    w.show();

    return a.exec();
}
