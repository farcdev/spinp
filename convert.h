/**
 * convert.h
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


#ifndef SPINTEST_CONVERT_H
#define SPINTEST_CONVERT_H

#include <string>

namespace Spin
{

std::string convertCP1252toUTF8(const std::string& _rString);
const char* convertCP1252toUTF8(const char* _pString);

std::string convertISO8859_15toUTF8(const std::string& _rString);
const char* convertISO8859_15toUTF8(const char* _pString);

std::string convertUTF8toCP1252(const std::string& _rString);
const char* convertUTF8toCP1252(const char* _pString);

std::string convertUTF8toISO8859_15(const std::string& _rString);
const char* convertUTF8toISO8859_15(const char* _pString);

std::string convertASCIIWithEscapedUTF16toUTF8(const std::string& _rString);
const char* convertASCIIWithEscapedUTF16toUTF8(const char* _pString);

void initConvert();
void uninitConvert();

}

#endif // SPINTEST_CONVERT_H
