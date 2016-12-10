/**
 * message.cpp
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


#include <algorithm>
#include <assert.h>
#include <iostream>

#include "message.h"
#include "common.h"
#include "logger.h"
#include "convert.h"
#include "base_connector.h"

namespace Spin
{
namespace Message
{

std::string SInitConnection::create()
{
    std::string result;

    replaceAll(m_sid, "-", "%2D");
    replaceAll(m_sid, "_", "%5F");
    replaceAll(m_version, "-", "%2D");

    result = "%7B4%0AAhtmlchat%20" + m_version + "%20ajax%0Aa" + m_username
             + "%23" + m_sid + "%0Au%23%23#%0Au%23e%23%0A";


    return result;
}

std::string SHearthbeat::create()
{
    return "Jp%0A";
}

std::string SIMInit::create()
{
    return "h" + Spin::urlEncode(Spin::convertUTF8toCP1252(m_userName)) + "%23" + "0" + "%23" + "0" + "%23" + "check" +
           "%0A";
}

std::string SIMMessage::create()
{
    char type;
    switch (m_messageType)
    {
        case ChatMessageType_Action:
            type = 'c';
            break;
        case ChatMessageType_Echo:
            type = 'e';
            break;
        default:
        case ChatMessageType_Normal:
            type = 'a';
    }

    // TODO urlencode
    return "h" + Spin::urlEncode(Spin::convertUTF8toCP1252(m_user)) + "%23" + "1" + "%23" + type + "%23" +
           Spin::urlEncode(m_message) + "%0A";
}

std::string SIMTyping::create()
{
    char state;

    switch (m_state)
    {
        case Spin::Typing_Typing:
            state = '1';
            break;
        default:
        case Spin::Typing_Nothing:
            state = '0';
            break;
    }

    return "h" + Spin::urlEncode(m_user) + "%232%230%23keypress%23" + state + "%0A";
}

std::string SChatMessage::create()
{
    char type;
    switch (m_messageType)
    {
        case ChatMessageType_Action:
            type = 'c';
            break;
        case ChatMessageType_Echo:
            type = 'e';
            break;
        default:
        case ChatMessageType_Normal:
            type = 'a';
    }

    // TODO urlencode
    return "g" + Spin::urlEncode(m_room) + "%23" + type + "%23" + Spin::urlEncode(m_message) + "%0A";
}

std::string SJoinChatRoom::create()
{
    std::string room = Spin::urlEncode(m_room);

    return "c" + room + "%0A";
}

std::string SLeaveChatRoom::create()
{
    std::string room = Spin::urlEncode(m_room);

    return "d" + room + "%0A";
}

} // namespace Message
} // namespace Spin
