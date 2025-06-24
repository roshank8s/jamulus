#include "peertopeer/peerconnection.h"

CPeerConnection::CPeerConnection ( QObject* parent ) : QObject ( parent ), PeerPort ( 0 )
{
    QObject::connect ( &Socket, &QUdpSocket::readyRead, this, &CPeerConnection::OnReadyRead );
    RecvBuffer.Init ( MAX_SIZE_BYTES_NETW_BUF );
    Socket.bind ( QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint );
}

void CPeerConnection::SetPeerAddress ( const QHostAddress& addr, quint16 port )
{
    PeerAddress = addr;
    PeerPort    = port;
}

void CPeerConnection::SendAudioData ( const CVector<uint8_t>& data )
{
    if ( !PeerAddress.isNull() && ( PeerPort != 0 ) && ( data.Size() > 0 ) )
    {
        Socket.writeDatagram ( reinterpret_cast<const char*> ( &data[0] ), data.Size(), PeerAddress, PeerPort );
    }
}

void CPeerConnection::OnReadyRead()
{
    while ( Socket.hasPendingDatagrams() )
    {
        QHostAddress sender;
        quint16      senderPort = 0;
        const qint64 len = Socket.readDatagram ( reinterpret_cast<char*> ( &RecvBuffer[0] ),
                                                MAX_SIZE_BYTES_NETW_BUF,
                                                &sender,
                                                &senderPort );
        if ( len > 0 )
        {
            CVector<uint8_t> data ( static_cast<int> ( len ) );
            std::copy_n ( RecvBuffer.begin(), len, data.begin() );
            emit AudioPacketReceived ( CHostAddress ( sender, senderPort ), data );
        }
    }
}
