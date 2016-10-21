/**
 * convert.cpp
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


#include "convert.h"
#include "logger.h"

#ifdef __MINGW32__
#include "extern/convert/convert.h"
#endif

#include <errno.h>
#include <iconv.h>
#include <string.h>
#include <iostream>

namespace
{

class CConvert
{
public:
    CConvert(const char* _sourceSet, const char* _targetSet);
    ~CConvert();

    CConvert(const CConvert&) = delete;
    CConvert& operator= (const CConvert&) = delete;

    const char* convert(const char* _pString);
    const char* convert(const char* _pString, size_t _length);

private:
    iconv_t m_pConverter;
    char*   m_pSource;
    char*   m_pTarget;
    size_t  m_sourceLength;
    size_t  m_targetLength;

};

CConvert::CConvert(const char* _sourceSet, const char* _targetSet)
    : m_pConverter(nullptr)
    , m_pSource(new char[64])
    , m_pTarget(&m_pSource[32])
    , m_sourceLength(32)
    , m_targetLength(32)
{
    memset(m_pSource, 0, m_sourceLength + m_targetLength);
    m_pConverter = iconv_open(_targetSet, _sourceSet);
}

CConvert::~CConvert()
{
}

const char* CConvert::convert(const char* _pString)
{
    return convert(_pString, strlen(_pString));
}

const char* CConvert::convert(const char* _pString, size_t _length)
{
    size_t sourceLength;
    size_t targetLength;

    size_t realLength;

    if (_length + 1 > m_sourceLength)
    {
        m_sourceLength = _length + 1;

        delete m_pSource;

        m_targetLength = static_cast<size_t>(m_sourceLength * 1.2f);

        m_pSource = new char[m_sourceLength + m_targetLength + 4];
        memset(m_pSource, 0, m_sourceLength + m_targetLength + 4);
        m_pTarget = &m_pSource[m_sourceLength + 2];
    }

    for (;;)
    {
        memcpy(m_pSource, _pString, _length + 2);

        char* pSource = &m_pSource[0];
        char* pTarget = &m_pTarget[0];
        sourceLength = m_sourceLength;
        targetLength = m_targetLength;

        size_t rc = iconv(m_pConverter, &pSource, &sourceLength, &pTarget, &targetLength);

        realLength = m_targetLength - targetLength;

        if (rc == size_t(-1))
        {
            int error = errno;

            switch (error)
            {
                case E2BIG:
                    // target lacks of space

                    m_targetLength = static_cast<size_t>(m_targetLength * 1.2f);
                    delete[] m_pSource;
                    m_pSource = new char[m_sourceLength + m_targetLength + 4];
                    memset(m_pSource, 0, m_sourceLength + m_targetLength + 4);
                    m_pTarget = &m_pSource[m_sourceLength + 2];

                    continue;
                case EBADF:
                    // converter is invalid
//                    Spin::CLogger::getInstance().log("convert", "error: EBADF\nsource is " + _pString);
                    std::cout << "EBADF" << std::endl;
                    break;
                case EINVAL:
                    // invalid char or shift sequence at end of input buffer
                    std::cout << "EINVAL" << std::endl;
                    break;
                case EILSEQ:
                    // input (source) has the wrong codeset
                    std::cout << "EILSEQ" << std::endl;
                    break;
                default:
                    // unknown error o.0
                    std::cout << "default" << std::endl;
                    break;
            }

            return _pString;
        }

        break;
    }

    std::cout << realLength << std::endl;

    return m_pTarget;
}


thread_local static CConvert* s_convertCP1252toUTF8     = nullptr;
thread_local static CConvert* s_convertUTF8toCP1252     = nullptr;
thread_local static CConvert* s_convertUTF16toUTF8      = nullptr;
thread_local static CConvert* s_convertISO8859_15toUTF8 = nullptr;
thread_local static CConvert* s_convertUTF8toISO8859_15 = nullptr;

thread_local static size_t s_convertMaskedUTF16StorageLength = 32;
thread_local static char* s_pConvertMaskedUTF16Storage = nullptr;

}

namespace Spin
{

void initConvert()
{
    s_convertCP1252toUTF8        = new CConvert("CP1252"    , "UTF-8"     );
    s_convertUTF8toCP1252        = new CConvert("UTF-8"     , "CP1252"    );
    s_convertUTF16toUTF8         = new CConvert("UTF-16"    , "UTF-8"     );
    s_convertISO8859_15toUTF8    = new CConvert("ISO8859-15", "UTF-8"     );
    s_convertUTF8toISO8859_15    = new CConvert("UTF-8"     , "ISO8859-15");
    s_pConvertMaskedUTF16Storage = new char[33];
}

void uninitConvert()
{
    if (s_convertCP1252toUTF8        != nullptr) delete s_convertCP1252toUTF8    ;
    if (s_convertUTF8toCP1252        != nullptr) delete s_convertUTF8toCP1252    ;
    if (s_convertUTF16toUTF8         != nullptr) delete s_convertUTF16toUTF8     ;
    if (s_convertISO8859_15toUTF8    != nullptr) delete s_convertISO8859_15toUTF8;
    if (s_convertUTF8toISO8859_15    != nullptr) delete s_convertUTF8toISO8859_15;
    if (s_pConvertMaskedUTF16Storage != nullptr) delete[] s_pConvertMaskedUTF16Storage;
}

std::string convertCP1252toUTF8(const std::string& _rString)
{
    const char* result = s_convertCP1252toUTF8->convert(_rString.c_str(), _rString.size());

    return std::string(result);
}

const char* convertCP1252toUTF8(const char* _pString)
{
    return s_convertCP1252toUTF8->convert(_pString);
}

std::string convertISO8859_15toUTF8(const std::string& _rString)
{
    const char* result = s_convertISO8859_15toUTF8->convert(_rString.c_str(), _rString.size());

    return std::string(result);
}

const char* convertISO8859_15toUTF8(const char* _pString)
{
    return s_convertISO8859_15toUTF8->convert(_pString);
}

std::string convertUTF8toCP1252(const std::string& _rString)
{
    const char* result = s_convertUTF8toCP1252->convert(_rString.c_str(), _rString.size());

    return std::string(result);
}

const char* convertUTF8toCP1252(const char* _pString)
{
    return s_convertUTF8toCP1252->convert(_pString);
}

std::string convertUTF8toISO8859_15(const std::string& _rString)
{
    const char* result = s_convertUTF8toISO8859_15->convert(_rString.c_str(), _rString.size());
    return std::string(result);
}

const char* convertUTF8toISO8859_15(const char* _pString)
{
    return s_convertUTF8toISO8859_15->convert(_pString);
}

std::string convertASCIIWithEscapedUTF16toUTF8(const std::string& _rString)
{
    std::string result = _rString;

    size_t pos = 0;
    char character[5];
    character[4] = '\0';
    const char* pResult;
    size_t length = 0;

#ifndef __MINGW32__
    union
    {
        char m_character[4];
        long m_number;
    } utf16Character;
#else
    union
    {
        char m_character[4];
        UTF16 m_number;
    } utf16Character;

    UTF16 utf16String[2];
    utf16String[1] = 0;
#endif

    for (;;)
    {
        pos = result.find("\\u", pos);

        if (pos == std::string::npos)
        {
            break;
        }

        const char* pString = result.c_str();

        memcpy(&character[0], &pString[pos + 2], 4);
        utf16Character.m_number = strtol(character, NULL, 16);

#ifndef __MINGW32__
        utf16Character.m_character[2] = '\0';
        utf16Character.m_character[3] = '\0';

        pResult = s_convertUTF16toUTF8->convert(utf16Character.m_character, 4);
        length = strlen(pResult);

        for (size_t index = 0; index < 4; ++ index)
        {
            std::cout << index << "\t" << pResult[index] << "\t" << +pResult[index] << std::endl;
        }

#else
        utf16String[0] = static_cast<UTF16>(utf16Character.m_number);
        const UTF16* pUtf16String = &utf16String[0];
        const UTF16** ppUtf16String = &pUtf16String;

        UTF8 resultChar[UNI_MAX_UTF8_BYTES_PER_CODE_POINT + 10 + 1];
        resultChar[UNI_MAX_UTF8_BYTES_PER_CODE_POINT + 10] = 0;
        UTF8* pResultChar = &resultChar[0];

		for (int i = 0; i < 6; ++ i)
		{
			purple_debug_info("crap", "the char %c \t %i", resultChar[i], +resultChar[i]);
			printf( " \r\n");
		}
        ConversionResult returnCode = ConvertUTF16toUTF8(ppUtf16String, &pUtf16String[1], &pResultChar, &resultChar[UNI_MAX_UTF8_BYTES_PER_CODE_POINT + 10], strictConversion);

		length = pResultChar - &resultChar[0];
        purple_debug_info("crap", "the number %i \r\n", utf16Character.m_number);
        purple_debug_info("crap", "the chars %c %c \r\n", utf16Character.m_character[0], utf16Character.m_character[1]);
        purple_debug_info("crap", "the chars %i %i \r\n", +utf16Character.m_character[0], +utf16Character.m_character[1]);
        purple_debug_info("crap", "the returncode %i \r\n", returnCode);
        purple_debug_info("crap", "the char %s \r\n", &resultChar[0]);
		for (int i = 0; i < 6; ++ i)
		{
			purple_debug_info("crap", "the char %c \t %i \r\n", resultChar[i], +resultChar[i]);
			printf( " \r\n");
		}
			purple_debug_info("crap", "the char %c \t %i \r\n", pResultChar[0], +pResultChar[0]);
        purple_debug_info("crap", "the length %i \r\n", length);

        pResult = reinterpret_cast<char*>(&resultChar[0]);
#endif

        if (length > 0 && length < 5)
        {
            // 6 is the size of the string "\uXXXX"
            result.replace(pos, 6, pResult, length);
        }

        pos += 1;
    }

    return result;
}

const char* convertASCIIWithEscapedUTF16toUTF8(const char* _pString)
{
    std::string str = _pString;

    std::string res = convertASCIIWithEscapedUTF16toUTF8(str);

    const char* pStr = res.c_str();

    size_t strLength = strlen(pStr);

    if (strLength > s_convertMaskedUTF16StorageLength)
    {
        delete[] s_pConvertMaskedUTF16Storage;
        s_convertMaskedUTF16StorageLength = strLength;
        s_pConvertMaskedUTF16Storage = new char[s_convertMaskedUTF16StorageLength + 1];
    }

    //s_pConvertMaskedUTF16Storage[strLength] = 0;
    memcpy(s_pConvertMaskedUTF16Storage, pStr, strLength + 1);

    return s_pConvertMaskedUTF16Storage;
}

}