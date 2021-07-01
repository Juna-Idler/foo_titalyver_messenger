
#include "TitalyverMessage.h"

#pragma comment(lib,"winmm.lib")



const wchar_t* const TitalyverMessage::MMF_Name = L"Titalyver Message Data MMF";
const wchar_t* const TitalyverMessage::UpdateEvent_Name = L"Titalyver Message Write Event";
const wchar_t* const TitalyverMessage::Mutex_Name = L"Titalyver Message Mutex";



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


TitalyverMessage::TitalyverMessage(void) : Mutex(NULL), UpdateEventHandle(NULL)
{
}


TitalyverMessage::~TitalyverMessage(void)
{
	Terminalize();
}



bool TitalyverMessage::Initialize(void)
{
	Terminalize();
	Mutex = ::CreateMutexW(NULL,FALSE, Mutex_Name);
	if (!Mutex)
		return false;
	UpdateEventHandle = ::CreateEventW(NULL, FALSE, FALSE, UpdateEvent_Name);
	if (!UpdateEventHandle)
	{
		::CloseHandle(Mutex);
		Mutex = NULL;
		return false;
	}
	return true;
}

void TitalyverMessage::Terminalize(void)
{
	if (UpdateEventHandle)
	{
		::CloseHandle(UpdateEventHandle);
		UpdateEventHandle = NULL;
	}
	if (Mutex)
	{
		::CloseHandle(Mutex);
		Mutex = NULL;
	}
}


bool TitalyverMessenger::Initialize(void)
{
	if (TitalyverMessage::Initialize())
	{
		MutexLock ml(Mutex);
		MemoryMappedFile = ::CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0, MMF_MaxSize, MMF_Name);
		if (MemoryMappedFile == NULL)
		{
			ml.Unlock();
			TitalyverMessage::Terminalize();
			return false;
		}
		bool already = ::GetLastError() == ERROR_ALREADY_EXISTS;
		if (already)
		{
			ml.Unlock();
			TitalyverMessage::Terminalize();
			return false;
		}
		return true;
	}
	return false;
}
void TitalyverMessenger::Terminalize(void)
{
	if (MemoryMappedFile)
	{
		::CloseHandle(MemoryMappedFile);
		MemoryMappedFile = NULL;
	}
	TitalyverMessage::Terminalize();
}


bool TitalyverMessenger::Update(EnumPlaybackEvent pb_event, double seek_time, const std::string &json)
{
	uint32_t time_of_day = GetDayOfTime();
	SIZE_T size = sizeof(pb_event) + sizeof(seek_time) + sizeof(time_of_day) + sizeof(uint32_t) + json.size();

	MutexLock ml(Mutex);
	if (ml.ret != WAIT_OBJECT_0 && ml.ret != WAIT_ABANDONED)
		return false;

	BYTE *address = static_cast<BYTE *>(::MapViewOfFile(MemoryMappedFile,FILE_MAP_WRITE,0,0,size));
	if (address == NULL)
		return false;

	size_t offset = 0;
	*(EnumPlaybackEvent*)(address + offset) = pb_event; offset += sizeof(pb_event);
	*(double*)(address + offset) = seek_time; offset += sizeof(seek_time);
	*(uint32_t*)(address + offset) = time_of_day; offset += sizeof(time_of_day);
	*(uint32_t*)(address + offset) = json.size(); offset += sizeof(uint32_t);

	::memcpy(address + offset, json.c_str(), json.size());
	::UnmapViewOfFile(address);

	::SetEvent(UpdateEventHandle);
	return true;
}

bool TitalyverMessenger::Update(EnumPlaybackEvent pb_event, double seek_time)
{
	uint32_t time_of_day = GetDayOfTime();

//	if (pb_event == EnumPlaybackEvent::PlayNew)
//		return false;
	MutexLock ml(Mutex);
	if (ml.ret != WAIT_OBJECT_0 && ml.ret != WAIT_ABANDONED)
		return false;

	SIZE_T size = sizeof(pb_event) + sizeof(seek_time) + sizeof(time_of_day);
	BYTE* address = static_cast<BYTE*>(::MapViewOfFile(MemoryMappedFile, FILE_MAP_WRITE, 0, 0, size));
	if (address == NULL)
		return false;

	size_t offset = 0;
	*(EnumPlaybackEvent*)(address + offset) = pb_event; offset += sizeof(pb_event);
	*(double*)(address + offset) = seek_time; offset += sizeof(seek_time);
	*(uint32_t*)(address + offset) = time_of_day; offset += sizeof(time_of_day);
	::UnmapViewOfFile(address);

	::SetEvent(UpdateEventHandle);
	return true;
}


