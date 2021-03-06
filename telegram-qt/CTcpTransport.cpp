/*
    Copyright (C) 2014-2015 Alexandr Akulich <akulichalexander@gmail.com>

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "CTcpTransport.hpp"

#include <QTcpSocket>
#include <QTimer>

#include <QDebug>

static const quint32 tcpTimeout = 15 * 1000;

CTcpTransport::CTcpTransport(QObject *parent) :
    CTelegramTransport(parent),
    m_socket(new QTcpSocket(this)),
    m_timeoutTimer(new QTimer(this)),
    m_firstPackage(true)
{
    connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(whenStateChanged(QAbstractSocket::SocketState)));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(whenError(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(readyRead()), SLOT(whenReadyRead()));

    m_timeoutTimer->setInterval(tcpTimeout);
    connect(m_timeoutTimer, SIGNAL(timeout()), SLOT(whenTimeout()));
}

CTcpTransport::~CTcpTransport()
{
    if (m_socket->isWritable()) {
        m_socket->waitForBytesWritten(100);
        m_socket->disconnectFromHost();
    }
}

void CTcpTransport::connectToHost(const QString &ipAddress, quint32 port)
{
#ifdef DEVELOPER_BUILD
    qDebug() << Q_FUNC_INFO << ipAddress << port;
#endif
    m_socket->connectToHost(ipAddress, port);
}

void CTcpTransport::disconnectFromHost()
{
#ifdef DEVELOPER_BUILD
    qDebug() << Q_FUNC_INFO;
#endif
    m_socket->disconnectFromHost();
}

bool CTcpTransport::isConnected() const
{
    return m_socket && (m_socket->state() == QAbstractSocket::ConnectedState);
}

void CTcpTransport::sendPackage(const QByteArray &payload)
{
    // quint32 length (included length itself + packet number + crc32 + payload // Length MUST be divisible by 4
    // quint32 packet number
    // quint32 CRC32 (length, quint32 packet number, payload)
    // Payload

    // Abridged version:
    // quint8: 0xef
    // DataLength / 4 < 0x7f ?
    //      (quint8: Packet length / 4) :
    //      (quint8: 0x7f, quint24: Packet length / 4)
    // Payload

    QByteArray package;

    if (m_firstPackage) {
        package.append(char(0xef)); // Start session in Abridged format
        m_firstPackage = false;
    }

    quint32 length = 0;
    length += payload.length();

    package.append(char(length / 4));
    package.append(payload);

    m_lastPackage = package;

    m_socket->write(package);
}

void CTcpTransport::whenStateChanged(QAbstractSocket::SocketState newState)
{
//    qDebug() << Q_FUNC_INFO << newState;
    switch (newState) {
    case QAbstractSocket::ConnectedState:
        m_expectedLength = 0;
        m_firstPackage = true;
        break;
    default:
        break;
    }

    switch (newState) {
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::HostLookupState:
        m_timeoutTimer->start();
        break;
    default:
        m_timeoutTimer->stop();
        break;
    }

    setState(newState);
}

void CTcpTransport::whenError(QAbstractSocket::SocketError error)
{
//    qDebug() << Q_FUNC_INFO << error << m_socket->errorString();
    setError(error);
}

void CTcpTransport::whenReadyRead()
{
    while (m_socket->bytesAvailable() > 0) {
        if (m_expectedLength == 0) {
            if (m_socket->bytesAvailable() < 4) {
                // Four bytes is minimum readable size for new package
                return;
            }

            char length;
            m_socket->getChar(&length);

            if (length < char(0x7f)) {
                m_expectedLength = length * 4;
            } else if (length == char(0x7f)) {
                m_socket->read((char *) &m_expectedLength, 3);
                m_expectedLength *= 4;
            } else {
                qDebug() << "Incorrect TCP package!";
            }
        }

        if (m_socket->bytesAvailable() < m_expectedLength)
            return;

        m_receivedPackage = m_socket->read(m_expectedLength);

        m_expectedLength = 0;

        emit readyRead();
    }
}

void CTcpTransport::whenTimeout()
{
#ifdef DEVELOPER_BUILD
    qDebug() << Q_FUNC_INFO << "(connection to " << m_socket->peerName() << m_socket->peerPort() << ").";
#endif
    m_socket->disconnectFromHost();
}
