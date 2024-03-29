//#include "stdafx.h"

#include <SDK/foobar2000.h>
#include <helpers/helpers.h>
//#include <pfc/pfc.h>


#include "json.hpp"

//せっかくだからnlohmann::json使ってみたが、めちゃくちゃ単純な書き出しのみなので
//手作業でやっても大して変らなかった気が

#include "TitalyverMessage.h"
#include "WebsocketMessenger.h"


DECLARE_COMPONENT_VERSION("Titalyver Messenger Component","0.5",
	"Send message to Titalyver for viewing lyrics\n"
	"latest 2023 2/04\n"
	"from 2021 6/23?"
	);

VALIDATE_COMPONENT_FILENAME("foo_titalyver_messenger.dll");



class LyricsMessenger
{
	TitalyverMessenger Sender;
	WebsocketMessenger GSender;

	bool Playing;
	nlohmann::json ws_data;


public:
	LyricsMessenger(void) : Sender(), GSender(), Playing(false)
	{}

public:
	void Init(void)
	{
		Sender.Initialize();
		GSender.Initialize();
	}
	void Quit(void)
	{
		Sender.Terminalize();
		GSender.Terminalize();
	}

public:
	void on_playback_new_track(metadb_handle_ptr track)
	{
		using json = nlohmann::json;

		json meta_data;

		::file_info_impl info;
		track->get_info(info);

		const t_size names_count = info.meta_get_count();
		for (unsigned i = 0; i < names_count; i++)
		{
			t_size values_count = info.meta_enum_value_count(i);
			std::string name = info.meta_enum_name(i);
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);
			if (values_count == 1)
			{
				const char *text = info.meta_enum_value(i, 0);
				meta_data[name] = text;
			}
			else
			{
				json a = json::array();
				for (unsigned j = 0; j < values_count; j++)
				{
					a.push_back(info.meta_enum_value(i, j));
				}
				meta_data[name] = a;
			}
		}
/*
		const t_size info_count = info.info_get_count();
		for (unsigned i = 0; i < info_count; i++)
		{
			const char* name = info.info_enum_name(i);
			const char* text = info.info_enum_value(i);
			meta_data[name] = text;
		}
*/
		Playing = true;
		double time = static_api_ptr_t<playback_control>()->playback_get_position();
		if (Sender.IsValid())
		{
			json send_data;
			send_data["path"] = track->get_path();
			json title = meta_data["title"];
			send_data["title"] = (title.is_string()) ? title : "";

			json artist = meta_data["artist"];
			if (artist.is_array())
				send_data["artists"] = artist;
			else
			{
				json a = json::array();
				if (artist.is_string())
					a.push_back(artist);
				else
					a.push_back("");
				send_data["artists"] = a;
			}

			json album = meta_data["album"];
			send_data["album"] = (album.is_string()) ? album : "";

			send_data["duration"] = track->get_length();
			send_data["meta"] = meta_data;
			Sender.Update(TitalyverMessage::EnumPlaybackEvent::SeekPlay, time, send_data.dump());
		}

		if (GSender.IsValid())
		{
			json ws_data;
			ws_data["path"] = track->get_path();
			json title = meta_data["title"];
			ws_data["title"] = (title.is_string()) ? title : "";

			json artist = meta_data["artist"];
			if (artist.is_array())
				ws_data["artists"] = artist;
			else
			{
				json a = json::array();
				if (artist.is_string())
					a.push_back(artist);
				else
					a.push_back("");
				ws_data["artists"] = a;
			}

			json album = meta_data["album"];
			ws_data["album"] = (album.is_string()) ? album : "";

			ws_data["duration"] = track->get_length();
			ws_data["meta"] = meta_data;

			std::string utf8 = ws_data.dump();
			GSender.UpdateFullData(WebsocketMessenger::EnumPlaybackEvent::SeekPlay, time, ws_data);
		}
	}
	void on_playback_stop(play_control::t_stop_reason p_reason)
	{
		if (p_reason == play_control::t_stop_reason::stop_reason_starting_another)
			return;
		Playing = false;
		double time = static_api_ptr_t<playback_control>()->playback_get_position();

		if (Sender.IsValid())
		{
			Sender.Update(TitalyverMessage::EnumPlaybackEvent::Stop, time);
		}
		if (GSender.IsValid())
		{
			GSender.Update(WebsocketMessenger::EnumPlaybackEvent::Stop,time);
		}


	}
	void on_playback_seek(double time)
	{
		if (GSender.IsValid())
		{
			GSender.Update(Playing ? WebsocketMessenger::EnumPlaybackEvent::SeekPlay : WebsocketMessenger::EnumPlaybackEvent::SeekStop,time);
		}

		if (!Sender.IsValid())
			return;
		Sender.Update(Playing ? TitalyverMessage::EnumPlaybackEvent::SeekPlay
							  : TitalyverMessage::EnumPlaybackEvent::SeekStop,
					  time);
	}

	void on_playback_pause(bool p_state)
	{
		Playing = !p_state;
		double time = static_api_ptr_t<playback_control>()->playback_get_position();

		if (GSender.IsValid())
		{
			GSender.Update(Playing ? WebsocketMessenger::EnumPlaybackEvent::SeekPlay
								   : WebsocketMessenger::EnumPlaybackEvent::SeekStop, time);
		}

		if (!Sender.IsValid())
			return;

		Sender.Update(Playing ? TitalyverMessage::EnumPlaybackEvent::SeekPlay
							  : TitalyverMessage::EnumPlaybackEvent::SeekStop,
					  time);
	}

	void on_playback_starting(play_control::t_track_command p_command,bool p_paused)
	{
		Playing = !p_paused;
		double time = static_api_ptr_t<playback_control>()->playback_get_position();

		if (GSender.IsValid())
		{
			GSender.Update(Playing ? WebsocketMessenger::EnumPlaybackEvent::SeekPlay
								   : WebsocketMessenger::EnumPlaybackEvent::SeekStop, time);
		}

		if (!Sender.IsValid())
			return;

		Sender.Update(Playing ? TitalyverMessage::EnumPlaybackEvent::SeekPlay
							  : TitalyverMessage::EnumPlaybackEvent::SeekStop,
					  time);

	}
/*
	void on_playback_edited(metadb_handle_ptr track)
	{
		console::info("on_playback_edited()");
	}

	void on_playback_time(double p_time)
	{
	}
	void on_playback_dynamic_info(const file_info & p_info)
	{
		console::info("on_playback_dynamic_info()");
	}
	void on_playback_dynamic_info_track(const file_info & p_info)
	{
		console::info("on_playback_dynamic_info_track()");
	}
	void on_volume_change(float p_new_val)
	{
		console::info("on_volume_change()");
	}
*/

};

static LyricsMessenger component_main;


class my_initquit : public initquit {
public:
	void on_init() {
		component_main.Init();
	}
	void on_quit() {
		component_main.Quit();
	}

};

static initquit_factory_t<my_initquit> g_myinitquit_factory;


class my_play_callback_static : public play_callback_static
{

public:
	virtual unsigned int get_flags(void)
	{
		return 
//			flag_on_playback_starting | 
			flag_on_playback_new_track | 
			flag_on_playback_stop |
			flag_on_playback_seek |
			flag_on_playback_pause |
//			flag_on_playback_edited |
//			flag_on_playback_dynamic_info |
//			flag_on_playback_dynamic_info_track |
//			flag_on_playback_time |
//			flag_on_volume_change |
			0;
	}


	virtual void on_playback_new_track(metadb_handle_ptr track)
	{
		component_main.on_playback_new_track(track);
	}
	virtual void on_playback_stop(play_control::t_stop_reason p_reason)
	{
		component_main.on_playback_stop(p_reason);
	}
	virtual void on_playback_time(double p_time)
	{
//		component_main.on_playback_time(p_time);
	}
	virtual void on_playback_seek(double time)
	{
		component_main.on_playback_seek(time);
	}

	virtual void on_playback_starting(play_control::t_track_command p_command,bool p_paused)
	{
		component_main.on_playback_starting(p_command,p_paused);
	}
	virtual void on_playback_pause(bool p_state)
	{
		component_main.on_playback_pause(p_state);
	}
	virtual void on_playback_edited(metadb_handle_ptr track)
	{
	}
	virtual void on_playback_dynamic_info(const file_info & p_info)
	{
	}
	virtual void on_playback_dynamic_info_track(const file_info & p_info)
	{
	}
	virtual void on_volume_change(float p_new_val)
	{
	}
};

static play_callback_static_factory_t<my_play_callback_static> pbsf;






