/**
 * cookie.h
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


#ifndef SPINTEST_SESSION_H
#define SPINTEST_SESSION_H

#include <string>
#include <map>

namespace Spin
{


class CCookie
{
public:
    CCookie();
    ~CCookie();

public:
    void addCookie(const std::string& _rKey, const std::string& _rValue);
    void addCookie(const std::string& _rKey, std::string&& _rrValue);
    void addCookie(std::string&& _rrKey, std::string&& _rrValue);

    std::string getCookie(const std::string& _rKey);

    const std::string& getCookieString();

private:
    bool                               m_changed;
    std::map<std::string, std::string> m_cookies;
    std::string                        m_cookie;

};

}

#endif // SPINTEST_SESSION_H
