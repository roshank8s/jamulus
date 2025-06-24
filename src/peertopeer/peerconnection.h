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
#include <QUdpSocket>
#include <QHostAddress>
#include "buffer.h"
#include "util.h"

class CPeerConnection : public QObject
{
    Q_OBJECT

public:
    explicit CPeerConnection ( QObject* parent = nullptr );

    void SetPeerAddress ( const QHostAddress& addr, quint16 port );
    void SendAudioData ( const CVector<uint8_t>& data );

signals:
    void AudioPacketReceived ( const CHostAddress& addr, const CVector<uint8_t>& data );

private slots:
    void OnReadyRead();

private:
    QUdpSocket       Socket;
    QHostAddress     PeerAddress;
    quint16          PeerPort;
    CVector<uint8_t> RecvBuffer;
};
