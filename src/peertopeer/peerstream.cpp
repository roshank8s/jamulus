#include "peertopeer/peerstream.h"

PeerStream::PeerStream(const CHostAddress& addr)
    : Address(addr), Gain(1.0f), Mute(false), Pan(0.5f)
{
}
