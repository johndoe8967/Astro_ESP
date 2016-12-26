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
#ifdef startSPI
	commandHandler.registerCommand(CommandDelegate("startSPI","Start SPI","Appl",commandFunctionDelegate(&EnableDebug::processStartSPI,this)));
#endif
	commandHandler.registerCommand(CommandDelegate("showIP","Show IP","Appl",commandFunctionDelegate(&EnableDebug::showIP,this)));
}


void EnableDebug::showIP(String commandLine, CommandOutput* commandOutput) {
	String IP = WifiStation.getIP().toString();
	commandOutput->printf(IP.c_str());
	commandOutput->printf("\r\n");
}

#ifdef startSPI
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
#endif

void EnableDebug::processEnableDebug(String commandLine, CommandOutput* commandOutput)
{
	Vector<String> commandToken;
	int numToken = splitString(commandLine, ' ' , commandToken);

	if (numToken == 1)
	{
		commandOutput->printf("on/off\r\n");
	}
	else
	{
		commandOutput->printf("Status ");
		if (commandToken[1] == "on")
		{
			status = true;
			commandOutput->printf("ON\r\n");
		}
		else if (commandToken[1] == "off")
		{
			status = false;
			commandOutput->printf("OFF\r\n");
		};
		telnet.enableDebug(status);
	}
}


