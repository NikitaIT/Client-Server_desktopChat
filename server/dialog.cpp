#include "dialog.h"
#include "ui_dialog.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
/**
 * messageFromGui->onMessageFromGui
 * addLogToGui->onAddLogToGui
 * emit messageFromGui
 * emit fileFromGui
 * @brief Dialog::Dialog
 * @param parent
 * создаем сервер MyServer(127.0.0.1:8888)
 */
Dialog::Dialog(QWidget *parent) :QDialog(parent), ui(new Ui::Dialog)
{
    ui->setupUi(this);

    //создаем сервер
    _serv = new MyServer(this, this);

    //подключаем сигналы от виджета к серверу
    connect(this, SIGNAL(messageFromGui(QString,QStringList)), _serv, SLOT(onMessageFromGui(QString,QStringList)));
    connect(_serv, SIGNAL(addLogToGui(QString,QColor)), this, SLOT(onAddLogToGui(QString,QColor)));

    if (_serv->doStartServer(QHostAddress::LocalHost, 8888))
    {
        ui->lwLog->insertItem(0, QTime::currentTime().toString()+" server strated at "+_serv->serverAddress().toString()+":"+QString::number(_serv->serverPort()));
        ui->lwLog->item(0)->setTextColor(Qt::yellow);
    }
    else
    {
        ui->lwLog->insertItem(0, QTime::currentTime().toString()+" server not strated at "+_serv->serverAddress().toString()+":"+QString::number(_serv->serverPort()));
        ui->lwLog->item(0)->setTextColor(Qt::red);
        ui->pbStartStop->setChecked(true);
    }
}

Dialog::~Dialog()
{
    delete ui;
}
/**
 * @brief Dialog::onAddUserToGui
 * @param name
 * Добавляем в список и сообщаем об этом
 */
void Dialog::onAddUserToGui(QString name)
{
    ui->lwUsers->addItem(name);
    ui->lwLog->insertItem(0, QTime::currentTime().toString()+" "+name+" joined");
    ui->lwLog->item(0)->setTextColor(Qt::green);
}
/**
 * @brief Dialog::onRemoveUserFromGui
 * @param name
 * Удаляем из списка
 */
void Dialog::onRemoveUserFromGui(QString name)
{
    for (int i = 0; i < ui->lwUsers->count(); ++i)
        if (ui->lwUsers->item(i)->text() == name)
        {
            ui->lwUsers->takeItem(i);
            ui->lwLog->insertItem(0, QTime::currentTime().toString()+" "+name+" left");
            ui->lwLog->item(0)->setTextColor(Qt::green);
            break;
        }
}
/**
 * @brief Dialog::onMessageToGui
 * @param message
 * @param from
 * @param users
 * Добавляем сообение в лог
 */
void Dialog::onMessageToGui(QString message, QString from, const QStringList &users)
{
    if (users.isEmpty())
        ui->lwLog->insertItem(0, QTime::currentTime().toString()+" message from "+from+": "+message+" to all");
    else
    {
        ui->lwLog->insertItem(0, QTime::currentTime().toString()+" message from "+from+": "+message+" to "+users.join(","));
        ui->lwLog->item(0)->setTextColor(Qt::blue);
    }
}

void Dialog::onAddLogToGui(QString string, QColor color)
{
    ui->lwLog->insertItem(0, QTime::currentTime().toString()+string);
    ui->lwLog->item(0)->setTextColor(color);
}

void Dialog::onAddFileToGui(QFile &file,QColor color)
{
    ui->lwFiles->insertItem(0, QTime::currentTime().toString()+file.fileName());
    ui->lwFiles->item(0)->setTextColor(color);
}
/**
 * @brief Dialog::on_pbSend_clicked
 * отправляем выбранным сигнал messageFromGui и лог
 */
void Dialog::on_pbSend_clicked()
{
    if (ui->lwUsers->count() == 0)
    {
        QMessageBox::information(this, "", "No clients connected");
        return;
    }
    QStringList l;
    if (!ui->cbToAll->isChecked())
        foreach (QListWidgetItem *i, ui->lwUsers->selectedItems())
            l << i->text();
    emit messageFromGui(ui->pteMessage->document()->toPlainText(), l);
    ui->pteMessage->clear();
    if (l.isEmpty())
        onAddLogToGui("Sended public server message", Qt::black);
    else
        onAddLogToGui("Sended private server message to "+l.join(","), Qt::black);

}

void Dialog::on_cbToAll_clicked()
{
    if (ui->cbToAll->isChecked())
        ui->pbSend->setText("Send To All");
    else
        ui->pbSend->setText("Send To Selected");
}


void Dialog::on_pbStartStop_toggled(bool checked)
{
    if (checked)
    {
        onAddLogToGui(" server stopped at "+_serv->serverAddress().toString()+":"+QString::number(_serv->serverPort()), Qt::green);
        _serv->close();
        ui->pbStartStop->setText("Start server");
    }
    else
    {
        QHostAddress addr;
        if (!addr.setAddress(ui->leHost->text()))
        {
            onAddLogToGui(" invalid address "+ui->leHost->text(), Qt::red);
            return;
        }
        if (_serv->doStartServer(addr, ui->lePort->text().toInt()))
        {
            onAddLogToGui(" server strated at "+ui->leHost->text()+":"+ui->lePort->text(), Qt::green);
            ui->pbStartStop->setText("Stop server");
        }
        else
        {
            onAddLogToGui(" server not strated at "+ui->leHost->text()+":"+ui->lePort->text(), Qt::red);
            ui->pbStartStop->setChecked(true);
        }
    }
}

void Dialog::on_pbAddFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "/home/jana", tr("Files (*)"));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return;
    if (ui->lwUsers->count() == 0)
    {
        QMessageBox::information(this, "", "No clients connected");
        return;
    }
    QStringList l;
    if (!ui->cbToAll->isChecked())
        foreach (QListWidgetItem *i, ui->lwUsers->selectedItems())
            l << i->text();
    if (l.isEmpty())
        onAddLogToGui("Sended public server file", Qt::black);
    else
        onAddLogToGui("Sended private server file to "+l.join(","), Qt::black);
        file.close();
        emit fileFromGui(file,l);
        onAddFileToGui(file,Qt::blue);
}
