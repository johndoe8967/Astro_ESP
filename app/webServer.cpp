/*
 * webServer.cpp
 *
 *  Created on: 10.09.2016
 *      Author: johndoe
 */
#include <SmingCore/SmingCore.h>
#include "webServer.h"
#include <Debug.h>
HttpServer server;
int totalActiveSockets = 0;
bool manualOpen=false;
bool paramOpen=false;

WebSocketsList &clients=server.getActiveWebSockets();

void sendSPIData(bool in, unsigned char bytes[11]) {
	String outData;
	for (char i=0; i<11; i++) {
		outData += String(bytes[i],16);
		outData += ',';
	}
	String message = "{\"type\": \"JSON\",\"msg\": ";
	if (in) {
		message +=  "\"SPIIN\"";
	}
	else {
		message +=  "\"SPIOUT\"";
	}
	message +=  ", \"value\": \"" + outData + "\"}";

	for (int i = 0; i < clients.count(); i++)
		clients[i].sendString(message);
}

void sendMessage(const char *msg, const char *value) {
	String msg_string = String(msg);
	String val_string = String(value);
	String message = "{\"type\": \"JSON\",\"msg\": \""+ msg_string +"\", \"value\": \"" + val_string + "\"}";
	for (int i = 0; i < clients.count(); i++)
		clients[i].sendString(message);
}

void sendActData() {
	char value_msg[10];
	ltoa(myMove->getPos(0),value_msg,10);
	sendMessage("incr0",value_msg);
	ltoa(myMove->getPos(1),value_msg,10);
	sendMessage("incr1",value_msg);

	if (manualOpen) {
		ltoa(myAI->getAI(0),value_msg,10);
		sendMessage("AI0",value_msg);
		ltoa(myAI->getAI(1),value_msg,10);
		sendMessage("AI1",value_msg);
	}
	if (paramOpen) {
		ltoa(SystemClock.now(),value_msg,10);
		sendMessage("UTC",value_msg);
	}

}

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
}

/***************************************************************
 * Connection established
 * 	start services
 */
void newConnectOk()
{
	String IP = WifiStation.getIP().toString();
	Debug.println(IP);
}

void wsMessageReceived(WebSocket& socket, const String& message)
{
	debugf("WebSocket message received:\r\n%s\r\n", message.c_str());
//	String response = "Echo: " + message;
//	socket.sendString(response);
	Debug.println(message);
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject(message);

	String value = root["type"].asString();
	if (value==String("JSON")) {
		value = root["msg"].asString();
		if (value==String("bytes")) {
			const char* buffer;
			char* end;
			value = root["value"].asString();
			buffer = value.c_str();

			char temp[10];
			for (char i=0; i<sizeof(bytes); i++) {
				bytes[i] = (unsigned char)strtol (buffer,&end,16);
				buffer = end + 1;
			}
		}
		if (value==String("mode")) {
			int temp = root["value"];
			setMode((MODES)temp);
		}

		if (value==String("enableDebug")) {
			enableBytesOut = root["value"];
		}
		if (value==String("magnet")) {
			if (root["value"] == 1) {
				myDDS->setMagnet();
			} else {
				myDDS->clrMagnet();
			}
		}

		{
			float val = root["value"];
			if (value==String("Pgain0")) {
				myMove->setPControl(0,val);
			}
			if (value==String("Pgain1")) {
				myMove->setPControl(1,val);
			}
		}
		{
			char pwm = root["value"];
			if (value==String("motor1")) {
				myMove->setPWM(0,pwm);
			}
			if (value==String("motor2")) {
				myMove->setPWM(1,pwm);
			}
		}

		{
			long targetPos = root["value"];
			if (value==String("target0")) {
				myMove->setPosition(0,targetPos);
			}
			if (value==String("target1")) {
				myMove->setPosition(1,targetPos);
			}
		}
		{
			int LED = root["value"];
			if (value==String("DDSLED")) {
				if (LED < 0) {
					LED *= -1;
					myDDS->clrLED(LED-1);
				} else {
					myDDS->setLED(LED-1);
				}
			}
			if (value==String("MOTLED")) {
				if (LED < 0) {
					LED *= -1;
					myMove->clrLED(LED-1);
				} else {
					myMove->setLED(LED-1);
				}
			}
			if (value==String("POSCTRL")) {
				if (LED == 0) {
					myMove->posControlEnable(0);
					usePoti = 0;
				}
				if (LED == 1) {
					myMove->posControlEnable(1);
					usePoti = 0;
				}
				if (LED == 2) {
					myMove->posControlEnable(0);
					usePoti = 1;
				}
			}
		}
		if (value==String("frequency")) {
			unsigned long freq = root["value"];
			myDDS->setDDSValue(freq);
		}
		if (value==String("filter")) {
			unsigned char val = root["value"];
			myAI->setFilter(val);
		}

		if (value==String("WIFI")) {
			String SSID = root["SSID"].asString();
			String PWD = root["PWD"].asString();
			WifiStation.config(SSID,PWD);
			WifiStation.waitConnection(newConnectOk);
		}
		if (value==String("UTC")) {
			time_t UTC = root["value"];
			SystemClock.setTime(UTC,eTZ_UTC);
		}
		if (value==String("PARAM")) {
			paramOpen = root["value"];
		}
		if (value==String("manuell")) {
			manualOpen = root["value"];
		}
	}
}

void wsBinaryReceived(WebSocket& socket, uint8_t* data, size_t size)
{
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

	Debug.printf("\r\n=== WEB SERVER STARTED ===");
}




