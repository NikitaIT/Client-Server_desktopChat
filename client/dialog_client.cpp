#include "dialog_client.h"
#include "ui_dialog_client.h"

#include <QtGui>
#include <QDebug>
#include <QFileDialog>
Dialog::Dialog(QWidget *parent) :QDialog(parent),ui(new Ui::Dialog)
{
    ui->setupUi(this);

    _name = "";
    _sok = new QTcpSocket(this);
    connect(_sok, SIGNAL(readyRead()), this, SLOT(onSokReadyRead()));
    connect(_sok, SIGNAL(connected()), this, SLOT(onSokConnected()));
    connect(_sok, SIGNAL(disconnected()), this, SLOT(onSokDisconnected()));
    connect(_sok, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(onSokDisplayError(QAbstractSocket::SocketError)));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::onSokDisplayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, "Error", "The host was not found");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, "Error", "The connection was refused by the peer.");
        break;
    default:
        QMessageBox::information(this, "Error", "The following error occurred: "+_sok->errorString());
    }
}

void Dialog::onSokReadyRead()
{
    QDataStream in(_sok);
    if (_blockSize == 0) {
        if (_sok->bytesAvailable() < (int)sizeof(quint16))
            return;
        in >> _blockSize;
        qDebug() << "_blockSize now " << _blockSize;
    }
    if (_sok->bytesAvailable() < _blockSize)
        return;
    else
        _blockSize = 0;
    quint8 command;
    in >> command;
    qDebug() << "Received command " << command;

    switch (command)
    {
        case MyClient::comAutchSuccess:
        {
            ui->pbSend->setEnabled(true);
            onAddLogToGui("Enter as "+_name,Qt::green);
        }
        break;
        case MyClient::comUsersOnline:
        {
            onAddLogToGui("Received user list "+_name,Qt::green);
            ui->pbSend->setEnabled(true);
            QString users;
            in >> users;
            if (users == "")
                return;
            QStringList l =  users.split(",");
            ui->lwUsers->addItems(l);
        }
        break;
        case MyClient::comPublicServerMessage:
        {
            QString message;
            in >> message;
            onAddLogToGui("[PublicServerMessage]: "+message, Qt::red);
        }
        break;
        case MyClient::comMessageToAll:
        {
            QString user;
            in >> user;
            QString message;
            in >> message;
            onAddLogToGui("["+user+"]: "+message);
        }
        break;
        case MyClient::comMessageToUsers:
        {
            QString user;
            in >> user;
            QString message;
            in >> message;
            onAddLogToGui("["+user+"](private): "+message, Qt::blue);
        }
        break;
        case MyClient::comPrivateServerMessage:
        {
            QString message;
            in >> message;
            onAddLogToGui("[PrivateServerMessage]: "+message, Qt::red);
        }
        break;
        case MyClient::comUserJoin:
        {
            QString name;
            in >> name;
            ui->lwUsers->addItem(name);
            onAddLogToGui(name+" joined", Qt::green);
        }
        break;
        case MyClient::comUserLeft:
        {
            QString name;
            in >> name;
            for (int i = 0; i < ui->lwUsers->count(); ++i)
                if (ui->lwUsers->item(i)->text() == name)
                {
                    ui->lwUsers->takeItem(i);
                    onAddLogToGui(name+" left", Qt::green);
                    break;
                }
        }
        break;
        case MyClient::comErrNameInvalid:
        {
            QMessageBox::information(this, "Error", "This name is invalid.");
            _sok->disconnectFromHost();
        }
        break;
        case MyClient::comErrNameUsed:
        {
            QMessageBox::information(this, "Error", "This name is already used.");
            _sok->disconnectFromHost();
        }
        break;
        //получаем comFileToAll sender filename file size-1
        case MyClient::comFileToAll:
        {
            QString user;
            in >> user;
            QString filename;
            in >>filename;
            QByteArray file_ByteArray;
            in >> file_ByteArray;
            QFile file(filename);
            file.open(QIODevice::WriteOnly | QIODevice::Append);
            file.write(file_ByteArray);
            file.close();
            onAddFileToGui(file,Qt::gray);
            onAddLogToGui("["+user+"] прислал всем файл ", Qt::blue);
        }
        break;
        //получаем comFileToUsers sender filename file size-1
        case MyClient::comFileToUsers:
        {
            QString user;
            in >> user;
            QString filename;
            in >>filename;
            QByteArray file_ByteArray;
            in >> file_ByteArray;
            QFile file(filename);
            file.open(QIODevice::WriteOnly | QIODevice::Append);
            file.write(file_ByteArray);
            file.close();
            onAddLogToGui("<"+user+"> прислал файл"+ file.fileName(), Qt::gray);
            onAddFileToGui(file,Qt::red);
        }
        break;
    }
}

void Dialog::onSokConnected()
{
    ui->pbConnect->setEnabled(false);
    ui->pbDisconnect->setEnabled(true);
    _blockSize = 0;
    onAddLogToGui("Connected to"+_sok->peerAddress().toString()+":"+QString::number(_sok->peerPort()),Qt::green);

    //try autch
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << (quint8)MyClient::comAutchReq;
    out << ui->leName->text();
    _name = ui->leName->text();
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    _sok->write(block);
}

void Dialog::onSokDisconnected()
{
    ui->pbConnect->setEnabled(true);
    ui->pbDisconnect->setEnabled(false);
    ui->pbSend->setEnabled(false);
    ui->lwUsers->clear();
    onAddLogToGui("Disconnected from"+_sok->peerAddress().toString()+":"+QString::number(_sok->peerPort()), Qt::green);
}

void Dialog::on_pbConnect_clicked()
{
    _sok->connectToHost(ui->leHost->text(), ui->sbPort->value());
}

void Dialog::on_pbDisconnect_clicked()
{
    _sok->disconnectFromHost();
}

void Dialog::on_cbToAll_clicked()
{
    if (ui->cbToAll->isChecked())
        ui->pbSend->setText("Send To All");
    else
        ui->pbSend->setText("Send To Selected");
}

void Dialog::on_pbSend_clicked()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    if (ui->cbToAll->isChecked())
        out << (quint8)MyClient::comMessageToAll;
    else
    {
        out << (quint8)MyClient::comMessageToUsers;
        QString s;
        foreach (QListWidgetItem *i, ui->lwUsers->selectedItems())
            s += i->text()+",";
        s.remove(s.length()-1, 1);
        out << s;
    }

    out << ui->pteMessage->document()->toPlainText();
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    _sok->write(block);
    ui->pteMessage->clear();
}

void Dialog::onAddLogToGui(QString text, QColor color)
{
    ui->lwLog->insertItem(0, QTime::currentTime().toString()+" "+text);
    ui->lwLog->item(0)->setTextColor(color);
}
//oтправляем 0 comFileToAll file size-1
//отправляем 0 comFileToUsers selectedUsers file size-1
void Dialog::on_pbAddFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "/home/jana", tr("Files (*)"));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return;
    file.open(QIODevice::ReadOnly);
    if (ui->lwUsers->count() == 0)
    {
        QMessageBox::information(this, "", "No clients connected");
        return;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    if (ui->cbToAll->isChecked())
        out << (quint8)MyClient::comFileToAll;
    else
    {
        out << (quint8)MyClient::comFileToUsers;
        QString s;
        foreach (QListWidgetItem *i, ui->lwUsers->selectedItems())
            s += i->text()+",";
        s.remove(s.length()-1, 1);
        out << s;
        if (s.isEmpty())
            onAddLogToGui("Sended public MY file", Qt::green);
        else
            onAddLogToGui("Sended private MY file to "+s, Qt::green);
    }
    out << fileName;
    out << file.readAll();
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    _sok->write(block);
    ui->pteMessage->setPlainText(block+"---"+file.readAll());
        file.close();
        onAddFileToGui(file,Qt::blue);
}
void Dialog::onAddFileToGui(QFile &file,QColor color)
{
    ui->lwFiles->insertItem(0, QTime::currentTime().toString()+file.fileName());
    ui->lwFiles->item(0)->setTextColor(color);
}
