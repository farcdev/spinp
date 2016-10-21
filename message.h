/**
 * messsage.h
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


#ifndef SPINTEST_MESSAGE_H
#define SPINTEST_MESSAGE_H

#include <string>
#include "connector.h"

namespace Spin
{

class CConnector;

namespace Job
{

enum EJobType
{
    JobType_Uninitialized = -1,
    JobType_OutRequest        ,
    JobType_Mixed             ,
};

enum EJobOutType
{
    JobOutType_Uninitialized  = -1,
    JobOutType_InitConnection = 0,
    JobOutType_Heartbeat      = 1,
    JobOutType_IMMessage      = 2,
    JobOutType_JoinChatRoom   = 3,
    JobOutType_LeaveChatRoom  = 4,
    JobOutType_ChatMessage    = 5,
    JobOutType_IMTyping       = 6,
};

enum EJobMixedType
{
    JobMixedType_Uninitialized = -1,
    JobMixedType_AcceptAuth  ,
    JobMixedType_DeclineAuth ,
    JobMixedType_AskAuthBuddy,
    JobMixedType_GetRoomList ,
};

enum EChatMessageType
{
    Normal,
    Action,
    Echo,
};

struct SBaseJob
{
    SBaseJob()
        : m_type(JobType_Uninitialized)
    {

    }

    SBaseJob(EJobType _type)
        : m_type(_type)
    {

    }

    const EJobType    m_type;
};

struct SOutJob : SBaseJob
{
    SOutJob()
        : SBaseJob ()
        , m_outType(JobOutType_Uninitialized)
    {

    }

    SOutJob(EJobOutType _outType)
        : SBaseJob (JobType_OutRequest)
        , m_outType(_outType)
    {

    }

    const EJobOutType m_outType = JobOutType_Uninitialized;
};


struct SInitConnection final : public SOutJob
{
    SInitConnection()
        : SOutJob(JobOutType_InitConnection)
    {

    }

    std::string m_version;
    std::string m_username;
    std::string m_sid;
};

struct SHearthbeat final : public SOutJob
{
    SHearthbeat()
        : SOutJob(JobOutType_Heartbeat)
    {

    }
};

struct SIMMessage final : public SOutJob
{
    SIMMessage()
        : SOutJob(JobOutType_IMMessage)
    {

    }

    std::string m_user;
    std::string m_message;
    EChatMessageType m_messageType;
};

struct SIMTyping final : public SOutJob
{
    SIMTyping()
        : SOutJob(JobOutType_IMTyping)
    {

    }

    std::string m_user;
    ETypingState m_state;
};

struct SJoinChatRoom final : public SOutJob
{
    SJoinChatRoom()
        : SOutJob(JobOutType_JoinChatRoom)
    {

    }

    std::string m_room;
};

struct SLeaveChatRoom final : public SOutJob
{
    SLeaveChatRoom()
        : SOutJob(JobOutType_LeaveChatRoom)
    {

    }

    std::string m_room;
};

struct SChatMessage final : public SOutJob
{
    SChatMessage()
        : SOutJob(JobOutType_ChatMessage)
    {

    }

    std::string m_room;
    std::string m_message;
    EChatMessageType m_messageType;
};

struct SMixedJob : public SBaseJob
{
    SMixedJob()
        : SBaseJob   ()
        , m_mixedType(JobMixedType_Uninitialized)
    {

    }

    SMixedJob(EJobMixedType _mixedType)
        : SBaseJob   (JobType_Mixed)
        , m_mixedType(_mixedType)
    {

    }

    bool runJob(CConnector* _pConnector);

    typedef bool (*TFunction)(void* _pData, CConnector* _pConnector);

    const EJobMixedType m_mixedType;
};

struct SAuthAcceptBuddy : public SMixedJob
{
    SAuthAcceptBuddy()
        : SMixedJob(JobMixedType_AcceptAuth)
    {

    }

    std::string m_userName;
};

struct SAuthDeclineBuddy : public SMixedJob
{
    SAuthDeclineBuddy()
        : SMixedJob(JobMixedType_DeclineAuth)
    {

    }

    std::string m_userName;
};

struct SAskAuthBuddy : public SMixedJob
{
    SAskAuthBuddy()
        : SMixedJob(JobMixedType_AskAuthBuddy)
    {

    }

    std::string m_userName;
    std::string m_message;
};

struct SGetRoomList : public SMixedJob
{
    SGetRoomList()
        : SMixedJob(JobMixedType_GetRoomList)
    {

    }
};

}

//std::string createMessage(Job::EJobType _type, void* _pDescriptor);
void pushJob(CConnector* _pConnector, Job::SBaseJob* _pDescriptor);

}

#endif // SPINTEST_MESSAGE_H
