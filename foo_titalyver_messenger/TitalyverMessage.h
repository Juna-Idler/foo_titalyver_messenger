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
		Pause = 3,
		PauseCancel = 4,
		SeekPlaying = 5,
		SeekPause = 6,
	};


protected:
	HANDLE Mutex;
	HANDLE UpdateEventHandle;


	bool Initialize(void);
	void Terminalize(void);

protected:
	TitalyverMessage(void);
	~TitalyverMessage();
};

class TitalyberMessenger : public TitalyverMessage
{
public:
	TitalyberMessenger(void) : TitalyverMessage(), MemoryMappedFile(NULL) {}
	~TitalyberMessenger() {Terminalize();}

	bool Initialize(void);
	void Terminalize(void);

	bool Update(EnumPlaybackEvent pb_event,float seek_time,float time_of_day,const std::string json);

private:
	HANDLE MemoryMappedFile;
};

/*
class TitalyverMessageReceiverListener : public TitalyverMessage
{
public:
	TitalyverMessageReceiverListener(void) : TitalyverMessage() {}
	~TitalyverMessageReceiverListener() {Terminalize();}

	bool Initialize(void);
	void Terminalize(void);



};
*/

