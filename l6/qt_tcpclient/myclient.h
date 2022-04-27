#ifndef MYCLIENT_H
#define MYCLIENT_H

#include <QWidget>
//#include <QTcpSocket>

#include <QSslSocket>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QLineEdit;
QT_END_NAMESPACE

class MyClient : public QWidget
{
    Q_OBJECT

private:
    QSslSocket *m_pTcpSocket = nullptr;
    QTextEdit  *m_ptxtInfo = nullptr;
    QLineEdit  *m_ptxtInput = nullptr;
    quint16    m_nNextBlockSize;
    qint32     m_blockSize;
    bool read_File();


public:
    MyClient(const QString& strHost,int nPort,QWidget* pwgt = 0);

private slots:
    void slotReadyRead();
    void slotError(QAbstractSocket::SocketError);
    void slotSendToServer();
    void slotConnected();

};

#endif
