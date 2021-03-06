/**
 * http_request.h
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


#ifndef SPINTEST_HTTP_REQUEST_H
#define SPINTEST_HTTP_REQUEST_H

#include <string>
#include <map>
#include <bits/atomic_base.h>
#include <vector>
#include <unordered_map>

namespace Spin
{

typedef size_t (*httpRequestCallback)(void*, double, double, double, double);
typedef size_t (*writeCallback)(char*, size_t, size_t, void*);
typedef std::unordered_map<std::string, std::string> TMaps;

class CHttpRequest
{
public:
    CHttpRequest();
    CHttpRequest(CHttpRequest&& _rrOther);
    ~CHttpRequest();

public:
    void setHttps(bool _isHttps);
    void setTimeout(long _timeoutInSeconds);
    void setQuery(const std::string& _rUrl);
    void setQuery(std::string&& _rrUrl);
    void setSubDomain(const std::string& _rSubDomain);
    void setSubDomain(std::string&& _rrSubDomain);
    void setGetParameter(const std::string& _rKey, const std::string& _rValue);
    void setGetParameter(std::string&& _rrKey, std::string&& _rrValue);
    void setPostParameter(const std::string& _rKey, const std::string& _rValue);
    void setPostParameter(std::string&& _rrKey, std::string&& _rrValue);
    void setHeader(const std::string& _rKey, const std::string& _rValue);
    void setHeader(std::string&& _rrKey, std::string&& _rrValue);
    void setFollow(bool _follow);
    void setPostRedirect(bool _postRedirect);
    void setLongPoll(bool _longPoll);

    void removeGetParameter(const std::string& _rKey);
    void removePostParameter(const std::string& _rKey);
    void removeHeaderParameter(const std::string& _rKey);

    void setProgressCallback(httpRequestCallback _pFn, void* _pData);
    void setWriteCallback(writeCallback _pFn, void* _pData);

public:
    bool exec();
    bool execSync();
    bool execSync(std::vector<char>* _pVector);
    bool execSync(std::string* _pResultBody);
    bool execSync(std::string* _pResultBody, std::string* _pResultHeader);

private:
    bool execRequest(void* _pCurl);

private:
    const static TMaps m_defaultHeaders;

private:
    std::string         m_query;
    std::string         m_domain;
    std::string         m_subDomain;
    TMaps               m_getParameters;
    TMaps               m_postParameters;
    TMaps               m_headers;
    bool                m_follow;
    bool                m_postRedirect;
    bool                m_isHttps;
    bool                m_longPoll;
    long                m_timeoutInSeconds;

    httpRequestCallback m_pProgressCallback;
    void*               m_pProgressCallbackData;

    writeCallback       m_pWriteCallback;
    void*               m_pWriteCallbackData;


#ifdef SPINPLOG
    static std::atomic_uint m_requestNumber;
#endif

};

}

#endif // SPINTEST_HTTP_REQUEST_H
