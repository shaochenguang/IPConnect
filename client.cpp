#include "client.h"

Client::Client(QObject *parent) : QObject(parent)
{
    qDebug() << this << "Client Created";

    m_ClientUsername = "test";
    m_MyUsername = "";
    m_detailSent = false;
    m_detailAccepted = false;
}

Client::~Client()
{
    qDebug() << this << "Client Destroyed";
    emit destroyingConnection(m_ClientUsername);
}

const QTcpSocket *Client::getSocket()
{
    return m_socket;
}

void Client::setClientUsername(const QString t_username)
{
    m_ClientUsername = t_username;
}

void Client::sendDetail()
{
    qDebug() << this << "Checking for sockets";
    if(m_socket)
    {
        QByteArray message = "IPC:CONNECT:REQUEST:"+m_MyUsername.toUtf8();
        m_response.insert("app","IPC");
        m_response.insert("method","CONNECT");
        m_response.insert("option","REQUEST");
        m_response.insert("data",m_MyUsername);
        m_socket->write(message);
        m_response.insert("message","IPC:CONNECT:REQUEST:"+m_MyUsername);
        m_socket->waitForBytesWritten();
        m_detailSent = true;
    }
    else
        qDebug() << this << "socket not available on greetings";
}

void Client::sendMessage(QString t_message)
{
    if(!m_socket) return;
    qDebug() << this << "writing to " << m_socket;
    QByteArray socketMessage = "IPC:MESSAGE:TEXT:"+t_message.toUtf8();
    m_socket->write(socketMessage);
    m_response.insert("message","IPC:MESSAGE:TEXT:"+t_message);
    m_socket->waitForBytesWritten();
}

void Client::setSocket(QTcpSocket *t_socket)
{
    m_socket = t_socket;
    connect(m_socket,&QTcpSocket::connected, this, &Client::connected,Qt::DirectConnection);
    connect(m_socket,&QTcpSocket::disconnected, this, &Client::disconnected,Qt::DirectConnection);
    connect(m_socket,&QTcpSocket::readyRead, this, &Client::readyRead,Qt::DirectConnection);
    connect(m_socket,&QTcpSocket::bytesWritten, this, &Client::bytesWritten,Qt::DirectConnection);
    connect(m_socket,&QTcpSocket::stateChanged, this, &Client::stateChanged,Qt::DirectConnection);
    connect(m_socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&Client::error,Qt::DirectConnection);
}

void Client::setUsername(const QString t_username)
{
    m_MyUsername = t_username;
}

void Client::processRead(QByteArray t_data)
{
    QString header = t_data;
    qDebug() << this << "Processing GET header = "<<header;
    m_request.insert("request",header);
    qDebug() << this << "request changed to " << m_request.value("request");

    //Header has 4 values APP METHOD OPTION DATA(optional)
    QStringList options = header.split(":");

    if(options.count() >= 1) m_request.insert("app", options.at(0).trimmed());

    if(options.count() >= 2) m_request.insert("method", options.at(1).trimmed());

    if(options.count() >= 3) m_request.insert("option", options.at(2).trimmed());

    if(options.count() >= 4)
    {
        //  IPC:MESSAGE:TEXT:hi
        int pos = header.indexOf(":") + m_request.value("method","").size() + m_request.value("option","").size() + 2;
        m_request.insert("data", header.mid(pos + 1));
    }

    handleRequest();
}

void Client::handleRequest()
{
    QString method = m_request.value("method","");
    QString option = m_request.value("option","");

    if(method == "CONNECT")
    {
        m_request.insert("message",m_request.value("request"));
        if(option == "REQUEST")
        {
            if(!m_detailAccepted)
            {
                if(m_request.value("option") == "REQUEST")
                {
                    qDebug() << this << "Accepting Detail";
                    QString name = m_request.value("data");
                    setClientUsername(name.trimmed());
                    qDebug() << this << "Accepted request from "<<m_ClientUsername;
                    m_response.insert("app","IPC");
                    m_response.insert("method","CONNECT");
                    m_response.insert("option","REQUEST");
                    m_response.insert("data",m_MyUsername);

                    QByteArray message = "IPC:CONNECT:REQUEST:"+m_MyUsername.toUtf8();
                    if(!m_detailSent)
                    {
                        m_socket->write(message);
                        m_response.insert("message",message);
                        m_socket->waitForBytesWritten(1000);
                    }
                    m_detailAccepted = true;
                    qDebug() <<this<< "emiting capturedDetail";
                    emit capturedDetail(name.trimmed(),this);
                }
            }
        }
    }
    if(method == "MESSAGE")
    {
        m_request.insert("message",m_request.value("request"));
        m_response.insert("app","IPC");
        m_response.insert("method","MESSAGE");

        if(m_request.value("option") == "TEXT") emit gotMessageRequest(m_ClientUsername+" : "+m_request.value("data"));
    }
}

void Client::connected()
{
    qDebug() << this << "Client Connected";
}

void Client::disconnected()
{
    qDebug() << this << "Client Disconnected";
}

void Client::readyRead()
{
    if(!m_socket) return;
    qDebug() << "Socket available on ready read " << m_socket->socketDescriptor();
    QString allData = m_socket->readAll();
    processRead(allData.toUtf8());
}

void Client::bytesWritten(qint64 bytes)
{
    qDebug() << this << "BytesWritten Called " << bytes;
}

void Client::stateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << this << "stateChanged" << m_socket << socketState;
}

void Client::error(QAbstractSocket::SocketError socketError)
{
    qDebug() << this << "error" << m_socket << socketError;
}