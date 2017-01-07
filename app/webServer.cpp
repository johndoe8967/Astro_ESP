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

char manualOpen=0;
char paramOpen=0;
char debugOpen=0;

// send given string message to all clients
WebSocketsList &clients=server.getActiveWebSockets();
void sendMessageToClients(const String &message, WebSocket &socket) {
	for (int i = 0; i < clients.count(); i++) {
		if (!(clients[i]==socket)) {
			clients[i].sendString(message);
		}
	}
}

void sendMessageToAllClients(const String &message) {
	for (int i = 0; i < clients.count(); i++)
		clients[i].sendString(message);
}

// parts to assemble json message
char const cjsonpart1[25] = "{\"type\": \"JSON\",\"msg\": \"";
char const cjsonpart2[14] = "\", \"value\": \"";

// calculate JSON message for SPI IN or OUT data and send to all clients
void sendSPIData(bool in, unsigned char bytes[11]) {
	String message = cjsonpart1;

	if (in) message +=  "SPIIN";
	else message +=  "SPIOUT";

	message +=  cjsonpart2;

	for (char i=0; i<11; i++) {
		message += String(bytes[i],16);
		message += ',';
	}

	message += "\"}";
	sendMessageToAllClients(message);
}

// calculate JSON message from single message and value
String sendString (const char *msg, const char *value) {
	String msg_string = String(msg);
	String val_string = String(value);
	return cjsonpart1 + msg_string +cjsonpart2  + val_string + "\"}";
}

// send message to all clients
void sendMessage(const char *msg, const char *value) {
	String message = sendString(msg, value);
	sendMessageToAllClients(message);
}

// send actual data to all clients
// - increment counter will be always sent
// - other values will be sent if at least one client has opened the section
void sendActData() {
	char value_msg[10];
	ltoa(myMove->getPos(0),value_msg,10);
	sendMessage("incr0",value_msg);
	ltoa(myMove->getPos(1),value_msg,10);
	sendMessage("incr1",value_msg);

	if (manualOpen > 0) {
		ltoa(myAI->getAI(0),value_msg,10);
		sendMessage("AI0",value_msg);
		ltoa(myAI->getAI(1),value_msg,10);
		sendMessage("AI1",value_msg);
	}
	if (paramOpen > 0) {
		ltoa(SystemClock.now(),value_msg,10);
		sendMessage("UTC",value_msg);
	}
}


//*********************************************
//* web server for default page
//*********************************************
void onIndex(HttpRequest &request, HttpResponse &response)
{
	if (totalActiveSockets < 4) {
		TemplateFileStream *tmpl = new TemplateFileStream("index.html");
		response.sendTemplate(tmpl); // this template object will be deleted automatically
	} else {
		response.notFound();
	}
}

//*********************************************
//* web server for used files (java scripts)
//*********************************************
void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/') file = file.substring(1);
	if (file[0] == '.') response.forbidden();
	else {
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}


//*********************************************
//* websocket methods
//*********************************************
void wsConnected(WebSocket& socket)
{
	totalActiveSockets++;								// count active clients
#ifdef trackConnections
	Debug.println(String("ActiveSockets ")+String(totalActiveSockets));
#endif
	if (totalActiveSockets >= 4) socket.close();			// allow up to 4 clients because of memory limitations
	else {
		// if valid client then send initial values
		String message;
		char value_msg[10];

		dtostrf_p(myMove->getRate(0),6,2,value_msg,'0');
		message = sendString ("Rate0", value_msg);
		socket.sendString(message);

		dtostrf_p(myMove->getRate(1),6,2,value_msg,'0');
		message = sendString ("Rate1", value_msg);
		socket.sendString(message);

		dtostrf_p(myMove->getAccel(0),6,2,value_msg,'0');
		message = sendString ("Accel0", value_msg);
		socket.sendString(message);

		dtostrf_p(myMove->getAccel(1),6,2,value_msg,'0');
		message = sendString ("Accel1", value_msg);
		socket.sendString(message);
	}

	WebSocketsList &clients = server.getActiveWebSockets();	// update list of active clients
}

// count number of clients that opened a specific section
void trackOpenDialog (bool open, char &counter) {
	if (open) counter++;
	else if (counter > 0) counter--;
#ifdef trackConnections
	Debug.println('0' + counter);
#endif
}


void workJsonObjekt(WebSocket& socket, JsonObject &root) {
	String value = root["msg"].asString();

#ifdef debugSPI
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
	if (value==String("enableDebug")) {
		enableBytesOut = root["value"];
	}
#endif
	{
		bool bVal = root["value"];
		if (value==String("magnet")) {
			if (bVal) 	myDDS->setMagnet();
			else 		myDDS->clrMagnet();
		}
		if (value==String("PARAM")) {
			trackOpenDialog(bVal, paramOpen);
		}
		if (value==String("manuell")) {
			trackOpenDialog(bVal, manualOpen);
		}

		if (value==String("Debug")) {
			trackOpenDialog(bVal, debugOpen);
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
		if (value==String("Igain0")) {
			myMove->setIControl(0,val);
		}
		if (value==String("Igain1")) {
			myMove->setIControl(1,val);
		}
		if (value==String("PosLim0")) {
			myMove->setPositionLimit(0,val);
		}
		if (value==String("PosLim1")) {
			myMove->setPositionLimit(1,val);
		}
		if (value==String("Rate0")) {
			myMove->setRate(0,val);
		}
		if (value==String("Rate1")) {
			myMove->setRate(1,val);
		}
		if (value==String("Accel0")) {
			myMove->setAccel(0,val);
		}
		if (value==String("Accel1")) {
			myMove->setAccel(1,val);
		}
	}
	{
		char cVal = root["value"];
		if (value==String("motor1")) {
			myMove->setPWM(0,cVal);
		}
		if (value==String("motor2")) {
			myMove->setPWM(1,cVal);
		}
		if (value==String("filter")) {
			myAI->setFilter((unsigned char)cVal);
		}

	}

	{
		long targetPos = root["value"];
		if (value==String("target0")) {
			myMove->setPosition(0,targetPos);
			sendMessage("target0",root["value"]);
		}
		if (value==String("target1")) {
			myMove->setPosition(1,targetPos);
			sendMessage("target1",root["value"]);
		}
	}
	{
		int iValue = root["value"];
		if (value==String("DDSLED")) {
			if (iValue < 0) {
				iValue *= -1;
				myDDS->clrLED(iValue-1);
			} else {
				myDDS->setLED(iValue-1);
			}
		}
		if (value==String("MOTLED")) {
			if (iValue < 0) {
				iValue *= -1;
				myMove->clrLED(iValue-1);
			} else {
				myMove->setLED(iValue-1);
			}
		}
		if (value==String("POSCTRL")) {
			if (iValue == 0) {
				myMove->posControlEnable(0);
				usePoti = 0;
			}
			if (iValue == 1) {
				myMove->posControlEnable(1);
				usePoti = 0;
			}
			if (iValue == 2) {
				myMove->posControlEnable(0);
				usePoti = 1;
			}
		}
		if (value==String("mode")) {
			myMove->setPosition(0,myMove->getPos(0));
			myMove->setPosition(1,myMove->getPos(1));
			setMode((MODES)iValue);
		}

	}
	if (value==String("frequency")) {
		unsigned long freq = root["value"];
		myDDS->setDDSValue(freq);
	}

	if (value==String("WIFI")) {
		String SSID = root["SSID"].asString();
		String PWD = root["PWD"].asString();
		WifiStation.config(SSID,PWD);
	}
	if (value==String("UTC")) {
		time_t UTC = root["value"];
		SystemClock.setTime(UTC,eTZ_UTC);
	}

}


void wsMessageReceived(WebSocket& socket, const String& message)
{
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject(message);
	sendMessageToClients(message,socket);
	Debug.println(message);

	String value = root["type"].asString();
	if (value==String("JSON")) {
		workJsonObjekt(socket, root);
	} else if (value==String("ARRAY")) {
		JsonArray& msg = root["msg"];
		for (int i = 0; i<msg.size(); i++) {
			workJsonObjekt(socket, msg[i]);
		}
	}
}

void wsBinaryReceived(WebSocket& socket, uint8_t* data, size_t size)
{
}

void wsDisconnected(WebSocket& socket)
{
	totalActiveSockets--;
	manualOpen = min(manualOpen, totalActiveSockets);
	debugOpen = min(debugOpen, totalActiveSockets);
	paramOpen = min(paramOpen, totalActiveSockets);
	WebSocketsList &clients = server.getActiveWebSockets();
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
}




