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
#include "base_connector.h"

namespace Spin
{

class CConnector;

namespace Message
{

enum EChatMessageType
{
    ChatMessageType_Normal,
    ChatMessageType_Action,
    ChatMessageType_Echo,
};

struct SOutJob
{
    virtual ~SOutJob()
    { }

private:
    virtual std::string create() = 0;

    friend class Spin::CConnector;
};


struct SInitConnection final : public SOutJob
{
    std::string m_version;
    std::string m_username;
    std::string m_sid;

private:
    virtual std::string create();
    friend class Spin::CConnector;
};

struct SHearthbeat final : public SOutJob
{
private:
    virtual std::string create();
    friend class Spin::CConnector;
};

struct SIMMessage final : public SOutJob
{
    std::string m_user;
    std::string m_message;
    EChatMessageType m_messageType;

private:
    virtual std::string create();
    friend class Spin::CConnector;
};

struct SIMInit final : public SOutJob
{
    std::string m_userName;

private:
    virtual std::string create();
    friend class Spin::CConnector;
};

struct SIMTyping final : public SOutJob
{
    std::string m_user;
    ETypingState m_state;

private:
    virtual std::string create();
    friend class Spin::CConnector;
};

struct SJoinChatRoom final : public SOutJob
{
    std::string m_room;

private:
    virtual std::string create();
    friend class Spin::CConnector;
};

struct SLeaveChatRoom final : public SOutJob
{
    std::string m_room;

private:
    virtual std::string create();
    friend class Spin::CConnector;
};

struct SChatMessage final : public SOutJob
{
    std::string m_room;
    std::string m_message;
    EChatMessageType m_messageType;

private:
    virtual std::string create();
    friend class Spin::CConnector;
};


}

}

#endif // SPINTEST_MESSAGE_H
