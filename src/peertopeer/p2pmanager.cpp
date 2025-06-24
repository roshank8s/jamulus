#include "peertopeer/p2pmanager.h"
#include "util.h"

CP2PManager::CP2PManager ( QObject* parent ) : QObject ( parent ) {}

CP2PManager::~CP2PManager()
{
    RemovePeers();
}

void CP2PManager::AddPeer ( const QHostAddress& addr, quint16 port )
{
    auto* peer = new CPeerConnection ( this );
    peer->SetPeerAddress ( addr, port );
    QObject::connect ( peer,
                      &CPeerConnection::AudioPacketReceived,
                      this,
                      &CP2PManager::PeerAudioReceived );
    Peers.append ( peer );
    qInfo() << qUtf8Printable ( QString ( "> Added peer %1:%2" ).arg ( addr.toString() ).arg ( port ) );
}

void CP2PManager::RemovePeers()
{
    qDeleteAll ( Peers );
    Peers.clear();
    qInfo() << "> Removed all peers";
}

void CP2PManager::SendAudioToPeers ( const CVector<uint8_t>& data )
{
    for ( auto* peer : Peers )
    {
        peer->SendAudioData ( data );
    }
}
