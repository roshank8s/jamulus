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
}

void CP2PManager::RemovePeers()
{
    qDeleteAll ( Peers );
    Peers.clear();
}

void CP2PManager::SendAudioToPeers ( const CVector<uint8_t>& data )
{
    for ( auto* peer : Peers )
    {
        peer->SendAudioData ( data );
    }
}
