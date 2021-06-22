#pragma once

#include <Windows.h>
#include <string>



class JunaLyricsMessage
{
public:
	static const wchar_t * const RegisterMessageString;

	static const wchar_t * const DataMutexName;
	static const wchar_t * const DataFileMappingName;
	static const DWORD DataFileMappingMaxSize = 1024 * 64;

	static const wchar_t * const ListenerListMutexName;
	static const wchar_t * const ListenerListFileMappingName;
	static const DWORD ListenerListFileMappingMaxSize = sizeof(unsigned int) + sizeof(HWND) * 255;

	enum enumPlaybackEvent {
		PBE_NULL = 0,
		PBE_New  = 1,
		PBE_Stop = 2,
		PBE_SeekPlaying = 3,
		PBE_SeekPause   = 4,
		PBE_PauseCancel = 5,
		PBE_Pause       = 6,
//		PBE_Time = 7,
//		PBE_Time = 8,
	};

protected:
	UINT RegisterMessage;

	HANDLE hDataMutex;
	HANDLE hDataFileMapping;

	HANDLE hListenerListMutex;
	HANDLE hListenerListFileMapping;


	bool Initialize(void);
	void Terminalize(void);

public:
	bool IsValid(void) {return hDataMutex != 0;}


protected:
	JunaLyricsMessage(void);
	~JunaLyricsMessage();
};

class JunaLyricsMessageSender : public JunaLyricsMessage
{
public:
	JunaLyricsMessageSender(void) : JunaLyricsMessage() {}
	~JunaLyricsMessageSender() {Terminalize();}

	bool Initialize(void);
	void Terminalize(void);

	bool PostMessage(enumPlaybackEvent pb_event,unsigned int milisec);

	bool SetData(DWORD wait_ms,
		unsigned int total_milisec,
		const wchar_t * path,size_t path_length,
		const wchar_t * title,size_t title_length,
		const wchar_t * artist,size_t artist_length,
		const wchar_t * album,size_t album_length,
		const wchar_t * genre,size_t genre_length,
		const wchar_t * date,size_t date_length,
		const wchar_t * comment,size_t comment_length);

};

class JunaLyricsMessageListener : public JunaLyricsMessage
{
	HWND hWindow;
public:
	JunaLyricsMessageListener(void) : JunaLyricsMessage() {}
	~JunaLyricsMessageListener() {Terminalize();}

	bool Initialize(HWND hwnd);
	void Terminalize(void);

	bool ChangeWindowHandle(HWND hwnd);

	UINT GetMessageValue(void) const {return RegisterMessage;}

	struct MessageParam
	{
		enumPlaybackEvent pb_event;
		unsigned int milisec;
	};
	bool TestMessage(MessageParam &dest,UINT uMsg,WPARAM w,LPARAM l) const
	{
		if (uMsg != RegisterMessage)
			return false;
		dest.pb_event = (enumPlaybackEvent)w;
		dest.milisec = l;
		return true;
	}

	bool GetData(unsigned int &total_milisec,
		std::wstring &path,
		std::wstring &title,std::wstring &artist,std::wstring &album,
		std::wstring &genre,std::wstring &date,std::wstring &comment,
		DWORD wait_ms = INFINITE);

};

