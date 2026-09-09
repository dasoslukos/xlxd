//
//  cudpsocket.cpp
//  xlxd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//
// ----------------------------------------------------------------------------
//    This file is part of xlxd.
//
//    xlxd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    xlxd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>. 
// ----------------------------------------------------------------------------

#include "main.h"
#include <string.h>
#include "cudpsocket.h"


////////////////////////////////////////////////////////////////////////////////////////
// constructor

CUdpSocket::CUdpSocket()
{
#ifdef _WIN32
    m_Socket = INVALID_SOCKET;
#else
    m_Socket = -1;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////
// destructor

CUdpSocket::~CUdpSocket()
{
#ifdef _WIN32
    if (m_Socket != INVALID_SOCKET)
#else
    if (m_Socket != -1)
#endif
    {
        Close();
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// open & close

bool CUdpSocket::Open(const CIp &ListenIp, uint16 uiPort)
{
    bool open = false;

    // create socket
    m_Socket = ::socket(PF_INET, SOCK_DGRAM, 0);

#ifdef _WIN32
    if (m_Socket != INVALID_SOCKET)
#else
    if (m_Socket != -1)
#endif
    {
        // initialize sockaddr struct
        ::memset(&m_SocketAddr, 0, sizeof(struct sockaddr_in));
        m_SocketAddr.sin_family = AF_INET;
        m_SocketAddr.sin_port = htons(uiPort);
        m_SocketAddr.sin_addr.s_addr = inet_addr(ListenIp);

        if (::bind(m_Socket,
                   (struct sockaddr *)&m_SocketAddr,
                   sizeof(struct sockaddr_in)) == 0)
        {
#ifdef _WIN32
            u_long nonBlocking = 1;

            if (::ioctlsocket(m_Socket, FIONBIO, &nonBlocking) == 0)
            {
                open = true;
            }
            else
            {
                ::closesocket(m_Socket);
                m_Socket = INVALID_SOCKET;
            }
#else
            if (::fcntl(m_Socket, F_SETFL, O_NONBLOCK) != -1)
            {
                open = true;
            }
            else
            {
                ::close(m_Socket);
                m_Socket = -1;
            }
#endif
        }
        else
        {
#ifdef _WIN32
            ::closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
#else
            ::close(m_Socket);
            m_Socket = -1;
#endif
        }
    }

    // done
    return open;
}

void CUdpSocket::Close(void)
{
#ifdef _WIN32
    if (m_Socket != INVALID_SOCKET)
    {
        ::closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }
#else
    if (m_Socket != -1)
    {
        ::close(m_Socket);
        m_Socket = -1;
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////
// read

int CUdpSocket::Receive(CBuffer *Buffer, CIp *Ip, int timeout)
{
    struct sockaddr_in Sin;
    fd_set FdSet;
    int iRecvLen = -1;
    struct timeval tv;

#ifdef _WIN32
    int uiFromLen = sizeof(struct sockaddr_in);
#else
    socklen_t uiFromLen = sizeof(struct sockaddr_in);
#endif

    // socket valid ?
#ifdef _WIN32
    if (m_Socket != INVALID_SOCKET)
#else
    if (m_Socket != -1)
#endif
    {
        // control socket
        FD_ZERO(&FdSet);
        FD_SET(m_Socket, &FdSet);

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

#ifdef _WIN32
        int selectResult = ::select(0, &FdSet, nullptr, nullptr, &tv);
#else
        int selectResult = ::select(m_Socket + 1, &FdSet, nullptr, nullptr, &tv);
#endif

        if (selectResult > 0 && FD_ISSET(m_Socket, &FdSet))
        {
            // allocate buffer
            Buffer->resize(UDP_BUFFER_LENMAX);

            // read
            iRecvLen = (int)::recvfrom(
                m_Socket,
#ifdef _WIN32
                reinterpret_cast<char *>(Buffer->data()),
#else
                reinterpret_cast<void *>(Buffer->data()),
#endif
                UDP_BUFFER_LENMAX,
                0,
                (struct sockaddr *)&Sin,
                &uiFromLen);

            // handle
            if (iRecvLen != -1)
            {
                // adjust buffer size
                Buffer->resize(iRecvLen);

                // get IP
                Ip->SetSockAddr(&Sin);
            }
        }
    }

    // done
    return iRecvLen;
}

////////////////////////////////////////////////////////////////////////////////////////
// write

int CUdpSocket::Send(const CBuffer &Buffer, const CIp &Ip)
{
    CIp temp(Ip);

    return (int)::sendto(
        m_Socket,
#ifdef _WIN32
        reinterpret_cast<const char *>(Buffer.data()),
#else
        reinterpret_cast<const void *>(Buffer.data()),
#endif
        (int)Buffer.size(),
        0,
        (struct sockaddr *)temp.GetSockAddr(),
        sizeof(struct sockaddr_in));
}

int CUdpSocket::Send(const char *Buffer, const CIp &Ip)
{
    CIp temp(Ip);

    return (int)::sendto(
        m_Socket,
        Buffer,
        (int)::strlen(Buffer),
        0,
        (struct sockaddr *)temp.GetSockAddr(),
        sizeof(struct sockaddr_in));
}

int CUdpSocket::Send(const CBuffer &Buffer, const CIp &Ip, uint16 destport)
{
    CIp temp(Ip);
    temp.GetSockAddr()->sin_port = htons(destport);

    return (int)::sendto(
        m_Socket,
#ifdef _WIN32
        reinterpret_cast<const char *>(Buffer.data()),
#else
        reinterpret_cast<const void *>(Buffer.data()),
#endif
        (int)Buffer.size(),
        0,
        (struct sockaddr *)temp.GetSockAddr(),
        sizeof(struct sockaddr_in));
}

int CUdpSocket::Send(const char *Buffer, const CIp &Ip, uint16 destport)
{
    CIp temp(Ip);
    temp.GetSockAddr()->sin_port = htons(destport);

    return (int)::sendto(
        m_Socket,
        Buffer,
        (int)::strlen(Buffer),
        0,
        (struct sockaddr *)temp.GetSockAddr(),
        sizeof(struct sockaddr_in));
}
