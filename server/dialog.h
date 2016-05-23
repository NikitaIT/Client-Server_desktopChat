#ifndef DIALOG_H
#define DIALOG_H

#include <QDebug>
#include <QDialog>
#include <QtCore>
#include "myserver.h"

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

public:
    Ui::Dialog *ui;
    MyServer *_serv;

signals:
    void messageFromGui(QString message, const QStringList &users);
    void fileFromGui(QFile &file, const QStringList &users);//+++++++++++

public slots:
    void onAddUserToGui(QString name);
    void onRemoveUserFromGui(QString name);
    void onMessageToGui(QString message, QString from, const QStringList &users);
    void onAddLogToGui(QString string, QColor color);
    void onAddFileToGui(QFile &file,QColor color);//+++++++++++++++
private slots:
    void on_pbSend_clicked();
    void on_cbToAll_clicked();
    void on_pbStartStop_toggled(bool checked);
    void on_pbAddFile_clicked();
};

#endif // DIALOG_H
