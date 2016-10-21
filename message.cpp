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


#include <unordered_map>
#include <algorithm>
#include <assert.h>
#include <iostream>

#include "message.h"
#include "common.h"
#include "logger.h"
#include "convert.h"

using namespace Spin;

namespace
{

void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if(from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::string createInitConnectionMessage(void* _pDescriptor)
{
    assert(_pDescriptor != nullptr);

    auto pDescriptor = reinterpret_cast<Spin::Job::SInitConnection*>(_pDescriptor);

    std::string result;

    replaceAll(pDescriptor->m_sid    , "-", "%2D");
    replaceAll(pDescriptor->m_sid    , "_", "%5F");
    replaceAll(pDescriptor->m_version, "-", "%2D");

    result = "%7B4%0AAhtmlchat%20" + pDescriptor->m_version + "%20ajax%0Aa" + pDescriptor->m_username
             + "%23" + pDescriptor->m_sid + "%0Au%23%23#%0Au%23e%23%0A";


    return result;
}

std::string createHeartbeatMessage(void* _pDescriptor)
{
    assert(_pDescriptor != nullptr);

    return "Jp%0A";
}

std::string createChatMessageMessage(void* _pDescriptor)
{
    assert(_pDescriptor != nullptr);

    auto descriptor = reinterpret_cast<Spin::Job::SIMMessage*>(_pDescriptor);

    char type;
    switch (descriptor->m_messageType)
    {
        case Spin::Job::Action: type = 'c'; break;
        case Spin::Job::Echo:   type = 'e'; break;
        default:
        case Spin::Job::Normal: type = 'a';
    }

    // TODO urlencode
    return "h" + Spin::urlEncode(Spin::convertUTF8toCP1252(descriptor->m_user)) + "%23" + "1" + "%23" + type + "%23" + Spin::urlEncode(descriptor->m_message) + "%0A";
}

std::string createIMTypingMessage(void* _pDescriptor)
{
    assert(_pDescriptor != nullptr);

    auto descriptor = reinterpret_cast<Spin::Job::SIMTyping*>(_pDescriptor);

    char state;

    switch (descriptor->m_state)
    {
        case Spin::Typing_Typing:
            state = '1';
            break;
        default:
        case Spin::Typing_Nothing:
            state = '0';
            break;
    }

    return "h" + Spin::urlEncode(descriptor->m_user) + "%232%230%23keypress%23" + state + "%0A";
}

std::string createChatRoomMessageMessage(void* _pDescriptor)
{
    assert(_pDescriptor != nullptr);

    auto descriptor = reinterpret_cast<Spin::Job::SChatMessage*>(_pDescriptor);

    char type;
    switch (descriptor->m_messageType)
    {
        case Spin::Job::Action: type = 'c'; break;
        case Spin::Job::Echo:   type = 'e'; break;
        default:
        case Spin::Job::Normal: type = 'a';
    }

    // TODO urlencode
    return "g" + Spin::urlEncode(descriptor->m_room) + "%23" + type + "%23" + Spin::urlEncode(descriptor->m_message) + "%0A";
}

std::string createJoinChatRoomMessage(void* _pDescriptor)
{
    assert(_pDescriptor != nullptr);

    Spin::Job::SJoinChatRoom* pDescriptor = reinterpret_cast<Spin::Job::SJoinChatRoom*>(_pDescriptor);

    std::string room = Spin::urlEncode(pDescriptor->m_room);

    return "c" + room + "%0A";
}

std::string createLeaveChatRoomMessage(void* _pDescriptor)
{
    assert(_pDescriptor != nullptr);

    Spin::Job::SLeaveChatRoom* pDescriptor = reinterpret_cast<Spin::Job::SLeaveChatRoom*>(_pDescriptor);

    std::string room = Spin::urlEncode(pDescriptor->m_room);

    return "d" + room + "%0A";
}

static bool internMixedJobAcceptDeclineBuddy(std::string&, Spin::CConnector*, bool);

bool mixedJobAcceptBuddy(void* _pData, Spin::CConnector* _pConnector)
{
    Spin::Job::SAuthAcceptBuddy* pJob = reinterpret_cast<Spin::Job::SAuthAcceptBuddy*>(_pData);

    return internMixedJobAcceptDeclineBuddy(pJob->m_userName, _pConnector, true);
}

bool mixedJobDeclineBuddy(void* _pData, Spin::CConnector* _pConnector)
{
    Spin::Job::SAuthDeclineBuddy* pJob = reinterpret_cast<Spin::Job::SAuthDeclineBuddy*>(_pData);

    return internMixedJobAcceptDeclineBuddy(pJob->m_userName, _pConnector, false);
}

static bool internMixedJobAcceptDeclineBuddy(std::string& _rUserName, Spin::CConnector* _pConnector, bool _accept)
{
    Spin::CLogger& rLogger = Spin::CLogger::getInstance();
    rLogger.logMutexed("spinp_debug.log", "mixedJobAcceptBuddy before Anything");

    Spin::CBaseConnector* pBaseConnector;
    Spin::CHttpRequest httpRequestAccept;

    std::string resultAccept;

    Spin::CConnector::SHomeRequestResult homeResult;

    _pConnector->execHomeRequest(true, homeResult);
    if (!homeResult.m_success)
    {
        Log::error("job_buddylist") << "mixedJobAcceptBuddy failed Home Request" << Log::end;
        return false;
    }

    Log::info("job_buddylist") << "found buddies:\n";

    for (auto& rCurrent : homeResult.m_buddyRequests)
    {
        Log::write() << rCurrent.m_buddyName << " " << rCurrent.m_id << "\n";
    }

    Log::write() << Log::end;

    Log::info("job_buddylist") << "searching user: " << _rUserName << Log::end;

    auto it = std::find_if(homeResult.m_buddyRequests.begin(), homeResult.m_buddyRequests.end(),
                           [_rUserName] (Spin::CConnector::SHomeRequestResult::SBuddyRequest& _rVal) -> bool
                           {
                               return _rVal.m_buddyName == _rUserName;
                           }
    );

    if (it == homeResult.m_buddyRequests.cend())
    {
        Log::error("job_buddylist") << "mixedJobAcceptBuddy cound't find buddy request" << Log::end;
        return false;
    }

    httpRequestAccept.setFollow(true);
    if (_accept) httpRequestAccept.setQuery("request/accept");
    else         httpRequestAccept.setQuery("request/reject");
    httpRequestAccept.setHeader("Accept-Encoding", "");
    httpRequestAccept.setGetParameter("id", it->m_id + ";private=1;familiar=0");

    Log::info("job_buddylist") << "mixedJobAcceptBuddy before Accept Request" << Log::end;

    if (!httpRequestAccept.execSync(&resultAccept))
    {
        Log::error("job_buddylist") << "mixedJobAcceptBuddy failed Accept Request" << Log::end;
        return false;
    }
    Log::info("job_buddylist") << "mixedJobAcceptBuddy after Accept Request" << Log::end;

    if (resultAccept.find("result") == std::string::npos ||
        resultAccept.find("1")      == std::string::npos    )
    {
        Log::error("job_buddylist") << "mixedJobAcceptBuddy result doesn't seems to be 1" << Log::end;
        return false;
    }

    pBaseConnector = _pConnector->getBaseConnector();

    Spin::CBaseConnector::SBuddyState buddyState;
    buddyState.m_flags = Spin::BuddyFlag_None;
    buddyState.m_name  = _rUserName;

    pBaseConnector->lock();
    pBaseConnector->buddyStateChange(&buddyState);
    pBaseConnector->unlock();

    return true;
}

bool mixedJobAskAuthBuddy(void* _pData, Spin::CConnector* _pConnector)
{
    Spin::CLogger& rLogger = Spin::CLogger::getInstance();
    Spin::Job::SAskAuthBuddy* pJob = reinterpret_cast<Spin::Job::SAskAuthBuddy*>(_pData);

    Spin::CHttpRequest httpRequestBuddyHomePage;
    Spin::CHttpRequest httpRequestFriendPage;

    size_t pos = 0;
    size_t endPos;

    std::string encodedUserName = Spin::urlEncode(Spin::convertUTF8toCP1252(pJob->m_userName));
    std::string userID;
    std::string httpRequestBuddyHomePageResult;
    std::string httpRequestFriendPageResult;

    httpRequestBuddyHomePage.setFollow(true);
    httpRequestBuddyHomePage.setQuery("hp/" + encodedUserName + "/");
    httpRequestBuddyHomePage.setHeader("Accept-Encoding", "");
    httpRequestBuddyHomePage.setHeader("Cache-Control", "max-age=0");
    httpRequestBuddyHomePage.setHeader("DNT", "1");
    httpRequestBuddyHomePage.setHeader("Update-Insecure-Requests", "1");

    if (!httpRequestBuddyHomePage.execSync(&httpRequestBuddyHomePageResult))
    {
        Log::error("job_buddylist") << "couldn't open homepage of user " << pJob->m_userName << Log::end;
        return false;
    }

    pos = httpRequestBuddyHomePageResult.find("/relations/friend?id=", pos);
    if (pos == std::string::npos)
    {
        Log::error("job_buddylist") << "couldn't find id of user " << pJob->m_userName << Log::end;
        return false;
    }
    pos += 21; // strlen("/relations/friend?id=")

    endPos = httpRequestBuddyHomePageResult.find("\"", pos);

    if (endPos == std::string::npos)
    {
        Log::error("job_buddylist") << "looks like the document is invalid for friend-request on user " << pJob->m_userName << Log::end;
        return false;
    }

    if (pos + 10 < endPos) // the userId is 7 characters long, but you never know
    {
        Log::error("job_buddylist") << "looks like the id is too long for friend-request on user " << pJob->m_userName << Log::end;
        return false;
    }

    userID.assign(httpRequestBuddyHomePageResult.substr(pos, endPos - pos));

    httpRequestFriendPage.setFollow(true);
    httpRequestFriendPage.setPostRedirect(true);
    httpRequestFriendPage.setQuery("relations/friend");
    httpRequestFriendPage.setHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    httpRequestFriendPage.setHeader("Accept-Encoding", "");
    httpRequestFriendPage.setHeader("Cache-Control", "max-age=0");
    httpRequestFriendPage.setHeader("DNT", "1");
    httpRequestFriendPage.setHeader("Update-Insecure-Requests", "1");
    httpRequestFriendPage.setHeader("Content-Type", "application/x-www-form-urlencoded");
    httpRequestFriendPage.setHeader("Origin", "http://www.spin.de");
    httpRequestFriendPage.setHeader("Referer", "http://www.spin.de/relations/friend?id=" + userID);

    httpRequestFriendPage.setPostParameter("_brix_detect_charset", "%A4+%26%23180%3B+%FC");
    httpRequestFriendPage.setPostParameter("session_id", _pConnector->getCookie("session1"));
    httpRequestFriendPage.setPostParameter("id", userID);
    httpRequestFriendPage.setPostParameter("message", pJob->m_message);
    httpRequestFriendPage.setPostParameter("type", "1");

    if (!httpRequestFriendPage.execSync(&httpRequestFriendPageResult))
    {
        Log::error("job_buddylist") << "friend page request failed for user " << pJob->m_userName << Log::end;
        return false;
    }

//Request URL:http://www.spin.de/relations/friend
//Request Method:POST
//Status Code:200 OK
//Remote Address:213.95.79.92:80
//
//     Headers:
//Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
//Accept-Encoding:gzip, deflate
//Accept-Language:de-DE,de;q=0.8,en-US;q=0.6,en;q=0.4
//Cache-Control:max-age=0
//Connection:keep-alive
//Content-Length:131
//Content-Type:application/x-www-form-urlencoded
//Cookie:settings=00110000300; setup=nologin=0; design=51-0; session1=ux3zgAJD5TYYchxegurAxghtFDwNVA5hG1dESJtMinhw9092396; session2=ux3zgAJD5TYYchxegurAxghtFDwNVA5hG1dESJtMinhw9092396
//DNT:1
//Host:www.spin.de
//Origin:http://www.spin.de
//Referer:http://www.spin.de/relations/friend?id=2532634
//Upgrade-Insecure-Requests:1
//User-Agent:Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36 Vivaldi/1.2.490.43
//
//  Form Data
//    _brix_detect_charset:%A4+%26%23180%3B+%FC
//    session_id:ux3zgAJD5TYYchxegurAxghtFDwNVA5hG1dESJtMinhw9092396
//    id:2532634
//    message:
//    type:1

    return true;
}

bool mixedJobGetRoomList(void* _pData, Spin::CConnector* _pConnector)
{
    Spin::CBaseConnector* pBaseConnector = nullptr;
    Spin::CLogger& rLogger = Spin::CLogger::getInstance();
    Spin::CHttpRequest httpRequest;
    std::vector<Spin::CBaseConnector::SRoom> rooms;
    std::string result;
    size_t pos = 0, endPos, tableClosingPos, tdClosingPos;
    std::string numberOfUsers;

    httpRequest.setFollow(true);
    httpRequest.setQuery("chat/index");
    httpRequest.setHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    httpRequest.setHeader("Accept-Encoding", "");
    httpRequest.setHeader("Cache-Control", "max-age=0");
    httpRequest.setHeader("DNT", "1");
    httpRequest.setHeader("Update-Insecure-Requests", "1");
    httpRequest.setHeader("Content-Type", "application/x-www-form-urlencoded");
    httpRequest.setHeader("Origin", "http://www.spin.de");
    httpRequest.setHeader("Referer", "http://www.spin.de/chat/index?cat=-2;sort=0");
    httpRequest.setGetParameter("cat", "-1");
    httpRequest.setGetParameter("sort", "0");

    if (!httpRequest.execSync(&result))
    {
        Log::error("job_roomlist") << "chatroomlist request failed" << Log::end;
        return false;
    }
//<table class="list ruler roomlist"
//    <tr class="odd first"><td class="col1"><a href="/chat/room-info?c=_%40g235890" class="ifrmlink" title="Raum-Infos anzeigen"><img src="/static/image/icon/room-group.png" width="46" height="25" alt=""></a></td><td class="col2"><a rel="nofollow" href="/chat/open?name=40636703323735f334" onclick="return j('_@g235890')">RandomRoomname</a></td><td class="col3"> <a href="#" onclick="return wh('_%40g235890')" title="Benutzerliste anzeigen">VOLL</a> </td></tr>

    pos = result.find("list ruler roomlist", pos);

    if (pos == std::string::npos)
    {
        Log::error("job_roomlist") << "couldn't find string \"list ruler room\"" << Log::end;
        return false;
    }

    tableClosingPos = result.find("</table>", pos);

    if (tableClosingPos == std::string::npos)
    {
        Log::error("job_roomlist") << "couldn't find end of table" << Log::end;
        return false;
    }

    Spin::CBaseConnector::SRoom room;

    while (pos < tableClosingPos)
    {
//        pos = result.find("/chat/room-info?c=", pos);
//        if (pos == std::string::npos) continue;
//        pos += 18;
//
//        endPos = result.find("\"", pos);
//        if (endPos == std::string::npos) continue;
//
//        std::string roomId = result.substr(pos, endPos - pos);
//
//        pos = endPos;


        pos = result.find("static/image/icon/", pos);
        if (pos == std::string::npos) continue;
        pos += 18;

        endPos = result.find(".png\"", pos);
        if (endPos == std::string::npos) continue;

        std::string roomType = result.substr(pos, endPos - pos);

             if (roomType == "room-spin" ) room.m_type = Spin::RoomType_Official;
        else if (roomType == "room-group") room.m_type = Spin::RoomType_Group   ;
        else if (roomType == "room-game" ) room.m_type = Spin::RoomType_Game    ;
        else if (roomType == "room"      ) room.m_type = Spin::RoomType_User    ;

        pos = endPos;


        pos = result.find("onclick", pos);
        if (pos == std::string::npos) continue;

        pos = result.find("j('", pos);
        if (pos == std::string::npos) continue;
        pos += 3;

        endPos = result.find("')\"", pos);
        if (endPos == std::string::npos) continue;

//        std::string roomId = result.substr(pos, endPos - pos);
        room.m_pId = result.substr(pos, endPos - pos);

        pos = endPos;


        pos = result.find("\">", pos);
        if (pos == std::string::npos) continue;

        pos += 2;

        endPos = result.find("</a>", pos);
        if (endPos == std::string::npos) continue;

        room.m_pName = result.substr(pos, endPos - pos);
        size_t bpos = room.m_pName.find("<b>");
        if (bpos != std::string::npos)
        {
            room.m_pName.replace(bpos, 3, nullptr, 0);
            bpos = room.m_pName.find("</b>");
            if (bpos != std::string::npos)
            {
                room.m_pName.replace(bpos, 4, nullptr, 0);
            }
        }

        room.m_pName = Spin::convertISO8859_15toUTF8(room.m_pName);

        pos = endPos;

        pos = result.find("col3", pos);
        if (pos == std::string::npos) continue;
        tdClosingPos = result.find("</td>");
        if (tdClosingPos == std::string::npos) continue;
        pos = result.find("title", pos);
        if (pos != std::string::npos && tdClosingPos < pos)
        {
            pos = result.find("\">", pos);
            if (pos == std::string::npos) continue;
            pos += 2;
            endPos = result.find("</a>", pos);
            if (endPos == std::string::npos) continue;

            numberOfUsers.assign(result.substr(pos, endPos - pos));

            if (numberOfUsers == "VOLL")
            {
                room.m_numberOfUsers = -1;
            }
            else
            {
                room.m_numberOfUsers = atoi(numberOfUsers.c_str());
            }
        }
        else
        {
            room.m_numberOfUsers = 0;
        }

        rooms.push_back(room);
        Log::error("job_roomlist") << roomType << " " << room.m_pId << " " << room.m_pName << " " << numberOfUsers << " " << room.m_numberOfUsers << Log::end;
    }

    pBaseConnector = _pConnector->getBaseConnector();

    pBaseConnector->lock();
    pBaseConnector->chatSetList(rooms);
    pBaseConnector->unlock();

    return true;
}

}

namespace Spin
{
namespace Job
{

typedef std::string (*createFunction)(void*);
typedef const std::unordered_map<int, Spin::Job::createFunction> TFunctionMap;
const TFunctionMap jobFunctionMap =
{
    { JobOutType_InitConnection, &createInitConnectionMessage  },
    { JobOutType_Heartbeat     , &createHeartbeatMessage       },
    { JobOutType_IMMessage     , &createChatMessageMessage     },
    { JobOutType_ChatMessage   , &createChatRoomMessageMessage },
    { JobOutType_JoinChatRoom  , &createJoinChatRoomMessage    },
    { JobOutType_LeaveChatRoom , &createLeaveChatRoomMessage   },
    { JobOutType_IMTyping      , &createIMTypingMessage        },
};

typedef const std::unordered_map<int, Spin::Job::SMixedJob::TFunction> TMixedJobFunctionMap;
const TMixedJobFunctionMap mixedJobFunctionMap =
{
    { JobMixedType_AcceptAuth      , &mixedJobAcceptBuddy      },
    { JobMixedType_DeclineAuth     , &mixedJobDeclineBuddy     },
    { JobMixedType_AskAuthBuddy    , &mixedJobAskAuthBuddy     },
    { JobMixedType_GetRoomList     , &mixedJobGetRoomList      }
};

bool SMixedJob::runJob(CConnector* _pConnector)
{
    auto jobFn = mixedJobFunctionMap.find(m_mixedType);

    if (jobFn == mixedJobFunctionMap.cend())
    {
        return false;
    }

    return (jobFn->second)(this, _pConnector);
}

//
//void SAuthAcceptBuddy::function(CConnector* _pConnector)
//{
//    CLogger& rLogger = CLogger::getInstance();
//    rLogger.logMutexed("spinp_debug.log", "before Anything");
//
//    CBaseConnector* pBaseConnector;
//    CHttpRequest httpRequestHome;
//    CHttpRequest httpRequestAccept;
//    size_t pos, endPos;
//    std::string resultHome;
//    std::string resultAccept;
//
//    httpRequestHome.setQuery("home");
//
//    rLogger.logMutexed("spinp_debug.log", "before Home Request");
//    if (_pConnector->execSyncHttpRequest(&httpRequestHome, resultHome))
//    {
//        rLogger.logMutexed("spinp_debug.log", "failed Home Request");
//        return;
//    }
//    rLogger.logMutexed("spinp_debug.log", "after Home Request");
//
//    pos = resultHome.find("data-request-who=\"" + m_userName + "\"");
//
//    if (pos == std::string::npos)
//    {
//        return;
//    }
//
//    pos = resultHome.find("href=\"", pos) + 6;
//
//    if (pos == std::string::npos)
//    {
//        return;
//    }
//
//    pos = resultHome.find("?id=", pos) + 4;
//
//    if (pos == std::string::npos)
//    {
//        return;
//    }
//
//    endPos = resultHome.find("\"", pos);
//
//    if (endPos == std::string::npos)
//    {
//        return;
//    }
//
//    std::string getParameter = resultHome.substr(pos, endPos - pos);
//
//    httpRequestAccept.setQuery("request/accept");
//    httpRequestAccept.setGetParameter("id", getParameter + ";private=1;familiar=0");
//
//    rLogger.logMutexed("spinp_debug.log", "before Accept Request");
//
//    if (_pConnector->execSyncHttpRequest(&httpRequestAccept, resultAccept))
//    {
//        rLogger.logMutexed("spinp_debug.log", "failed Accept Request");
//        return;
//    }
//    rLogger.logMutexed("spinp_debug.log", "after Accept Request");
//
//    if (resultAccept.find("result") == std::string::npos ||
//        resultAccept.find("1")      == std::string::npos    )
//    {
//        return;
//    }
//
//    pBaseConnector = _pConnector->getBaseConnector();
//
//    CBaseConnector::SBuddyState buddyState;
//    buddyState.m_flags = BuddyFlag_None;
//    buddyState.m_pName = m_userName.c_str();
//
//    pBaseConnector->lock();
//    pBaseConnector->buddyStateChange(&buddyState);
//    pBaseConnector->unlock();
//}
//
//void SAuthDeclineBuddy::function(CConnector* _pConnector)
//{
//
//}


}

void pushJob(CConnector* _pConnector, Job::SBaseJob* _pDescriptor)
{
    CLogger& rLogger = CLogger::getInstance();

    Log::info("job_sys") << "pushing job TYPE=" << _pDescriptor->m_type << " JOBTYPE=";

    switch (_pDescriptor->m_type)
    {
        case Job::JobType_OutRequest:
            {
                Job::SOutJob* pOutJob = reinterpret_cast<Job::SOutJob*>(_pDescriptor);
                Log::write() << "OutRequest" << Log::end;
                Job::TFunctionMap::const_iterator fn = Job::jobFunctionMap.find(pOutJob->m_outType);

                if (fn == Job::jobFunctionMap.cend())
                {
                    Log::error("job_sys") << "job isn't in the function-map" << Log::end;
                    break;
                }

                _pConnector->pushOut((*fn->second)(_pDescriptor));
            }
            break;

        case Job::JobType_Mixed:
            {
                Log::write() << "Mixed" << Log::end;
                _pConnector->pushJob(_pDescriptor);
            }
            break;

        case Job::JobType_Uninitialized:
            {
                Log::error("job_sys") << "unknown job" << Log::end;
            }
            break;
    }

}

}