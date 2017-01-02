/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** HttpServerHandler.cpp
** 
** -------------------------------------------------------------------------*/

#include <iostream>

#include "webrtc/base/pathutils.h"

#include "HttpServerRequestHandler.h"

/* ---------------------------------------------------------------------------
**  http callback
** -------------------------------------------------------------------------*/
void HttpServerRequestHandler::OnRequest(rtc::HttpServer*, rtc::HttpServerTransaction* t) 
{
	std::string host;
	std::string path;
	t->request.getRelativeUri(&host, &path);
	std::cout << "===> HTTP request " <<  path << std::endl;

	// read body
	size_t size = 0;
	t->request.document->GetSize(&size);
	t->request.document->Rewind();
	char buffer[size];
	size_t readsize = 0;
	rtc::StreamResult res = t->request.document->ReadAll(&buffer, size, &readsize, NULL);
	
	if (res == rtc::SR_SUCCESS)
	{
		std::string body(buffer, readsize);			
		std::cout << "body:" << body << std::endl;

		std::string peerid;	
		t-> request.hasHeader("peerid", &peerid);				
		
		if (path == "/getDeviceList")
		{
			std::string answer(Json::StyledWriter().write(m_webRtcServer->getDeviceList()));
			
			rtc::MemoryStream* mem = new rtc::MemoryStream(answer.c_str(), answer.size());			
			t->response.set_success("text/plain", mem);			
		}
		else if (path == "/call")
		{
			Json::Reader reader;
			Json::Value  jmessage;
			
			if (!reader.parse(body, jmessage)) 
			{
				LOG(WARNING) << "Received unknown message:" << body;
			}
			else
			{			
				std::string answer(Json::StyledWriter().write(m_webRtcServer->call(peerid, jmessage)));
				std::cout << peerid << ":" << answer << std::endl;
				
				if (answer.empty() == false)
				{
					rtc::MemoryStream* mem = new rtc::MemoryStream(answer.c_str(), answer.size());			
					t->response.addHeader("peerid",peerid);	
					t->response.set_success("text/plain", mem);			
				}
			}
		}
		else if (path == "/hangup")
		{
			m_webRtcServer->hangUp(peerid);
			t->response.set_success();			
		}
		else if (path == "/getIceCandidate")
		{		
			std::string answer(Json::StyledWriter().write(m_webRtcServer->getIceCandidateList(peerid)));	
			std::cout << peerid << ":" << answer << std::endl;
			
			rtc::MemoryStream* mem = new rtc::MemoryStream(answer.c_str(), answer.size());			
			t->response.set_success("text/plain", mem);			
		}
		else if (path == "/addIceCandidate")
		{
			Json::Reader reader;
			Json::Value  jmessage;
			
			if (!reader.parse(body, jmessage)) 
			{
				LOG(WARNING) << "Received unknown message:" << body;
			}
			else
			{
				m_webRtcServer->addIceCandidate(peerid, jmessage);			
				t->response.set_success();			
			}
		}
		else
		{
			rtc::Pathname pathname("index.html");
			rtc::FileStream* fs = rtc::Filesystem::OpenFile(pathname, "rb");
			if (fs)
			{
				t->response.set_success("text/html", fs);			
			}
		}
	}
	
	t->response.setHeader(rtc::HH_CONNECTION,"Close");
	m_server->Respond(t);
}
