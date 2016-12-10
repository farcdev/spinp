/**
 * common.cpp
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


#ifndef SPINTEST_COMMON_H
#define SPINTEST_COMMON_H

#include <string>


namespace Spin
{

constexpr int ceStringLength(const char* _pString, int _length = 0)
{
    return _pString[_length] == '\0' ? _length : ceStringLength(_pString, ++ _length);
}

void replaceAll(std::string& str, const std::string& from, const std::string& to);

std::string urlEncode(const std::string& _rString);

std::string urlDecode(const std::string& _rString);

std::string hexEncode(const std::string& _rString);
//size_t stdStringFind(const std::string& _rString, const std::string& _rSearch, size_t _offset = 0);
//size_t stdStringFind(const std::string& _rString, const char* _pSearch, size_t _offset = 0);


}

#endif // SPINTEST_COMMON_H
