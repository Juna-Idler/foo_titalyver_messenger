#pragma once

#include <Windows.h>
#include <string>



class TitalyverMessage
{
public:
	static const DWORD MMF_MaxSize = 1024 * 1024 * 64;

	static const wchar_t* const  MMF_Name;
	static const wchar_t* const  UpdateEvent_Name;
	static const wchar_t* const  Mutex_Name;

	enum EnumPlaybackEvent
	{
		NULL_ = 0,
		PlayNew = 1,
		Stop = 2,
		PauseCancel = 3,
		Pause = 4,
		SeekPlaying = 5,
		SeekPause = 6,
	};


	static inline uint32_t GetDayOfTime(void)
	{
		::SYSTEMTIME time;
		::GetLocalTime(&time);
		return ((time.wHour * 60 + time.wMinute) * 60 + time.wSecond) * 1000 + time.wMilliseconds;
	}

protected:
	HANDLE Mutex;
	HANDLE UpdateEventHandle;


	bool Initialize(void);
	void Terminalize(void);

protected:
	TitalyverMessage(void);
	~TitalyverMessage();
};

class TitalyverMessenger : public TitalyverMessage
{
public:
	TitalyverMessenger(void) : TitalyverMessage(), MemoryMappedFile(NULL) {}
	~TitalyverMessenger() {Terminalize();}

	bool Initialize(void);
	void Terminalize(void);

	bool Update(EnumPlaybackEvent pb_event,double seek_time, uint32_t time_of_day,const std::string &json);
	bool Update(EnumPlaybackEvent pb_event, double seek_time, uint32_t time_of_day);

private:
	HANDLE MemoryMappedFile;
};

/*
class TitalyverMessageReceiver : public TitalyverMessage
{
public:
	TitalyverMessageReceiverListener(void) : TitalyverMessage() {}
	~TitalyverMessageReceiverListener() {Terminalize();}

	bool Initialize(void);
	void Terminalize(void);



};
*/

