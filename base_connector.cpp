/**
 * base_connector.cpp
 * Copyright (C) 2016 Farci <farci@rehpost.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "base_connector.h"
#include "connector.h"
#include "logger.h"

#include <assert.h>


namespace Spin
{

CBaseConnector::CBaseConnector()
    : m_pConnector(nullptr)
{
}

CBaseConnector::~CBaseConnector()
{
    Log::info("sys") << "~CBaseConnector 1" << Log::end;

//    std::this_thread::sleep_for(std::chrono::seconds(10));

    if (m_pConnector != nullptr) delete m_pConnector;
    Log::info("sys") << "~CBaseConnector 2" << Log::end;
}

bool CBaseConnector::start(const char* _pUsername, const char* _pPassword, unsigned short _port)
{
    assert(_pUsername != nullptr);
    assert(_pPassword != nullptr);
    assert(_port      != 0      );

    m_pConnector = new CConnector(this);
    m_pConnector->setLogin(_pUsername, _pPassword, _port);
    return m_pConnector->start();
}

bool CBaseConnector::stop()
{
    m_pConnector->setShutdown(true);
    return true;
}

CConnector* CBaseConnector::getConnector()
{
    assert(m_pConnector != nullptr);
    return m_pConnector;
}



}