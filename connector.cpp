/**
 * connector.cpp
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


#include "connector.h"
#include "message.h"
#include "logger.h"
#include "convert.h"
#include "extern/json/json.hpp"
#include "purple_connector.h"
#include "common.h"
#include "job.h"


#include <curl/curl.h>
#include <chrono>
#include <iostream>
#include <string.h>
#include <assert.h>
#include <sstream>

#ifdef CMAKE
#include <libpurple/debug.h>
#else
#include <debug.h>
#endif


namespace Spin
{

size_t writeMessage(char* _pBuffer, size_t _size, size_t _numberOfElements, void* _pInstance)
{
    CConnector* pConn = reinterpret_cast<CConnector*>(_pInstance);

    size_t n = _size * _numberOfElements;

    if (n > 0)
    {
        pConn->pushIn(_pBuffer, n);
    }

    return n;
}

size_t progressCallback(void* _pInstance, double _dlTotal, double _dlNow, double _ulTotal, double _ulNow)
{
    assert(_pInstance != nullptr);

    CLogger& rLogger = CLogger::getInstance();

    CConnector* pConn = reinterpret_cast<CConnector*>(_pInstance);

    if (!pConn->isHeartbeating() &&
        pConn->isConnected() &&
        pConn->getTimestamp() > pConn->getLastSentTimestamp() + std::chrono::milliseconds(15000).count())
    {
        pConn->setHearthbeat(true);

        Message::SHearthbeat message;

        pConn->pushOut(message);
//        Spin::pushJob(pConn, &message);
    }

    if (pConn->isShutdown())
    {
        return 1;
    }

    return 0;
}

char fromHex(char ch)
{
    return static_cast<char>(isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10);
}

char urlDecode(const char* _pStr)
{
//    if (_pStr[0] == '%')
//    {
    if (_pStr[1] && _pStr[2])
    {
        return fromHex(_pStr[1]) << 4 | fromHex(_pStr[2]);
    }
//    }

    return '\0';
}

CConnector::CConnector(CBaseConnector* _pBaseConnector)
    : m_pUsername             (nullptr)
    , m_pPassword             (nullptr)
    , m_port                  (0)
    , m_pCookie               (nullptr)
    , m_pBaseConnector        (_pBaseConnector)
    , m_pInThread             (nullptr)
    , m_pOutThread            (nullptr)
    , m_inMessageMutex        ()
    , m_outMessageMutex       ()
    , m_mixedJobMutex         ()
    , m_hasMessages           ()
    , m_connReferer           ()
    , m_in                    ()
    , m_pOutQueue             (nullptr)
    , m_pMixedJobQueue        (nullptr)
    , m_pOutQueueThreaded     (nullptr)
    , m_pMixedJobQueueThreaded(nullptr)
    , m_isConnected           (false)
    , m_isConnecting          (false)
    , m_heartBeats            ()
    , m_lastSentTimestamp     ()
    , m_messageID             (0)
    , m_sndID                 (0)
    , m_sentMessageID         (-1)
    , m_sentSndID             (-1)
    , m_inLines               ()
    , m_semicolonCount        ()
    , m_version               ()
    , m_shutdown              (false)
    , m_inLine                ()
    , m_anchorLengths         ()
    , m_pAnchors              ()
    , m_numberOfAnchors       (0)
    , m_secret                ()
    , m_userInfos             ()
{
    assert(_pBaseConnector != nullptr);

    CLogger::getInstance().init();

    m_pOutQueue               = new TMessageQueue();
    m_pOutQueueThreaded       = new TMessageQueue();
    m_pMixedJobQueue          = new TMixedJobQueue();
    m_pMixedJobQueueThreaded  = new TMixedJobQueue();
//    m_pInternJobQueue         = new TInternJobQueue();
//    m_pInternJobQueueThreaded = new TInternJobQueue();
}

CConnector::~CConnector()
{
    purple_debug_info( "prpl-spinchat-protocol", "~CConnector 1\n");

    try
    {
        m_pInThread->join();
        purple_debug_info("prpl-spinchat-protocol", "~CConnector 2\n");
        m_pOutThread->join();
        purple_debug_info("prpl-spinchat-protocol", "~CConnector 3\n");
    }
    catch (std::exception _e)
    {
        purple_debug_info("prpl-spinchat-protocol", _e.what());
        purple_debug_info("prpl-spinchat-protocol", "\n");
    }

    purple_debug_info( "prpl-spinchat-protocol", "~CConnector 4\n");
    if (m_pInThread  != nullptr) delete m_pInThread;
    purple_debug_info( "prpl-spinchat-protocol", "~CConnector 5\n");
    if (m_pOutThread != nullptr) delete m_pOutThread;
    purple_debug_info( "prpl-spinchat-protocol", "~CConnector 6\n");


    purple_debug_info( "prpl-spinchat-protocol", "~CConnector 8\n"); if (m_pOutQueue               != nullptr) delete m_pOutQueue              ;
    purple_debug_info( "prpl-spinchat-protocol", "~CConnector 9\n"); if (m_pOutQueueThreaded       != nullptr) delete m_pOutQueueThreaded      ;
    purple_debug_info( "prpl-spinchat-protocol", "~CConnector 10\n"); if (m_pMixedJobQueue          != nullptr) delete m_pMixedJobQueue         ;
    purple_debug_info( "prpl-spinchat-protocol", "~CConnector 11\n"); if (m_pMixedJobQueueThreaded  != nullptr) delete m_pMixedJobQueueThreaded ;
    purple_debug_info( "prpl-spinchat-protocol", "~CConnector 14\n");
}

void CConnector::setLogin(const char* _pUsername, const char* _pPassword, unsigned short _port)
{
    m_pUsername = new char[strlen(_pUsername) + 1];
    strcpy(m_pUsername, _pUsername);

    m_pPassword = new char[strlen(_pPassword) + 1];
    strcpy(m_pPassword, _pPassword);

    m_port = _port;
}

bool CConnector::start()
{
    return initThreads();

//    if (!login())
//    {
//        return false;
//    }
//
//    return this->initConnection();
}

CConnector::ELoginRC CConnector::login()
{
    assert(m_pUsername != nullptr);
    assert(m_pPassword != nullptr);
    assert(m_port      != 0      );

    if (strlen(m_pUsername) == 0) return LoginRC_FailedAuth;
    if (strlen(m_pPassword) == 0) return LoginRC_FailedAuth;

    CHttpRequest requestLogin;
    CHttpRequest requestLoginScreen;
    requestLoginScreen.setHeader("Cache-Control", "max-age=0");
    requestLoginScreen.setHeader("Connection", "keep-alive");
    requestLoginScreen.setHeader("DNT", "1");
    requestLoginScreen.setHeader("Accept-Encoding", "");

    std::string content;
    std::string header;

    if (!requestLoginScreen.execSync(&content, &header))
    {
        return LoginRC_InvalidResponse;
    }

    size_t pos, endPos;

    pos = header.find("loginid=") + 8;
    endPos = header.find(";", pos);

    std::string loginID = header.substr(pos, endPos - pos);


    std::vector<std::pair<std::string, std::string>> hiddenFields;
    std::string key;
    std::string value;
    std::string form;
    pos = 0;

    for (;;)
    {
        pos = content.find("<form");
        if (pos == std::string::npos)
        {
            Log::error("request_loginscreen") << "couldn't find '<form' in response-body" << Log::end;
            return LoginRC_InvalidResponse;
        }

        endPos = content.find("loginform", pos);
        if (endPos == std::string::npos)
        {
            Log::error("request_loginscreen") << "couldn't find 'loginform' in response-body" << Log::end;
            return LoginRC_InvalidResponse;
        }

        if (endPos - pos > 100)
        {
            continue;
        }

        endPos = content.find("</form>", pos);
        if (endPos == std::string::npos)
        {
            Log::error("request_loginscreen") << "couldn't find '</form>' in response-body" << Log::end;
            return LoginRC_InvalidResponse;
        }

        form = content.substr(pos, endPos - pos);

        break;
    }

    pos = 0;
    for (;;)
    {
        pos = form.find("<input type=\"hidden\"", pos);

        if (pos == std::string::npos) break;

        pos += 21;

        pos = form.find("name=\"", pos) + 6;
        endPos = form.find("\"", pos);
        key = form.substr(pos, endPos - pos);
        pos = endPos;

        pos = form.find("value=\"", pos) + 7;
        endPos = form.find("\"", pos);
        value = form.substr(pos, endPos - pos);
        pos = endPos;

        hiddenFields.push_back(std::pair<std::string, std::string>(key, value));
    }


    auto endOfHiddenFields = hiddenFields.cend();
    for (auto currentHiddenField = hiddenFields.cbegin(); currentHiddenField != endOfHiddenFields; ++ currentHiddenField)
    {
        requestLogin.setPostParameter(currentHiddenField->first, currentHiddenField->second);
    }

    requestLogin.setPostParameter("user", std::string(m_pUsername));
    requestLogin.setPostParameter("cr", std::string(m_pPassword));
    requestLogin.setPostParameter("server", std::to_string(m_port));

    std::string cookie = "settings=00000000000; design=0-0; cto_rtt=; loginid=" + loginID;

    header.clear();
    content.clear();

    requestLogin.setQuery("login");
    requestLogin.setHeader(":authority", ":www.spin.de");
    requestLogin.setHeader(":method", "POST");
    requestLogin.setHeader(":path", "/login");
    requestLogin.setHeader(":scheme", "https");
    requestLogin.setHeader("accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    requestLogin.setHeader("cache-control", "max-age=0");
    requestLogin.setHeader("content-type", "application/x-www-form-urlencoded");
    requestLogin.setHeader("origin", "http://www.spin.de");
    requestLogin.setHeader("upgrade-insecure-requests", "1");
    requestLogin.setHeader("cookie", cookie);

    if (!requestLogin.execSync(nullptr, &header))
    {
        return LoginRC_InvalidResponse;
    }

    if (header.find("login/failed?user=") != std::string::npos)
    {
        return LoginRC_FailedAuth;
    }

    if (header.find("login/notreg?user=") != std::string::npos)
    {
        return LoginRC_Unregistered;
    }

    const size_t numberOfCookies = 5;
    const std::string cookieKeys[5] =
        {
            "settings",
            "cto_rtt",
            "loginid",
            "session1",
            "session2",
        };


    m_pCookie = new CCookie();

    for (size_t index = 0; index < numberOfCookies; ++ index)
    {
        pos = header.find(cookieKeys[index]);

        if (pos != std::string::npos)
        {
            pos += cookieKeys[index].length() + 1;
            endPos = header.find(";", pos);

            m_pCookie->addCookie(cookieKeys[index], header.substr(pos, endPos - pos));
        }
    }
//    "settings=00000000000; cto_rtt=; setup=nologin=0

    m_pCookie->addCookie("nologin", "0");

    //https://curl.haxx.se/libcurl/c/CURLOPT_TCP_KEEPALIVE.html

    // TODO check if any cookies are missing

    return LoginRC_OK;
}

bool CConnector::initLoggedIn()
{
    size_t pos, endPos;
    std::string content;
    CHttpRequest request;

    request.setQuery("loggedin");
    request.setHeader("Cache-Control", "max-age=0");
    request.setHeader("DNT", "1");
    request.setHeader("Cookie", m_pCookie->getCookieString());

    if (!request.execSync(&content))
    {
        Log::error("request_loggedin") << "request failed" << Log::end;
        return false;
    }

    const size_t maxUserNameLength = 256;

    char userName[maxUserNameLength];

    pos = content.find("secret=") + 7;
    char* secret = &m_secret[0];
    while (content[pos] != '"' && pos != std::string::npos)
    {
        *secret = content[pos];

        ++ secret;
        ++ pos;
    }
    *secret = '\0';

    // "friends": [[8902635,"someUsername",1,"",0,"","y",1],[3589301,"D\u00e4mmerle",1,"",0,"","y",1]],

    Log::info("request_loggedin") << "parsing buddies:\n" << Log::end;
    std::vector<CBaseConnector::SBuddyState> buddyList;

    pos = content.find("\"friends\":") + 13;
    if (pos != std::string::npos)
    {
        endPos = content.find("]],", pos);
        if (endPos != std::string::npos)
        {
            while (pos < endPos)
            {
                for (; pos != endPos; ++ pos)
                {
                    if (content[pos] == ',')
                        break;
                }

                ++ pos;
                ++ pos;

                for (size_t indexOfUserName = 0; pos != endPos || indexOfUserName == 64; ++ indexOfUserName)
                {
                    userName[indexOfUserName] = content[pos];

                    if (content[pos] == '"')
                    {
                        userName[indexOfUserName] = '\0';
                        break;
                    }

                    ++ pos;
                }

                ++ pos;
                ++ pos;

                CBaseConnector::SBuddyState buddyState;
                buddyState.m_flags = BuddyFlag_None;

                if (content[pos] == '1') buddyState.m_flags |= BuddyFlag_Online;
                else                     buddyState.m_flags |= BuddyFlag_Offline;

                for (; pos != endPos; ++ pos)
                {
                    if (content[pos] == ']')
                    {
                        pos += 3;
                        break;
                    }
                }

                buddyState.m_name = convertASCIIWithEscapedUTF16toUTF8(userName);

                Log::write() << "\t" << buddyState.m_name << "(" << userName << ") - flags: " << buddyState.m_flags;

                buddyList.push_back(buddyState);
            }
        }
    }

    Log::write() << Log::end;

    m_pBaseConnector->lock();
    m_pBaseConnector->buddyListRefresh(buddyList);
    m_pBaseConnector->unlock();

    return true;
}

bool CConnector::initThreads()
{
    try
    {
        Log::info("sys") << "before starting threads" << Log::end;
#ifndef __MINGW32__
        std::stringstream ss;
        ss << std::this_thread::get_id();
        uint64_t id = std::stoull(ss.str());
        Log::info("sys") << "MAIN THREAD-ID: " << id << Log::end;
#endif
        m_pInThread = new std::thread(&CConnector::runConnector, this);
#ifndef __MINGW32__
        std::stringstream ssa;
        ssa << m_pInThread->get_id();
        id = std::stoull(ssa.str());
        Log::info("sys") << "IN THREAD-ID: " << id << Log::end;
#endif
        m_pOutThread = new std::thread(&CConnector::runOut, this);
#ifndef __MINGW32__
        std::stringstream ssb;
        ssb << m_pOutThread->get_id();
        id = std::stoull(ssb.str());
        Log::info("sys") << "OUT THREAD-ID: " << id << Log::end;
#endif
        Log::info("sys") << "after starting thread" << Log::end;
    }
    catch (const std::exception& _rException)
    {
        Log::fatal("threads") << "The threads couldn't be started" << Log::end;

        return false;
    }

    return true;
}

bool CConnector::initConnection()
{
    std::string content;
    CHttpRequest request;

    request.setSubDomain("hc");
    request.setQuery("connector");
    request.setHeader("Cache-Control", "max-age=0");
    request.setHeader("DNT", "1");
    request.setHeader("Accept-Language", "de-DE,de;q=0.8,en-US;q=0.6,en;q=0.4");
    request.setGetParameter("sid", m_pCookie->getCookie("session1"));
    request.setGetParameter("ign", std::to_string(getTimestamp()));
    request.setTimeout(10L);

    if (!request.execSync(&content))
    {
        Log::error("request_initconnection") << "request failed" << Log::end;
        return false;
    }

    const std::string searchValue = "www.spin.de/static/js/htmlchat.js?v=";
    size_t pos = content.find(searchValue) + searchValue.length();
    size_t endPos = content.find("\"", pos);

    m_version = content.substr(pos, endPos - pos);
    m_version = "2015-04-01";

    setIsConnecting(true);
//    m_isConnecting = true;
    m_pBaseConnector->lock();
    m_pBaseConnector->setConnectionState(ConnectionState_Connecting);
    m_pBaseConnector->unlock();
    if (!initLoggedIn())
    {
        return false;
    }

//    Log::info("sys") << "before starting threads" << Log::end;
//#ifndef __MINGW32__
//    std::stringstream ss; ss << std::this_thread::get_id(); uint64_t id = std::stoull(ss.str());
//    Log::info("sys") << "MAIN THREAD-ID: " << id << Log::end;
//#endif
//    m_pInThread = new std::thread(&CConnector::runConnector, this);
//#ifndef __MINGW32__
//    std::stringstream ssa; ssa << m_pInThread->get_id(); id = std::stoull(ssa.str());
//    Log::info("sys") << "IN THREAD-ID: " << id << Log::end;
//#endif
//    m_pOutThread = new std::thread(&CConnector::runOut, this);
//#ifndef __MINGW32__
//    std::stringstream ssb; ssb << m_pOutThread->get_id(); id = std::stoull(ssb.str());
//    Log::info("sys") << "OUT THREAD-ID: " << id << Log::end;
//#endif
//    Log::info("sys") << "after starting thread" << Log::end;

    return true;
}

void CConnector::runConnector()
{
    Log::getInstance().init();
    initConvert();

    for (;;)
    {
        std::cout << "start" << std::endl;

        ELoginRC loginRC = login();

        switch (loginRC)
        {
            case LoginRC_OK:
                if (!connect())
                {
                    Log::error("conn") << "connect() failed" << Log::end;
                }
                std::this_thread::sleep_for(std::chrono::seconds(5));
                break;
            case LoginRC_Unregistered:
                setShutdown(true);
                break;
            case LoginRC_FailedAuth:
                setShutdown(true);
                break;
            case LoginRC_InvalidResponse:
                setShutdown(true);
                break;
        }

        if (isShutdown())// && !m_reconnect.load())
        {
            break;
        }
    }

    uninitConvert();
    Log::getInstance().uninit();
}

void CConnector::runOut()
{
    Log::getInstance().init();
    initConvert();

    for (;;)
    {
        if (!isConnected() && !isShutdown())
        {
            std::unique_lock<std::mutex> lock(m_isConnectedMutex);
            m_isConnectedCV.wait(lock);
        }

        if (isShutdown())
        {
            break;
        }

        m_outMessageMutex.lock();

        TMessageQueue* tmp = m_pOutQueue;
        m_pOutQueue = m_pOutQueueThreaded;
        m_pOutQueueThreaded = tmp;

        TMixedJobQueue* tmp2 = m_pMixedJobQueue;
        m_pMixedJobQueue = m_pMixedJobQueueThreaded;
        m_pMixedJobQueueThreaded = tmp2;

        m_outMessageMutex.unlock();

        while (!m_pOutQueueThreaded->empty())
        {
            const std::string& msg = m_pOutQueueThreaded->front();
            Log::info("request_out") << "Sending msg:\n" << msg << Log::end;

            if (sendOut(msg))
            {
                Log::info("request_out") << "Request successful" << Log::end;
            }
            else
            {
                Log::error("request_out") << "Request failed" << Log::end;
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }

            m_pOutQueueThreaded->pop();
        }

        while (!m_pMixedJobQueueThreaded->empty())
        {
            Log::info("spinp_debug.log") << "on mixedJobQueue" << Log::end;

            Job::CJob& rJob = m_pMixedJobQueueThreaded->front();

            std::cout << "HAHA 1" << std::endl;
            Log::info("spinp_debug.log") << "before function" << Log::end;
            std::cout << "HAHA 2" << std::endl;
            rJob.run(this);
            m_pMixedJobQueueThreaded->pop();

            std::cout << "HAHA 3" << std::endl;
            Log::info("spinp_debug.log") << "after function" << Log::end;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    uninitConvert();
    Log::getInstance().uninit();
}

bool CConnector::sendOut(const std::string& _rMessage)
{
    int sndID = getSndID();
    int messageID = getMessageID();
    int_least64_t timestamp = getTimestamp();

    Log::info("out") << "sending message\nsnd: " << sndID << "\nack: " << messageID << "\nmsg: " << _rMessage << Log::end;

    CHttpRequest request;
    request.setSubDomain("hc");
    request.setQuery("in");
    request.setHeader("Origin", "http://hc.spin.de");
    request.setHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setHeader("Referer", m_connReferer);
    request.setPostParameter("sid" , m_pCookie->getCookie("session1"));
    request.setPostParameter("msg" , _rMessage);
    request.setPostParameter("snd" , std::to_string(sndID));
    request.setPostParameter("ack" , std::to_string(messageID));
    request.setPostParameter("ajax", "1");
    request.setPostParameter("ign" , std::to_string(timestamp));

    std::string response;

    if (!request.execSync(&response))
    {
        return false;
    }

    if (response.find("accepted") == std::string::npos)
    {
        return false;
    }

    m_sentSndID         = sndID;
    m_sentMessageID     = messageID;
    m_lastSentTimestamp = timestamp;

    if (_rMessage == "Jp%0A")
    {
        setHearthbeat(false);
    }

    return true;
}

bool CConnector::connect()
{
    CHttpRequest request;
    request.setSubDomain("hc");
    request.setQuery("conn");
    request.setHeader("Cache-Control", "max-age=0");
    request.setHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setHeader("DNT", "1");
    request.setHeader("Referer", m_connReferer);
    request.setHeader("Cookie", m_pCookie->getCookieString());
    request.setPostParameter("ajax", "1");
    request.setPostParameter("sid", m_pCookie->getCookie("session1"));
    request.setPostParameter("ign", std::to_string(getTimestamp()));
    request.setLongPoll(true);
    request.setProgressCallback(progressCallback, this);
    request.setWriteCallback(writeMessage, this);

    if (request.execSync())
    {
        setIsConnected(false);
        setIsConnecting(false);
        return false;
    }

    if (!isShutdown())
    {
        setIsConnected(false);
        setIsConnecting(false);
        m_pBaseConnector->lock();
        m_pBaseConnector->setConnectionState(ConnectionState_Disconnected);
        m_pBaseConnector->unlock();
    }

    return true;
}

void CConnector::pushIn(const char* _pBuffer, size_t _length)
{
    std::string& currentLine = m_in;
    for (size_t index = 0; index < _length; ++ index)
    {
        if (_pBuffer[index] != '\r' &&
            _pBuffer[index] != '\n'    )
        {
            currentLine.push_back(_pBuffer[index]);
        }
        else
        {
            if (currentLine.size() > 0)
            {
                m_inLines.push_back(currentLine);
                currentLine.clear();
            }
        }
    }

    m_inMessageMutex.lock();

    if (m_inLines.size() > 0)
    {
        m_pBaseConnector->lock();
        for (auto line : m_inLines)
        {
            interpretInLine(line);
        }
        m_pBaseConnector->unlock();

        m_inLines.clear();
    }

    m_inMessageMutex.unlock();
}

void CConnector::interpretInLine(const std::string& _rLine)
{
    if (_rLine.length() == 0) return;

    Log::info("conn_interpreter") << "interpreting line: " << _rLine << Log::end;

    const char* pEscapedLine = _rLine.c_str();

    const char* pCurrentChar = &pEscapedLine[0];
    char* pBufferPosition = &m_inLine[0];
    char** pCurrentAnchor = m_pAnchors;

    *pCurrentAnchor = pBufferPosition;
    size_t* pCurrentLength = &m_anchorLengths[0];
    *pCurrentLength = 0;

    *pCurrentAnchor = pBufferPosition;
    size_t numberOfAnchors = 1;

    for (; *pCurrentChar != '\0'; ++ pCurrentChar)
    {
        if (*pCurrentChar == '#')
        {
            ++ pCurrentLength;
            *pCurrentLength = 0;
            ++ pCurrentAnchor;
            ++ numberOfAnchors;
            *pBufferPosition = '\0';
            *pCurrentAnchor = pBufferPosition + 1;
        }
        else if (*pCurrentChar == '%')
        {
            *pBufferPosition = urlDecode(pCurrentChar);
            ++ *pCurrentLength;
            pCurrentChar += 2;
        }
        else
        {
            *pBufferPosition = *pCurrentChar;
            ++ *pCurrentLength;
        }

        ++ pBufferPosition;
    }

    *pBufferPosition = '\0';

    m_numberOfAnchors = numberOfAnchors;
//    char* pCmd = m_pAnchors[0];

    switch (m_pAnchors[0][0])
    {
        case 'T': interpretInLine_T         (); break;
        case '+': interpretInLine_plus      (); break;
        case 'g': interpretInLine_g         (); break;
        case 'h': interpretInLine_h         (); break;
        case 'x': interpretInLine_x         (); break;
        case 'j': interpretInLine_j         (); break;
        case '=': interpretInLine_equals    (); break;
        case '>': interpretInLine_biggerThan(); break;
        case ';': interpretInLine_semicolon (_rLine.length()); break;
        default:
            Log::error("conn_interpreter") << "unhandled cmd m_pAnchors[0][0] == " << m_pAnchors[0][0] << Log::end;
            break;
    }
}


void CConnector::interpretInLine_g()
{
    // gRandomRoomname # RandomUsername # 3 # a # RandomMessage
    // 0                 1                2   3   4

    if (m_numberOfAnchors < 4)
    {
        Log::error("conn_interpreter") << "cmd \"g\" seems to miss some anchors, count == " << m_numberOfAnchors << Log::end;
        return;
    }

    const char* utf8Username = convertCP1252toUTF8(m_pAnchors[1]);

    CLogger::getInstance().logMutexed(
        "chat.log",
        "CRM IN: "
        + std::string(&m_pAnchors[0][1])
        + " " + std::string(m_pAnchors[1])
        + " " + std::string(m_pAnchors[3])
        + " " + std::string(m_pAnchors[4])
    );

    switch (*(m_pAnchors[3]))
    {
        case 'a':
            m_pBaseConnector->chatAddMessage(&m_pAnchors[0][1], utf8Username, *m_pAnchors[3], m_pAnchors[4]);
            break;
        case '0':
            CPurpleConnector::SChatUserState chatUserState;
            chatUserState.m_pRoomName = &m_pAnchors[0][1];
            chatUserState.m_pUsername = utf8Username;
            chatUserState.m_flags = Spin::ChatUserFlag_Away;
            m_pBaseConnector->chatUserStateChange(&chatUserState, 1);
            break;
        default:
            Log::error("conn_interpreter") << "unhandled cmd in \"g\" m_pAnchors[3] == " << *m_pAnchors[3] << Log::end;
//                rLogger.logMutexed("chat.log", "CRM IN: previous message-type unsupported yet");
            m_pBaseConnector->chatAddMessage(&m_pAnchors[0][1], utf8Username, *m_pAnchors[3], m_pAnchors[4]);
            break;
    }

}

void CConnector::interpretInLine_h()
{
    // hRandomUsername # 0 # 256 # 0 # check
    // 0                 1   2     3   4
    // hRandomUsername # 0 # 256 # 0 # keypress # 1
    // 0                 1   2     3   4          5
    // hRandomUsername # 0 # 256 # a # RandomMessage
    // 0                 1   2     3   4

    const char* utf8Username = convertCP1252toUTF8(&m_pAnchors[0][1]);

    if (*m_pAnchors[1] == '0') // in
    {
        if (*m_pAnchors[3] == '0') // some command?
        {
            std::cout << m_pAnchors[4] << std::endl;

            if (strcmp(m_pAnchors[4], "check") == 0) // TODO, not sure, might be a blocked (ignored) user
            {
                Log::error("conn_interpreter") << "unhandled cmd in \"h\" m_pAnchors[4] == " << *m_pAnchors[4]  << Log::end;
            }
            else if (strcmp(m_pAnchors[4], "keypress") == 0)
            {
                CLogger::getInstance().logMutexed(
                    "chat.log",
                    "PC IN: "
                    + std::string(&m_pAnchors[0][1])
                    + " KEYPRESS=" + m_pAnchors[5]);

                m_pBaseConnector->imSetTypingState(utf8Username, m_pAnchors[5][0] == '1' ? Typing_Typing : Typing_Nothing);
            }
            else
            {
                Log::error("conn_interpreter") << "unhandled cmd in \"h\" m_pAnchors[4] == " << *m_pAnchors[4] << Log::end;
            }
        }
        else // a message
        {
            CLogger::getInstance().logMutexed(
                "chat.log",
                "PCM IN: "
                + std::string(m_pAnchors[3]) + " "
                + std::string(&m_pAnchors[0][1]) + " "
                + std::string(m_pAnchors[4])
            );

            m_pBaseConnector->imAddMessage(utf8Username, m_pAnchors[3][0], m_pAnchors[4]);
        }
    }
    else if (*m_pAnchors[1] == '1') // out
    {
        Log::error("conn_interpreter") << "unhandled cmd in \"h\" m_pAnchors[4] == " << *m_pAnchors[4] << Log::end;
    }
    else
    {
        Log::error("conn_interpreter") << "unhandled cmd in \"h\" m_pAnchors[4] == " << *m_pAnchors[4]  << Log::end;
    }
}

void CConnector::interpretInLine_j()
{
    // list of users in the channel at the moment of joining
    // j20%2B#RandomUserName1:3::m::#RandomUserName2:3::m::#RandomUserName3:3::m::#RandomUserName4:3::m::#RandomUserName5:3::m::#RandomUserName6:3::m::#RandomUserName7:3::m::#RandomUserName8:3::m::#RandomUserName9:3::m:p:RandomUserName10:3::m::#RandomUserName11:3::m::#RandomUserName12:3::m::#RandomUserName13:3::m:p#:RandomUserName14:3::m::#RandomUserName15:3::m::#
    // 0      1                      2                     3                       4                      5...

    std::string logMessage = "CRS IN: ";

    const char* pChatRoomName = &m_pAnchors[0][1];

    CBaseConnector::SChatUserState userStates[128];

    logMessage += pChatRoomName;

    struct SChatter
    {
        struct SDetail
        {
            char* m_pName;
            char* m_pUnknown1;
            char* m_pUnknown2;
            char* m_pGender;
            char* m_pUnknown3;
            char* m_pUnknown4;
        };

        union
        {
            SDetail m_detail;
            char* m_pAll[6];
        };
    };

    SChatter chatter;

    for (size_t indexOfAnchors = 1; indexOfAnchors < m_numberOfAnchors - 1; ++ indexOfAnchors)
    {
        logMessage += '\n';

        chatter.m_detail.m_pName = m_pAnchors[indexOfAnchors];

        char* currentChar = m_pAnchors[indexOfAnchors];
        ++ currentChar;

        size_t currentEntry = 0;

        while (*currentChar != '\0')
        {
            if (*currentChar == ':')
            {
                ++ currentEntry;
                *currentChar = '\0';

                chatter.m_pAll[currentEntry] = currentChar + 1;
            }

            ++ currentChar;
        }

        for (size_t index = 0; index < 6; ++ index)
        {
            logMessage += chatter.m_pAll[index] + std::string(",");
        }

        logMessage += ";";

        CBaseConnector::SChatUserState& rCurrentUserState = userStates[indexOfAnchors - 1];

        rCurrentUserState.m_pRoomName = pChatRoomName;

        const char* utf8Username = convertCP1252toUTF8(m_pAnchors[indexOfAnchors]);
        char* username = new char[strlen(utf8Username) + 1];
        strcpy(username, utf8Username);
        username[strlen(utf8Username)] = 0;

        rCurrentUserState.m_pUsername = username;
        rCurrentUserState.m_flags     = static_cast<EChatUserFlags>(0);

        rCurrentUserState.m_flags |= ChatUserFlag_Joining;

             if (*chatter.m_detail.m_pGender == 'm') rCurrentUserState.m_flags |= ChatUserFlag_Male;
        else if (*chatter.m_detail.m_pGender == 'f') rCurrentUserState.m_flags |= ChatUserFlag_Female;

        CLogger::getInstance().logMutexed(
            "chat.log",
            "index=" + std::to_string(indexOfAnchors) + "\n" +
            "flags=" + std::to_string(static_cast<int>(rCurrentUserState.m_flags)) + "\n" +
            "username=" + rCurrentUserState.m_pUsername + "\n" +
            "roomname=" + rCurrentUserState.m_pRoomName + "\n"
        );
    }

    CLogger::getInstance().logMutexed(
        "chat.log",
        "numberofAnchors=" + std::to_string(m_numberOfAnchors)
    );

    m_pBaseConnector->chatJoin(pChatRoomName);
    m_pBaseConnector->chatUserStateChange(&userStates[0], m_numberOfAnchors - 2);

    for (size_t indexOfAnchors = 1; indexOfAnchors < m_numberOfAnchors - 1; ++ indexOfAnchors)
    {
        CBaseConnector::SChatUserState& rCurrentUserState = userStates[indexOfAnchors - 1];
        delete[] rCurrentUserState.m_pUsername;
    }
}

void CConnector::interpretInLine_T()
{
    // T#s#Number -> ack
    // 0 1 2
    if (*m_pAnchors[1] == 's')
    {
        setMessageID(atoi(m_pAnchors[2]));
        Log::info("conn_interpreter") << "setting ack to " << getMessageID() << Log::end;
    }
    // T#c# -> snd
    else if (*m_pAnchors[1] == 'c')
    {
        int snd = atoi(m_pAnchors[2]);

        if (getSndID() != snd)
        {
            std::cout << snd << std::endl;
        }
        setSndID(snd + 1);
        Log::info("conn_interpreter") << "setting snd to " << getSndID() << Log::end;
    }
    else
    {
        Log::error("conn_interpreter") << "unhandled cmd in \"T\" m_pAnchors[1] == " << *m_pAnchors[1] << Log::end;
    }
}

void CConnector::interpretInLine_x()
{
    // xUsername
    // user is offline
    m_pBaseConnector->imAddStatusMessage(&m_pAnchors[0][1], Spin::IMSystemFlag_Offline);
}

void CConnector::interpretInLine_plus()
{
    // [RandomUsername1 verlässt den Raum.]
    // %2B20%2B#A#RandomUsername1#-#3#:f::##
    // 0       1 2            3 4 5    67

    // [RandomUsername2 verlässt den Raum.]
    // %2B20%2B#A#RandomUsername2#-#3#:m::##
    // 0        1 2           3 4 5    67

    // [RandomUsername3 verlässt den Raum (Logout).]
    // %2B20%2B#R#RandomUsername3#-#3#:m::##
    // 0        1 2               3 4 5    67

    // [RandomUsername4 verlässt den Raum (Logout).]
    // %2B20%2B#Q#RandomUsername4#-#3#::p:##
    // 0        1 2       3 4 5    67

    // [RandomUsername5 betritt den Raum.]
    // %2B20%2B#a#RandomUsername5#-#3#:m::##
    // 0        1 2               3 4 5    67

    if (m_numberOfAnchors < 7)
    {
        Log::error("conn_interpreter") << "cmd \"+\" seems to miss some anchors, count == " << m_numberOfAnchors << Log::end;
        return;
    }

    std::string io;

    CBaseConnector::SChatUserState userState;

    const char* utf8Username = convertCP1252toUTF8(m_pAnchors[2]);

    userState.m_pRoomName = &m_pAnchors[0][1];
    userState.m_pUsername = utf8Username;
    userState.m_flags     = ChatUserFlag_None;

    switch (*m_pAnchors[1])
    {
        case 'a':
            userState.m_flags |= ChatUserFlag_Joining;
            userState.m_flags |= ChatUserFlag_Announce;
            io = "JOIN";
            break;
        case 'A':
            userState.m_flags |= ChatUserFlag_Leaving;
            io = "LEAVE";
            break;
        case 'R':
            userState.m_flags |= ChatUserFlag_Leaving;
            userState.m_flags |= ChatUserFlag_Disconnected;
            io = "LEAVE R";
            break;
        case 'Q':
            userState.m_flags |= ChatUserFlag_Leaving;
            userState.m_flags |= ChatUserFlag_Logout;
            io = "LEAVE Q";
            break;
        case 'L':
            userState.m_flags |= ChatUserFlag_Leaving;
            userState.m_flags |= ChatUserFlag_IdleKick;
            io = "LEAVE IDLEKICK";
            break;
        case 'B':
            userState.m_flags |= ChatUserFlag_Leaving;
            userState.m_flags |= ChatUserFlag_KickedByOp;
            io = "LEAVE KICK BY OP";
            break;
        case 'D':
            // can't join room because of ban/tempban
            Log::error("conn_interpreter") << "unhandled cmd in \"+\" m_pAnchors[1] == " << *m_pAnchors[1] << Log::end;
            io = "D";
            break;
        default:
            Log::error("conn_interpreter") << "unhandled cmd in \"+\" m_pAnchors[1] == " << *m_pAnchors[1] << Log::end;
            io = "unknown";
            break;
    }

    if (((userState.m_flags & ChatUserFlag_Leaving) == ChatUserFlag_Leaving) && strcasecmp(userState.m_pUsername, m_pUsername) == 0)
    {
        m_pBaseConnector->chatLeave(userState.m_pRoomName, &userState);
    }

    Log::info("conn_interpreter") << "chat user state change; "
                                  << std::string(&m_pAnchors[0][1]) << " "
                                  << io << " "
                                  << std::string(m_pAnchors[2]) << " "
                                  << std::string(m_pAnchors[5]) << " "
                                  << userState.m_flags << Log::end;

    m_pBaseConnector->chatUserStateChange(&userState, 1);
}

void CConnector::interpretInLine_equals()
{
    // %3Dh#RandomUsername
    // user logs out

    // %3Dg#RandomUsername
    // user logs in

    const char* utf8Username = convertCP1252toUTF8(m_pAnchors[1]);

    CBaseConnector::SBuddyState buddyState;
    buddyState.m_name  = utf8Username;
    buddyState.m_flags = BuddyFlag_None;

    switch (m_pAnchors[0][1])
    {
        case 'h':
            buddyState.m_flags |= BuddyFlag_Offline;
            m_pBaseConnector->buddyStateChange(&buddyState);
            CLogger::getInstance().logMutexed("chat.log", "B: " + buddyState.m_name + " goes offline");
            break;
        case 'g':
            buddyState.m_flags |= BuddyFlag_Online;
            m_pBaseConnector->buddyStateChange(&buddyState);
            CLogger::getInstance().logMutexed("chat.log", "B: " + buddyState.m_name + " comes online");
            break;
        case 'k':
            Log::error("conn_interpreter") << "unhandled cmd in \"=\" m_pAnchors[0][1] == " << m_pAnchors[0][1] << Log::end;
            // TODO (maybe reconnect)
            break;
        default:
            Log::error("conn_interpreter") << "unhandled cmd in \"=\" m_pAnchors[0][1] == " << m_pAnchors[0][1] << Log::end;
            break;
    }
}

void CConnector::interpretInLine_biggerThan()
{
    switch (m_pAnchors[0][1])
    {
        case 'q':
            // Incoming Mail
            //    SETTING ACK: 85
            //    LINE: %3Eq#RandomUsername#user#8235908#1#
            //    LINE: T#s#86
            //    SETTING ACK: 86
            //    LINE: %3Ep#1
            //    LINE: T#s#87
            //    SETTING ACK: 87
            //    LINE: %3Eq#RandomUsername#user#8235908#1#
            //    LINE: T#s#88

            Log::error("conn_interpreter") << "unhandled cmd in \">\" m_pAnchors[0][1] == " << m_pAnchors[0][1] << Log::end;
            break;
        case 'l':
            {
                // if Buddy removed you (without letting notify you) or buddy authorized your buddy-request
                // refresh buddy-list
                pushJob(Job::SRefreshBuddyList());
            }
            break;
        case 'e':
            {
                // ISO-8859-15
                const char* utf8Username = convertCP1252toUTF8(m_pAnchors[1]);

                //            size_t index = 0;
                //            while (m_pAnchors[1][index] != '\0')
                //            {
                //                std::cout << m_pAnchors[1][index] << " " << +m_pAnchors[1][index] << std::endl;
                //                ++ index;
                //            }
                //
                //            index = 0;
                //            while (utf8Username[index] != '\0')
                //            {
                //                std::cout << utf8Username[index] << " " << +utf8Username[index] << std::endl;
                //                ++ index;
                //            }

                CBaseConnector::SBuddyState buddyState;
                buddyState.m_name = utf8Username;
                buddyState.m_flags = BuddyFlag_AskAuth;

                m_pBaseConnector->buddyStateChange(&buddyState);
                // https://github.com/felipec/msn-pecan/blob/master/msn.c#L1428
                // buddy-request
                // LINE: %3Ee#RandomUsername

                //        <a rel="nofollow" class="button" data-action="accept" data-request-id="36323752" data-request-type="0" data-request-who="RandomUsername" href="/request/accept?id=36323752;secret=8293a672b93768ef23865951a9523687">Freundschaft&nbsp;annehmen</a>
                //        id:36323752;secret=8293a672b93768ef23865951a9523687;private=1;familiar=0
            }
            break;
        default:
            Log::error("conn_interpreter") << "unhandled cmd in \">\" pCmd[1] == " << m_pAnchors[0][1]  << Log::end;
            break;
    }
}

void CConnector::interpretInLine_semicolon(size_t _lineLength)
{
    ++ m_semicolonCount;

    if (_lineLength > 4094)
    {
        setIsConnected(true);

        Message::SHearthbeat heartbeat;
        Message::SInitConnection initConnection;

        initConnection.m_username = "Raum";
        initConnection.m_sid      = m_pCookie->getCookie("session1");
        initConnection.m_version  = m_version;

        pushOut(initConnection);
        pushOut2("u%23%23#%0Au%23e%23");
        pushOut(heartbeat);
        pushJob(Job::SLoadLoggedInPage());

        m_pBaseConnector->setConnectionState(ConnectionState_Connected);
    }
}

void CConnector::execHomeRequest(bool _sync, CConnector::SHomeRequestResult& _rResult)
{
    Log::info("request_home") << "on exec home request" << Log::end;

    Spin::CHttpRequest httpRequestHome;
    size_t pos, endPos;
    std::string resultHome;
    SHomeRequestResult::SBuddyRequest buddyRequest;

    httpRequestHome.setFollow(true);
    httpRequestHome.setQuery("home");
    httpRequestHome.setHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    httpRequestHome.setHeader("Accept-Encoding", "");
    httpRequestHome.setHeader("Accept-Language", "de-DE,de;q=0.8,en-US;q=0.6,en;q=0.4");
    httpRequestHome.setHeader("Cache-Control", "no-cache");
    httpRequestHome.setHeader("Pragma", "no-cache");
    httpRequestHome.setHeader("Referer", "http://www.spin.de/loggedin");
    httpRequestHome.setHeader("Upgrade-Insecure-Requests", "1");

    Log::info("request_home") << "before sending home request" << Log::end;
    if (!httpRequestHome.execSync(&resultHome)) //  !execSyncHttpRequest(&httpRequestHome, resultHome))
    {
        Log::error("request_home") << "sending of home request failed" << Log::end;
        _rResult.m_success = false;
        return;
    }

    _rResult.m_success = true;
    Log::info("request_home") << "after sending home request" << Log::end;

    pos = 0;

    // <a rel="nofollow" class="button" data-action="accept" data-request-id="34637634" data-request-type="0" data-request-who="RandomUsername" href="/request/accept?id=34637634;secret=2436572836a89a5t26e35879dc623a958">Freundschaft&nbsp;annehmen</a>
    // <a rel="nofollow" class="button" data-action="reject" data-request-id="34637634" data-request-type="0" href="/request/reject?id=34637634;secret=2436572836a89a5t26e35879dc623a958">Ignorieren</a>

    Log::info("request_home") << "searching for buddy-requests" << Log::end;
    for (;;)
    {
        pos = resultHome.find("data-request-who=\"", pos);

        if (pos == std::string::npos)
        {
            Log::warn("request_home") << "couldn't find find any(more) buddy-request" << Log::end;
            break;
        }

        pos += 18;
        endPos = resultHome.find("\"", pos);

        buddyRequest.m_buddyName = Spin::convertCP1252toUTF8(resultHome.substr(pos, endPos - pos));

        if (pos + 40 < endPos)
        {
            Log::error("request_home") << "pos is " << pos << " and endpos is " << endPos << " that's kind of a fail." << Log::end;
            continue;
        }

        pos = resultHome.find("href=\"", endPos) + 6;

        if (pos == std::string::npos)
        {
            Log::error("request_home") << "couldn't find href for user " << buddyRequest.m_buddyName << Log::end;
            continue;
        }

        pos = resultHome.find("?id=", pos) + 4;

        if (pos == std::string::npos)
        {
            Log::error("request_home") << "couldn't find ?id= for user " << buddyRequest.m_buddyName << Log::end;
            continue;
        }

        endPos = resultHome.find("\"", pos);

        if (endPos == std::string::npos)
        {
            Log::error("request_home") << "couldn't find ending \"" << Log::end;
            continue;
        }

        buddyRequest.m_id = resultHome.substr(pos, endPos - pos);

        Log::info("request_home") << "found id=" << buddyRequest.m_id << " name=" << buddyRequest.m_buddyName << Log::end;

        _rResult.m_buddyRequests.push_back(buddyRequest);
    }
}

void CConnector::setIsConnected(bool _isConnected)
{
    m_isConnected = _isConnected;

    if (_isConnected)
    {
        m_isConnectedCV.notify_one();
        m_isConnecting = false;
    }
}

void CConnector::setIsConnecting(bool _isConnecting)
{
    m_isConnecting = _isConnecting;

    if (_isConnecting)
    {
        m_isConnected = false;
    }
}

bool CConnector::isConnected() const
{
    return m_isConnected;
}

bool CConnector::isConnecting() const
{
    return m_isConnecting;
}

void CConnector::setShutdown(bool _shutdown)
{
    m_shutdown.store(_shutdown);
}

bool CConnector::isShutdown() const
{
    return m_shutdown;
}

int_least64_t CConnector::getTimestamp() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


int CConnector::getMessageID()
{
    return m_messageID;
}

int CConnector::getSndID()
{
    return m_sndID;
}

void CConnector::setMessageID(int _id)
{
    m_messageID = _id;
}

void CConnector::setSndID(int _id)
{
    m_sndID = _id;
}

int_least64_t CConnector::getLastSentTimestamp() const
{
    return m_lastSentTimestamp;
}

bool CConnector::isHeartbeating()
{
    return m_heartBeats;
}

void CConnector::setHearthbeat(bool _state)
{
    m_heartBeats = _state;
}

void CConnector::setBaseConnector(CBaseConnector* _pBaseConnector)
{
    assert(_pBaseConnector != nullptr);
    m_pBaseConnector = _pBaseConnector;
}

CBaseConnector* CConnector::getBaseConnector()
{
    assert(m_pBaseConnector != nullptr);
    return m_pBaseConnector;
}

const char* CConnector::getSecret() const
{
    return &m_secret[0];
}

const std::string& CConnector::getCookie()
{
    return m_pCookie->getCookieString();
}

const std::string CConnector::getCookie(const std::string& _rString)
{
    return m_pCookie->getCookie(_rString);
}

void CConnector::setUserInfo(const std::string& _rUserName, SUserInfo& _rUserInfo)
{
    auto userInfoIt = m_userInfos.find(_rUserName);
//    bool requestImage;

    // FIXME Icon will always loaded on restarting pidgin

    if (userInfoIt == m_userInfos.cend())
    {
        m_userInfos.emplace(std::make_pair(_rUserName, _rUserInfo));
        userInfoIt = m_userInfos.find(_rUserName);
//        requestImage = true;
    }
    else
    {
//        requestImage = userInfoIt->second.m_thumbURL != _rUserInfo.m_thumbURL;
        userInfoIt->second = _rUserInfo;
    }

    SUserInfo& rUserInfo = userInfoIt->second;

//    if (rUserInfo.m_registered)
//    {
//        if (requestImage && rUserInfo.m_thumbURL.length() > 0)
//        {
//            Job::SRequestBuddyImage* pJob = new Job::SRequestBuddyImage();
//            pJob->m_userName = _rUserName;
//            pJob->m_url      = rUserInfo.m_thumbURL;
//            Spin::pushJob(this, pJob);
//        }
//    }
//    else
    if (!rUserInfo.m_registered)
    {
        m_pBaseConnector->lock();
        m_pBaseConnector->imAddStatusMessage(_rUserName.c_str(), Spin::IMSystemFlag_NotRegistered);
        m_pBaseConnector->unlock();
    }
}

unsigned int CConnector::getSemicolonCount()
{
    return m_semicolonCount.load();
}

void CConnector::pushOut2(std::string&& _rMessage)
{
    m_outMessageMutex.lock();
    m_pOutQueue->push(std::move(_rMessage));
    m_outMessageMutex.unlock();
}

void CConnector::setSecret(const char* _pSecret)
{
    // TODO this could interfere with other threads... maybe a fix :D

    assert(strlen(_pSecret) <= c_secretMaxLength);
    strcpy(m_secret, _pSecret);
}

}