#ifndef INCLUDE_NTPCLIENT_H_
#define INCLUDE_NTPCLIENT_H_


class ntpClient
{
public:
	ntpClient()
	{
		ntpcp = new NtpClient("pool.ntp.org", 30, NtpTimeResultDelegate(&ntpClient::ntpResult, this));
	};

	void ntpResult(NtpClient& client, time_t ntpTime)
	{
		SystemClock.setTime(ntpTime, eTZ_UTC);
		Debug.print("Time_t = ");
		Debug.print(ntpTime);
		Debug.print(" Time = ");
		Debug.println(SystemClock.getSystemTimeString());
	}

private:
	NtpClient *ntpcp;
};




#endif /* INCLUDE_NTPCLIENT_H_ */
