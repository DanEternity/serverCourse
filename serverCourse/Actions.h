#pragma once

enum Actions
{
	action_ping = 0,
	action_ping_response,
	action_auth,
	action_auth_fail,
	action_auth_success,
	action_conf_user,
	action_conf_user_responce,
	action_conf_full_info,
	action_conf_full_info_response,
	action_error,
	action_get_conf_members,
	action_get_conf_members_response,
	action_update_conf_info,
	action_update_conf_info_response,
	action_leave_conf,
	action_leave_conf_response,
	action_delete_conf,
	action_delete_conf_response,
	action_message_list,
	action_message_list_response,
	action_get_message,
	action_get_message_response,
	action_search_conf,
	action_search_conf_response,
	action_join_conf,
	action_join_conf_response,
	action_send_message,
	action_send_message_response,
	action_profile_all_conferences_full,
	action_profile_all_conferences_full_response,
	action_scientist_info,
	action_scientist_info_response,
	action_add_conference,
	action_add_conference_responce,
	action_get_profile,
	action_get_profile_response,
	action_create_account,
	action_create_account_response,
	action_add_city,
	action_add_city_response,
	action_add_theme,
	action_add_theme_response,
	//action_search_conf_city,
	//action_search_conf_city_response,
	//action_search_conf_theme,
	//action_search_conf_theme_response,
};

struct DataFormat
{
	__int64 Account;
	Actions ActionID;
	int PacketID;
	int PacketCountExpected;
};