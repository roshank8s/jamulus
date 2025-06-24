#ifndef PEERSTREAM_H
#define PEERSTREAM_H

#include "buffer.h"
#include "util.h"

class PeerStream
{
public:
    explicit PeerStream(const CHostAddress& addr);

    CHostAddress     Address;
    CNetBufWithStats JitterBuffer;
    float            Gain;
    bool             Mute;
    float            Pan;
};

#endif // PEERSTREAM_H
