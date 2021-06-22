
#include "JunaLyricsMessage.h"

#pragma comment(lib,"winmm.lib")


const wchar_t * const JunaLyricsMessage::RegisterMessageString = L"Juna Lyrics Message Broadcast";

const wchar_t * const JunaLyricsMessage::DataFileMappingName = L"Juna Lyrics Message Data";
const wchar_t * const JunaLyricsMessage::DataMutexName = L"Juna Lyrics Message Data Mutex";

const wchar_t * const JunaLyricsMessage::ListenerListMutexName = L"Juna Lyrics Message Listener List Mutex";
const wchar_t * const JunaLyricsMessage::ListenerListFileMappingName = L"Juna Lyrics Message Listener List";

namespace {
class MutexLock
{
public:
	HANDLE hMutex;
	DWORD ret;

	MutexLock(HANDLE hmutex,DWORD milisec = INFINITE) : hMutex(hmutex)
	{
		ret = ::WaitForSingleObject(hmutex,milisec);
	}
	void Unlock(void)
	{
		if (hMutex)
			::ReleaseMutex(hMutex);
		hMutex = 0;
	}
	~MutexLock()
	{
		if (hMutex)
			::ReleaseMutex(hMutex);
	}
};
}//anonymous namespace


JunaLyricsMessage::JunaLyricsMessage(void) : RegisterMessage(WM_NULL),
		hDataMutex(NULL), hDataFileMapping(NULL),
		hListenerListMutex(NULL), hListenerListFileMapping(NULL)
{
	RegisterMessage = ::RegisterWindowMessageW(RegisterMessageString);
	::timeBeginPeriod(1);
}


JunaLyricsMessage::~JunaLyricsMessage(void)
{
	Terminalize();
	::timeEndPeriod(1);
}



bool JunaLyricsMessage::Initialize(void)
{
	Terminalize();
	hDataMutex = ::CreateMutexW(NULL,FALSE,DataMutexName);
	if (!hDataMutex)
		return false;
	hListenerListMutex = ::CreateMutexW(NULL,FALSE,ListenerListMutexName);
	if (!hListenerListMutex)
	{
		::CloseHandle(hDataMutex);
		hDataMutex = NULL;
		return false;
	}
	return true;
}

void JunaLyricsMessage::Terminalize(void)
{
	if (hDataFileMapping != NULL)
	{
		::CloseHandle(hDataFileMapping);
		hDataFileMapping = NULL;
	}
	if (hDataMutex != NULL)
	{
		::CloseHandle(hDataMutex);
		hDataMutex = NULL;
	}

	if (hListenerListFileMapping != NULL)
	{
		::CloseHandle(hListenerListFileMapping);
		hListenerListFileMapping = NULL;
	}
	if (hListenerListMutex != NULL)
	{
		::CloseHandle(hListenerListMutex);
		hListenerListMutex = NULL;
	}
}


bool JunaLyricsMessageSender::Initialize(void)
{
	if (JunaLyricsMessage::Initialize())
	{
		MutexLock ml(hDataMutex);
		hDataFileMapping = ::CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,DataFileMappingMaxSize,DataFileMappingName);
		if (hDataFileMapping == NULL)
		{
			ml.Unlock();
			JunaLyricsMessage::Terminalize();
			return false;
		}
		bool already = ::GetLastError() == ERROR_ALREADY_EXISTS;
		BYTE *address = static_cast<BYTE *>(::MapViewOfFile(hDataFileMapping,FILE_MAP_WRITE,0,0,sizeof(DWORD)));
		if (address == NULL)
		{
			ml.Unlock();
			JunaLyricsMessage::Terminalize();
			return false;
		}
		if (already && *(DWORD *)(address) != 0)
		{
			::UnmapViewOfFile(address);
			ml.Unlock();
			JunaLyricsMessage::Terminalize();
			return false;
		}
		*(DWORD *)(address) = 1;
		::UnmapViewOfFile(address);
		return true;
	}
	return false;
}
void JunaLyricsMessageSender::Terminalize(void)
{
	if (IsValid())
	{
		MutexLock ml(hDataMutex);
		BYTE *address = static_cast<BYTE *>(::MapViewOfFile(hDataFileMapping,FILE_MAP_WRITE,0,0,sizeof(DWORD)));
		*(DWORD *)(address) = 0;
		::UnmapViewOfFile(address);
	}
	JunaLyricsMessage::Terminalize();
}


bool JunaLyricsMessageSender::PostMessage(enumPlaybackEvent pb_event,unsigned int milisec)
{
	MutexLock ml(hListenerListMutex);
	if (hListenerListFileMapping == NULL)
	{
		hListenerListFileMapping = ::OpenFileMappingW(FILE_MAP_READ,FALSE,ListenerListFileMappingName);
		if (hListenerListFileMapping == NULL)
			return false;
	}

	BYTE *address = static_cast<BYTE *>(::MapViewOfFile(hListenerListFileMapping,FILE_MAP_READ,0,0,0));
	if (address)
	{
		unsigned int count = *(unsigned int *)(address);
		size_t offset = sizeof(unsigned int);

		for (unsigned int i = 0;i < count;i++)
		{
			::PostMessageW(*(reinterpret_cast<HWND *>(address + offset) + i),RegisterMessage,pb_event,milisec);
		}
		::UnmapViewOfFile(address);
		return true;
	}
	return false;
}




bool JunaLyricsMessageSender::SetData(DWORD wait_ms,
		unsigned int total_milisec,
		const wchar_t *path,size_t path_length,
		const wchar_t * title,size_t title_length,
		const wchar_t * artist,size_t artist_length,
		const wchar_t * album,size_t album_length,
		const wchar_t * genre,size_t genre_length,
		const wchar_t * date,size_t date_length,
		const wchar_t * comment,size_t comment_length)
{
	const size_t char_size = sizeof(wchar_t);
	DWORD size = sizeof(DWORD) + sizeof(unsigned int) + (sizeof(WORD) + path_length * char_size) +
				(sizeof(WORD) + title_length * char_size) + (sizeof(WORD) + artist_length * char_size) + (sizeof(WORD) + album_length * char_size) +
				(sizeof(WORD) + genre_length * char_size) + (sizeof(WORD) + date_length * char_size) + (sizeof(WORD) + comment_length * char_size);

	MutexLock ml(hDataMutex,wait_ms);
//	if (ml.ret != WAIT_OBJECT_0 && ml.ret != WAIT_ABANDONED)
	if (ml.ret == WAIT_TIMEOUT)
		return false;

	BYTE *address = static_cast<BYTE *>(::MapViewOfFile(hDataFileMapping,FILE_MAP_WRITE,0,0,size));
	if (address == NULL)
		return false;

	size_t offset = 0;
	*(DWORD *)(address + offset) = size; offset += sizeof(DWORD);
	*(unsigned int *)(address + offset) = total_milisec; offset += sizeof(unsigned int);

	*(WORD *)(address + offset) = path_length * char_size; offset += sizeof(WORD);
	::memcpy(address + offset,path,path_length * 2); offset += path_length * char_size;

	*(WORD *)(address + offset) = title_length * char_size; offset += sizeof(WORD);
	::memcpy(address + offset,title,title_length * char_size); offset += title_length * char_size;

	*(WORD *)(address + offset) = artist_length * char_size; offset += sizeof(WORD);
	::memcpy(address + offset,artist,artist_length * char_size); offset += artist_length * char_size;

	*(WORD *)(address + offset) = album_length * char_size; offset += sizeof(WORD);
	::memcpy(address + offset,album,album_length * char_size); offset += album_length * char_size;

	*(WORD *)(address + offset) = genre_length * char_size; offset += sizeof(WORD);
	::memcpy(address + offset,genre,genre_length * char_size); offset += genre_length * char_size;

	*(WORD *)(address + offset) = date_length * char_size; offset += sizeof(WORD);
	::memcpy(address + offset,date,date_length * char_size); offset += date_length * char_size;

	*(WORD *)(address + offset) = comment_length * char_size; offset += sizeof(WORD);
	::memcpy(address + offset,comment,comment_length * char_size); offset += comment_length * char_size;

	::UnmapViewOfFile(address);

	return true;
}



bool JunaLyricsMessageListener::Initialize(HWND hwnd)
{
	if (JunaLyricsMessage::Initialize())
	{
		MutexLock ml(hListenerListMutex);
		hListenerListFileMapping = ::CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,ListenerListFileMappingMaxSize,ListenerListFileMappingName);
		if (hListenerListFileMapping == NULL)
		{
			ml.Unlock();
			JunaLyricsMessage::Terminalize();
			return false;
		}
		bool already = ::GetLastError() == ERROR_ALREADY_EXISTS;
		BYTE *address = static_cast<BYTE *>(::MapViewOfFile(hListenerListFileMapping,FILE_MAP_WRITE,0,0,0));
		if (address == NULL)
		{
			ml.Unlock();
			JunaLyricsMessage::Terminalize();
			return false;
		}
		if (already)
		{
			unsigned int count = *reinterpret_cast<unsigned int *>(address);
			if (count >= 255)
			{
				::UnmapViewOfFile(address);
				ml.Unlock();
				JunaLyricsMessage::Terminalize();
				return false;
			}
			*reinterpret_cast<unsigned int *>(address) = count + 1;
			size_t offset = sizeof(unsigned int) + sizeof(HWND) * count;
			*reinterpret_cast<HWND *>(address + offset) = hwnd;
		}
		else
		{
			*reinterpret_cast<unsigned int *>(address) = 1;
			size_t offset = sizeof(unsigned int);
			*reinterpret_cast<HWND *>(address + offset) = hwnd;
		}
		::UnmapViewOfFile(address);
		hWindow = hwnd;
		return true;
	}
	return false;
}
void JunaLyricsMessageListener::Terminalize(void)
{
	if (IsValid())
	{
		MutexLock ml(hListenerListMutex);
		BYTE *address = static_cast<BYTE *>(::MapViewOfFile(hListenerListFileMapping,FILE_MAP_WRITE,0,0,0));
		if (address)
		{
			unsigned int count = *reinterpret_cast<unsigned int *>(address);
			size_t offset = sizeof(unsigned int);

			if (*(reinterpret_cast<HWND *>(address + offset) + count - 1) != hWindow)
			{
				unsigned int i;
				for (i = 0;i < count - 1;i++)
				{
					if (*(reinterpret_cast<HWND *>(address + offset) + i) == hWindow)
					{
						::memmove(reinterpret_cast<HWND *>(address + offset) + i,reinterpret_cast<HWND *>(address) + i + 1,count - i);
					}
				}
				if (i == count - 1)
					count++;
			}
			*(unsigned int *)(address) = count - 1;
			::UnmapViewOfFile(address);
		}
	}
	hWindow = 0;
	JunaLyricsMessage::Terminalize();
}


bool JunaLyricsMessageListener::ChangeWindowHandle(HWND hwnd)
{
	if (IsValid())
	{
		MutexLock ml(hListenerListMutex);
		BYTE *address = static_cast<BYTE *>(::MapViewOfFile(hListenerListFileMapping,FILE_MAP_WRITE,0,0,0));
		if (address)
		{
			unsigned int count = *reinterpret_cast<unsigned int *>(address);
			size_t offset = sizeof(unsigned int);

			unsigned int i;
			for (i = 0;i < count;i++)
			{
				if (*(reinterpret_cast<HWND *>(address + offset) + i) == hWindow)
				{
					*(reinterpret_cast<HWND *>(address + offset) + i) = hwnd;
					::UnmapViewOfFile(address);
					hWindow = hwnd;
					return true;
				}
			}
			::UnmapViewOfFile(address);
			return false;
		}
	}
	return false;
}


bool JunaLyricsMessageListener::GetData(unsigned int &total_milisec,
								std::wstring &path,
								std::wstring &title,std::wstring &artist,std::wstring &album,
								std::wstring &genre,std::wstring &date,std::wstring &comment,
								DWORD wait_ms)
{
	if (!hDataMutex)
		return false;
	MutexLock ml(hDataMutex,wait_ms);
//	if (ml.ret != WAIT_OBJECT_0 && ml.ret != WAIT_ABANDONED)
	if (ml.ret == WAIT_TIMEOUT)
		return false;

	if (hDataFileMapping == NULL)
	{
		hDataFileMapping = ::OpenFileMappingW(FILE_MAP_READ,FALSE,DataFileMappingName);
		if (hDataFileMapping == NULL)
			return false;
	}

	BYTE *address = static_cast<BYTE *>(::MapViewOfFile(hDataFileMapping,FILE_MAP_READ,0,0,0));
	if (address == NULL)
		return false;

	DWORD size = *(DWORD *)(address);
	if (size < sizeof(DWORD))
	{
		::UnmapViewOfFile(address);
		return false;
	}

	size_t offset = sizeof(DWORD);
	total_milisec = *(unsigned int *)(address + offset); offset += sizeof(unsigned int);

	const size_t char_size = sizeof(wchar_t);
	WORD string_size;

	string_size = *(WORD *)(address + offset); offset += sizeof(WORD);
	path.resize(string_size / char_size);
	if (string_size)
		::memcpy(&path[0],address + offset,string_size); offset += string_size;

	string_size = *(WORD *)(address + offset); offset += sizeof(WORD);
	title.resize(string_size / char_size);
	if (string_size)
		::memcpy(&title[0],address + offset,string_size); offset += string_size;

	string_size = *(WORD *)(address + offset); offset += sizeof(WORD);
	artist.resize(string_size / char_size);
	if (string_size)
		::memcpy(&artist[0],address + offset,string_size); offset += string_size;

	string_size = *(WORD *)(address + offset); offset += sizeof(WORD);
	album.resize(string_size / char_size);
	if (string_size)
		::memcpy(&album[0],address + offset,string_size); offset += string_size;

	string_size = *(WORD *)(address + offset); offset += sizeof(WORD);
	genre.resize(string_size / char_size);
	if (string_size)
		::memcpy(&genre[0],address + offset,string_size); offset += string_size;

	string_size = *(WORD *)(address + offset); offset += sizeof(WORD);
	date.resize(string_size / char_size);
	if (string_size)
		::memcpy(&date[0],address + offset,string_size); offset += string_size;

	string_size = *(WORD *)(address + offset); offset += sizeof(WORD);
	comment.resize(string_size / char_size);
	if (string_size)
		::memcpy(&comment[0],address + offset,string_size); offset += string_size;

	::UnmapViewOfFile(address);
	return true;
}
