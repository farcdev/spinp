/**
 * connector.h
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


#ifndef SPINTEST_CONNTECTION_H
#define SPINTEST_CONNTECTION_H

#include "cookie.h"
#include "base_connector.h"
#include "http_request.h"
#include "message.h"

#include <string>
#include <thread>
#ifdef __MINGW32__
#include <mingw.thread.h>
#endif
#include <vector>
#include <mutex>
#ifdef __MINGW32__
#include <mingw.mutex.h>
#endif
#include <atomic>
#include <queue>
#include <condition_variable>
#ifdef __MINGW32__
#include <mingw.condition_variable.h>
#endif

namespace Spin
{

class CBaseConnector;

namespace Job
{

struct SBaseJob;
struct SMixedJob;

}

class CConnector
{
public:
    struct SHomeRequestResult
    {
        struct SBuddyRequest
        {
            std::string m_buddyName;
            std::string m_id;
        };

        bool m_success;
        std::vector<SBuddyRequest> m_buddyRequests;
    };

    enum EInternJobType
    {
        InternJob_RefreshBuddies
    };

public:
    CConnector(CBaseConnector* _pBaseConnector);
//    CConnector(CCookie& _rCookie, CBaseConnector* _pBaseConnector);
    ~CConnector();

    void setLogin(const char* _pUsername, const char* _pPassword, unsigned short _port);

    bool start();


    bool initConnection();
    bool initLoggedIn();
    void runConnector();
    void runOut();

    int_least64_t getTimestamp() const;

    bool connect();
    bool reconnect();

    bool isConnecting() const;
    bool isConnected() const;
    void setShutdown(bool _shutdown);
    bool isShutdown() const;

    int_least64_t getLastSentTimestamp() const;

    void pushIn(const char* _pBuffer, size_t _length);
    void pushOut(const std::string& _rMessage);
    void pushJob(Job::SBaseJob* _pJob);
    void pushOut(std::string&& _rrMessage);

    void execHomeRequest(bool _sync, CConnector::SHomeRequestResult&);

    bool sendOut(const std::string& _rMessage);

    void interpretInLine(const std::string& _rLine);
    void interpretInLine_j(size_t _numberOfAnchors);

    bool isHeartbeating();
    void setHearthbeat(bool _state);

    void setBaseConnector(CBaseConnector* _pBaseConnector);
    CBaseConnector* getBaseConnector();
    const char* getSecret() const;

    const std::string getCookie(const std::string& _rString);

    unsigned int getSemicolonCount();

private:
    bool login();
    void requestBuddyListUpdate();

private:
    int getSndID();
    int getMessageID();
    void setSndID(int _id);
    void setMessageID(int _id);


    std::string m_response;

private:
    typedef std::queue<std::string>    TMessageQueue;
    typedef std::queue<void*>          TMixedJobQueue;
    typedef std::queue<EInternJobType> TInternJobQueue;

    char*          m_pUsername;
    char*          m_pPassword;
    unsigned short m_port;

    CCookie* m_pCookie;
    CBaseConnector* m_pBaseConnector;

    std::thread* m_pInThread;
    std::thread* m_pOutThread;
    std::mutex   m_inMessageMutex;
    std::mutex   m_outMessageMutex;
    std::mutex   m_mixedJobMutex;
    std::condition_variable m_hasMessages;

    std::string m_connReferer;
    std::string m_in;

    TMessageQueue*  m_pOutQueue;
    TMixedJobQueue* m_pMixedJobQueue;
    TInternJobQueue* m_pInternJobQueue;

    TMessageQueue*  m_pOutQueueThreaded;
    TMixedJobQueue* m_pMixedJobQueueThreaded;
    TInternJobQueue* m_pInternJobQueueThreaded;

    std::atomic_bool m_isConnected;
    std::atomic_bool m_isConnecting;
    std::atomic_bool m_heartBeats;
    std::atomic_uint_least64_t  m_lastSentTimestamp;

    int m_messageID;
    int m_sndID;

    int m_sentMessageID;
    int m_sentSndID;

    std::vector<std::string> m_inLines;

    std::atomic_uint m_semicolonCount;

    std::string m_version;

    std::atomic_bool m_shutdown;

    static const size_t c_maxInLine = 8192;
    static const size_t c_maxAnchors = 256;
    static const size_t c_secretMaxLength = 64;

    char m_inLine[c_maxInLine];
    size_t m_anchorLengths[c_maxAnchors];
    char* m_pAnchors[c_maxAnchors];

    char m_secret[c_secretMaxLength + 1];

};

}

#endif // SPINTEST_CONNTECTION_H
