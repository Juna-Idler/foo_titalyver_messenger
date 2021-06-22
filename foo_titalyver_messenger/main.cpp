//#include "stdafx.h"

#include "../foobar2000_SDK/foobar2000/SDK/foobar2000.h"
#include "../foobar2000_SDK/foobar2000/helpers/helpers.h"
//#include "../foobar2000_SDK/pfc/pfc.h"


#include "json.hpp"



#include "TitalyverMessage.h"


DECLARE_COMPONENT_VERSION("Titalyver Messenger Component","0.1",
	"Send message to Titalyver for viewing lyrics\n"
	"latest 2021 6/23\n"
	"from 2021 6/23?"
	);

VALIDATE_COMPONENT_FILENAME("foo_titalyver_messenger.dll");




class LyricsMessenger
{
	TitalyberMessenger Sender;

	bool Playing;

public:
	LyricsMessenger(void) : Sender(), Playing(false)
	{}

public:
	void Init(void)
	{
		Sender.Initialize();
	}
	void Quit(void)
	{
		Sender.Terminalize();
	}

private:
	pfc::stringcvt::string_wide_from_utf8 path;
	pfc::stringcvt::string_wide_from_utf8 title;
	pfc::stringcvt::string_wide_from_utf8 artist;
	pfc::stringcvt::string_wide_from_utf8 album;
	pfc::stringcvt::string_wide_from_utf8 genre;
	pfc::stringcvt::string_wide_from_utf8 date;
	pfc::stringcvt::string_wide_from_utf8 comment;


public:
	void on_playback_new_track(metadb_handle_ptr track)
	{
		using json = nlohmann::json;

		json send_data;

		path.convert(track->get_path());

		::file_info_impl info;
		track->get_info(info);

		const t_size names_count = info.meta_get_count();
		for (int i = 0; i < names_count; i++)
		{
			t_size values_count = info.meta_enum_value_count(i);
			json a = json::array();
			for (int j = 0; j < values_count; j++)
			{
				a.push_back(info.meta_enum_value(i, j));
			}
			const char* name = info.meta_enum_name(i);
			send_data[name] = a;
		}

		SYSTEMTIME time;
		GetLocalTime(&time);
		float dayoftime = (time.wHour * 60 + time.wMinute) * 60 + time.wSecond + time.wMilliseconds / 1000.0;
		Sender.Update(TitalyverMessage::EnumPlaybackEvent::PlayNew, 0, dayoftime, send_data.dump());
		Playing = true;
	}
	void on_playback_stop(play_control::t_stop_reason p_reason)
	{
		unsigned int milisec = unsigned int(static_api_ptr_t<playback_control>()->playback_get_position() * 1000);

		Playing = false;
//		Sender.PostMessage(TitalyverMessage::PBE_Stop,milisec);
	}
	void on_playback_seek(double time)
	{
		if (Playing)
			;
		//			Sender.PostMessage(TitalyverMessage::PBE_SeekPlaying,unsigned int(time * 1000));
		else
			;
//			Sender.PostMessage(TitalyverMessage::PBE_SeekPause,unsigned int(time * 1000));
	}

	void on_playback_pause(bool p_state)
	{
		unsigned int milisec = unsigned int(static_api_ptr_t<playback_control>()->playback_get_position() * 1000);
		if (p_state)
		{
			Playing = false;
//			Sender.PostMessage(TitalyverMessage::PBE_Pause,milisec);
		}
		else
		{
			Playing = true;
//			Sender.PostMessage(TitalyverMessage::PBE_PauseCancel,milisec);
		}
	}
/*
	void on_playback_starting(play_control::t_track_command p_command,bool p_paused)
	{
		if (Message.GetEnterMode() != JunaLyricsMessage::EM_Sender)
			return;
	}
	void on_playback_time(double p_time)
	{
		if (Message.GetEnterMode() != JunaLyricsMessage::EM_Sender)
			return;
		unsigned int milisec = unsigned int(static_api_ptr_t<playback_control>()->playback_get_position() * 1000);
		PostMessage(JunaLyricsMessage::PBE_Time,milisec);
	}
*/
/*
	void on_playback_edited(metadb_handle_ptr track)
	{
		console::info("on_playback_edited()");
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
//		component_main.on_playback_starting(p_command,p_paused);
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






