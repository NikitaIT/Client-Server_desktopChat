#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTcpSocket>
#include <QHostAddress>
#include "D:\Program\123\server/myclient.h"
class MyClient;

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
private slots:
    void onSokConnected();
    void onSokDisconnected();
    void onSokReadyRead();
    void onSokDisplayError(QAbstractSocket::SocketError socketError);

    void on_pbConnect_clicked();
    void on_pbDisconnect_clicked();
    void on_cbToAll_clicked();
    void on_pbSend_clicked();

    void on_pbAddFile_clicked();
    void on_lwFiles_doubleClicked(const QModelIndex &index);

signals:
    void fileFromGui(QFile &file, const QStringList &users);//+++++++++++
private:
    Ui::Dialog *ui;
    QTcpSocket *_sok;
    quint16 _blockSize;
    QString _name;
    void onAddLogToGui(QString text, QColor color = Qt::black);
    void onAddFileToGui(QFile &file,QColor color);

};

#endif // DIALOG_H
