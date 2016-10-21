/**
 * purple_connector.h
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


#ifndef SPINTEST_PURPLE_CONNECTOR_H
#define SPINTEST_PURPLE_CONNECTOR_H

#include "base_connector.h"
#include "logger.h"

#ifdef CMAKE
#include <libpurple/connection.h>
#else
#include <connection.h>
#endif

#include <string>
#include <map>
#include <vector>
#include <mutex>
#ifdef __MINGW32__
#include <mingw.mutex.h>
#endif

namespace Spin
{

class CPurpleConnector final : public CBaseConnector
{
public:

    struct SBuddyData
    {
        int  m_id;
        bool m_hasAuthorizedAccount;
    };

private:

    struct SChatMessage
    {
        const char* m_pChatRoomName;
        const char* m_pUsername;
        const char* m_pMessage;
        time_t      m_timestamp;
    };

    struct SIMMessage
    {
        const char* m_pUsername;
        const char* m_pMessage;
        time_t      m_timestamp;
    };

    struct SIMTyping
    {
        const char*       m_pUsername;
        PurpleTypingState m_typingState;
    };

public:
    CPurpleConnector();
    virtual ~CPurpleConnector();

public:
    void setPluginId(const char* _pPluginId);
    const char* getPluginId();
    void setPurpleConnection(PurpleConnection* _pConnection);

//    void purpleAddChatRoom(int _id, const char* _pName);
//    void purpleRemoveChatRoom(int _id, const char* _pName);
//    int purpleGetChatRoomId(const char* _pName) const;
    const char* purpleGetChatRoomName(int _id) const;

    PurpleConvIm* purpleGetConvIm(const char* _pUserName);
    PurpleConvChat* purpleCreateConvChat(const char* _pRoomName);
    PurpleConvChat* purpleGetConvChat(const char * _pRoomName);
    PurpleConvChat* purpleGetConvChat(int _id);
    void            purpleRemoveConvChat(int _id);

    void purpleOnLoop();

private:
    void purpleChangeConnectionState();
    void purpleRefreshBuddyList();
    void purpleChangeBuddyState();
    void purplePushChat();
    void purplePushChatMessages();
    void purplePushIMMessages();
    void purplePushTypingStates();
    void purpleChangeChatUserStates();

    void purplePushChatRoomList();

    void purplePushLogs();
    const std::string& purpleFindLogCategory(uint16_t _threadId, int16_t _categoryId);

    void purpleSetBuddyState(SBuddyState* _pBuddyState);


public:
    void purpleSetRoomList(void* _pRoomList);
    void* purpleGetRoomList();


public:
    virtual void lock();
    virtual void unlock();

    virtual void setConnectionState(EConnectionState _state);

    virtual void buddyListRefresh(std::vector<SBuddyState>& _rBuddies);
    virtual void buddyStateChange(SBuddyState* _pBuddy);
//    virtual void buddyList(SBuddyState* _pBuddies, size_t _numberOfBuddies);
//    virtual void buddyAdd();
//    virtual void buddyRemoved();
//
//    virtual void imOpen();
//    virtual void imClose();
    virtual void imSetTypingState(const char* _pUserName, ETypingState _state);
    virtual void imAddMessage(const char* _pUsername, char _type, const char* _pMessage);

    virtual void chatSetList(std::vector<SRoom>& _rRooms);
    virtual void chatUserStateChange(const SChatUserState* _pUserStates, size_t _numberOfUserStates);
    virtual void chatJoin(const char* _pRoomName, const char* _pSpinId = nullptr);
    virtual void chatLeave(const char* _pRoomName, const SChatUserState* _pState);
    virtual void chatAddMessage(const char* _pRoomName, const char* _pUsername, char _type, const char* _pMessage);

private:
    const char*       m_pPluginId;
    PurpleConnection* m_pPurpleConnection;

    std::map<int, std::string> m_idRoomMap;
    std::map<std::string, int> m_roomIdMap;
    std::map<int, std::string> m_idSpinIdMap;
    std::map<std::string, int> m_spinIdIdMap;

    std::mutex                  m_mutex;
    EConnectionState            m_connectionState;
    std::vector<SChatMessage>   m_chatMessages;
    std::vector<SChatUserState> m_chatUserStates;
    std::vector<SIMMessage>     m_imMessages;
    std::vector<SIMTyping>      m_imTyping;
    std::vector<std::string>    m_chatJoin;
    std::vector<std::string>    m_chatLeave;
    std::vector<SBuddyState>    m_buddyStates;
    std::vector<SRoom>          m_rooms;
    std::vector<SBuddyState>    m_buddyListStates;
    Spin::CLogger::TMessageList m_logMessages;
    Spin::CLogger::TThreadIdCategoryMap m_logCategories;

    void*                       m_pPurpleRoomList;

    int                         m_currentId;

    const char* c_pStrUnknown      = "unknown";
    const char* c_pStrKickedByOp   = "kicked by operator";
    const char* c_pStrIdleKick     = "idlekick";
    const char* c_pStrDisconnected = "disconnected";
    const char* c_pStrLogout       = "logout";
    const std::string c_undefinedCategory = "undefined_category";

private:
    CPurpleConnector(const CPurpleConnector&) = delete;
    CPurpleConnector operator= (const CPurpleConnector&) = delete;

};

}

#endif // SPINTEST_PURPLE_CONNECTOR_H
