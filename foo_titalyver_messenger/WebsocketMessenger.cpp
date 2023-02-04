#include "WebsocketMessenger.h"

#pragma comment(lib,"winhttp.lib")


bool WebsocketMessenger::Initialize(void)
{
	Terminalize();
	
	session_handle = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (session_handle == NULL)
	{
		return false;
	}

	return Connect();
}


void WebsocketMessenger::Terminalize(void)
{
	if (session_handle)
	{
		Disconnect();
		session_handle = NULL;
	}
}



bool WebsocketMessenger::Update(const std::string& json)
{
	DWORD err = WinHttpWebSocketSend(websocket_handle, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, const_cast<char*>(json.c_str()), json.size());
	if (err != NO_ERROR)
	{
		Disconnect();
		if (!Connect())
			return false;
		err = WinHttpWebSocketSend(websocket_handle, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, const_cast<char*>(json.c_str()), json.size());
		if (err != NO_ERROR)
			return false;
	}
	return true;
}


WebsocketMessenger::WebsocketMessenger(void)
{
}

WebsocketMessenger::~WebsocketMessenger()
{
	Terminalize();
}

bool WebsocketMessenger::Connect()
{
	connect_handle = WinHttpConnect(session_handle, L"127.0.0.1", 14738, 0);
	if (connect_handle == NULL)
	{
		return false;
	}
	HINTERNET request_handle = WinHttpOpenRequest(connect_handle, L"GET", L"/",
		NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
	if (request_handle == NULL)
	{
		Disconnect();
		return false;
	}
	if (!WinHttpSetOption(request_handle, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0))
	{
		WinHttpCloseHandle(request_handle);
		Disconnect();
		return false;
	}
	if (!WinHttpSendRequest(request_handle,
		WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0,
		0, 0))
	{
		Disconnect();
		return false;
	}
	if (!WinHttpReceiveResponse(request_handle, NULL))
	{
		Disconnect();
		return false;
	}
	websocket_handle = WinHttpWebSocketCompleteUpgrade(request_handle, NULL);
	if (websocket_handle == NULL)
	{
		DWORD err = GetLastError();
		WinHttpCloseHandle(request_handle);
		Disconnect();
		return false;
	}
	WinHttpCloseHandle(request_handle);

	return true;
}

void WebsocketMessenger::Disconnect()
{
	if (connect_handle)
	{
		if (websocket_handle)
		{
			WinHttpWebSocketClose(websocket_handle, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
			WinHttpCloseHandle(websocket_handle);
			websocket_handle = NULL;
		}
		WinHttpCloseHandle(connect_handle);
		connect_handle = NULL;
	}
}
