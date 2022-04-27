#include <QMessageBox>
#include <QTcpServer>
#include <QTextEdit>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QLabel>

#include <QFile>
#include <QDir>

#include <QSslSocket>

#include "myserver.h"

class QTcpServer;

MyServer::MyServer(int nPort, QWidget* pwgt): QWidget(pwgt),
                                              m_nNextBlockSize(0)
{
    m_ptcpServer = new SslServer(this);
    m_ptcpServer->setSslLocalCertificate("sslserver.pem");
    m_ptcpServer->setSslPrivateKey("sslserver.key");
    m_ptcpServer->setSslProtocol(QSsl::TlsV1_2);

    if (!m_ptcpServer->listen(QHostAddress::Any, nPort))
    {
        QMessageBox::critical(0,
                              "Server Error",
                              "Unable to start the server:"
                              + m_ptcpServer->errorString()
                              );
        m_ptcpServer->close();
        return;
    }
    connect(m_ptcpServer, SIGNAL(newConnection()),
            this, SLOT(slotNewConnection())
            );
    m_ptxt = new QTextEdit;
    m_ptxt->setReadOnly(true);

    QVBoxLayout* pvbxLayout = new QVBoxLayout;
    pvbxLayout->addWidget(new QLabel("<H1>Server</H1>"));
    pvbxLayout->addWidget(m_ptxt);
    setLayout(pvbxLayout);

    m_ptxt->append("Running echo server...");
    m_ptxt->append("Command:");
    m_ptxt->append(" exit - to quit");
    m_ptxt->append(" get <filename> <beans/size> <number>");
}

void MyServer::disconnect()
{
    QSslSocket* pClientSocket = (QSslSocket*) sender();
    pClientSocket->disconnectFromHost();
    //pClientSocket->waitForDisconnected();
}

void MyServer::slotNewConnection()
{
    QSslSocket* pClientSocket = dynamic_cast<QSslSocket*>(m_ptcpServer->nextPendingConnection());
    connect(pClientSocket, SIGNAL(disconnected()),
            this, SLOT(disconnect())
            );
    connect(pClientSocket, SIGNAL(readyRead()),
            this, SLOT(slotReadClient())
            );
    sendToClient(pClientSocket, "Server Response: Connected!");
}

void MyServer::slotReadClient()
{
    QSslSocket* pClientSocket = (QSslSocket*) sender();
    QDataStream in(pClientSocket);
    in.setVersion(QDataStream::Qt_5_3);
    for (;;)
    {
        if (!m_nNextBlockSize)
        {
            if (pClientSocket->bytesAvailable() < sizeof(quint16))
            {
                break;
            }
            in >> m_nNextBlockSize;
        }
        if (pClientSocket->bytesAvailable() < m_nNextBlockSize)
        {
            break;
        }
        QString str;
        in >> str;

        QString strMessage = /*time.toString() + " " + */
                "Client has sent - " + str;        
        m_ptxt->append(strMessage);
        m_nNextBlockSize = 0;
        sendToClient(pClientSocket,
                     "Server Response: Received \"" +
                     str +
                     "\""
                     );

        // Command analyse
        QTextStream iss(&str,QIODevice::ReadOnly);
        QString command = "\0";
        QString filename = "\0";
        QString fiction = "\0";
        size_t fict = 0;
        size_t size_fict = 0;
        iss >> command >> filename >> fiction >> size_fict;

        if ("exit" == command)
        {
            m_ptxt->append("Breaking");
            sendToClient(pClientSocket,command);
            this->close();


        }

        if ("get" == command && sizeof(filename) > 0)
        {
          if ("beans" == fiction)
           {
             fict = 1;
           }
           else if ("size" == fiction)
           {
             fict = 2;
           }

           sendToClient(pClientSocket,command);
           sendToFile(pClientSocket,filename,fict,size_fict);

        }
    }
}

void MyServer::sendToClient(QSslSocket *pSocket, const QString &str)
{
    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << quint16(0)/* << QTime::currentTime()*/ << str;
    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));
    pSocket->write(arrBlock);
}


bool MyServer::sendToFile(QSslSocket *pSocket, const QString &filename,
                          const size_t &fict, const size_t &size_fict)
{
    if (!filename.size())
    {
        return false;
    }
    QString path = QDir::current().path()+"/"+filename;
    QFile appFile(path);
    appFile.open(QFile::ReadOnly);
    if (!appFile.exists() && !appFile.isOpen())
    {
        return false;
    }

    size_t size_fict_=size_fict;
    size_t count = appFile.size();

    if (size_fict_ > count)
        size_fict_ = count;

    if (1 == fict) {
        appFile.seek(size_fict_);
        count=count-size_fict_;
    } else if (2 == fict) {
        count = size_fict_;
    }

    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << quint32(0) << appFile.fileName();

    QByteArray q = appFile.read(count);
    arrBlock.append(q);
    appFile.close();

    out.device()->seek(0);
    out << quint32(arrBlock.size() - sizeof(quint32));

    qint64 x=0;
    while (x < arrBlock.size())
    {
        qint64 y = pSocket->write(arrBlock);
        x += y;
    }
    return true;
}
