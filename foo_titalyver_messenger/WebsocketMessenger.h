#pragma once

#include <Windows.h>
#include <string>

//#include <websocket.h>
#include <winhttp.h>


class WebsocketMessenger
{
public:
	enum EnumPlaybackEvent
	{
		Bit_Play = 1,
		Bit_Stop = 2,
		Bit_Seek = 4,

		NULL_ = 0,
		Play = 1,
		Stop = 2,

		Seek = 4,
		SeekPlay = 5,
		SeekStop = 6,
	};



	inline bool IsValid(void) { return session_handle != NULL; }

	static inline uint32_t GetDayOfTime(void)
	{
		::SYSTEMTIME time;
		::GetLocalTime(&time);
		return ((time.wHour * 60 + time.wMinute) * 60 + time.wSecond) * 1000 + time.wMilliseconds;
	}

	bool Initialize(void);
	void Terminalize(void);

	bool Update(const std::string& json);

	WebsocketMessenger(void);
	~WebsocketMessenger();

private:
	HINTERNET session_handle = NULL;
	HINTERNET connect_handle = NULL;
	HINTERNET websocket_handle = NULL;
	std::string host = "127.0.0.1";

	bool Connect();
	void Disconnect();
};

