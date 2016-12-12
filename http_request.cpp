/**
 * http_request.cpp
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


#include "http_request.h"

#include <assert.h>
#include <curl/curl.h>
#include <fstream>
#include <chrono>
#include <functional>
#include <iostream>

namespace
{

size_t writeToVector(char* _pBuffer, size_t _size, size_t _nmemb, void* _pVector)
{
    size_t length = _size * _nmemb;
    std::vector<char>* pVector = reinterpret_cast<std::vector<char>*>(_pVector);

    pVector->insert(pVector->end(), &_pBuffer[0], &_pBuffer[length]);

    return length;
}

size_t writeToStdString(char* _pBuffer, size_t _size, size_t _nmemb, void* _pString)
{
    size_t length = _size * _nmemb;
    reinterpret_cast<std::string*>(_pString)->append(_pBuffer, length);

    return length;
}

#ifdef SPINPLOG
static int traceRequest(CURL *_pHandle, curl_infotype _type, char* _pData, size_t _size, void* _pUserData)
{
    std::ofstream* pStream = reinterpret_cast<std::ofstream*>(_pUserData);

    switch (_type)
    {
        case CURLINFO_TEXT:
            *pStream << "\n----- info";
            break;
        case CURLINFO_HEADER_OUT:
            *pStream << "\n>>>>> Send header";
            break;
        case CURLINFO_DATA_OUT:
            *pStream << "\n>>>>> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            *pStream << "\n>>>>> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            *pStream << "\n<<<<< Recv header";
            break;
        case CURLINFO_DATA_IN:
            *pStream << "\n<<<<< Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            *pStream << "\n<<<<< Recv SSL data";
            break;
        default: /* in case a new one is introduced to shock us */
            *pStream << std::endl << "----- undefined (" << _type << ")";
            break;
    }

    *pStream << std::endl;

    pStream->write(_pData, _size);

    return 0;
}
#endif

}


namespace Spin
{

const TMaps CHttpRequest::m_defaultHeaders =
    {
        { "Connection"     , "keep-alive"                                                                                                         },
        { "Accept"         , "*/*"                                                                                                                },
        { "User-Agent"     , "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.75 Safari/537.36" },
        { "Accept-Language", "de-DE,de;q=0.8,en-US;q=0.6,en;q=0.4"                                                                                },
        { "Referer"        , "http://www.spin.de"                                                                                                 },
//    {"Accept-Encoding", "gzip, deflate"},
//    {"Origin", "http://hc.spin.de"},
//    {"Content-Type", "application/x-www-form-urlencoded"},
    };

#ifdef SPINPLOG
std::atomic_uint CHttpRequest::m_requestNumber(0);
#endif

CHttpRequest::CHttpRequest()
    : m_query                ()
    , m_subDomain            ()
    , m_getParameters        ()
    , m_postParameters       ()
    , m_headers              ()
    , m_follow               (false)
    , m_postRedirect         (false)
    , m_isHttps              (false)
    , m_longPoll             (false)
    , m_timeoutInSeconds     (0)
    , m_pProgressCallback    (nullptr)
    , m_pProgressCallbackData(nullptr)
    , m_pWriteCallback       (nullptr)
    , m_pWriteCallbackData   (nullptr)
{
}

CHttpRequest::CHttpRequest(CHttpRequest&& _rrOther)
    : m_query                (std::move(_rrOther.m_query         ))
    , m_subDomain            (std::move(_rrOther.m_subDomain     ))
    , m_getParameters        (std::move(_rrOther.m_getParameters ))
    , m_postParameters       (std::move(_rrOther.m_postParameters))
    , m_headers              (std::move(_rrOther.m_headers       ))
    , m_follow               (_rrOther.m_follow                   )
    , m_postRedirect         (_rrOther.m_postRedirect             )
    , m_isHttps              (_rrOther.m_isHttps                  )
    , m_longPoll             (_rrOther.m_longPoll                 )
    , m_timeoutInSeconds     (0                                   )
    , m_pProgressCallback    (nullptr                             )
    , m_pProgressCallbackData(nullptr                             )
    , m_pWriteCallback       (nullptr                             )
    , m_pWriteCallbackData   (nullptr                             )
{
}

CHttpRequest::~CHttpRequest()
{
//    if (m_pCurl    != nullptr) curl_easy_cleanup(m_pCurl);
//    if (m_pHeaders != nullptr) curl_slist_free_all(reinterpret_cast<curl_slist*>(m_pHeaders));
}

void CHttpRequest::setHttps(bool _isHttps)
{
    m_isHttps = _isHttps;
}

void CHttpRequest::setTimeout(long _timeoutInSeconds)
{
    m_timeoutInSeconds = _timeoutInSeconds;
}

void CHttpRequest::setQuery(const std::string& _rQuery)
{
    m_query = _rQuery;
}

void CHttpRequest::setQuery(std::string&& _rrUrl)
{
    m_query = std::move(_rrUrl);
}

void CHttpRequest::setSubDomain(const std::string& _rSubDomain)
{
    m_subDomain = _rSubDomain;
}

void CHttpRequest::setSubDomain(std::string&& _rrSubDomain)
{
    m_subDomain = std::move(_rrSubDomain);
}

void CHttpRequest::setGetParameter(const std::string& _rKey, const std::string& _rValue)
{
    m_getParameters[_rKey] = _rValue;
}

void CHttpRequest::setGetParameter(std::string&& _rrKey, std::string&& _rrValue)
{
    auto it = m_getParameters.find(_rrKey);
    if (it != m_getParameters.cend())
    {
        it->second = std::move(_rrValue);
    }
    else
    {
        m_getParameters.emplace(std::make_pair(_rrKey, _rrValue));
    }
}

void CHttpRequest::setPostParameter(const std::string& _rKey, const std::string& _rValue)
{
    m_postParameters[_rKey] = _rValue;
}

void CHttpRequest::setPostParameter(std::string&& _rrKey, std::string&& _rrValue)
{
    auto it = m_postParameters.find(_rrKey);
    if (it != m_postParameters.cend())
    {
        it->second = std::move(_rrValue);
    }
    else
    {
        m_postParameters.emplace(std::make_pair(_rrKey, _rrValue));
    }
}

void CHttpRequest::setHeader(const std::string& _rKey, const std::string& _rValue)
{
    m_headers[_rKey] = _rValue;
}

void CHttpRequest::setHeader(std::string&& _rrKey, std::string&& _rrValue)
{
    auto it = m_headers.find(_rrKey);
    if (it != m_headers.cend())
    {
        it->second = std::move(_rrValue);
    }
    else
    {
        m_headers.emplace(std::make_pair(_rrKey, _rrValue));
    }
}

void CHttpRequest::removeGetParameter(const std::string& _rKey)
{
    m_getParameters.erase(_rKey);
}

void CHttpRequest::removePostParameter(const std::string& _rKey)
{
    m_postParameters.erase(_rKey);
}

void CHttpRequest::removeHeaderParameter(const std::string& _rKey)
{
    m_headers.erase(_rKey);
}

void CHttpRequest::setFollow(bool _follow)
{
    m_follow = _follow;
}

void CHttpRequest::setPostRedirect(bool _postRedirect)
{
    m_postRedirect = _postRedirect;
}

void CHttpRequest::setLongPoll(bool _longPoll)
{
    m_longPoll = _longPoll;
}

void CHttpRequest::setProgressCallback(httpRequestCallback _pFn, void* _pData)
{
    assert(_pFn != nullptr);

    m_pProgressCallback     = _pFn  ;
    m_pProgressCallbackData = _pData;
}

void CHttpRequest::setWriteCallback(writeCallback _pFn, void* _pData)
{
    assert(_pFn != nullptr);

    m_pWriteCallback     = _pFn  ;
    m_pWriteCallbackData = _pData;
}

bool CHttpRequest::exec()
{
    assert(false);

    return false;
}

bool CHttpRequest::execSync()
{
    CURL* pCurl = curl_easy_init();

    if (pCurl == nullptr)
    {
        return false;
    }

    bool result = execRequest(pCurl);

    curl_easy_cleanup(pCurl);

    return result;
}

bool CHttpRequest::execSync(std::vector<char>* _pVector)
{
    CURL* pCurl = curl_easy_init();

    if (pCurl == nullptr)
    {
        return false;
    }

    if (_pVector != nullptr)
    {
        curl_easy_setopt(pCurl, CURLOPT_WRITEDATA    , _pVector      );
        curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &writeToVector);
    }

    bool result = execRequest(pCurl);

    curl_easy_cleanup(pCurl);

    return result;
}

bool CHttpRequest::execSync(std::string* _pResultBody)
{
    CURL* pCurl = curl_easy_init();

    if (pCurl == nullptr)
    {
        return false;
    }

    if (_pResultBody != nullptr)
    {
        curl_easy_setopt(pCurl, CURLOPT_WRITEDATA    , _pResultBody     );
        curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &writeToStdString);
    }

    bool result = execRequest(pCurl);

    curl_easy_cleanup(pCurl);

    return result;
}

bool CHttpRequest::execSync(std::string* _pResultBody, std::string* _pResultHeader)
{
    CURL* pCurl = curl_easy_init();

    if (pCurl == nullptr)
    {
        return false;
    }

    if (_pResultBody != nullptr)
    {
        curl_easy_setopt(pCurl, CURLOPT_WRITEDATA    , _pResultBody     );
        curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &writeToStdString);
    }

    if (_pResultHeader != nullptr)
    {
        curl_easy_setopt(pCurl, CURLOPT_HEADERDATA    , _pResultHeader   );
        curl_easy_setopt(pCurl, CURLOPT_HEADERFUNCTION, &writeToStdString);
    }

    bool result = execRequest(pCurl);

    curl_easy_cleanup(pCurl);

    return result;
}

bool CHttpRequest::execRequest(void* _pCurl)
{
    struct curl_slist* pHeaders = nullptr;

#ifdef SPINPLOG
    std::string filename;
    filename.append("request_")
            .append(std::to_string(m_requestNumber.fetch_add(1)))
            .append("_")
            .append(std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())))
            .append(".txt");

    std::ofstream outFile;
    outFile.open(filename);

    curl_easy_setopt(_pCurl, CURLOPT_DEBUGFUNCTION, traceRequest);
    curl_easy_setopt(_pCurl, CURLOPT_DEBUGDATA    , &outFile    );
    curl_easy_setopt(_pCurl, CURLOPT_VERBOSE      , 1L          );
#endif

    TMaps::iterator it;

    auto endOfDefault = m_defaultHeaders.cend();
    for (auto currentOfDefault = m_defaultHeaders.cbegin(); currentOfDefault != endOfDefault; ++ currentOfDefault)
    {
        it = m_headers.find(currentOfDefault->first);
        if (it == m_headers.cend())
        {
            m_headers[currentOfDefault->first] = currentOfDefault->second;
        }
    }

    std::string header;

    auto endOfHeaders = m_headers.cend();
    for (auto currentHeader = m_headers.cbegin(); currentHeader != endOfHeaders; ++ currentHeader)
    {
        header.append(currentHeader->first).append(":").append(currentHeader->second);
        pHeaders = curl_slist_append(pHeaders, header.c_str());
        header.clear();
    }

    // TODO!!!
//    pHeaders = curl_slist_append(pHeaders, (m_pCookie->getCookieString() + "settings=00000000000; cto_rtt=; setup=nologin=0").c_str());

    std::string url = "http://";

    if (m_subDomain != "")
    {
        url += m_subDomain;
    }
    else
    {
        url += "www";
    }

    url += ".spin.de";

    if (m_query != "")
    {
        url += "/" + m_query;
    }

    auto endGet = m_getParameters.cend();
    auto currentGet = m_getParameters.cbegin();
    if (currentGet != endGet)
    {
        url += "?" + currentGet->first + "=" + currentGet->second;
        ++ currentGet;

        for (;currentGet != endGet; ++ currentGet)
        {
            url += "&" + currentGet->first + "=" + currentGet->second;
        }
    }

    std::string postFields = "";
    auto endPost = m_postParameters.cend();
    auto currentPost = m_postParameters.cbegin();
    if (currentPost != endPost)
    {
        postFields += currentPost->first + "=" + currentPost->second;
        ++ currentPost;

        for (;currentPost != endPost; ++ currentPost)
        {
            postFields += "&" + currentPost->first + "=" + currentPost->second;
        }

        curl_easy_setopt(_pCurl, CURLOPT_POST, 1L);
        curl_easy_setopt(_pCurl, CURLOPT_POSTFIELDS, postFields.c_str());
    }

    curl_easy_setopt(_pCurl, CURLOPT_URL            , url.c_str());
    curl_easy_setopt(_pCurl, CURLOPT_HTTPHEADER     , pHeaders   );
    curl_easy_setopt(_pCurl, CURLOPT_ACCEPT_ENCODING, ""         );

    if (m_timeoutInSeconds)
    {
        curl_easy_setopt(_pCurl, CURLOPT_TIMEOUT, m_timeoutInSeconds);
    }

    if (m_follow)
    {
        curl_easy_setopt(_pCurl, CURLOPT_FOLLOWLOCATION, 1L);

        if (m_postRedirect)
        {
            curl_easy_setopt(_pCurl, CURLOPT_POSTREDIR, 1L);
        }
    }

    if (m_pProgressCallback != nullptr)
    {
        curl_easy_setopt(_pCurl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(_pCurl, CURLOPT_XFERINFOFUNCTION, m_pProgressCallback);

        if (m_pProgressCallbackData != nullptr)
        {
            curl_easy_setopt(_pCurl, CURLOPT_XFERINFODATA, m_pProgressCallbackData);
        }
    }

    if (m_pWriteCallback != nullptr)
    {
        curl_easy_setopt(_pCurl, CURLOPT_WRITEFUNCTION, m_pWriteCallback);

        if (m_pWriteCallbackData != nullptr)
        {
            curl_easy_setopt(_pCurl, CURLOPT_WRITEDATA, m_pWriteCallbackData);
        }
    }

    CURLcode res;
    res = curl_easy_perform(_pCurl);

#ifdef SPINPLOG
    outFile.close();
#endif

    curl_slist_free_all(pHeaders);

    return res == CURLE_OK;
}


}