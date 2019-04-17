
#ifndef __S_HTTP_CLIENT_H__
#define __S_HTTP_CLIENT_H__

#include <WinSock2.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")
extern "C" 
{
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/thread.h>
#include <event2/bufferevent.h>


}

namespace SCarabLib
{
	class SHttpResponse
	{
	public:
		SHttpResponse();
		~SHttpResponse();

		void SetResponse(const char* response, int len);

		void ApendBinary(const char* data, int len);

		char* GetResponse()const;

		bool WriteToText(const char* szFileName);

		bool WriteToBinary(const char* szFileName);
	public:
		bool Success;
	private:
		char* m_pResponse;
		int m_nLen;
	};

	class SHttpClient
	{
	protected:
		SHttpClient();
		~SHttpClient();
	public:
		static SHttpClient* CreateClient();
		static void DestroyClient(SHttpClient** client);

		int SendRequest(const char* url, evhttp_cmd_type type = EVHTTP_REQ_GET);

		SHttpResponse* GetResponse()const;
	protected:
		static void RemoteReadCallback(evhttp_request* remote_rsp, void* args);

		static int ReadHeaderDoneCallback(evhttp_request* remote_rsp, void* args);

		static void ReadChunkCallback(evhttp_request* remote_rsp, void* args);

		static void RemoteRequestErrorCallback(evhttp_request_error error, void* args);

		static void RemoteConnectionCloseCallback(evhttp_connection* conn, void* args);
	protected:
		event_base* m_pBase;
		SHttpResponse* m_pResponse;
	};
}

#endif	//__S_HTTP_CLIENT_H__