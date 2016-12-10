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
#include "job.h"

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
#include <unordered_map>

#ifdef __MINGW32__
#include <mingw.condition_variable.h>
#endif

namespace Spin
{

class CBaseConnector;

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

    struct SUserInfo
    {
        SUserInfo()
            : m_buddyName ("")
            , m_id        ("")
            , m_thumbURL  ("")
            , m_photoURL  ("")
            , m_age       (0)
            , m_gender    (0)
            , m_registered(false)
            , m_noFake    (false)
            , m_underage  (false)
            , m_isValid   (false)
        {

        }

        std::string m_buddyName;
        std::string m_id;
        std::string m_thumbURL;
        std::string m_photoURL;
        char        m_age;
        char        m_gender;
        bool        m_registered;
        bool        m_noFake;
        bool        m_underage;
        bool        m_isValid;
    };

//    enum EInternJobType
//    {
//        InternJob_RefreshBuddies,
//        InternJob_RequestBuddyImage,
//    };

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

    template <typename T>
    void pushOut(T&& _rMessageDescriptor);

    void pushOut2(std::string&& _rMessage);

    template <typename T>
    void pushJob(T&& _rrJobDescriptor);
    template <typename T>
    void pushJob(const T& _rrJobDescriptor);

    void execHomeRequest(bool _sync, CConnector::SHomeRequestResult&);

    bool sendOut(const std::string& _rMessage);

    void interpretInLine(const std::string& _rLine);
    void interpretInLine_g();
    void interpretInLine_h();
    void interpretInLine_j();
    void interpretInLine_T();
    void interpretInLine_x();
    void interpretInLine_plus();
    void interpretInLine_equals();
    void interpretInLine_biggerThan();
    void interpretInLine_semicolon(size_t _lineLength);


    bool isHeartbeating();
    void setHearthbeat(bool _state);

    void setBaseConnector(CBaseConnector* _pBaseConnector);
    CBaseConnector* getBaseConnector();
    const char* getSecret() const;

    const std::string getCookie(const std::string& _rString);
    void setUserInfo(const std::string& _rUserName, SUserInfo& _rUserInfo);
//    void updateUserInfo(const std::string& _rUserName, const std::string& _rBuddyImageHash);

    unsigned int getSemicolonCount();

private:
    bool login();

private:
    int getSndID();
    int getMessageID();
    void setSndID(int _id);
    void setMessageID(int _id);


    std::string m_response;

private:
    typedef std::queue<std::string>    TMessageQueue;
    typedef std::queue<Job::CJob>      TMixedJobQueue;

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
//    TInternJobQueue* m_pInternJobQueue;

    TMessageQueue*  m_pOutQueueThreaded;
    TMixedJobQueue* m_pMixedJobQueueThreaded;
//    TInternJobQueue* m_pInternJobQueueThreaded;

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
    size_t m_numberOfAnchors;

    char m_secret[c_secretMaxLength + 1];

    std::unordered_map<std::string, SUserInfo> m_userInfos;

};

template <typename T>
void CConnector::pushJob(T&& _rrJobDescriptor)
{
    static_assert(std::is_base_of<Spin::Job::SJob, T>::value, "T must inherit from SJob");
//    assert(_pJob != nullptr);

//    Log::info("sys") << "locking outMessageMutex" << Log::end;
    m_outMessageMutex.lock();
//    Log::info("sys") << "locked outMessageMutex" << Log::end;

//    if (_pJob->m_type == Job::JobType_Mixed)
//    {
//        Log::info("sys") << "push MixedJob" << Log::end;
//        m_pMixedJobQueue->push(_pJob);
//    }

    m_pMixedJobQueue->emplace(std::forward<T>(_rrJobDescriptor));

//    Log::info("sys") << "unlocking outMessageMutex" << Log::end;
    m_outMessageMutex.unlock();
//    Log::info("sys") << "unlocked outMessageMutex" << Log::end;
}

template <typename T>
void CConnector::pushJob(const T& _rJobDescriptor)
{
    static_assert(std::is_base_of<Spin::Job::SJob, T>::value, "T must inherit from SJob");
//    assert(_pJob != nullptr);

//    Log::info("sys") << "locking outMessageMutex" << Log::end;
    m_outMessageMutex.lock();
//    Log::info("sys") << "locked outMessageMutex" << Log::end;

//    if (_pJob->m_type == Job::JobType_Mixed)
//    {
//        Log::info("sys") << "push MixedJob" << Log::end;
//        m_pMixedJobQueue->push(_pJob);
//    }

    m_pMixedJobQueue->emplace(_rJobDescriptor);

//    Log::info("sys") << "unlocking outMessageMutex" << Log::end;
    m_outMessageMutex.unlock();
//    Log::info("sys") << "unlocked outMessageMutex" << Log::end;
}

template <typename T>
void CConnector::pushOut(T&& _rMessageDescriptor)
{
    // this fails for some reason :(
//    static_assert(std::is_base_of<Spin::Message::SOutJob, T>::value, "T must inherit from SOutJob");
    m_outMessageMutex.lock();
    m_pOutQueue->push(_rMessageDescriptor.create());
    m_outMessageMutex.unlock();
}

}

#endif // SPINTEST_CONNTECTION_H
