/*
 * Debug.cpp
 *
 */

#include "../include/EnableDebug.h"
#include <SmingCore/Network/TelnetServer.h>

EnableDebug::EnableDebug()
{
}

EnableDebug::~EnableDebug()
{
}

void EnableDebug::initCommand()
{
	commandHandler.registerCommand(CommandDelegate("enDebug","Enable Debug ","Appl",commandFunctionDelegate(&EnableDebug::processEnableDebug,this)));
	commandHandler.registerCommand(CommandDelegate("startSPI","Start SPI","Appl",commandFunctionDelegate(&EnableDebug::processStartSPI,this)));
	commandHandler.registerCommand(CommandDelegate("showIP","Show IP","Appl",commandFunctionDelegate(&EnableDebug::showIP,this)));
}


void EnableDebug::showIP(String commandLine, CommandOutput* commandOutput) {
	String IP = WifiStation.getIP().toString();
	commandOutput->printf(IP.c_str());
	commandOutput->printf("\r\n");
}

void EnableDebug::processStartSPI(String commandLine, CommandOutput* commandOutput)
{
	Vector<String> commandToken;
	int numToken = splitString(commandLine, ' ' , commandToken);

	if (numToken == 1) {
		commandOutput->printf("Start SPI\r\n");
		initSPI(1000);
	} else if (numToken == 2) {
		initSPI(atoi(commandToken[1].c_str()));

	}

}
void EnableDebug::processEnableDebug(String commandLine, CommandOutput* commandOutput)
{
	Vector<String> commandToken;
	int numToken = splitString(commandLine, ' ' , commandToken);

	if (numToken == 1)
	{
		commandOutput->printf("on/off/status\r\n");
	}
	else
	{
		if (commandToken[1] == "on")
		{
			status = true;
			commandOutput->printf("Status ON\r\n");
		}
		else if (commandToken[1] == "off")
		{
			status = false;
			commandOutput->printf("Status OFF\r\n");
		}
		else if (commandToken[1] == "status")
		{
			String tempString = status ? "ON" : "OFF";
			commandOutput->printf("Status is %s\r\n",tempString.c_str());
		};
		telnet.enableDebug(status);
	}
}


