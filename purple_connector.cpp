/**
 * purple_connector.cpp
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

#include "purple_connector.h"
#include "connector.h"
#include "common.h"

#ifdef CMAKE
#include <libpurple/conversation.h>
#include <libpurple/debug.h>
#else
#include <conversation.h>
#include <debug.h>
#endif

#include <string.h>
#include <assert.h>
#include <iostream>
#include <algorithm>

namespace
{

struct SCBInfo
{
    Spin::CConnector* m_pConnector;
    std::string       m_pName;
};

void purpleUserAuthAcceptBuddyCallback(void* _pData)
{
    SCBInfo* pCBInfo = reinterpret_cast<SCBInfo*>(_pData);

    Spin::Job::SAuthAcceptBuddy acceptBuddy;
    acceptBuddy.m_userName = std::move(pCBInfo->m_pName);
//    Spin::Job::SAuthAcceptBuddy* pAcceptBuddy = new Spin::Job::SAuthAcceptBuddy();
//    pAcceptBuddy->m_userName = pCBInfo->m_pName;

//    std::cout << pAcceptBuddy->m_userName << std::endl;
    std::cout << acceptBuddy.m_userName << std::endl;

//    Spin::pushJob(pCBInfo->m_pConnector, acceptBuddy)
    pCBInfo->m_pConnector->pushJob(std::move(acceptBuddy));

    delete pCBInfo;
}

void purpleUserAuthDeclineBuddyCallback(void* _pData)
{
    SCBInfo* pCBInfo = reinterpret_cast<SCBInfo*>(_pData);

//    Spin::Job::SAuthDeclineBuddy* pDeclineBuddy = new Spin::Job::SAuthDeclineBuddy();
//    pDeclineBuddy->m_userName = pCBInfo->m_pName;
//
//    Spin::pushJob(pCBInfo->m_pConnector, pDeclineBuddy);

    Spin::Job::SAuthDeclineBuddy declineBuddy;
    declineBuddy.m_userName = std::move(pCBInfo->m_pName);

//    Spin::pushJob(pCBInfo->m_pConnector, declineBuddy);
    pCBInfo->m_pConnector->pushJob(std::move(declineBuddy));

    delete pCBInfo;
}

}

namespace Spin
{

CPurpleConnector::CPurpleConnector()
    : m_pPluginId        (nullptr)
    , m_pPurpleConnection(nullptr)
    , m_idRoomMap        ()
    , m_roomIdMap        ()
    , m_idSpinIdMap      ()
    , m_spinIdIdMap      ()
    , m_mutex            ()
    , m_connectionState  ()
    , m_buddyListStates  ()
    , m_images           ()
    , m_rooms            ()
    , m_chatMessages     ()
    , m_chatUserStates   ()
    , m_buddyStates      ()
    , m_chatJoin         ()
    , m_chatLeave        ()
    , m_imMessages       ()
    , m_imTyping         ()
    , m_imChatBuddyFlags ()
    , m_userProfiles     ()
    , m_logMessages      ()
    , m_pPurpleRoomList  (nullptr)
    , m_currentId        (0)
{
}

CPurpleConnector::~CPurpleConnector()
{
    Log::write("sys") << "destroying CPurpleConnector" << Log::end;
}

void CPurpleConnector::setPluginId(const char* _pPluginId)
{
    m_pPluginId = _pPluginId;
}

const char* CPurpleConnector::getPluginId()
{
    return m_pPluginId;
}

void CPurpleConnector::setPurpleConnection(PurpleConnection* _pConnection)
{
    m_pPurpleConnection = _pConnection;
}

const char* CPurpleConnector::purpleGetChatRoomName(int _id) const
{
    auto it = m_idRoomMap.find(_id);

    if (it == m_idRoomMap.cend())
    {
        purple_debug_warning(m_pPluginId, "couldn't find chatroom with id: %i\n", _id);

        return nullptr;
    }

    return it->second.c_str();
}

void CPurpleConnector::lock()
{
    m_mutex.lock();
}

void CPurpleConnector::unlock()
{
    m_mutex.unlock();
}

void CPurpleConnector::setConnectionState(EConnectionState _state)
{
    m_connectionState = _state;
}

void CPurpleConnector::buddyListRefresh(std::vector<SBuddyState>& _rBuddies)
{
    _rBuddies.swap(m_buddyListStates);
}

void CPurpleConnector::buddyStateChange(SBuddyState* _pBuddy)
{
    std::string name = _pBuddy->m_name;

    m_buddyStates.push_back({ _pBuddy->m_id, name, _pBuddy->m_flags });
}

void CPurpleConnector::buddyAddImage(const char* _pUsername, void* _pImageData, size_t _imageSizeInBytes, const char* _pChecksum)
{
    char* pUsername = new char[strlen(_pUsername) + 1];
    strcpy(pUsername, _pUsername);

    void* pImageData = new char[_imageSizeInBytes];
    memcpy(pImageData, _pImageData, _imageSizeInBytes);

    char* pChecksum = new char[strlen(_pChecksum) + 1];
    strcpy(pChecksum, _pChecksum);

    m_images.push_back(
        {
            pUsername,
            pImageData,
            _imageSizeInBytes,
            pChecksum
        }
    );
}

void CPurpleConnector::buddySetProfile(const char* _pUserName, const TUserProfile& _rInfos)
{
    TUserProfile copyOfInfos = _rInfos;

    m_userProfiles.emplace_back(std::make_pair(_pUserName, copyOfInfos));
}

void CPurpleConnector::buddySetProfile(const char* _pUserName, TUserProfile&& _rrInfos)
{
    m_userProfiles.emplace_back(std::make_pair(_pUserName, std::move(_rrInfos)));
}

void CPurpleConnector::chatAddMessage(const char* _pRoomName, const char* _pUsername, char _type, const char* _pMessage)
{
    size_t roomNameLength = strlen(_pRoomName);
    size_t usernameLength = strlen(_pUsername);
    size_t messageLength  = strlen(_pMessage );

    char* pString = new char[roomNameLength + usernameLength + messageLength + 3];
    char* pRoomName = &pString[0];
    char* pUsername = pRoomName + roomNameLength + 1;
    char* pMessage  = pUsername + usernameLength + 1;

    strcpy(pRoomName, _pRoomName);
    strcpy(pUsername, _pUsername);
    strcpy(pMessage , _pMessage );

    m_chatMessages.push_back({
        pRoomName,
        pUsername,
        pMessage,
        time(0)
    });
}

void CPurpleConnector::imAddMessage(const char* _pUsername, char _type, const char* _pMessage)
{
    size_t usernameLength = strlen(_pUsername);

    char* pString = new char[usernameLength + strlen(_pMessage) + 2];
    strcpy(pString, _pUsername);
    strcpy(&pString[usernameLength + 1], _pMessage);

    m_imMessages.push_back({
        pString,
        &pString[usernameLength + 1],
        time(0)
    });
}

void CPurpleConnector::imAddStatusMessage(const char* _pUsername, EIMSystemFlag _flags)
{
    size_t usernameLength = strlen(_pUsername);

    char* pString = new char[usernameLength + 1];
    strcpy(pString, _pUsername);

    std::cout << "imADDStatusMessage: " << _pUsername << " " << pString << std::endl;
    std::cout << "imADDStatusMessage: " << _pUsername << " " << pString << std::endl;

    m_imChatBuddyFlags.push_back({
        pString,
        _flags,
        time(0)
    });
}

void CPurpleConnector::imSetTypingState(const char* _pUsername, ETypingState _state)
{
    size_t usernameLength = strlen(_pUsername);

    char* pString = new char[usernameLength + 1];
    strcpy(pString, _pUsername);

    Log::info("im") << "typeing name: " << pString << Log::end;
    CLogger::getInstance().logMutexed("spinp_debug.log", "typing name: " + std::string(pString));

    PurpleTypingState type;

    switch (_state)
    {
        case Typing_Typing:
            Log::write("im", "conversation partner " + std::string(pString) + " is typing");
            type = PURPLE_TYPING;
            break;
        case Typing_StoppedTyping:
            Log::write("im", "conversation partner " + std::string(pString) + " stopped typing");
            type = PURPLE_TYPED;
            break;
        default:
        case Typing_Nothing:
            Log::write("im", "conversation partner " + std::string(pString) + " is not typing");
            type = PURPLE_NOT_TYPING;
            break;
    }

    m_imTyping.push_back({
        pString,
        type
    });
}

void CPurpleConnector::chatSetList(std::vector<SRoom>& _rRooms)
{
    m_rooms = std::move(_rRooms);
}

void CPurpleConnector::chatUserStateChange(const SChatUserState* _pUserStates, size_t _numberOfUserStates)
{
    for (size_t index = 0; index < _numberOfUserStates; ++ index)
    {
        const SChatUserState& rState = _pUserStates[index];

        SChatUserState newState;

        size_t usernameLength = strlen(rState.m_pUsername);

        char* pUsername = new char[usernameLength + 1];
        strcpy(pUsername, rState.m_pUsername);
        newState.m_pUsername = pUsername;

        size_t roomNameLength = strlen(rState.m_pRoomName);
        char* pRoomName = new char[roomNameLength + 1];
        strcpy(pRoomName, rState.m_pRoomName);
        newState.m_pRoomName = pRoomName;

        newState.m_flags     = rState.m_flags;

        m_chatUserStates.push_back(newState);
    }
}

void CPurpleConnector::chatJoin(const char* _pRoomName, const char* _pSpinId)
{
    m_idRoomMap.emplace(std::pair<int, std::string>(m_currentId, _pRoomName));
    m_roomIdMap.emplace(std::pair<std::string, int>(_pRoomName, m_currentId));

    std::string spinId;

    if (_pSpinId != nullptr) spinId.assign(_pSpinId);
    else                     spinId.assign(_pRoomName);

    m_idSpinIdMap.emplace(std::pair<int, std::string>(m_currentId, spinId));
    m_spinIdIdMap.emplace(std::pair<std::string, int>(spinId, m_currentId));

    m_chatJoin.push_back(std::string(_pRoomName));

    ++ m_currentId;
}

void CPurpleConnector::chatLeave(const char* _pRoomName, const SChatUserState* _pType)
{
    m_chatLeave.push_back(std::string(_pRoomName));
}

void CPurpleConnector::purpleOnLoop()
{
    if (m_mutex.try_lock())
    {
        purpleChangeConnectionState();
        purpleBuddyListRefresh();
        purpleBuddyListChangeBuddyStates();
        purplePushUserProfiles();

        purpleIMSetSystemMessages();
        purpleIMPushTypingStates();
        purpleIMPushMessages();

        purpleChatPushChat();
        purpleChatPushMessages();
        purpleChatChangeUserStates();

        purpleSetBuddyImages();

        if (m_rooms.size() > 0) purplePushChatRoomList();

        purplePushLogs();

        m_mutex.unlock();
    }
}

void CPurpleConnector::purpleChangeConnectionState()
{
    switch (m_connectionState)
    {
        case ConnectionState_Connected:
            purple_connection_set_state(m_pPurpleConnection, PURPLE_CONNECTED);
            break;
        case ConnectionState_Reconnecting:
        case ConnectionState_Connecting:
            purple_connection_set_state(m_pPurpleConnection, PURPLE_CONNECTING);
            break;
        case ConnectionState_Disconnected:
            purple_connection_set_state(m_pPurpleConnection, PURPLE_DISCONNECTED);
            break;
        default:
        case ConnectionState_None:
            break;
    }
}

void CPurpleConnector::purpleBuddyListRefresh()
{
    PurpleAccount* pAccount = purple_connection_get_account(m_pPurpleConnection);

    GSList* pPurpleBuddies = purple_blist_get_buddies();
    GSList* pPurpleCurrent;
    PurpleBuddy* pPurpleCurrentBuddy;

    auto end = m_buddyListStates.cend();
    for (auto current = m_buddyListStates.begin(); current != end; ++ current)
    {
        purpleBuddyListChangeBuddyState(&*current);
    }

    if (m_buddyListStates.size() > 0)
    {
        for (pPurpleCurrent = pPurpleBuddies; pPurpleCurrent != NULL; pPurpleCurrent = g_slist_next(pPurpleCurrent))
        {
            pPurpleCurrentBuddy = reinterpret_cast<PurpleBuddy*>(pPurpleCurrent->data);

            Log::write("buddylist") << "checking buddy (" << purple_buddy_get_name(pPurpleCurrentBuddy) << ") for auth" << Log::end;

            if (purple_buddy_get_account(pPurpleCurrentBuddy) == pAccount) // same account
            {
                const char* pBuddyName = purple_buddy_get_name(pPurpleCurrentBuddy);

                auto it = std::find_if(m_buddyListStates.begin(), m_buddyListStates.end(),
                                       [pBuddyName] (Spin::CPurpleConnector::SBuddyState& _rVal) -> bool
                                       {
                                           return strcmp(_rVal.m_name.c_str(), pBuddyName) == 0;
                                       }
                );

                SBuddyData* pBuddyData = reinterpret_cast<SBuddyData*>(purple_buddy_get_protocol_data(pPurpleCurrentBuddy));

                if (pBuddyData != NULL)
                {
                    pBuddyData->m_hasAuthorizedAccount = it != m_buddyListStates.cend();
                }
                else
                {
                    pBuddyData = new SBuddyData();
                    purple_buddy_set_protocol_data(pPurpleCurrentBuddy, pBuddyData);
                    pBuddyData->m_id = -1;
                    pBuddyData->m_hasAuthorizedAccount = false;
                }

                Log::write("buddylist") << "buddy ("
                                         << purple_buddy_get_name(pPurpleCurrentBuddy)
                                         << ") " << (pBuddyData->m_hasAuthorizedAccount ? "has" : "hasn't")
                                         << "authorized" << Log::end;
            }

        }

        m_buddyListStates.clear();
    }
}

void CPurpleConnector::purpleBuddyListChangeBuddyStates()
{
    auto end = m_buddyStates.cend();
    for (auto current = m_buddyStates.begin(); current != end; ++ current)
    {
        purpleBuddyListChangeBuddyState(&*current);
    }

    m_buddyStates.clear();
}

void CPurpleConnector::purpleBuddyListChangeBuddyState(SBuddyState* _pBuddyState)
{
    PurpleAccount* pAccount = purple_connection_get_account(m_pPurpleConnection);
    PurpleBuddy*   pBuddy   = NULL;
    PurpleGroup*   pGroup   = NULL;
    SBuddyData*    pBuddyData = nullptr;

    assert(pAccount != NULL);

    const char* pBuddyName = _pBuddyState->m_name.c_str();
    pBuddy = purple_find_buddy(pAccount, pBuddyName);

    if (pBuddy == NULL)
    {
        if ((_pBuddyState->m_flags & BuddyFlag_AskAuth) == _pBuddyState->m_flags)
        {
            // https://developer.pidgin.im/doxygen/2.10.5/html/account_8h.html
            SCBInfo* CBInfo = new SCBInfo();
            CBInfo->m_pName = pBuddyName;
            CBInfo->m_pConnector = getConnector();

            purple_account_request_authorization(
                pAccount,
                pBuddyName,
                NULL, NULL, NULL,
                TRUE,
                purpleUserAuthAcceptBuddyCallback,
                purpleUserAuthDeclineBuddyCallback,
                CBInfo);
            return;
        }

        pBuddy = purple_buddy_new(pAccount, pBuddyName, NULL);
        assert(pBuddy != NULL);

        pGroup = purple_find_group("Spin");
        Log::write("buddylist") << "couldn't find group 'Spin'" << Log::end;

        if (pGroup == NULL)
        {
            pGroup = purple_group_new("Spin");
            assert(pGroup != NULL);
            purple_blist_add_group(pGroup, NULL);
            Log::write("buddylist") << "created and added group 'Spin'" << Log::end;
        }
//        }

        purple_blist_add_buddy(pBuddy, NULL, pGroup, NULL);
        Log::write("buddylist") << "added buddy " << pBuddyName << " group 'Spin'" << Log::end;
    }

    if ((_pBuddyState->m_flags & BuddyFlag_Removed) == _pBuddyState->m_flags)
    {
        purple_blist_remove_buddy(pBuddy);
        Log::write("buddylist") << "removed buddy" << pBuddyName << Log::end;
    }
    else
    {
        if ((_pBuddyState->m_flags & BuddyFlag_Offline) == _pBuddyState->m_flags)
        {
            purple_prpl_got_user_status(pAccount, pBuddyName, "offline", NULL);
            Log::write("buddylist") << "set buddy-state to offline for " << pBuddyName << Log::end;
        }
        else if ((_pBuddyState->m_flags & BuddyFlag_Online) == _pBuddyState->m_flags)
        {
            if ((_pBuddyState->m_flags & BuddyFlag_Away) == _pBuddyState->m_flags)
            {
                purple_prpl_got_user_status(pAccount, pBuddyName, "away", NULL);
                Log::write("buddylist") << "set buddy-state to away for " << pBuddyName << Log::end;
            }
            else
            {
                purple_prpl_got_user_status(pAccount, pBuddyName, "online", NULL);
                Log::write("buddylist") << "set buddy-state to online for " << pBuddyName << Log::end;
            }
        }
    }

    pBuddyData = reinterpret_cast<SBuddyData*>(purple_buddy_get_protocol_data(pBuddy));
    if (pBuddyData == NULL)
    {
        pBuddyData = new SBuddyData();
        pBuddyData->m_hasAuthorizedAccount = true;
        pBuddyData->m_id                   = _pBuddyState->m_id;

        purple_buddy_set_protocol_data(pBuddy, pBuddyData);
    }

    if (_pBuddyState->m_id != -1)
    {
        pBuddyData->m_id = _pBuddyState->m_id;
    }

    pBuddyData->m_hasAuthorizedAccount = true;
}

void CPurpleConnector::purpleChatPushChat()
{
    auto end = m_chatJoin.cend();

    for (auto current = m_chatJoin.begin(); current != end; ++ current)
    {
        auto it = m_roomIdMap.find(*current);

        if (it == m_roomIdMap.cend())
        {
            purple_debug_error(m_pPluginId, "couldn't find id for room %s\n", current->c_str());
            Log::error("chat_sys") << "couldn't find id for room '" << current->c_str() << "'" << Log::end;
            continue;
        }

        Log::write("chat") << "entering room " << *current << Log::end;
        serv_got_joined_chat(m_pPurpleConnection, it->second, current->c_str());
    }

    m_chatJoin.clear();

    auto end2 = m_chatLeave.cend();

    for (auto current = m_chatLeave.begin(); current != end2; ++ current)
    {
        auto it = m_roomIdMap.find(*current);

        if (it == m_roomIdMap.cend())
        {
            Log::error("chat_sys") << "couldn't find id for room '" << current->c_str() << "'" << Log::end;
            continue;
        }

        Log::write("chat") << "leaving room " << *current << Log::end;
        serv_got_chat_left(m_pPurpleConnection, it->second);
    }

    m_chatLeave.clear();
}

void CPurpleConnector::purpleChatPushMessages()
{
    auto end = m_chatMessages.cend();

    for (auto current = m_chatMessages.begin(); current != end; ++ current)
    {
        SChatMessage& rMessage = *current;

        PurpleConvChat* pConvChat = purpleGetConvChat(rMessage.m_pChatRoomName);

        assert(pConvChat != NULL);

        Log::write("chat_msg") << "pushing message '" << rMessage.m_pMessage
                                << "'\n send by '" << rMessage.m_pUsername
                                << "'\n to room '" << rMessage.m_pChatRoomName
                                << " == " << purple_conversation_get_name(purple_conv_chat_get_conversation(pConvChat))
                                << "'[" << pConvChat->id
                                << "]\n at " << rMessage.m_timestamp
                                << Log::end;

        purple_conv_chat_write(
            pConvChat,
            rMessage.m_pUsername,
            rMessage.m_pMessage,
            PURPLE_MESSAGE_RECV,
            rMessage.m_timestamp
        );

        delete[] rMessage.m_pChatRoomName;
    }

    m_chatMessages.clear();
}

/**
 * IM
 */

PurpleConvIm* CPurpleConnector::purpleGetConvIm(const char* _pUserName)
{
    Log::info("im_sys") << "searching conversation for user '" << _pUserName << "'" << Log::end;

    PurpleConversation* pConversation = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, _pUserName, m_pPurpleConnection->account);

    if (pConversation == NULL)
    {
        Log::info("im_sys") << "creating conversation for user '" << _pUserName << "'" << Log::end;

        pConversation = purple_conversation_new(PURPLE_CONV_TYPE_IM, m_pPurpleConnection->account, _pUserName);

        if (pConversation == NULL)
        {
            Log::fatal("im_sys") << "weren't able to initiate converation with '" << _pUserName << "'" << Log::end;
            return NULL;
        }
    }

    PurpleConvIm* pConvIm = purple_conversation_get_im_data(pConversation);

    if (pConvIm == NULL)
    {
        Log::error("im_sys") << "couldn't retrieve im for '" << _pUserName << "'" << Log::end;
        return NULL;
    }

    return pConvIm;
}

PurpleConvChat* CPurpleConnector::purpleCreateConvChat(const char* _pRoomName)
{
    PurpleConvChat* pConvChat = NULL;
    PurpleConversation* pConversation = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, _pRoomName, m_pPurpleConnection->account);

    if (pConversation == NULL)
    {
        pConversation = purple_conversation_new(PURPLE_CONV_TYPE_CHAT, m_pPurpleConnection->account, _pRoomName);

        if (pConversation == NULL)
        {
            Log::error("chat_sys") << "weren't able to initiate conversation for chatroom '" << _pRoomName << "'" << Log::end;
            return NULL;
        }

        pConvChat = purple_conversation_get_chat_data(pConversation);

        int id = purple_conv_chat_get_id(pConvChat);

        m_idRoomMap.emplace(std::pair<int, std::string>(id, _pRoomName));
        m_roomIdMap.emplace(std::pair<std::string, int>(_pRoomName, id));
    }
    else
    {
        pConvChat = purple_conversation_get_chat_data(pConversation);
    }

    if (pConvChat == NULL)
    {
        Log::error("chat_sys") << "couldn't retrieve chatroom '" << _pRoomName << "'" << Log::end;
        return NULL;
    }

    return pConvChat;
}

PurpleConvChat* CPurpleConnector::purpleGetConvChat(const char* _pRoomName)
{
    PurpleConvChat* pConvChat = NULL;
    Log::info("chat") << "searching conversation for room '" << _pRoomName << "' by name" << Log::end;

    PurpleConversation* pConversation = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, _pRoomName, m_pPurpleConnection->account);

    if (pConversation == NULL)
    {
        Log::error("chat_sys") << "couldn't retrieve conversation for chatroom '" << _pRoomName << "' by name" << Log::end;
        return NULL;
    }

    purple_debug_error(m_pPluginId, "purpleGetConvChat char*\n");
    pConvChat = purple_conversation_get_chat_data(pConversation);
    purple_debug_error(m_pPluginId, "purpleGetConvChat char* end\n");

    if (pConvChat == NULL)
    {
        Log::error("chat_sys") << "couldn't retrieve chatroom '" << _pRoomName << "' by name" << Log::end;
        return NULL;
    }

    return pConvChat;
}

PurpleConvChat* CPurpleConnector::purpleGetConvChat(int _id)
{
    PurpleConvChat* pConvChat = NULL;
    PurpleConversation* pConversation = purple_find_chat(m_pPurpleConnection, _id);

    Log::info("chat_sys") << "searching conversation for room with id '" << _id << "' by id" << Log::end;

    if (pConversation == NULL)
    {
        Log::error("chat_sys") << "couldn't retrieve conversation with id '" << _id << "' by id" << Log::end;
        return NULL;
    }

    pConvChat = purple_conversation_get_chat_data(pConversation);

    if (pConvChat == NULL)
    {
        Log::error("chat_sys") << "couldn't retrieve conversation with id '" << _id << "' by id" << Log::end;
        return NULL;
    }

    return pConvChat;
}
void CPurpleConnector::purpleRemoveConvChat(int _id)
{
    PurpleConvChat* pConvChat = purpleGetConvChat(_id);

    purple_conv_chat_left(pConvChat);
    purple_conversation_destroy(purple_conv_chat_get_conversation(pConvChat));
}

void CPurpleConnector::purpleIMPushMessages()
{
    auto end = m_imMessages.cend();

    for (auto current = m_imMessages.begin(); current != end; ++ current)
    {
        SIMMessage& rMessage = *current;

        PurpleConvIm* pConvIm = purpleGetConvIm(rMessage.m_pUsername);

        if (pConvIm == NULL)
        {
            continue;
        }

        purple_conv_im_write(
            pConvIm,
            rMessage.m_pUsername,
            rMessage.m_pMessage,
            PURPLE_MESSAGE_RECV,
            rMessage.m_timestamp
        );


        delete[] rMessage.m_pUsername;
    }

    m_imMessages.clear();
}

void CPurpleConnector::purpleIMPushTypingStates()
{
    auto end = m_imTyping.cend();

    for (auto current = m_imTyping.begin(); current != end; ++ current)
    {
        SIMTyping& rTyping = *current;

        PurpleConvIm* pConvIm = purpleGetConvIm(rTyping.m_pUsername);

        if (pConvIm == NULL)
        {
            continue;
        }

        purple_conv_im_set_typing_state(pConvIm, rTyping.m_typingState);

        delete[] rTyping.m_pUsername;
    }

    m_imTyping.clear();
}

void CPurpleConnector::purpleIMSetSystemMessages()
{
    auto end = m_imChatBuddyFlags.cend();

    for (auto current = m_imChatBuddyFlags.begin(); current != end; ++ current)
    {
        SIMSystemMessage& rIMSystemMessage = *current;

        PurpleConvIm* pConvIm = purpleGetConvIm(rIMSystemMessage.m_pUsername);

        if (pConvIm == NULL)
        {
            continue;
        }

        const char* pMessage = nullptr;

        switch (rIMSystemMessage.m_flags)
        {
            case IMSystemFlag_DoesNotAcceptIM: pMessage = c_pStrIMSystemMessageBlocked     ; break;
            case IMSystemFlag_NotRegistered  : pMessage = c_pStrIMSystemMessageUnregistered; break;
            case IMSystemFlag_Offline        : pMessage = c_pStrIMSystemMessageOffline     ; break;
            default                          : pMessage = c_pStrUnknown                    ; break;
        }

        purple_conv_im_write(
            pConvIm,
            rIMSystemMessage.m_pUsername,
            pMessage,
            PURPLE_MESSAGE_SYSTEM,
            rIMSystemMessage.m_timestamp
        );

        delete[] rIMSystemMessage.m_pUsername;
    }

    m_imChatBuddyFlags.clear();
}

void CPurpleConnector::purpleChatChangeUserStates()
{
    PurpleConvChatBuddyFlags flags;
    PurpleConvChat* pConvChat;
    auto end = m_chatUserStates.cend();

    for (auto current = m_chatUserStates.begin(); current != end; ++ current)
    {
        SChatUserState& rState = *current;

//        PURPLE_CBFLAGS_NONE          = 0x0000, /**< No flags                     */
//        PURPLE_CBFLAGS_VOICE         = 0x0001, /**< Voiced user or "Participant" */
//        PURPLE_CBFLAGS_HALFOP        = 0x0002, /**< Half-op                      */
//        PURPLE_CBFLAGS_OP            = 0x0004, /**< Channel Op or Moderator      */
//        PURPLE_CBFLAGS_FOUNDER       = 0x0008, /**< Channel Founder              */
//        PURPLE_CBFLAGS_TYPING        = 0x0010, /**< Currently typing             */
//        PURPLE_CBFLAGS_AWAY          = 0x0020  /**< Currently away. @since 2.8.0 */

        Log::info("chat_sys") << "getting convchat " << rState.m_pRoomName << Log::end;
        pConvChat = purpleGetConvChat(rState.m_pRoomName);

        if (pConvChat == NULL)
        {
            purple_debug_error(m_pPluginId, "couldn't retrieve convchat in purpleChatChangeUserStates with name: %s \n", rState.m_pRoomName);
            continue;
        }

        Log::info("chat_sys") << "got convchat for room '" << rState.m_pRoomName << "'" << Log::end;

        flags = PURPLE_CBFLAGS_NONE;

        if ((rState.m_flags & ChatUserFlag_Joining) == ChatUserFlag_Joining)
        {
            if ((rState.m_flags & ChatUserFlag_Operator    ) == ChatUserFlag_Operator    ) flags = static_cast<PurpleConvChatBuddyFlags>(static_cast<int>(flags) | static_cast<int>(PURPLE_CBFLAGS_OP    ));
            if ((rState.m_flags & ChatUserFlag_HelpOperator) == ChatUserFlag_HelpOperator) flags = static_cast<PurpleConvChatBuddyFlags>(static_cast<int>(flags) | static_cast<int>(PURPLE_CBFLAGS_HALFOP));
            if ((rState.m_flags & ChatUserFlag_Away        ) == ChatUserFlag_Away        ) flags = static_cast<PurpleConvChatBuddyFlags>(static_cast<int>(flags) | static_cast<int>(PURPLE_CBFLAGS_AWAY  ));

            purple_debug_info(m_pPluginId, "flags=%i  user %s joined channel %s\n", rState.m_flags, rState.m_pUsername, rState.m_pRoomName);

            purple_conv_chat_add_user(
                pConvChat,
                rState.m_pUsername,
                NULL,
                flags,
                (rState.m_flags & ChatUserFlag_Announce) == ChatUserFlag_Announce
            );
        }
        else if ((rState.m_flags & ChatUserFlag_Leaving) == ChatUserFlag_Leaving)
        {
            purple_debug_info(m_pPluginId, "flags=%i  user %s left channel %s\n", rState.m_flags, rState.m_pUsername, rState.m_pRoomName);

            const char* pReason;

                 if ((rState.m_flags & ChatUserFlag_Disconnected) == ChatUserFlag_Disconnected) { pReason = c_pStrDisconnected; }
            else if ((rState.m_flags & ChatUserFlag_Logout      ) == ChatUserFlag_Logout      ) { pReason = c_pStrLogout      ; }
            else if ((rState.m_flags & ChatUserFlag_IdleKick    ) == ChatUserFlag_IdleKick    ) { pReason = c_pStrIdleKick    ; }
            else if ((rState.m_flags & ChatUserFlag_KickedByOp  ) == ChatUserFlag_KickedByOp  ) { pReason = c_pStrKickedByOp  ; }
            else                                                                                { pReason = c_pStrUnknown     ; }

            purple_conv_chat_remove_user(pConvChat, rState.m_pUsername, pReason);
        }
        else if (rState.m_flags == ChatUserFlag_Away)
        {
            flags = purple_conv_chat_user_get_flags(pConvChat, rState.m_pUsername);
            flags = static_cast<PurpleConvChatBuddyFlags>(static_cast<int>(flags) | static_cast<int>(PURPLE_CBFLAGS_AWAY));

            purple_conv_chat_user_set_flags(pConvChat, rState.m_pUsername, flags);
        }
        else if (rState.m_flags == ChatUserFlag_BackFromAway)
        {
            flags = purple_conv_chat_user_get_flags(pConvChat, rState.m_pUsername);
            flags = static_cast<PurpleConvChatBuddyFlags>(static_cast<int>(flags) & ~static_cast<int>(PURPLE_CBFLAGS_AWAY));

            purple_conv_chat_user_set_flags(pConvChat, rState.m_pUsername, flags);
        }

        delete[] rState.m_pRoomName;
        delete[] rState.m_pUsername;
    }

    m_chatUserStates.clear();
}

void CPurpleConnector::purpleSetBuddyImages()
{
    std::vector<SImage>::const_iterator endOfImages = m_images.cend();
    for (std::vector<SImage>::iterator currentImage = m_images.begin(); currentImage != endOfImages; ++ currentImage)
    {
        purple_buddy_icon_new(
            purple_connection_get_account(m_pPurpleConnection),
            currentImage->m_pUsername,
            currentImage->m_pImageData,
            currentImage->m_imageSize,
            currentImage->m_pChecksum
        );
//        purple_buddy_icons_set_for_user(
//            purple_connection_get_account(m_pPurpleConnection),
//            currentImage->m_pUsername,
//            currentImage->m_pImageData,
//            currentImage->m_imageSize,
//            NULL
//        );

        delete[] reinterpret_cast<char*>(currentImage->m_pImageData);
        delete[] currentImage->m_pUsername;
    }

    m_images.clear();
}

void CPurpleConnector::purplePushChatRoomList()
{
    PurpleRoomlist* pRoomList;
    PurpleRoomlistRoom* pRoom;
    const char* pId;
    const char* pName;
    const char* pType;

    pRoomList = reinterpret_cast<PurpleRoomlist*>(m_pPurpleRoomList);

    if (pRoomList == nullptr)
    {
        purple_debug_info(m_pPluginId, "roomlist == nullptr");
        Log::error("roomlist") << "roomlist == nullptr" << Log::end;
        return;
    }

    auto endOfRooms = m_rooms.cend();
    for (auto currentRoom = m_rooms.cbegin(); currentRoom != endOfRooms; ++ currentRoom)
    {
        pId   = currentRoom->m_pId.c_str();
        pName = currentRoom->m_pName.c_str();

        switch (currentRoom->m_type)
        {
            default:
            case RoomType_Official:
                pType = "Official";
                break;
            case RoomType_Group:
                pType = "Group";
                break;
            case RoomType_User:
                pType = "User";
                break;
            case RoomType_Game:
                pType = "Game";
                break;
        }

        pRoom = purple_roomlist_room_new(PURPLE_ROOMLIST_ROOMTYPE_ROOM, pName, NULL);
        std::string numberOfUsers;
        if (currentRoom->m_numberOfUsers == -1)
        {
            numberOfUsers = "FULL";
        }
        else
        {
            numberOfUsers = std::to_string(currentRoom->m_numberOfUsers);
        }

        purple_roomlist_room_add_field(pRoomList, pRoom, pType);
        purple_roomlist_room_add_field(pRoomList, pRoom, pId  );
        purple_roomlist_room_add_field(pRoomList, pRoom, numberOfUsers.c_str());

        purple_roomlist_room_add(pRoomList, pRoom);
    }

    purple_roomlist_set_in_progress(pRoomList, FALSE);

    m_rooms.clear();
}

void CPurpleConnector::purplePushUserProfiles()
{
    TUserProfiles::const_iterator endOfProfiles = m_userProfiles.cend();
    for (TUserProfiles::const_iterator currentProfile = m_userProfiles.cbegin(); currentProfile != endOfProfiles; ++ currentProfile)
    {
        PurpleNotifyUserInfo* pNotifyUserInfo = purple_notify_user_info_new();

        const char* pUserName = currentProfile->first.c_str();
        const TUserProfile& rProfile = currentProfile->second;

        TUserProfile::const_iterator endOfEntries = rProfile.cend();
        for (TUserProfile::const_iterator currentEntry = rProfile.cbegin(); currentEntry != endOfEntries; ++ currentEntry)
        {
            switch (currentEntry->getType())
            {
                case Spin::CUserProfileEntry::UserInfoType_Headline:
                    purple_notify_user_info_add_section_header(pNotifyUserInfo, currentEntry->getHeadline().c_str());
                    break;
                case Spin::CUserProfileEntry::UserInfoType_Line:
                    purple_notify_user_info_add_section_break(pNotifyUserInfo);
                    break;
                case Spin::CUserProfileEntry::UserInfoType_Pair:
                    purple_notify_user_info_add_pair(pNotifyUserInfo, currentEntry->getFirst().c_str(), currentEntry->getSecond().c_str());
                    break;
                default:
                    break;
            }
        }

        // FIXME might be freed on close?!?
        purple_notify_userinfo(m_pPurpleConnection, pUserName, pNotifyUserInfo, NULL, NULL);
    }

    m_userProfiles.clear();
}

void CPurpleConnector::purplePushLogs()
{
    Spin::CLogger& rLogger = Spin::CLogger::getInstance();

    if (!rLogger.acquireLogsLock())
    {
        return;
    }

    m_logMessages.splice(m_logMessages.end(), rLogger.getLogs());
    m_logCategories = rLogger.getCategories();

    rLogger.freeLogsLock();

    m_logMessages.sort(
        [] (const Spin::CLogger::SLogMessage& _rLeft, const Spin::CLogger::SLogMessage& _rRight)
        {
            return _rLeft.m_timeStamp < _rRight.m_timeStamp;
        }
    );

    std::string msg;

    auto endOfLogs = m_logMessages.cend();
    for (auto currentLog = m_logMessages.cbegin(); currentLog != endOfLogs; ++ currentLog)
    {
        const std::string& rCategory = purpleFindLogCategory(currentLog->m_threadId, currentLog->m_category);

        PurpleDebugLevel level;

        switch (currentLog->m_level)
        {
            default:
            case CLogger::LEVEL_INFO   : level = PURPLE_DEBUG_INFO   ; break;
            case CLogger::LEVEL_WARNING: level = PURPLE_DEBUG_WARNING; break;
            case CLogger::LEVEL_ERROR  : level = PURPLE_DEBUG_ERROR  ; break;
            case CLogger::LEVEL_FATAL  : level = PURPLE_DEBUG_FATAL  ; break;
        }

        if (currentLog->m_message.find('%') != std::string::npos)
        {
            msg.assign(currentLog->m_message);
            replaceAll(msg, "%", "%%");
            purple_debug(level, rCategory.c_str(), msg.c_str());
        }
        else
        {
            purple_debug(level, rCategory.c_str(), currentLog->m_message.c_str());
        }
    }

    m_logMessages.clear();
}

const std::string& CPurpleConnector::purpleFindLogCategory(uint16_t _threadId, int16_t _categoryId)
{
    auto it = m_logCategories.find(_threadId);

    if (it == m_logCategories.cend())
    {
        return c_undefinedCategory;
    }

    auto it2 = it->second.find(_categoryId);

    if (it2 == it->second.cend())
    {
        return c_undefinedCategory;
    }

    return it2->second;
}

void CPurpleConnector::purpleSetRoomList(void* _pRoomList)
{
    assert(_pRoomList != nullptr);

    m_pPurpleRoomList = _pRoomList;
}

void* CPurpleConnector::purpleGetRoomList()
{
    return m_pPurpleRoomList;
}

void CPurpleConnector::purpleOnIMConversationCreated(PurpleConversation* _pConversation)
{
    CConnector* pConnector = getConnector();
    const char* pUserName = purple_conversation_get_name(_pConversation);
    const char* pHash     = NULL;

    Spin::Message::SIMInit imInit;
    imInit.m_userName = pUserName;
    pConnector->pushOut(imInit);

//        PurpleBuddyIcon* pBuddyIcon = purple_buddy_icons_find(purple_conversation_get_account(_pConversation), pUserName);

//        if (pBuddyIcon != NULL)
//            pHash = purple_buddy_icon_get_checksum(pBuddyIcon);

//        purple_buddy_icons_get_checksum_for_user()

    Spin::Job::SUpdateUserInfo updateUserInfo;

    PurpleBuddy* pBuddy = purple_find_buddy(purple_conversation_get_account(_pConversation), pUserName);
    if (pBuddy != NULL)
    {
        updateUserInfo.m_isBuddy = true;
        PurpleBuddyIcon* pBuddyIcon = purple_buddy_get_icon(pBuddy);

        if (pBuddyIcon != NULL)
            pHash = purple_buddy_icon_get_checksum(pBuddyIcon);
    }
    else
    {
        updateUserInfo.m_isBuddy = false;
    }

                       updateUserInfo.m_userName      .assign(pUserName);
    if (pHash != NULL) updateUserInfo.m_buddyImageHash.assign(pHash);
//    Spin::pushJob(getConnector(), updateUserInfo);
    pConnector->pushJob(std::move(updateUserInfo));
}

} // namespace Spin