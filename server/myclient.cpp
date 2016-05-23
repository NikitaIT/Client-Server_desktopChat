#include "myclient.h"

const QString MyClient::constNameUnknown = QString(".Unknown");
/**
 * _sok.this disconnected -> MyClient.this onDisconnect
 * readyRead -> onReadyRead
 * emit removeUserFromGui
 * emit removeUser
 * emit addUserToGui
 * emit messageToGui x3
 * emit fileToGui x2
 * @brief MyClient::MyClient
 * @param desc
 * @param serv
 * @param parent
 */
MyClient::MyClient(int desc, MyServer *serv, QObject *parent) :QObject(parent)
{
    //храниим указатель на объект-сервер
    _serv = serv;
    //клиент не прошел авторизацию
    _isAutched = false;
    _name = constNameUnknown;
    //размер принимаемого блока 0
    _blockSize = 0;
    //создаем сокет MyClient
    _sok = new QTcpSocket(this);
    //устанавливаем дескриптор из incomingConnection() заранее _sok.connected()==true
    _sok->setSocketDescriptor(desc);
    //подключаем сигналы
    connect(_sok, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    connect(_sok, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(_sok, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

    qDebug() << "Client connected" << desc;
}

MyClient::~MyClient()
{

}

void MyClient::onDisconnect()
{
    qDebug() << "Client disconnected";
    //если авторизован
    if (_isAutched)
    {
        //убирием из интерфейса
        emit removeUserFromGui(_name);
        //сообщаем всем, что клиент вышел
        _serv->doSendToAllUserLeft(_name);
        //убираем из списка
        emit removeUser(this);
    }
    deleteLater();
}

void MyClient::onError(QAbstractSocket::SocketError socketError) const
{
    //w нужна для обсвобождения памяти от QMessageBox (посредством *parent = &w)
    QWidget w;
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(&w, "Error", "The host was not found");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(&w, "Error", "The connection was refused by the peer.");
        break;
    default:
        QMessageBox::information(&w, "Error", "The following error occurred: "+_sok->errorString());
    }
    //тут вызовутся деструктор w и соответственно QMessageBox (по правилам с++)
}

void MyClient::onReadyRead()
{
    QDataStream in(_sok);
    //если считываем новый блок первые 2 байта это его размер
    if (_blockSize == 0) {
        //если пришло меньше 2 байт ждем пока будет 2 байта
        if (_sok->bytesAvailable() < (int)sizeof(quint16))
            return;
        //считываем размер (2 байта)
        in >> _blockSize;
        qDebug() << "_blockSize now " << _blockSize;
    }
    //ждем пока блок прийдет полностью
    if (_sok->bytesAvailable() < _blockSize)
        return;
    else
        //можно принимать новый блок
        _blockSize = 0;
    //3 байт - команда серверу
    quint8 command;
    in >> command;
    qDebug() << "Received command " << command;
    //для неавторизованный пользователей принимается только команда "запрос на авторизацию"
    if (!_isAutched && command != comAutchReq)
        return;

    switch(command)
    {
        //запрос на авторизацию
        case comAutchReq:
        {
            //считываем имя
            QString name;
            in >> name;
            //проверяем его
            if (!_serv->isNameValid(name))
            {
                //отправляем ошибку
                doSendCommand(comErrNameInvalid);
                return;
            }
            //проверяем не используется ли имя
            if (_serv->isNameUsed(name))
            {
                //отправляем ошибку
                doSendCommand(comErrNameUsed);
                return;
            }
            //авторизация пройдена
            _name = name;
            _isAutched = true;
            //отправляем новому клиенту список активных пользователей
            doSendUsersOnline();
            //добавляем в интерфейс
            emit addUserToGui(name);
            //сообщаем всем про нового ползователя
            _serv->doSendToAllUserJoin(_name);
        }
        break;
        //от текущего пользователя пришло сообщение для всех
        case comMessageToAll:
        {
            QString message;
            in >> message;
            //отправляем его всем
            _serv->doSendToAllMessage(message, _name);
            //обновляем лог событий
            emit messageToGui(message, _name, QStringList());
        }
        break;
        //от текущего пользователя пришло сообщение для некоторых пользователей
        case comMessageToUsers:
        {
            QString users_in;
            in >> users_in;
            QString message;
            in >> message;
            //разбиваем строку на имена
            QStringList users = users_in.split(",");
            //отправляем нужным
            _serv->doSendMessageToUsers(message, users, _name);
            //обновляем интерфейс
            emit messageToGui(message, _name, users);
        }
        break;

//        static const quint8 comPublicServerFile = 12;
//        static const quint8 comPrivateServerFile = 13;

        //от текущего пользователя пришло сообщение для всех
        case comFileToAll:
        {
            QByteArray file_ByteArray;
            in >> file_ByteArray;
            QFile file;
            file.write(file_ByteArray);
            //отправляем его всем
            _serv->doSendToAllFile(file, _name);
            //обновляем лог событий
            emit fileToGui(file, _name, QStringList());
        }
        break;
        //от текущего пользователя пришло сообщение для некоторых пользователей
        case comFileToUsers:
        {
            QString users_in;
            in >> users_in;
            QByteArray file_ByteArray;
            in >> file_ByteArray;
            QFile file;
            file.write(file_ByteArray);
            //разбиваем строку на имена
            QStringList users = users_in.split(",");
            //отправляем нужным
            _serv->doSendFileToUsers(file, users, _name);
            //обновляем интерфейс
            emit fileToGui(file, _name, users);
        }
        break;
    }

    //for (long long i = 0; i < 4000000000; ++i){}
}

void MyClient::doSendCommand(quint8 comm) const
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << comm;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    _sok->write(block);
    qDebug() << "Send to" << _name << "command:" << comm;
}

void MyClient::doSendUsersOnline() const
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << comUsersOnline;
    QStringList l = _serv->getUsersOnline();
    QString s;
    for (int i = 0; i < l.length(); ++i)
        if (l.at(i) != _name)
            s += l.at(i)+(QString)",";
    s.remove(s.length()-1, 1);
    out << s;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    _sok->write(block);
    qDebug() << "Send user list to" << _name << ":" << s;
}


