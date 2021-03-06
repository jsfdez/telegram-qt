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

#ifndef CTCPTRANSPORT_HPP
#define CTCPTRANSPORT_HPP

#include "CTelegramTransport.hpp"

QT_BEGIN_NAMESPACE
class QTcpSocket;
class QTimer;
QT_END_NAMESPACE

class CTcpTransport : public CTelegramTransport
{
    Q_OBJECT
public:
    explicit CTcpTransport(QObject *parent = 0);
    ~CTcpTransport();

    void connectToHost(const QString &ipAddress, quint32 port);
    void disconnectFromHost();

    bool isConnected() const;

    QByteArray getPackage() { return m_receivedPackage; }

    // Method for testing
    QByteArray lastPackage() const { return m_lastPackage; }

public slots:
    void sendPackage(const QByteArray &payload);

private slots:
    void whenStateChanged(QAbstractSocket::SocketState newState);
    void whenError(QAbstractSocket::SocketError error);
    void whenReadyRead();
    void whenTimeout();

private:
    quint32 m_packetNumber;
    quint32 m_expectedLength;

    QByteArray m_receivedPackage;
    QByteArray m_lastPackage;

    QTcpSocket *m_socket;
    QTimer *m_timeoutTimer;

    bool m_firstPackage;

};

#endif // CTCPTRANSPORT_HPP
