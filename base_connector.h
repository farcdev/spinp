/**
 * base_connector.h
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


#ifndef SPINTEST_BASE_CONNECTOR_H
#define SPINTEST_BASE_CONNECTOR_H

#include <stddef.h>
#include <vector>
#include <string>

namespace Spin
{

enum EMessageType
{
    Default,
    Action,
    Echo
};

enum EConnectionState
{
    ConnectionState_None         = -1,
    ConnectionState_Disconnected,
    ConnectionState_Connecting,
    ConnectionState_Connected,
    ConnectionState_Reconnecting
};

enum ETypingState
{
    Typing_Typing,
    Typing_StoppedTyping,
    Typing_Nothing
};

//enum EChatRoomLeavingType
//{
//    ChatRoomLeaving_Default,
//    ChatRoomLeaving_Disconnected,
//    ChatRoomLeaving_Exited,
//    ChatRoomLeaving_IdleKick
//};

enum EChatUserFlags
{
    ChatUserFlag_None         = 0x0000,
    ChatUserFlag_Male         = 0x0001,
    ChatUserFlag_Female       = 0x0002,
    ChatUserFlag_Operator     = 0x0004,
    ChatUserFlag_HelpOperator = 0x0008,
    ChatUserFlag_Joining      = 0x0010,
    ChatUserFlag_Leaving      = 0x0020,
    ChatUserFlag_IdleKick     = 0x0040,
    ChatUserFlag_Disconnected = 0x0080,
    ChatUserFlag_Logout       = 0x0100,
    ChatUserFlag_KickedByOp   = 0x0200,
    ChatUserFlag_Announce     = 0x0400,
    ChatUserFlag_Away         = 0x0800,
    ChatUserFlag_BackFromAway = 0x1000,
};

inline EChatUserFlags operator| (EChatUserFlags _lhs, EChatUserFlags _rhs)
{
    return static_cast<EChatUserFlags>(static_cast<int>(_lhs) | static_cast<int>(_rhs));
}

inline EChatUserFlags& operator|= (EChatUserFlags& _lhs, const EChatUserFlags& _rhs)
{
    _lhs = static_cast<EChatUserFlags>(static_cast<int>(_lhs) | static_cast<int>(_rhs));
    return _lhs;
}

enum EBuddyFlags
{
    BuddyFlag_None    = 0x0000,
    BuddyFlag_Offline = 0x0001,
    BuddyFlag_Online  = 0x0002,
    BuddyFlag_Away    = 0x0004,
    BuddyFlag_Removed = 0x0008,
    BuddyFlag_AskAuth = 0x0010
};

inline EBuddyFlags operator| (EBuddyFlags _lhs, EBuddyFlags _rhs)
{
    return static_cast<EBuddyFlags>(static_cast<int>(_lhs) | static_cast<int>(_rhs));
}

inline EBuddyFlags& operator|= (EBuddyFlags& _lhs, const EBuddyFlags& _rhs)
{
    _lhs = static_cast<EBuddyFlags>(static_cast<int>(_lhs) | static_cast<int>(_rhs));
    return _lhs;
}

enum ERoomType
{
    RoomType_Official = 0,
    RoomType_User     = 1,
    RoomType_Group    = 2,
    RoomType_Game     = 3,
};

enum EIMSystemFlag
{
    IMSystemFlag_NotRegistered   = 0,
    IMSystemFlag_DoesNotAcceptIM = 1,
    IMSystemFlag_Offline         = 2
};

inline EIMSystemFlag operator| (EIMSystemFlag _lhs, EIMSystemFlag _rhs)
{
    return static_cast<EIMSystemFlag>(static_cast<int>(_lhs) | static_cast<int>(_rhs));
}

inline EIMSystemFlag& operator|= (EIMSystemFlag& _lhs, const EIMSystemFlag& _rhs)
{
    _lhs = static_cast<EIMSystemFlag>(static_cast<int>(_lhs) | static_cast<int>(_rhs));
    return _lhs;
}

class CUserProfileEntry
{
public:
    enum EUserProfileEntryType
    {
        UserInfoType_Headline,
        UserInfoType_Pair    ,
        UserInfoType_Line    ,
    };

public:
    CUserProfileEntry(EUserProfileEntryType _type)
        : m_type  (_type)
        , m_first ()
        , m_second()
    { }

    CUserProfileEntry()
        : m_type  (UserInfoType_Line)
        , m_first ()
        , m_second()
    { }

    CUserProfileEntry(EUserProfileEntryType _type, const char* _pFirst)
        : m_type  (_type)
        , m_first (_pFirst)
        , m_second()
    { }

    CUserProfileEntry(const char* _pFirst)
        : m_type  (UserInfoType_Headline)
        , m_first (_pFirst)
        , m_second()
    { }

    CUserProfileEntry(EUserProfileEntryType _type, const char* _pFirst, const char* _pSecond)
        : m_type  (_type)
        , m_first (_pFirst)
        , m_second(_pSecond)
    { }

    CUserProfileEntry(const char* _pFirst, const char* _pSecond)
        : m_type  (UserInfoType_Pair)
        , m_first (_pFirst)
        , m_second(_pSecond)
    { }

    CUserProfileEntry(EUserProfileEntryType _type, std::string&& _rrFirst, const char* _pSecond)
        : m_type  (_type)
        , m_first (std::move(_rrFirst))
        , m_second(_pSecond)
    { }

    CUserProfileEntry(std::string&& _rrFirst, const char* _pSecond)
        : m_type  (UserInfoType_Pair)
        , m_first (std::move(_rrFirst))
        , m_second(_pSecond)
    { }

    CUserProfileEntry(EUserProfileEntryType _type, const char* _pFirst, std::string&& _rrSecond)
        : m_type  (_type)
        , m_first (_pFirst)
        , m_second(std::move(_rrSecond))
    { }

    CUserProfileEntry(const char* _pFirst, std::string&& _rrSecond)
        : m_type  (UserInfoType_Pair)
        , m_first (_pFirst)
        , m_second(std::move(_rrSecond))
    { }

    CUserProfileEntry(EUserProfileEntryType _type, std::string&& _rrFirst)
        : m_type  (_type)
        , m_first (std::move(_rrFirst))
        , m_second()
    { }

    CUserProfileEntry(std::string&& _rrFirst)
        : m_type  (UserInfoType_Headline)
        , m_first (std::move(_rrFirst))
        , m_second()
    { }

    CUserProfileEntry(EUserProfileEntryType _type, std::string&& _rrFirst, std::string&& _rrSecond)
        : m_type  (_type)
        , m_first (std::move(_rrFirst))
        , m_second(std::move(_rrSecond))
    { }

    CUserProfileEntry(std::string&& _rrFirst, std::string&& _rrSecond)
        : m_type  (UserInfoType_Pair)
        , m_first (std::move(_rrFirst))
        , m_second(std::move(_rrSecond))
    { }

    ~CUserProfileEntry()
    {

    }

private:
    EUserProfileEntryType m_type;

    std::string m_first;
    std::string m_second;

public:
    inline CUserProfileEntry& setHeadline(const std::string& _rHeadline)
    {
        m_type  = UserInfoType_Headline;
        m_first = _rHeadline;

        return *this;
    }

    inline CUserProfileEntry& setPair(const std::string& _rLabel, const std::string& _rValue)
    {
        m_type   = UserInfoType_Pair;
        m_first  = _rLabel;
        m_second = _rValue;

        return *this;
    }

    inline CUserProfileEntry& setLine()
    {
        m_type = UserInfoType_Line;

        return *this;
    }

    inline EUserProfileEntryType getType() const
    {
        return m_type;
    }

    inline const std::string& getHeadline() const
    {
        return m_first;
    }

    inline const std::string& getFirst() const
    {
        return m_first;
    }

    inline const std::string& getSecond() const
    {
        return m_second;
    }
};

class CConnector;

class CBaseConnector
{
public:
    typedef std::vector<CUserProfileEntry> TUserProfile;

    struct SBuddyState
    {
        int         m_id;
        std::string m_name;
        EBuddyFlags m_flags;
    };

    struct SChatUserState
    {
        const char*    m_pRoomName;
        const char*    m_pUsername;
        EChatUserFlags m_flags;
    };

    struct SRoom
    {
        std::string m_pId;
        std::string m_pName;
        ERoomType   m_type;
        int         m_numberOfUsers;
    };

public:
    CBaseConnector();
    virtual ~CBaseConnector();

public:
    CConnector* getConnector();

    bool start(const char* _pUsername, const char* _pPassword, unsigned short _port);
    bool stop();

private:
    CConnector* m_pConnector;

public:
    virtual void lock() = 0;
    virtual void unlock() = 0;

    virtual void setConnectionState(EConnectionState _state) = 0;

    virtual void buddyListRefresh(std::vector<SBuddyState>& _rBuddies) = 0;
    virtual void buddyStateChange(SBuddyState* _pBuddy) = 0;
    virtual void buddyAddImage(const char* _pUsername, void* _pImageData, size_t _imageSize, const char* _pChecksum) = 0;
    virtual void buddySetProfile(const char* _pUserName, const TUserProfile& _rInfos) = 0;
    virtual void buddySetProfile(const char* _pUserName, TUserProfile&& _rrInfos) = 0;
//    virtual void buddyList(SBuddyState* _pBuddies, size_t _numberOfBuddies) = 0;
//    virtual void buddyAdd() = 0;
//    virtual void buddyRemoved() = 0;
//
//    virtual void imOpen() = 0;
//    virtual void imClose() = 0;
    virtual void imSetTypingState(const char* _pUserName, ETypingState _state) = 0;
    virtual void imAddMessage(const char* _pUsername, char _type, const char* _pMessage) = 0;
    virtual void imAddStatusMessage(const char* _pUsername, EIMSystemFlag _flag) = 0;

    virtual void chatSetList(std::vector<SRoom>& _rRooms) = 0;
    virtual void chatUserStateChange(const SChatUserState* _pUsers, size_t _numberOfUserStates) = 0;
    virtual void chatJoin(const char* _pRoomName, const char* _pSpinId = nullptr) = 0;
    virtual void chatLeave(const char* _pRoomName, const SChatUserState* _pState) = 0;
    virtual void chatAddMessage(const char* _pRoomName, const char* _pUsername, char _type, const char* _pMessage) = 0;
};

}

#endif // SPINTEST_INTERFACE_CONNECTOR_H
