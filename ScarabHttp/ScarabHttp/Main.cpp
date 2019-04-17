
#include "SHttpClient.h"

using namespace SCarabLib;

int main()
{
	SHttpClient* httpClient = SHttpClient::CreateClient();	
	int ret = httpClient->SendRequest("https://www.baidu.com/", evhttp_cmd_type::EVHTTP_REQ_GET);
	SHttpResponse* pResponse = httpClient->GetResponse();
	pResponse->WriteToText("111.txt");
	SHttpClient::DestroyClient(&httpClient);
	return 0;
}