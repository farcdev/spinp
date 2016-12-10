/**
 * libspinp.cpp
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



#include "connector.h"
#include "message.h"
#include "purple_connector.h"
#include "logger.h"
#include "convert.h"

#ifndef CMAKE
#include <plugin.h>
#include <prpl.h>
#include <debug.h>
#include <version.h>
#else
#include <libpurple/plugin.h>
#include <libpurple/prpl.h>
#include <libpurple/debug.h>
#include <libpurple/version.h>
#endif

#include <assert.h>
#include <iostream>

#ifdef CMAKE
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"
#endif

#define SPINP_PRPL_ID "prpl-spinchat-protocol"

//static Spin::CPurpleConnector* g_pPurpleConnector = nullptr;
//static PurplePlugin*           g_pPurplePlugin    = nullptr;

static guint g_derp;

static Spin::CPurpleConnector* getPurpleConnector(PurpleConnection* _pConnection)
{
    g_return_val_if_fail(purple_connection_get_protocol_data(_pConnection) != NULL, nullptr);

    return reinterpret_cast<Spin::CPurpleConnector*>(purple_connection_get_protocol_data(_pConnection));
}

static gboolean plugin_load(PurplePlugin* _pPlugin)
{
    Spin::Log::getInstance().init();
    Spin::initConvert();
    purple_debug_info(SPINP_PRPL_ID, "plugin_load\n");
    return TRUE;
}

static gboolean plugin_unload(PurplePlugin* _pPlugin)
{
    Spin::uninitConvert();
    Spin::Log::getInstance().uninit();
    purple_debug_info(SPINP_PRPL_ID, "plugin_unload\n");
    return TRUE;
}

static gboolean spinp_event_loop(gpointer _pData)
{
    Spin::CPurpleConnector* pPurpleConnector = reinterpret_cast<Spin::CPurpleConnector*>(_pData);

    if (pPurpleConnector == nullptr)
    {
        return FALSE;
    }

    pPurpleConnector->purpleOnLoop();

    return TRUE;
}

static GList* spinp_status_types(PurpleAccount* _pAccount)
{
    GList* pTypes = NULL;
    PurpleStatusType* pStatus;

    pStatus = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, "online", NULL, TRUE, TRUE, FALSE);
    pTypes = g_list_append(pTypes, pStatus);

    pStatus = purple_status_type_new_full(PURPLE_STATUS_AWAY, "away", NULL, TRUE, TRUE, FALSE);
    pTypes = g_list_append(pTypes, pStatus);

    pStatus = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, "offline", NULL, TRUE, TRUE, FALSE);
    pTypes = g_list_append(pTypes, pStatus);

//    pStatus = purple_status_type_new_full(PURPLE_STATUS_UNSET)

    return pTypes;
}

static GList* spinp_chat_info(PurpleConnection* _pConnection)
{
    GList* pList = NULL;
    struct proto_chat_entry* pEntry;

    pEntry = g_new0(struct proto_chat_entry, 1);
    pEntry->label      = "_Room:";
    pEntry->identifier = "room";
    pEntry->required   = TRUE;

    pList = g_list_append(pList, pEntry);

    return pList;
}

static int* spinp_get_handle()
{
    static int s_handle;

    return &s_handle;
}

static void spinp_event_conversation_created(PurpleConversation* _pConversation)
{
    PurpleConversationType type = purple_conversation_get_type(_pConversation);

//    PurpleBuddyIcon* pIcon = purple_buddy_icons_find(purple_conversation_get_account(_pConversation), purple_conversation_get_name(_pConversation));
//
//    if (pIcon != NULL)
//    {
//        std::cout << "ICON FOUND" << std::endl;
//        std::cout << "ICON FOUND" << std::endl;
//        std::cout << "ICON FOUND" << std::endl;
//        std::cout << "ICON FOUND" << std::endl;
//        std::cout << "ICON FOUND" << std::endl;
//        std::cout << "ICON FOUND" << std::endl;
//        std::cout << "ICON FOUND" << std::endl;
//        std::cout << "ICON FOUND" << std::endl;
//    }

    if (type == PURPLE_CONV_TYPE_IM)
    {
        Spin::CPurpleConnector* pPurpleConnector =
            reinterpret_cast<Spin::CPurpleConnector*>(
                purple_connection_get_protocol_data(
                    purple_account_get_connection(
                        purple_conversation_get_account(_pConversation)
                    )
                )
            );

        pPurpleConnector->purpleOnIMConversationCreated(_pConversation);

//        const char* pUserName = purple_conversation_get_name(_pConversation);
//        const char* pHash     = NULL;
//
//        Spin::Job::SIMInit imInit;
//        imInit.m_userName = pUserName;
//        Spin::pushJob(pPurpleConnector->getConnector(), &imInit);
//
////        PurpleBuddyIcon* pBuddyIcon = purple_buddy_icons_find(purple_conversation_get_account(_pConversation), pUserName);
//
////        if (pBuddyIcon != NULL)
////            pHash = purple_buddy_icon_get_checksum(pBuddyIcon);
//
////        purple_buddy_icons_get_checksum_for_user()
//
//
//        PurpleBuddy* pBuddy = purple_find_buddy(purple_conversation_get_account(_pConversation), pUserName);
//        if (pBuddy != NULL)
//        {
//            PurpleBuddyIcon* pBuddyIcon = purple_buddy_get_icon(pBuddy);
//
//            if (pBuddyIcon != NULL)
//                pHash = purple_buddy_icon_get_checksum(pBuddyIcon);
//        }
//
//        Spin::Job::SUpdateUserInfo* pUpdateUserInfo = new Spin::Job::SUpdateUserInfo();
//                           pUpdateUserInfo->m_userName      .assign(pUserName);
//        if (pHash != NULL) pUpdateUserInfo->m_buddyImageHash.assign(pHash);
//        Spin::pushJob(pPurpleConnector->getConnector(), pUpdateUserInfo);
    }
}

static void spinp_login(PurpleAccount* _pAccount)
{
    purple_debug_info(SPINP_PRPL_ID, "spinp_login\n");

    Spin::CPurpleConnector* pPurpleConnector = new Spin::CPurpleConnector();
    assert(pPurpleConnector != nullptr);
    pPurpleConnector->setPluginId(purple_account_get_protocol_id(_pAccount));//purple_plugin_get_id(g_pPurplePlugin));

    PurpleConnection* pConnection = purple_account_get_connection(_pAccount);
    pPurpleConnector->setPurpleConnection(pConnection);
    purple_connection_set_protocol_data(pConnection, pPurpleConnector);

    const char* pUsername = purple_account_get_username(_pAccount);
    const char* pPassword = purple_account_get_password(_pAccount);
    unsigned short port = static_cast<unsigned short>(purple_account_get_int(_pAccount, "port", 3003));

    if (!pPurpleConnector->start(pUsername, pPassword, port))
    {
        purple_connection_error(pConnection, "whatever :I");
        return;
    }

    g_derp = purple_timeout_add(300, spinp_event_loop, pPurpleConnector);

    purple_signal_connect(
        purple_conversations_get_handle(),
        "conversation-created",
        spinp_get_handle(),
        PURPLE_CALLBACK(spinp_event_conversation_created),
        NULL
    );
}

static void spinp_close(PurpleConnection* _pConnection)
{
    try
    {
        purple_debug_info(SPINP_PRPL_ID, "spinp_close 0\n");

        if (!purple_timeout_remove(g_derp))
        {
            purple_debug_info(SPINP_PRPL_ID, "couldn't stop timeout\n");
        }

        purple_debug_info(SPINP_PRPL_ID, "spinp_close 1\n");

        Spin::CPurpleConnector* pPurpleConnector = getPurpleConnector(_pConnection);

        purple_debug_info(SPINP_PRPL_ID, "spinp_close 2\n");

        if (pPurpleConnector != nullptr)
        {
            purple_debug_info(SPINP_PRPL_ID, "spinp_close 3\n");
            pPurpleConnector->stop();

            purple_debug_info(SPINP_PRPL_ID, "spinp_close 4\n");
            delete pPurpleConnector;

            purple_debug_info(SPINP_PRPL_ID, "spinp_close 5\n");
        }
        purple_debug_info(SPINP_PRPL_ID, "spinp_close 6\n");

        purple_connection_set_protocol_data(_pConnection, nullptr);

        purple_debug_info(SPINP_PRPL_ID, "spinp_close 7\n");
    }
    catch (std::exception e)
    {
        purple_debug_info(SPINP_PRPL_ID, e.what());
    }

    purple_debug_info(SPINP_PRPL_ID, "spinp_close 8\n");
}

static int spinp_send_im(PurpleConnection* _pConnection, const char* _pWho, const char* _pMessage, PurpleMessageFlags _flags)
{
    Spin::CPurpleConnector* pPurpleConnector = getPurpleConnector(_pConnection);
    Spin::Message::SIMMessage message;

    message.m_user = _pWho;
    message.m_message = _pMessage;
    message.m_messageType = Spin::Message::ChatMessageType_Normal;

    purple_debug_info(pPurpleConnector->getPluginId(), "sending message \"%s\" to %s\n", _pMessage, _pWho);

//    Spin::CConnector* pConnector = pPurpleConnector->getConnector();
    pPurpleConnector->getConnector()->pushOut(message);
//    pConnector->pushOut(Spin::createMessage(Spin::Job::IMMessage, &message));

    return 1;
}

static void spinp_set_info(PurpleConnection* _pConnection, const char* _pInfo)
{
    purple_debug_error(SPINP_PRPL_ID, "called spinp_set_info with info %s", _pInfo);
}

static unsigned int spinp_send_typing(PurpleConnection* _pConnection, const char* _pName, PurpleTypingState _state)
{
    Spin::CPurpleConnector* pPurpleConnector = getPurpleConnector(_pConnection);
    Spin::Message::SIMTyping message;

    message.m_user = _pName;
    message.m_state = _state == PURPLE_TYPING ? Spin::Typing_Typing : Spin::Typing_Nothing;

//    Spin::CConnector* pConnector = pPurpleConnector->getConnector();
//    pConnector->pushOut(Spin::createMessage(Spin::Job::IMTyping, &message));
    pPurpleConnector->getConnector()->pushOut(message);

    return 0;
}

static void spinp_get_info(PurpleConnection* _pConnection, const char* _pWho)
{
    purple_debug_info(SPINP_PRPL_ID, "called spinp_get_info for user %s", _pWho);
    Spin::CPurpleConnector* pPurpleConnector = getPurpleConnector(_pConnection);

//    PurpleNotifyUserInfo* pN = purple_notify_user_info_new();
//
//    purple_notify_user_info_add_section_header(pN, "Allgemein");
//    purple_notify_user_info_add_pair(pN, "my label", "http://www.somerandompage.com");
//    purple_notify_user_info_add_pair(pN, "my label", "http://www.somerandompage.com");
//    purple_notify_user_info_add_pair(pN, "my label", "http://www.somerandompage.com");
//    purple_notify_user_info_add_pair(pN, "my label", "http://www.somerandompage.com");
//    purple_notify_user_info_add_section_break(pN);
//
//    purple_notify_userinfo(_pConnection, _pWho, pN, NULL, NULL);
//

//    Spin::Job::SRequestUserProfile* pJob = new Spin::Job::SRequestUserProfile();
//    pJob->m_userName = _pWho;

    Spin::Job::SRequestUserProfile userProfile;
    userProfile.m_userName = _pWho;

    pPurpleConnector->getConnector()->pushJob(std::move(userProfile));
}

static void spinp_set_status(PurpleAccount* _pAccount, PurpleStatus* _pStatus)
{
    return;
}

static PurpleRoomlist* spinp_roomlist_get_list(PurpleConnection* _pConnection)
{
    purple_debug_info(SPINP_PRPL_ID, "spinp_roomlist_get_list called\n");

    PurpleRoomlistField* pField  = NULL;
    GList*               pFields = NULL;
    PurpleRoomlist*      pRoomList = NULL;
    Spin::CPurpleConnector* pPurpleConnector = reinterpret_cast<Spin::CPurpleConnector*>(purple_connection_get_protocol_data(_pConnection));
    pRoomList = reinterpret_cast<PurpleRoomlist*>(pPurpleConnector->purpleGetRoomList());

    if (pRoomList != nullptr)
    {
        purple_roomlist_unref(pRoomList);
    }

//    if (pRoomList == nullptr)
//    {
    pRoomList = purple_roomlist_new(purple_connection_get_account(_pConnection));
    pPurpleConnector->purpleSetRoomList(pRoomList);

    pField = purple_roomlist_field_new(PURPLE_ROOMLIST_FIELD_STRING, "type", "type" , FALSE); pFields = g_list_append(pFields, pField);
    pField = purple_roomlist_field_new(PURPLE_ROOMLIST_FIELD_STRING, "id"  , "id"   , TRUE ); pFields = g_list_append(pFields, pField);
//    pField = purple_roomlist_field_new(PURPLE_ROOMLIST_FIELD_STRING, "name", "name" , FALSE); pFields = g_list_append(pFields, pField);
    pField = purple_roomlist_field_new(PURPLE_ROOMLIST_FIELD_STRING, "#"   , "count", FALSE); pFields = g_list_append(pFields, pField);

    purple_roomlist_set_fields(pRoomList, pFields);
    purple_roomlist_set_in_progress(pRoomList, TRUE);

//    Spin::Job::SGetRoomList* pGetRoomListJob = new Spin::Job::SGetRoomList();
//    Spin::pushJob(pPurpleConnector->getConnector(), Spin::SGetRoomList());
    pPurpleConnector->getConnector()->pushJob(Spin::Job::SGetRoomList());
//    }

    return pRoomList;
}

static void spinp_roomlist_cancel(PurpleRoomlist* _pRoomList)
{

}

static void spinp_add_buddy_invite(PurpleConnection* _pConnection, PurpleBuddy* _pBuddy, PurpleGroup* _pGroup, const char* _pMessage)
{
    purple_debug_info(SPINP_PRPL_ID, "%s\n", _pBuddy->name);
    purple_debug_info(SPINP_PRPL_ID, "%s\n", _pGroup->name);
    purple_debug_info(SPINP_PRPL_ID, "%s\n", _pMessage);

    Spin::CPurpleConnector* pPurpleConnector = getPurpleConnector(_pConnection);

//    Spin::Job::SAskAuthBuddy* pAuthBuddy = new Spin::Job::SAskAuthBuddy();
    Spin::Job::SAskAuthBuddy authBuddy;
    authBuddy.m_userName = _pBuddy->name;
    if (_pMessage != NULL) authBuddy.m_message = _pMessage;
    else                   authBuddy.m_message = "";

//    Spin::pushJob(pPurpleConnector->getConnector(), authBuddy);
    pPurpleConnector->getConnector()->pushJob(std::move(authBuddy));
}

static void spinp_add_permit(PurpleConnection* _pConnection, const char* _pWho)
{
    // TODO?
//    Spin::CPurpleConnector* pPurpleConnector = getPurpleConnector(_pConnection);
//
//    Spin::Job::SAskAuthBuddy authBuddy;
//    authBuddy.m_userName = _pWho;
//
//    Spin::pushJob(pPurpleConnector->getConnector(), &authBuddy);
}

static void spinp_remove_permit(PurpleConnection* _pConnection, const char* _pWho)
{
    // TODO?
}

static void spinp_chat_join(PurpleConnection* _pConnection, GHashTable* _pComponents)
{
    Spin::CPurpleConnector* pPurpleConnector = getPurpleConnector(_pConnection);
    char* pRoomName = reinterpret_cast<char*>(g_hash_table_lookup(_pComponents, "room"));
    PurpleConvChat* pChat;
    if (pRoomName == NULL)
    {
//        PurpleConversation* pConversation = purple_conversation_new(PURPLE_CONV_TYPE_CHAT,
//                                                                    purple_connection_get_account(_pConnection),
//                                                                    reinterpret_cast<char*>(g_hash_table_lookup(_pComponents, "name")));

        pRoomName = reinterpret_cast<char*>(g_hash_table_lookup(_pComponents, "name"));
        if (pRoomName == NULL)
        {
            purple_debug_error(pPurpleConnector->getPluginId(), "room not found: %s\n", pRoomName);
            return;
        }

        pChat = pPurpleConnector->purpleGetConvChat(pRoomName);
        pRoomName = reinterpret_cast<char*>(g_hash_table_lookup(_pComponents, "id"));

        purple_debug_info(pPurpleConnector->getPluginId(), "roomname: %s\n", reinterpret_cast<char*>(g_hash_table_lookup(_pComponents, "name" )));
        purple_debug_info(pPurpleConnector->getPluginId(), "roomid: %s\n"  , reinterpret_cast<char*>(g_hash_table_lookup(_pComponents, "id"   )));
        purple_debug_info(pPurpleConnector->getPluginId(), "count: %s\n"   , reinterpret_cast<char*>(g_hash_table_lookup(_pComponents, "count")));
        purple_debug_info(pPurpleConnector->getPluginId(), "type: %s\n"    , reinterpret_cast<char*>(g_hash_table_lookup(_pComponents, "type" )));
    }
    else
    {
        pChat = pPurpleConnector->purpleGetConvChat(pRoomName);

//        purple_debug_info(pPurpleConnector->getPluginId(), "after purpleGetConvChat");
//        std::this_thread::sleep_for(std::chrono::seconds(30));
    }

//    int roomId = g_str_hash(pRoomName);



    if (pChat != NULL)
    {
        purple_debug_info(pPurpleConnector->getPluginId(), "already in room: %s\n", pRoomName);
        return;
    }

//    PurpleConvChat* pChat = g_pPurpleConnector->purpleGetConvChat(pRoomName);

//    if (pChat == NULL)
//    {
//        return;
//    }

    Spin::Message::SJoinChatRoom message;
    message.m_room = pRoomName;

//    Spin::CConnector* pConnector = pPurpleConnector->getConnector();
//    pConnector->pushOut(Spin::createMessage(Spin::Job::JoinChatRoom, &message));
    pPurpleConnector->getConnector()->pushOut(message);

//    void* conversationsHandle = purple_conversations_get_handle();

}

static void spinp_chat_leave(PurpleConnection* _pConnection, int _id)
{
    Spin::CPurpleConnector* pPurpleConnector = getPurpleConnector(_pConnection);
    PurpleConvChat* pChat = pPurpleConnector->purpleGetConvChat(_id);

    Spin::Message::SLeaveChatRoom message;
    message.m_room = purple_conversation_get_name(purple_conv_chat_get_conversation(pChat));

//    Spin::CConnector* pConnector = pPurpleConnector->getConnector();
//    pConnector->pushOut(Spin::createMessage(Spin::Job::LeaveChatRoom, &message));
    pPurpleConnector->getConnector()->pushOut(message);

//    PurpleConvChat* pChat = g_pPurpleConnector->purpleGetConvChat(_id);
//    const char* pChatName = purple_conversation_get_name(purple_conv_chat_get_conversation(pChat));
//
//    if (pChat == NULL)
//    {
//        purple_debug_error(g_pPurpleConnector->getPluginId(), "couldn't leave chatroom %s", pChatName);
//        return;
//    }
//
//    Spin::Job::SLeaveChatRoom message;
//    message.m_room = pChatName;
//
//    Spin::CConnector* pConnector = g_pPurpleConnector->getConnector();
//    pConnector->pushOut(Spin::createMessage(Spin::Job::LeaveChatRoom, &message));
//    g_pPurpleConnector->purpleRemoveConvChat(_id);
}

static char* spinp_get_chat_name(GHashTable* _pComponents)
{
    char* pRoomName = g_strdup(reinterpret_cast<char*>(g_hash_table_lookup(_pComponents, "room")));
    purple_debug_info(SPINP_PRPL_ID, "spinp_get_chat_name %s\n", pRoomName);
    return pRoomName;
//    return NULL;
}

static int spinp_chat_send(PurpleConnection* _pConnection, int _id, const char* _pMessage, PurpleMessageFlags _flags)
{
    Spin::CPurpleConnector* pPurpleConnector = getPurpleConnector(_pConnection);
    const char* pName = pPurpleConnector->purpleGetChatRoomName(_id);

    if (pName == nullptr) return 0;

    Spin::Message::SChatMessage message;

    message.m_room        = pName;
    message.m_message     = _pMessage;
    message.m_messageType = Spin::Message::ChatMessageType_Normal;

//    Spin::CConnector* pConnector = pPurpleConnector->getConnector();
//    pConnector->pushOut(Spin::createMessage(Spin::Job::ChatMessage, &message));
    pPurpleConnector->getConnector()->pushOut(message);

    return 1;
}

static const char* spinp_normalize(const PurpleAccount* _pAccount, const char* _pWho)
{
    return _pWho;
}

static const char* spinp_list_icon(PurpleAccount* _pAccount, PurpleBuddy* _pBuddy)
{
    return "spinp";
}

static const char* spinp_list_emblems(PurpleBuddy* _pBuddy)
{
    Spin::CPurpleConnector::SBuddyData* pBuddyData = reinterpret_cast<Spin::CPurpleConnector::SBuddyData*>(purple_buddy_get_protocol_data(_pBuddy));

    if (pBuddyData == NULL)
    {
        return NULL;
    }

    if (!pBuddyData->m_hasAuthorizedAccount)
    {
        return "not-authorized";
    }

    return NULL;
}

extern "C"
{

static char g_icon_format[] = "png,gif,bmp,tiff,jpg";

static PurplePluginProtocolInfo spinp_protocol_info =
{
    (PurpleProtocolOptions)(OPT_PROTO_CHAT_TOPIC            |
                            OPT_PROTO_SLASH_COMMANDS_NATIVE |
                            OPT_PROTO_INVITE_MESSAGE          ), /* options */
    NULL,    /* user_splits */
    NULL,            /* protocol_options */
    {			/* icon_spec, a PurpleBuddyIconSpec */
        g_icon_format,			/* format */
        1,			/* min_width */
        1,			/* min_height */
        4096,			/* max_width */
        4096,			/* max_height */
        8*1024*1024,	/* max_filesize */
        PURPLE_ICON_SCALE_SEND,	/* scale_rules */
    },
    spinp_list_icon,    /* list_icon */
    spinp_list_emblems,            /* list_emblems */
    NULL,    /* status_text */
    NULL,            /* tooltip_text */
    spinp_status_types,    /* status_types */
    NULL,            /* blist_node_menu */
    spinp_chat_info,    /* chat_info */
    NULL,            /* chat_info_defaults */
    spinp_login,        /* login */
    spinp_close,        /* close */
    spinp_send_im,    /* send_im */
    spinp_set_info,            /* set_info */
    spinp_send_typing, /* send_typing */
    spinp_get_info,            /* get_info */
    spinp_set_status,    /* set_status */
    NULL,            /* set_idle */
    NULL,            /* change_passwd */
    NULL,            /* add_buddy */
    NULL,            /* add_buddies */
    NULL,            /* remove_buddy */
    NULL,            /* remove_buddies */
    spinp_add_permit,            /* add_permit */
    NULL,            /* add_deny */
    spinp_remove_permit,            /* rem_permit */
    NULL,            /* rem_deny */
    NULL,            /* set_permit_deny */
    spinp_chat_join,    /* join_chat */
    NULL,            /* reject chat invite */
    spinp_get_chat_name,    /* get_chat_name */
    NULL,            /* chat_invite */
    spinp_chat_leave,    /* chat_leave */
    NULL,            /* chat_whisper */
    spinp_chat_send,    /* chat_send */
    NULL,            /* keepalive */
    NULL,            /* register_user */
    NULL,            /* get_cb_info */
    NULL,            /* get_cb_away */
    NULL,            /* alias_buddy */
    NULL,            /* group_buddy */
    NULL,            /* rename_group */
    NULL,            /* buddy_free */
    NULL,            /* convo_closed */
    spinp_normalize,            /* normalize */
    NULL,            /* set_buddy_icon */
    NULL,            /* remove_group */
    NULL,            /* get_cb_real_name */
    NULL,            /* set_chat_topic */
    NULL,            /* find_blist_chat */
    spinp_roomlist_get_list,            /* roomlist_get_list */
    spinp_roomlist_cancel,            /* roomlist_cancel */
    NULL,            /* roomlist_expand_category */
    NULL,            /* can_receive_file */
    NULL,            /* send_file */
    NULL,            /* new_xfer */
    NULL,            /* offline_message */
    NULL,            /* whiteboard_prpl_ops */
    NULL,            /* send_raw */
    NULL,            /* roomlist_room_serialize */
    NULL,            /* unregister_user */
    NULL,            /* send_attention */
    NULL,            /* attention_types */
    sizeof(PurplePluginProtocolInfo),    /* struct_size */
    NULL,            /* get_account_text_table */
    NULL,            /* initiate_media */
    NULL,            /* get_media_caps */
    NULL,            /* get_moods */
    NULL,            /* set_public_alias */
    NULL,            /* get_public_alias */
    spinp_add_buddy_invite,            /* add_buddy_with_invite */
    NULL,            /* add_buddies_with_invite */
};


static char g_plugin_id         [] = SPINP_PRPL_ID;
static char g_plugin_name       [] = "Spinchat";
static char g_plugin_version    [] = "0.1";
static char g_plugin_summary    [] = "Spinchat";
static char g_plugin_description[] = "Spinchat Protocol Plugin";
static char g_plugin_author     [] = "My Name <email@helloworld.tld>";
static char g_plugin_homepage   [] = "http://rehpost.de";

static PurplePluginInfo info =
{
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_PROTOCOL,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,
    g_plugin_id         ,
    g_plugin_name       ,
    g_plugin_version    ,
    g_plugin_summary    ,
    g_plugin_description,
    g_plugin_author     ,
    g_plugin_homepage   ,
    plugin_load,
    plugin_unload,
    NULL,
    NULL,
    &spinp_protocol_info,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static void init_plugin(PurplePlugin* plugin)
{
    purple_debug_info(g_plugin_id, "init_plugin\n");
}

PURPLE_INIT_PLUGIN(spinp, init_plugin, info)

} // extern "C"

#if CMAKE
#pragma clang diagnostic pop
#endif