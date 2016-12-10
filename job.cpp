//
// Created by flex on 07.12.16.
//

#include <assert.h>
#include <algorithm>
#include "job.h"
#include "common.h"
#include "convert.h"
#include "logger.h"
#include "extern/json/json.hpp"
#include "htmlparser.h"
#include "connector.h"

namespace Spin
{
namespace Job
{

bool SAskAuthBuddy::run(CConnector* _pConnector)
{
    Spin::CHttpRequest httpRequestBuddyHomePage;
    Spin::CHttpRequest httpRequestFriendPage;

    size_t pos = 0;
    size_t endPos;

    std::string encodedUserName = Spin::urlEncode(Spin::convertUTF8toCP1252(m_userName));
    std::string userID;
    std::string httpRequestBuddyHomePageResult;
    std::string httpRequestFriendPageResult;

    httpRequestBuddyHomePage.setFollow(true);
    httpRequestBuddyHomePage.setQuery("hp/" + encodedUserName + "/");
    httpRequestBuddyHomePage.setHeader("Accept-Encoding", "");
    httpRequestBuddyHomePage.setHeader("Cache-Control", "max-age=0");
    httpRequestBuddyHomePage.setHeader("DNT", "1");
    httpRequestBuddyHomePage.setHeader("Update-Insecure-Requests", "1");

    if (! httpRequestBuddyHomePage.execSync(&httpRequestBuddyHomePageResult))
    {
        Log::error("job_askauthbuddy") << "couldn't open homepage of user " << m_userName << Log::end;
        return false;
    }

    pos = httpRequestBuddyHomePageResult.find("/relations/friend?id=", pos);
    if (pos == std::string::npos)
    {
        Log::error("job_askauthbuddy") << "couldn't find id of user " << m_userName << Log::end;
        return false;
    }
    pos += 21; // strlen("/relations/friend?id=")

    endPos = httpRequestBuddyHomePageResult.find("\"", pos);

    if (endPos == std::string::npos)
    {
        Log::error("job_askauthbuddy") << "looks like the document is invalid for friend-request on user " << m_userName
                                       << Log::end;
        return false;
    }

    if (pos + 10 < endPos) // the userId is 7 characters long, but you never know
    {
        Log::error("job_askauthbuddy") << "looks like the id is too long for friend-request on user " << m_userName
                                       << Log::end;
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
    httpRequestFriendPage.setPostParameter("message", m_message);
    httpRequestFriendPage.setPostParameter("type", "1");

    if (! httpRequestFriendPage.execSync(&httpRequestFriendPageResult))
    {
        Log::error("job_askauthbuddy") << "friend page request failed for user " << m_userName << Log::end;
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


static bool internMixedJobAcceptDeclineBuddy(std::string& _rUserName, Spin::CConnector* _pConnector, bool _accept)
{
    Spin::CLogger& rLogger = Spin::CLogger::getInstance();
    rLogger.logMutexed("spinp_debug.log", "mixedJobAcceptBuddy before Anything");

    Spin::CBaseConnector* pBaseConnector;
    Spin::CHttpRequest httpRequestAccept;

    std::string resultAccept;

    Spin::CConnector::SHomeRequestResult homeResult;

    _pConnector->execHomeRequest(true, homeResult);
    if (! homeResult.m_success)
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
                           [_rUserName](Spin::CConnector::SHomeRequestResult::SBuddyRequest& _rVal) -> bool
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
    else httpRequestAccept.setQuery("request/reject");
    httpRequestAccept.setHeader("Accept-Encoding", "");
    httpRequestAccept.setGetParameter("id", it->m_id + ";private=1;familiar=0");

    Log::info("job_buddylist") << "mixedJobAcceptBuddy before Accept Request" << Log::end;

    if (! httpRequestAccept.execSync(&resultAccept))
    {
        Log::error("job_buddylist") << "mixedJobAcceptBuddy failed Accept Request" << Log::end;
        return false;
    }
    Log::info("job_buddylist") << "mixedJobAcceptBuddy after Accept Request" << Log::end;

    if (resultAccept.find("result") == std::string::npos ||
        resultAccept.find("1") == std::string::npos)
    {
        Log::error("job_buddylist") << "mixedJobAcceptBuddy result doesn't seems to be 1" << Log::end;
        return false;
    }

    pBaseConnector = _pConnector->getBaseConnector();

    Spin::CBaseConnector::SBuddyState buddyState;
    buddyState.m_flags = Spin::BuddyFlag_None;
    buddyState.m_name = _rUserName;

    pBaseConnector->lock();
    pBaseConnector->buddyStateChange(&buddyState);
    pBaseConnector->unlock();

    return true;
}

bool SAuthAcceptBuddy::run(CConnector* _pConnector)
{
    return internMixedJobAcceptDeclineBuddy(m_userName, _pConnector, true);
}

bool SAuthDeclineBuddy::run(CConnector* _pConnector)
{
    return internMixedJobAcceptDeclineBuddy(m_userName, _pConnector, false);
}

bool SGetRoomList::run(CConnector* _pConnector)
{
    Spin::CBaseConnector* pBaseConnector = nullptr;
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

    if (! httpRequest.execSync(&result))
    {
        Log::error("job_getroomlist") << "chatroomlist request failed" << Log::end;
        return false;
    }
//<table class="list ruler roomlist"
//    <tr class="odd first"><td class="col1"><a href="/chat/room-info?c=_%40g235890" class="ifrmlink" title="Raum-Infos anzeigen"><img src="/static/image/icon/room-group.png" width="46" height="25" alt=""></a></td><td class="col2"><a rel="nofollow" href="/chat/open?name=40636703323735f334" onclick="return j('_@g235890')">RandomRoomname</a></td><td class="col3"> <a href="#" onclick="return wh('_%40g235890')" title="Benutzerliste anzeigen">VOLL</a> </td></tr>

    pos = result.find("list ruler roomlist", pos);

    if (pos == std::string::npos)
    {
        Log::error("job_getroomlist") << "couldn't find string \"list ruler room\"" << Log::end;
        return false;
    }

    tableClosingPos = result.find("</table>", pos);

    if (tableClosingPos == std::string::npos)
    {
        Log::error("job_getroomlist") << "couldn't find end of table" << Log::end;
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

        if (roomType == "room-spin") room.m_type = Spin::RoomType_Official;
        else if (roomType == "room-group") room.m_type = Spin::RoomType_Group;
        else if (roomType == "room-game") room.m_type = Spin::RoomType_Game;
        else if (roomType == "room") room.m_type = Spin::RoomType_User;

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
                room.m_numberOfUsers = - 1;
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
        Log::error("job_getroomlist") << roomType << " " << room.m_pId << " " << room.m_pName << " " << numberOfUsers
                                      << " " << room.m_numberOfUsers << Log::end;
    }

    pBaseConnector = _pConnector->getBaseConnector();

    pBaseConnector->lock();
    pBaseConnector->chatSetList(rooms);
    pBaseConnector->unlock();

    return true;
}

bool SRefreshBuddyList::run(CConnector* _pConnector)
{
    Spin::CLogger& rLogger = Spin::CLogger::getInstance();
    Spin::CHttpRequest httpRequest;
    std::string result;

    rLogger.logMutexed("spinp_debug.log", "on requestBuddyListUpdate 1");
//General:
//    Request URL:http://www.spin.de/api/friends
//    Request Method:POST
//    Status Code:200 OK
//    Remote Address:213.95.79.92:80

//Headers:
//Accept:application/json, text/javascript, */*; q=0.01
//Accept-Encoding:gzip, deflate
//Accept-Language:de-DE,de;q=0.8,en-US;q=0.6,en;q=0.4
//Cache-Control:no-cache
//Connection:keep-alive
//Content-Length:74
//Content-Type:application/x-www-form-urlencoded; charset=UTF-8
//Cookie:...
//Host:www.spin.de
//Origin:http://www.spin.de
//Pragma:no-cache
//Referer:http://www.spin.de/loggedin
//User-Agent:...
//X-Requested-With:XMLHttpRequest

//Form Data:
//    session:ux3zgAJD5TYYchxegurAxghtFDwNVA5hG1dESJtMinhw9092396
//    photo:1
//    utf8:1

    httpRequest.setQuery("api/friends");
    httpRequest.setFollow(true);
    httpRequest.setPostRedirect(true);
    httpRequest.setHeader("Accept", "application/json, text/javascript, */*; q=0.01");
    httpRequest.setHeader("Accept-Encoding", "");
    httpRequest.setHeader("Cache-Control", "no-cache");
    httpRequest.setHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
    httpRequest.setHeader("Origin", "http://www.spin.de");
    httpRequest.setHeader("Pragma", "no-cache");
    httpRequest.setHeader("Referer", "http://www.spin.de/loggedin");
    httpRequest.setHeader("X-Requested-With", "XMLHttpRequest");

    httpRequest.setPostParameter("session", _pConnector->getCookie("session1"));
    httpRequest.setPostParameter("photo", "1");
    httpRequest.setPostParameter("utf8", "1");

    if (! httpRequest.execSync(&result))
    {
        rLogger.log("spinp_debug.log", "api-friends-request failed");
        return false;
    }

    // [
    //      [3428590,"RandomUsername1",1,"",0  ,""                                                         ,"y",1],
    //      [35806  ,"RandomUsername2",0,"",34 ,"\/\/p2.spin.de\/user\/mini\/e4\/0f\/71957025-23623632.jpg","y",1],
    //      [843569 ,"RandomUsername3",0,"",245,""                                                         ,"y",1]
    // ]

    rLogger.logMutexed("spinp_debug.log", "on requestBuddyListUpdate 2");
    rLogger.logMutexed("spinp_debug.log", result);
    nlohmann::json jsonObject = nlohmann::json::parse(result);

    rLogger.logMutexed("spinp_debug.log", "on requestBuddyListUpdate 2.1");

    std::vector<CBaseConnector::SBuddyState> buddyList;

    rLogger.logMutexed("spinp_debug.log", "on requestBuddyListUpdate 3");
    for (auto& current : jsonObject)
    {
        if (current.is_array())
        {
            CBaseConnector::SBuddyState buddyState;
            buddyState.m_flags = BuddyFlag_None;

            auto& el = current.at(0);;
            if (el.is_number()) buddyState.m_id = el.get<int>();
            else buddyState.m_id = - 1;

            el = current.at(1);
            if (el.is_string()) buddyState.m_name = el.get<std::string>();
            else
            {
                buddyState.m_name = "";
                continue;
            }

            buddyState.m_name = convertASCIIWithEscapedUTF16toUTF8(buddyState.m_name);

            el = current.at(2);
//            if (el.is_number()) buddyData.m_isOnline = el.get<int>();
//            else                buddyData.m_isOnline = 0;
            int isOnline = 0;
            if (el.is_number()) isOnline = el.get<int>();
            if (isOnline) buddyState.m_flags |= BuddyFlag_Online;
            else buddyState.m_flags |= BuddyFlag_Offline;

            el = current.at(5);
//            if (!el.is_string()) buddyData.m_thumbnailURL = el.get<std::string>();
//            else                 buddyData.m_thumbnailURL = "";

            buddyList.push_back(buddyState);
        }
    }

    Spin::CBaseConnector* pBaseConnector = _pConnector->getBaseConnector();
    pBaseConnector->lock();
    pBaseConnector->buddyListRefresh(buddyList);
    pBaseConnector->unlock();

    return true;
}

bool SRequestBuddyImage::run(CConnector* _pConnector)
{
    std::string& rUrl = m_url;

    Spin::CHttpRequest httpRequest;

    std::vector<char> result;
    size_t pos = rUrl.find("spin.de");

    if (pos == std::string::npos)
    {
        Log::error("job_request_buddyimage") << "invalid url for image-request - url: " << rUrl << Log::end;
        return false;
    }

    // cutting and set the query-url for CHttpRequest
    httpRequest.setSubDomain(rUrl.substr(2, pos - 3));
    httpRequest.setQuery(rUrl.substr(pos + 7));

    Log::info("job_request_buddyimage") << "starting request" << Log::end;
    if (! httpRequest.execSync(&result))
    {
        Log::error("job_request_buddyimage") << "image-request to " << rUrl << " failed" << Log::end;
        return false;
    }
    Log::info("job_request_buddyimage") << "image size: " << result.size() << " bytes" << Log::end;

    char* pImageData = result.data();
    size_t imageLength = result.size();

    Spin::CBaseConnector* pBaseConnector = _pConnector->getBaseConnector();

    pBaseConnector->lock();
    pBaseConnector->buddyAddImage(m_userName.c_str(), pImageData, imageLength, rUrl.c_str());
    pBaseConnector->unlock();

    return true;
}

bool SRequestUserProfile::run(CConnector* _pConnector)
{
    struct SInfos
    {
        SInfos()
            : m_personalInfo()
            , m_photos()
        {

        }

        Spin::CBaseConnector::TUserProfile m_personalInfo;
        Spin::CBaseConnector::TUserProfile m_photos;
    };

    SInfos infos;

    infos.m_personalInfo.emplace_back(Spin::CUserProfileEntry::UserInfoType_Headline, "Persönliches"); // ("Persönliches"));
    infos.m_photos.emplace_back(Spin::CUserProfileEntry::UserInfoType_Headline, "Fotos");

    std::string result;
    Spin::CHttpRequest httpRequest;

    httpRequest.setQuery("hp/" + m_userName);
    httpRequest.setFollow(true);
    httpRequest.setHeader("Accept", "*/*; q=0.01");
    httpRequest.setHeader("Accept-Encoding", "");
    httpRequest.setHeader("Cache-Control", "no-cache");
    httpRequest.setHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
    httpRequest.setHeader("Origin", "http://www.spin.de");
    httpRequest.setHeader("Referer", "http://www.spin.de/home");

    if (! httpRequest.execSync(&result))
    {
        Log::error("job_request_userprofile") << "request for user " << m_userName << " failed" << Log::end;
        return false;
    }

    Log::info("job_request_profile") << result << Log::end;

    Spin::CHTMLParser<SInfos> parser;
    parser.parse(
        result.c_str(),
        [](SInfos* _pInfos, Spin::CHTMLNode* _pNode) -> bool
        {
            // <a href="/hp/varie/photo" style=""><img itemprop="image" class="thumb photo" src="//p3.spin.de/user/mini/56/82/8890c4d5-74911046.jpg" alt="" width="90" height="90" data-userid="7c2079" data-skip-default="1"></a>
            if (_pNode->hasAttributeContainingValue("itemprop", "image") &&
                _pNode->hasAttributeContainingValue("class", "thumb") &&
                _pNode->hasAttributeContainingValue("class", "photo"))
            {
                std::string photoURL = _pNode->getAttributeValue("src");
                size_t pos = photoURL.find("/user/mini/");

                if (pos != std::string::npos)
                {
                    photoURL.replace(pos + 6, 4, "full");

                    _pInfos->m_photos.emplace_back(Spin::CUserProfileEntry::UserInfoType_Pair, "Profilbild", std::move(photoURL));
                }
            }

            // <tr class="data">
            if (_pNode->hasAttributeContainingValue("class", "data"))
            {
                std::string label;
                std::string value;

                Spin::CHTMLNode child = _pNode->getFirstChild();

                // <td class="label col1">
                if (child.hasAttributeContainingValue("class", "col1"))
                {
                    label = child.getContent();

                    // <td class="col2">
                    if (child.next().hasAttributeContainingValue("class", "col2"))
                    {
                        value = child.getContent();

                        if (value.length() == 0)
                        {
                            // <span>
                            child = child.getFirstChild();

                            value = child.getContent();
                        }
                    }

                    _pInfos->m_personalInfo.emplace_back(Spin::CUserProfileEntry::UserInfoType_Pair, std::move(label), std::move(value));

                    return false;
                }
            }

            return true;
        },
        &infos);

    infos.m_photos.emplace_back();
    Spin::CBaseConnector::TUserProfile resultingInfos;

    resultingInfos = std::move(infos.m_personalInfo);
    std::copy(infos.m_photos.begin(), infos.m_photos.end(), std::back_inserter(resultingInfos));

    Spin::CBaseConnector* pBaseConnector = _pConnector->getBaseConnector();

    pBaseConnector->lock();
    pBaseConnector->buddySetProfile(m_userName.c_str(), std::move(resultingInfos));
    pBaseConnector->unlock();


    return true;
}

bool SUpdateUserInfo::run(CConnector* _pConnector)
{
    Spin::CConnector::SUserInfo userInfo;
    const std::string& rUserName = m_userName;

    Spin::CHttpRequest httpRequest;
    std::string result;
    httpRequest.setQuery("api/userinfo");
//    httpRequest.setFollow(true);
//    httpRequest.setPostRedirect(true);
    httpRequest.setHeader("Accept", "*/*; q=0.01");
    httpRequest.setHeader("Accept-Encoding", "");
    httpRequest.setHeader("Cache-Control", "no-cache");
    httpRequest.setHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
    httpRequest.setHeader("Origin", "http://www.spin.de");
    httpRequest.setHeader("Pragma", "no-cache");
    httpRequest.setHeader("Referer", "http://www.spin.de/tabs/dialog?v=2016-06-02");
    httpRequest.setHeader("X-Requested-With", "XMLHttpRequest");
    httpRequest.setGetParameter("hexlogin", hexEncode(rUserName));
    httpRequest.setGetParameter("utf8", "1");
    httpRequest.setGetParameter("session_id", _pConnector->getCookie("session1"));

    Log::info("job_update_userinfo") << "starting api-userinfo-request" << Log::end;
    if (! httpRequest.execSync(&result))
    {
        Log::error("intern_job") << "api-userinfo-request failed for user " << rUserName << " failed" << Log::end;
        return false;
    }

    Log::info("job_update_userinfo") << "result:\n" << result << Log::end;

    nlohmann::json jsonObject = nlohmann::json::parse(result);

    if (! jsonObject.is_object())
    {
        Log::error("intern_job") << "result for api-userinfo-request isn't an object, for user " << rUserName
                                 << " failed" << Log::end;
        return false;
    }

    auto obj = jsonObject.find("registered");
    if (obj != jsonObject.cend() && obj->is_number()) userInfo.m_registered = static_cast<bool>(obj->get<int>());
    else userInfo.m_registered = false;

    obj = jsonObject.find("photourl");
    if (obj != jsonObject.cend() && obj->is_string()) userInfo.m_thumbURL.assign(obj->get<std::string>());
    else userInfo.m_thumbURL.assign("");

    obj = jsonObject.find("fullphoto");
    if (obj != jsonObject.cend() && obj->is_string()) userInfo.m_photoURL.assign(obj->get<std::string>());
    else userInfo.m_photoURL.assign("");

    obj = jsonObject.find("age");
    if (obj != jsonObject.cend() && obj->is_number()) userInfo.m_age = static_cast<char>(obj->get<int>());
    else userInfo.m_age = - 1;

    obj = jsonObject.find("id");
    if (obj != jsonObject.cend() && obj->is_string()) userInfo.m_id.assign(obj->get<std::string>());
    else userInfo.m_id.assign("");

    obj = jsonObject.find("login");
    if (obj != jsonObject.cend() && obj->is_string()) userInfo.m_buddyName.assign(obj->get<std::string>());
    else userInfo.m_buddyName.assign("");

    obj = jsonObject.find("sex");
    if (obj != jsonObject.cend() && obj->is_string()) userInfo.m_gender = obj->get<std::string>()[0];
    else userInfo.m_gender = - 1;

    obj = jsonObject.find("nofake");
    if (obj != jsonObject.cend() && obj->is_number()) userInfo.m_noFake = obj->get<int>();
    else userInfo.m_noFake = false;

    obj = jsonObject.find("underage");
    if (obj != jsonObject.cend() && obj->is_number()) userInfo.m_underage = obj->get<int>();
    else userInfo.m_underage = false;

    userInfo.m_isValid = true;

    _pConnector->setUserInfo(rUserName, userInfo);

//    std::cout << " AAAAAAAAAAAA " << std::endl;
//    std::cout << pData->m_buddyImageHash << std::endl;
//    std::cout << " BBBBBBBBBBBB " << std::endl;

    if (m_isBuddy &&
        userInfo.m_registered &&
        m_buddyImageHash != userInfo.m_thumbURL &&
        userInfo.m_thumbURL.length() > 0)
    {
        SRequestBuddyImage job;
//        std::swap(job.m_userName, rUserName          );
//        std::swap(job.m_url     , userInfo.m_thumbURL);
        job.m_userName = std::move(m_userName);
        job.m_url = std::move(m_buddyImageHash);

        _pConnector->pushJob(std::move(job));
//        Spin::pushJob(_pConnector, std::move(job));
//        Job::SRequestBuddyImage* pJob = new Job::SRequestBuddyImage();
//        pJob->m_userName = rUserName;
//        pJob->m_url      = userInfo.m_thumbURL;
//        Spin::pushJob(_pConnector, pJob);
    }

    return true;

//    if (!rUserInfo.m_isValid)
//    {
//        Log::error("job_userinfo") << "userinfo for user " << pJobData->m_userName << " is invalid";
//        return false;
//    }

//    Spin::CHttpRequest httpRequest;

//    std::vector<char> result;
//    size_t pos = rUserInfo.m_photoURL.find("spin.de");
//
//    if (pos == std::string::npos)
//    {
//        Log::error("job_userinfo") << "invalid url for image-request - url: " << rUserInfo.m_photoURL << Log::end;
//        return false;
//    }

//    // cutting and set the query-url for CHttpRequest
//    httpRequest.setSubDomain(_rImageURL.substr(2, pos - 3));
//    httpRequest.setQuery(_rImageURL.substr(pos + 7));
//
//    if (!httpRequest.execSync(&result))
//    {
//        Log::error("intern_job") << "image-request to " << _rImageURL << " failed" << Log::end;
//        return;
//    }
//
//
//    char* pData = result.data();
}


CJob::CJob()
    : m_pJob(nullptr)
{

}

CJob::~CJob()
{
    if (m_pJob != nullptr) delete m_pJob;
}

bool CJob::run(CConnector* _pConnector)
{
    assert(m_pJob != nullptr);
    std::cout << "HOHO 2" << std::endl;
    std::cout << m_pJob << std::endl;
    std::cout << _pConnector << std::endl;
    return m_pJob->run(_pConnector);
}

} // namespace Job
} // namespace Spin