/////////////////////////////////////////////////////////////////////////////
// SMSEngine.h
//
// Copyright (C) 2013 Veria Kalantari
// veria.kalantary@gmail.com
// https://github.com/veriak
//

#ifndef SMS_ENGINE_INCLUDED
#define SMS_ENGINE_INCLUDED
#pragma once

#include <list>

#include "SerialCommPort.h"
#include "StrConversion.h"

using namespace SerialCommPort;
using namespace StrConversion;


namespace SMSEngine
{

#define MAXEXPECTEDRESULTS			5

struct ReadMessage
{
    char szMessageBody[161];
    char szMobileNumber[15];
    char szMessageTime[20];
    char szMessageCenter[15];
    int nMessageSegmentCount;
    int nMessageSegmentNumber;
    int nMessageIndex;
    int nMessageNestNumber;
    int nMessageType;
};

typedef std::list<ReadMessage> ReadMessageList;

class CSMSEngine : public CSerialCommPort
{
private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;
	LPTSTR m_ptcsPortName;	
	LPVOID m_pReadBuff;
	int m_nReadBuffSize;
	int m_nTotalReadSize;
	int m_nExpectedResultsCount;
	LPCSTR m_pExpectedResults[MAXEXPECTEDRESULTS];
	LPCSTR m_pszFoundResult;
	HANDLE m_hExpectedResultEvent;
	bool m_bMessageFormat;
	CRITICAL_SECTION m_csLock;
	int m_nRefNum;
	
protected:
	void ClearBuffer(LPVOID &lpBuff);
	virtual void Clear(void);
	void CleanUp(void);

	static LRESULT CALLBACK OnCommReadEvent(CSMSEngine *_this, DWORD dwBytesRead);
	
public:
	CSMSEngine(void);
	CSMSEngine(LPCTSTR lptcsPortName, DWORD dwBaudRate = 9600);
	~CSMSEngine(void);	

	void Init(void);
	void Init(LPCTSTR lptcsPortName, DWORD dwBaudRate = 9600);	

	void LockThis();
	void UnLockThis();
	void InitLock();
	void DelLock();

	char *SwapBuffer(char *lpBuffer, int nBuffSize);
	char *SwapMobileNumber(char *lpszBuffer);
	int StrToPDU(LPSTR lpszBuffer);
	int PDUToStr(LPBYTE lpPDUBuffer, int nPDULen, LPSTR lpszStrBuffer);

	bool CheckGSMModemActivity(void);
	bool SetCMGF(bool bMessageFormat = false);
	bool SetCPMS(LPCSTR lpszMS = "SM");
	bool SetSMSC(LPCSTR lpszSMSCNumber);
	bool SendTextMessage(LPCSTR lpszMobileNumber, LPCSTR lpszMessageBody, int nWaitTimeInSec = 60);
	bool SendPDUMessage(LPCSTR lpszMobileNumber, LPCSTR lpszMessageBody, int nWaitTimeInSec = 60);
	bool SendUnicodeMessage(LPCSTR lpszMobileNumber, LPCWSTR lpwcsMessageBody, int nWaitTimeInSec = 60);
	bool GetIMEI(LPSTR lpszIMEIBuff);
	bool DeleteMessage(LPCSTR lpszMessageIndex);
	bool SendVCard(LPCSTR lpszMobileNumber,
		LPCSTR lpszVCardName, LPCSTR lpszVCardNumber, int nWaitTimeInSec = 60);
	bool GetReadMessageList(ReadMessageList *lpReadMessageList,
		int nMessageType = 0, int nWaitTimeInSec = 60);
	bool SendLongPDUMessage(LPCSTR lpszMobileNumber, LPCSTR lpszMessageBody, int nWaitTimeInSec = 60);
	bool SendLongUnicodeMessage(LPCSTR lpszMobileNumber, LPCWSTR lpwcsMessageBody, int nWaitTimeInSec = 60);
};

}

#endif