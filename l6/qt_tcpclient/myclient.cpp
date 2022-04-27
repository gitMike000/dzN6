#include <iostream>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QDir>

#include "myclient.h"

MyClient::MyClient(const QString& strHost,int nPort, QWidget* pwgt): QWidget(pwgt),
                    m_nNextBlockSize(0), m_blockSize(0)
{
    m_pTcpSocket = new QSslSocket(this);
    m_pTcpSocket->addCaCertificates("sslserver.pem");
    m_pTcpSocket->connectToHostEncrypted(strHost, nPort);
    connect(m_pTcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(m_pTcpSocket, SIGNAL(readyRead()), SLOT(slotReadyRead()));
    connect(m_pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(slotError(QAbstractSocket::SocketError))
            );
    m_ptxtInfo = new QTextEdit;
    m_ptxtInput = new QLineEdit;
    m_ptxtInfo->setReadOnly(true);
    QPushButton* pcmd = new QPushButton("&Send");
    connect(pcmd, SIGNAL(clicked()), SLOT(slotSendToServer()));
    connect(m_ptxtInput, SIGNAL(returnPressed()),
            this, SLOT(slotSendToServer())
            );
    QVBoxLayout* pvbxLayout = new QVBoxLayout;
    pvbxLayout->addWidget(new QLabel("<H1>Client</H1"));
    pvbxLayout->addWidget(m_ptxtInfo);
    pvbxLayout->addWidget(m_ptxtInput);
    pvbxLayout->addWidget(pcmd);
    setLayout(pvbxLayout);

    m_ptxtInfo->append("Running echo server...");
    m_ptxtInfo->append("Command:");
    m_ptxtInfo->append(" exit - to quit");
    m_ptxtInfo->append(" get <filename> <beans/size> <number>");
    m_ptxtInfo->append(" ");
}

bool MyClient::read_File()
{
    QDataStream in(m_pTcpSocket);
    in.setVersion(QDataStream::Qt_5_3);

    if (m_blockSize == 0)
    {
        if (m_pTcpSocket->bytesAvailable() < sizeof(quint32))
            return false;
        in >> m_blockSize;
    }
    if (m_pTcpSocket->bytesAvailable() < m_blockSize)
        return false;

    m_blockSize = 0;
    QString fileName;
    in >> fileName;
    QByteArray line = m_pTcpSocket->readAll();

    QString filePath = QDir::current().path();
    fileName = fileName.section("/", -1);
    QFile target(filePath + "/" + fileName);

    if (!target.open(QIODevice::WriteOnly))
    {
        return false;
    }
    target.write(line);
    target.close();
    return true;
}

void MyClient::slotReadyRead()
{
    QDataStream in(m_pTcpSocket);
    in.setVersion(QDataStream::Qt_5_3);
    for (;;)
    {
        if (!m_nNextBlockSize)
        {
            if (m_pTcpSocket->bytesAvailable() < sizeof(quint16))
            {
                break;
            }
            in >> m_nNextBlockSize;
        }

        if (m_pTcpSocket->bytesAvailable() < m_nNextBlockSize)
        {
            break;
        }
        QString str;
        in >> str;
        m_ptxtInfo->append(str);
        m_nNextBlockSize = 0;

        if ("exit" == str)
        {
            m_ptxtInfo->append("Breaking");
            this->close();
        }
        if ("get" == str)
        {
            if (!read_File())
            {
                m_ptxtInfo->append("File can't write");
            }
            else
            {
                m_ptxtInfo->append("File write");
            }
        }
    }
}

void MyClient::slotError(QAbstractSocket::SocketError err)
{
    QString strError =
       "Error:" + (err == QAbstractSocket::HostNotFoundError ?
                       "The host was not found." :
                       err == QAbstractSocket::RemoteHostClosedError ?
                           "The remote host is closed." :
                           err == QAbstractSocket::ConnectionRefusedError ?
                               "The connection was refused." :
                               QString(m_pTcpSocket->errorString())
                   );
    m_ptxtInfo->append(strError);
}

void MyClient::slotSendToServer()
{
    QByteArray arrBlock;
    QDataStream out(&arrBlock,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << quint16(0) << m_ptxtInput->text();
    out.device()->seek(0);
    out << quint16(arrBlock.size()-sizeof(quint16));
    m_pTcpSocket->write(arrBlock);
    m_ptxtInput->setText("");
}

void MyClient::slotConnected()
{
    m_ptxtInfo->append("Received the connected() signal");
}

