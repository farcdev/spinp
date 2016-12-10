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


#include <curl/curl.h>
#include <string.h>
#include "common.h"

namespace
{

class CUrl
{
public:
    static CUrl& getInstance();
    CURL* getCURL();

private:
    CUrl();
    ~CUrl();

    CURL* m_pCurl;

};

CUrl& CUrl::getInstance()
{
    static CUrl s_instance;

    return s_instance;
}

CURL* CUrl::getCURL()
{
    return m_pCurl;
}

CUrl::CUrl()
    : m_pCurl(nullptr)
{
    m_pCurl = curl_easy_init();
}

CUrl::~CUrl()
{
    if (m_pCurl != nullptr) curl_easy_cleanup(m_pCurl);
}

}

void Spin::replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

//                     ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ? @ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \ ] ^ _ ` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~
// %0E %0F %10 %11 %12 %13 %14 %15 %16 %17 %18 %19 %1A %1B %1C %1D %1E %1F %20 %21 %22 %23 %24 %25 %26 %27 %28 %29 %2A %2B %2C %2D %2E %2F 0 1 2 3 4 5 6 7 8 9 %3A %3B %3C %3D
// %3E %3F %40 A B C D E F G H I J K L M N O P Q R S T U V W X Y Z %5B %5C %5D %5E %5F %60 a b c d e f g h i j k l m n o p q r s t u v w x
// y z %7B %7C %7D %7E %7F

std::string Spin::urlEncode(const std::string& _rString)
{
    CURL* pCurl = CUrl::getInstance().getCURL();

    char* result = curl_easy_escape(pCurl, _rString.c_str(), static_cast<int>(_rString.size()));

    if (result == NULL) return _rString;

    std::string str = std::string(result);

    curl_free(result);

    replaceAll(str, "-", "%2D");
    replaceAll(str, "_", "%5F");
    replaceAll(str, ".", "%2E");
    replaceAll(str, "~", "%7E");

    return str;
}

std::string Spin::urlDecode(const std::string& _rString)
{
    CURL* pCurl = CUrl::getInstance().getCURL();
    int outLength;

    char* result = curl_easy_unescape(pCurl, _rString.c_str(), static_cast<int>(_rString.size()), &outLength);

    std::string str(result);

    curl_free(result);

    return str;
}

std::string Spin::hexEncode(const std::string& _rString)
{
    static const char s_hexLetters[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    size_t stringLength = _rString.length();
    std::string result(stringLength << 1, ' ');

    size_t targetIndex = 0;
    for (size_t index = 0; index < stringLength; ++ index)
    {
        char currentChar = _rString.at(index);

        result.at(targetIndex ++) = s_hexLetters[(currentChar & static_cast<char>(0xF0)) >> 4];
        result.at(targetIndex ++) = s_hexLetters[ currentChar & static_cast<char>(0x0F)      ];
    }

    return result;
}

//size_t Spin::stdStringFind(const std::string& _rString, const std::string& _rSearch, size_t _offset)
//{
//    return stdStringFind(_rString, _rSearch.c_str(), _offset);
//}
//
//size_t Spin::stdStringFind(const std::string& _rString, const char* _pSearch, size_t _offset)
//{
//    if (*_pSearch == '\0') return std::string::npos;
//
//    auto currentIt = _rString.cbegin();
//    currentIt += _offset;
//    auto endIt = _rString.cend();
//
//    const char* pSearch = _pSearch;
//
//    for (currentIt; currentIt != endIt; ++ currentIt)
//    {
//        if (*currentIt == *pSearch)
//        {
//            auto possibleResult = currentIt;
//
//            for (;;)
//            {
//                if (pSearch == '\0')
//                {
//                    return static_cast<size_t>(currentIt - possibleResult) + _offset;
//                }
//
//                const char currentChar = *pSearch;
//
//                if (currentChar == '%')
//                {
//
//                }
//
//                if (*currentIt != currentChar)
//                {
//                    pSearch = _pSearch;
//                    break;
//                }
//
//                ++ pSearch;
//                ++ possibleResult;
//            }
//        }
//
//    }
//
//}