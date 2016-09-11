/*
 * Debug.cpp
 *
 */

#include "../include/EnableDebug.h"
#include <SmingCore/Network/TelnetServer.h>

EnableDebug::EnableDebug()
{
	debugf("ExampleCommand Instantiating");
}

EnableDebug::~EnableDebug()
{
}

void EnableDebug::initCommand()
{
	commandHandler.registerCommand(CommandDelegate("enDebug","Enable Debug Command from Class","Application",commandFunctionDelegate(&EnableDebug::processEnableDebug,this)));
	commandHandler.registerCommand(CommandDelegate("startSPI","Start SPI","Application",commandFunctionDelegate(&EnableDebug::processStartSPI,this)));
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
		commandOutput->printf("Enable Debug Commands available : \r\n");
		commandOutput->printf("on   : Set example status ON\r\n");
		commandOutput->printf("off  : Set example status OFF\r\n");
		commandOutput->printf("status : Show example status\r\n");
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
			commandOutput->printf("Example Status is %s\r\n",tempString.c_str());
		};
		telnet.enableDebug(status);
		Debug.printf("This is debug after telnet start\r\n");
	}
}


