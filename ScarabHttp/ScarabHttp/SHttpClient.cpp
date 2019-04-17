#include "SHttpClient.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#pragma comment(lib, "libevent.lib")
SCarabLib::SHttpClient::SHttpClient()
{
	m_pResponse = new SHttpResponse();
	m_pBase = event_base_new();
}


SCarabLib::SHttpClient::~SHttpClient()
{
	event_base_free(m_pBase);
	delete m_pResponse;
}

SCarabLib::SHttpClient * SCarabLib::SHttpClient::CreateClient()
{
	WSAData wsaData = { 0 };
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		return nullptr;
	}
	SHttpClient* httpClient = new SHttpClient();
	return httpClient;
}

void SCarabLib::SHttpClient::DestroyClient(SHttpClient ** client)
{
	assert(client);
	assert(*client);
	delete (*client);
	WSACleanup();
}

int SCarabLib::SHttpClient::SendRequest(const char * url, evhttp_cmd_type type)
{
	evhttp_uri* uri = evhttp_uri_parse(url);
	if (!uri)
	{
		return 1;	//parse url failed!
	}
	evdns_base* dnsbase = evdns_base_new(m_pBase, 1);
	if (!dnsbase)
	{
		evhttp_uri_free(uri);
		return 2;
	}
	evhttp_request* request = evhttp_request_new(RemoteReadCallback, this);
	evhttp_request_set_header_cb(request, ReadHeaderDoneCallback);
	evhttp_request_set_chunked_cb(request, ReadChunkCallback);
	evhttp_request_set_error_cb(request, RemoteRequestErrorCallback);
	const char* host = evhttp_uri_get_host(uri);
	if (!host)
	{
		evhttp_uri_free(uri);
		return 3;	//Host error
	}
	
	int port = evhttp_uri_get_port(uri);
	if (port < 0)
	{
		port = 80;
	}
	const char* request_url = url;
	const char* path = evhttp_uri_get_path(uri);
	if (!path || strlen(path) == 0)
	{
		request_url = "/";
	}

	evhttp_connection* connection = evhttp_connection_base_new(m_pBase, dnsbase, host, port);
	if (!connection)
	{
		evhttp_uri_free(uri);
		return 4;
	}

	evhttp_connection_set_closecb(connection, RemoteConnectionCloseCallback, this);
	evhttp_add_header(evhttp_request_get_output_headers(request), "Host", host);
	evhttp_make_request(connection, request, type, request_url);
	int nRet = event_base_dispatch(m_pBase);
	evhttp_uri_free(uri);
	evhttp_connection_free(connection);
	return nRet;
}

void SCarabLib::SHttpClient::RemoteReadCallback(evhttp_request * remote_rsp, void * args)
{
	SHttpClient* pThis = reinterpret_cast<SHttpClient*>(args);
	if (pThis)
	{
		event_base_loopexit(pThis->m_pBase, NULL);
	}
}


int SCarabLib::SHttpClient::ReadHeaderDoneCallback(evhttp_request * remote_rsp, void * args)
{
	fprintf(stderr, "< HTTP/1.1 %d %s\n", evhttp_request_get_response_code(remote_rsp), evhttp_request_get_response_code_line(remote_rsp));
	evkeyvalq* headers = evhttp_request_get_input_headers(remote_rsp);
	/*for (headers->tqh_first)
	{
	}*/
	/*struct evkeyval* header;
	TAILQ_FOREACH(header, headers, next)
	{
		fprintf(stderr, "< %s: %s\n", header->key, header->value);
	}
	fprintf(stderr, "< \n");*/
	return 0;
}

void SCarabLib::SHttpClient::ReadChunkCallback(evhttp_request * remote_rsp, void * args)
{
	SHttpClient* pThis = reinterpret_cast<SHttpClient*>(args);
	if (!pThis)
	{
		event_base_loopexit(pThis->m_pBase, NULL);
		return;
	}
	char buf[1024] = { 0 };
	struct evbuffer* evbuf = evhttp_request_get_input_buffer(remote_rsp);
	int n = 0;
	while (true)
	{
		n = evbuffer_remove(evbuf, buf, 1024);
		if (n > 0)
		{
			pThis->m_pResponse->ApendBinary(buf, n);
		}	
		else
		{
			break;
		}
	}
}

void SCarabLib::SHttpClient::RemoteRequestErrorCallback(evhttp_request_error error, void * args)
{
	SHttpClient* pThis = reinterpret_cast<SHttpClient*>(args);
	if (pThis)
	{
		fprintf(stderr, "request failed\n");
		event_base_loopexit(pThis->m_pBase, NULL);
	}	
}

void SCarabLib::SHttpClient::RemoteConnectionCloseCallback(evhttp_connection * conn, void * args)
{
	SHttpClient* pThis = reinterpret_cast<SHttpClient*>(args);
	if (pThis)
	{
		fprintf(stderr, "remote connection closed\n");
		event_base_loopexit(pThis->m_pBase, NULL);
	}
}

SCarabLib::SHttpResponse * SCarabLib::SHttpClient::GetResponse() const
{
	return m_pResponse;
}

SCarabLib::SHttpResponse::SHttpResponse()
	: Success(false)
	, m_pResponse(nullptr)
	, m_nLen(0)
{
	m_pResponse = (char*)malloc(1);
	*m_pResponse = '\0';
}

SCarabLib::SHttpResponse::~SHttpResponse()
{
	free(m_pResponse);
}

void SCarabLib::SHttpResponse::SetResponse(const char * response, int len)
{
	free(this->m_pResponse);
	this->m_pResponse = (char*)malloc(len + 1);
	this->m_nLen = len;
	memcpy(this->m_pResponse, response, len);
	this->m_pResponse[m_nLen] = '\0';
}

void SCarabLib::SHttpResponse::ApendBinary(const char * data, int len)
{
	int nLen = len + m_nLen + 1;
	char* buf = (char*)malloc(nLen);
	memcpy(buf, m_pResponse, m_nLen);
	memcpy(buf + m_nLen, data, len);
	buf[nLen - 1] = '\0';
	free(m_pResponse);
	m_pResponse = buf;
	m_nLen = nLen;
}

char * SCarabLib::SHttpResponse::GetResponse() const
{
	return m_pResponse;
}

bool SCarabLib::SHttpResponse::WriteToText(const char * szFileName)
{
	assert(szFileName);
	std::ofstream os(szFileName);
	if (!os.is_open())
	{
		return false;
	}
	os.write(m_pResponse, m_nLen);
	os.close();
	return true;
}

bool SCarabLib::SHttpResponse::WriteToBinary(const char * szFileName)
{
	assert(szFileName);
	std::ofstream os(szFileName, std::ios::binary);
	if (!os.is_open())
	{
		return false;
	}
	os.write(m_pResponse, m_nLen);
	//os << m_pResponse;
	os.close();
	return true;
}
