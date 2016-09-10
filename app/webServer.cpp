/*
 * webServer.cpp
 *
 *  Created on: 10.09.2016
 *      Author: johndoe
 */
#include <SmingCore/SmingCore.h>
#include "webServer.h"

HttpServer server;
int totalActiveSockets = 0;

void onIndex(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("index.html");
//	auto &vars = tmpl->variables();
	//vars["counter"] = String(counter);
	response.sendTemplate(tmpl); // this template object will be deleted automatically
}

void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

void wsConnected(WebSocket& socket)
{
	totalActiveSockets++;

	// Notify everybody about new connection
	WebSocketsList &clients = server.getActiveWebSockets();
//	for (int i = 0; i < clients.count(); i++)
//		clients[i].sendString("New friend arrived! Total: " + String(totalActiveSockets));
}

void wsMessageReceived(WebSocket& socket, const String& message)
{
	debugf("WebSocket message received:\r\n%s\r\n", message.c_str());
//	String response = "Echo: " + message;
//	socket.sendString(response);
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject(message);

	String value = root["type"].asString();
	if (value==String("JSON")) {
		value = root["msg"].asString();
		if (value==String("bytes")) {
			String temp;
			const char* buffer;
			char* end;
			temp = root["value"].asString();
			buffer = temp.c_str();

			for (char i=0; i<sizeof(bytes); i++) {
				bytes[i] = (unsigned char)strtol (buffer,&end,16);
				buffer = end + 1;
			}
		}
		if (value==String("WIFI")) {
			String SSID = root["SSID"].asString();
			String PWD = root["PWD"].asString();
			WifiStation.config(SSID,PWD);
		}
	}
}

void wsBinaryReceived(WebSocket& socket, uint8_t* data, size_t size)
{
	Serial.printf("Websocket binary data recieved, size: %d\r\n", size);
}

void wsDisconnected(WebSocket& socket)
{
	totalActiveSockets--;

	// Notify everybody about lost connection
	WebSocketsList &clients = server.getActiveWebSockets();
//	for (int i = 0; i < clients.count(); i++)
//		clients[i].sendString("We lost our friend :( Total: " + String(totalActiveSockets));
}

void startWebServer()
{
	server.listen(80);
	server.addPath("/", onIndex);
	server.setDefaultHandler(onFile);

	// Web Sockets configuration
	server.enableWebSockets(true);
	server.setWebSocketConnectionHandler(wsConnected);
	server.setWebSocketMessageHandler(wsMessageReceived);
	server.setWebSocketBinaryHandler(wsBinaryReceived);
	server.setWebSocketDisconnectionHandler(wsDisconnected);

	Serial.println("\r\n=== WEB SERVER STARTED ===");
	Serial.println(WifiStation.getIP());
	Serial.println("==============================\r\n");
}



