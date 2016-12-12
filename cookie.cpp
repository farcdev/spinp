/**
 * cookie.cpp
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


#include "cookie.h"

namespace Spin
{

CCookie::CCookie()
    : m_changed(true)
    , m_cookies()
    , m_cookie ()
{
}

CCookie::~CCookie()
{
}

void CCookie::addCookie(const std::string& _rKey, const std::string& _rValue)
{
    m_cookies.insert(std::pair<std::string, std::string>(_rKey, _rValue));
    m_changed = true;
}

void CCookie::addCookie(const std::string& _rKey, std::string&& _rrValue)
{
    m_cookies.insert(std::pair<std::string, std::string>(_rKey, _rrValue));
    m_changed = true;
}

void CCookie::addCookie(std::string&& _rrKey, std::string&& _rrValue)
{
    m_cookies.emplace(_rrKey, _rrValue);
    m_changed = true;
}

std::string CCookie::getCookie(const std::string& _rKey) const
{
    auto cookieIt = m_cookies.find(_rKey);

    if (cookieIt == m_cookies.cend())
    {
        return "";
    }

    return cookieIt->second;
}

const std::string& CCookie::getCookieString()
{
    if (m_changed)
    {
        m_cookie.clear();
        
        auto endOfCookies = m_cookies.cend();
        for (auto currentCookie = m_cookies.cbegin(); currentCookie != endOfCookies; ++ currentCookie)
        {
            m_cookie += currentCookie->first + "=" + currentCookie->second + "; ";
        }
        m_changed = false;
    }

    return m_cookie;
}



}