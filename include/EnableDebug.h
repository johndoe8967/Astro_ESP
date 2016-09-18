/*
 * ENABLE_DEBUG.h
 *
 */

#ifndef SMINGCORE_ENABLE_DEBUG_H_
#define SMINGCORE_ENABLE_DEBUG_H_

#include "WString.h"
#include "../Services/CommandProcessing/CommandProcessingIncludes.h"
#include <SmingCore/Network/TelnetServer.h>

extern TelnetServer telnet;
extern void initSPI(unsigned int time);

class EnableDebug
{
public:
	EnableDebug();
	virtual ~EnableDebug();
	void initCommand();
	bool getDebugEnabled() {return status;};

private:
	bool status = true;
	void processEnableDebug(String commandLine, CommandOutput* commandOutput);
	void processStartSPI(String commandLine, CommandOutput* commandOutput);
};


#endif /* SMINGCORE_DEBUG_H_ */
