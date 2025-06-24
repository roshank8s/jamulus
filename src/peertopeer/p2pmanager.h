/******************************************************************************\
 * Copyright (c) 2025
 *
 * Author(s):
 *  Jamulus Contributors
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 \******************************************************************************/

#pragma once

#include <QObject>
#include <QList>
#include <QHostAddress>
#include "peertopeer/peerconnection.h"

class CP2PManager : public QObject
{
    Q_OBJECT

public:
    explicit CP2PManager ( QObject* parent = nullptr );
    ~CP2PManager();

    void AddPeer ( const QHostAddress& addr, quint16 port );
    void RemovePeers();
    void SendAudioToPeers ( const CVector<uint8_t>& data );

signals:
    void PeerAudioReceived ( const CVector<uint8_t>& data );

private:
    QList<CPeerConnection*> Peers;
};
