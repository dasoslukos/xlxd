//
//  cip.cpp
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
#include "cip.h"

#ifndef _WIN32
#include <netdb.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////
// constructors

CIp::CIp()
{
    ::memset(&m_Addr, 0, sizeof(m_Addr));
    m_Addr.sin_family = AF_INET;
}

CIp::CIp(const char *sz)
{
    ::memset(&m_Addr, 0, sizeof(m_Addr));
    m_Addr.sin_family = AF_INET;

    // Try a numeric IPv4 address first.
    if (::inet_pton(AF_INET, sz, &m_Addr.sin_addr) != 1)
    {
        // Otherwise resolve the name via DNS.
        struct addrinfo hints;
        ::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        struct addrinfo *result = nullptr;

        if (::getaddrinfo(sz, nullptr, &hints, &result) == 0)
        {
            if (result != nullptr && result->ai_addr != nullptr)
            {
                const struct sockaddr_in *resolved =
                    reinterpret_cast<const struct sockaddr_in *>(result->ai_addr);

                m_Addr.sin_addr = resolved->sin_addr;
            }

            ::freeaddrinfo(result);
        }
    }
}

CIp::CIp(const struct sockaddr_in *sa)
{
    ::memcpy(&m_Addr, sa, sizeof(m_Addr));
}


CIp::CIp(const CIp &ip)
{
    ::memcpy(&m_Addr, &ip.m_Addr, sizeof(m_Addr));
}

////////////////////////////////////////////////////////////////////////////////////////
// set

void CIp::SetSockAddr(struct sockaddr_in *sa)
{
    ::memcpy(&m_Addr, sa, sizeof(m_Addr));
}

////////////////////////////////////////////////////////////////////////////////////////
// operator

bool CIp::operator ==(const CIp &ip) const
{
    return ( (ip.m_Addr.sin_family == m_Addr.sin_family) &&
             (ip.m_Addr.sin_addr.s_addr == m_Addr.sin_addr.s_addr) &&
             (ip.m_Addr.sin_port == m_Addr.sin_port)) ;
}

CIp::operator const char *() const
{
    static thread_local char szAddress[INET_ADDRSTRLEN];

    if (::inet_ntop(AF_INET, &m_Addr.sin_addr, szAddress, sizeof(szAddress)) == nullptr)
    {
        szAddress[0] = '\0';
    }

    return szAddress;
}
