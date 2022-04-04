/////////////////////////////////////////////////////////////////////////////
// SMS.cpp
//
// Copyright (C) 2013 Veria Kalantari
// veria.kalantary@gmail.com
// https://github.com/veriak
//

#include "stdafx.h"
#include "SMSEngine.h"

using namespace SMSEngine;


/////////////////////////////////////////////////////////////////////////////
// CSMSEngine

CSMSEngine::CSMSEngine(void)
{
	Init();
}

CSMSEngine::CSMSEngine(LPCTSTR lptcsPortName, DWORD dwBaudRate)
{
	Init(lptcsPortName, dwBaudRate);
}

CSMSEngine::~CSMSEngine(void)
{
	Clear();
}

void CSMSEngine::Init(void)
{
	m_hInstance = NULL;
	m_hWnd = NULL;	
	m_ptcsPortName = NULL;		
	m_pReadBuff = NULL;
	m_nReadBuffSize = SIZEOFCOMMBUFF;
	m_nTotalReadSize = 0;
	m_nExpectedResultsCount = 0;
	m_pszFoundResult = NULL;	
	m_nRefNum = 1;

	memset(&m_pExpectedResults, 0, sizeof(m_pExpectedResults));
	SetLength(&m_pReadBuff, sizeof(BYTE), m_nReadBuffSize);

	m_hExpectedResultEvent = ::CreateEvent(0, TRUE, FALSE, 0);

	InitLock();
	SetCMGF(false);
}

void CSMSEngine::Init(LPCTSTR lptcsPortName, DWORD dwBaudRate)
{
	Init();
	__super::Init((LRESULT CALLBACK) CSMSEngine::OnCommReadEvent, lptcsPortName, dwBaudRate);

	SetCMGF();
}

void CSMSEngine::LockThis()
{
	::EnterCriticalSection(&m_csLock);
}

void CSMSEngine::UnLockThis()
{
	::LeaveCriticalSection(&m_csLock);
}

void CSMSEngine::InitLock()
{
	::InitializeCriticalSection(&m_csLock);
}

void CSMSEngine::DelLock()
{
	::DeleteCriticalSection(&m_csLock);
}

void CSMSEngine::ClearBuffer(LPVOID &lpBuff)
{
	if (lpBuff)
	{
		free(lpBuff);
		lpBuff = NULL;
	}
}

void CSMSEngine::Clear(void)
{
	::CloseHandle(m_hExpectedResultEvent);
	ClearBuffer((LPVOID &) m_ptcsPortName);
	SetLength(&m_pReadBuff, sizeof(BYTE), 0);
	DelLock();
}

void CSMSEngine::CleanUp(void)
{
	LockThis();

	::ResetEvent(m_hExpectedResultEvent);
	memset(m_pReadBuff, 0, m_nReadBuffSize);
	m_nTotalReadSize = 0;

	if (m_nReadBuffSize > SIZEOFCOMMBUFF)
	{
		m_nReadBuffSize = SIZEOFCOMMBUFF;	
		SetLength(&m_pReadBuff, sizeof(BYTE), m_nReadBuffSize);
	}

	m_nExpectedResultsCount = 0;
	memset(&m_pExpectedResults, 0, sizeof(m_pExpectedResults));
	m_pszFoundResult = NULL;

	UnLockThis();
}

LRESULT CALLBACK CSMSEngine::OnCommReadEvent(CSMSEngine *_this, DWORD dwBytesRead)
{
	LRESULT lResult = 0;	

	if (dwBytesRead > 0)
	{		
		UINT nNeededSize = 0;

		_this->LockThis();

		if (_this->m_nTotalReadSize + dwBytesRead > _this->m_nReadBuffSize)
		{
			_this->m_nReadBuffSize += dwBytesRead;
			SetLength(&_this->m_pReadBuff, sizeof(char), _this->m_nReadBuffSize);
		}		

		if (_this->ReadExact((char *) _this->m_pReadBuff + _this->m_nTotalReadSize,
				_this->m_nReadBuffSize - _this->m_nTotalReadSize,
				dwBytesRead, &nNeededSize))
		{
			_this->m_nTotalReadSize += dwBytesRead;

			char *pBuff = (char *) _this->m_pReadBuff;

			for (int i = 0; i < _this->m_nExpectedResultsCount; i++)
			{
				if (strstr(pBuff, _this->m_pExpectedResults[i]))
				{					
					_this->m_pszFoundResult = _this->m_pExpectedResults[i];
					::SetEvent(_this->m_hExpectedResultEvent);
					break;
				}
			}
		}

		_this->UnLockThis();
	}

	return lResult;
}

bool CSMSEngine::CheckGSMModemActivity(void)
{
	bool bResult = false;

	try
	{
		CleanUp();
		m_nExpectedResultsCount = 1;
		m_pExpectedResults[0] = "OK";

		if (Write("AT\r", 3) == S_OK)
			if (::WaitForSingleObject(m_hExpectedResultEvent, 3000) == WAIT_OBJECT_0)
				bResult = true;
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::SetCMGF(bool bMessageFormat)
{
	bool bResult = false;

	try
	{
		CleanUp();
		m_nExpectedResultsCount = 2;
		m_pExpectedResults[0] = "OK";
		m_pExpectedResults[1] = "Error";

		char szATCommand[16];

		sprintf_s(szATCommand, sizeof szATCommand, "AT+CMGF=%d\r", bMessageFormat ? 1 : 0);

		if (Write(szATCommand, strlen(szATCommand)) == S_OK)
		{
			if (::WaitForSingleObject(m_hExpectedResultEvent, 3000) == WAIT_OBJECT_0)
			{
				if (strcmp(m_pszFoundResult, "OK") == 0)
				{
					m_bMessageFormat = bMessageFormat;
					bResult = true;
				}
			}
		}
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::SetCPMS(LPCSTR lpszMS)
{
	bool bResult = false;

	try
	{
		CleanUp();
		m_nExpectedResultsCount = 2;
		m_pExpectedResults[0] = "OK";
		m_pExpectedResults[1] = "Error";

		char szATCommand[20];

		sprintf_s(szATCommand, sizeof szATCommand, "AT+CPMS=\"%s\",\"SM\"\r", lpszMS);

		if (Write(szATCommand, strlen(szATCommand)) == S_OK)
			if (::WaitForSingleObject(m_hExpectedResultEvent, 5000) == WAIT_OBJECT_0)
				if (strcmp(m_pszFoundResult, "OK") == 0)
					bResult = true;
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::SetSMSC(LPCSTR lpszSMSCNumber)
{
	bool bResult = false;

	try
	{
		CleanUp();
		m_nExpectedResultsCount = 2;
		m_pExpectedResults[0] = "OK";
		m_pExpectedResults[1] = "Error";

		char szATCommand[32];

		sprintf_s(szATCommand, sizeof szATCommand, "AT+CSCA=\"%s\"\r", lpszSMSCNumber);

		if (Write(szATCommand, strlen(szATCommand)) == S_OK)
			if (::WaitForSingleObject(m_hExpectedResultEvent, 5000) == WAIT_OBJECT_0)
				if (strcmp(m_pszFoundResult, "OK") == 0)
					bResult = true;
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::SendTextMessage(LPCSTR lpszMobileNumber, LPCSTR lpszMessageBody, int nWaitTimeInSec)
{
	bool bResult = false;

	try
	{
		char szMessageBody[141];

		if (strlen(lpszMessageBody) > 140)
		{
			strncpy(szMessageBody, lpszMessageBody, 140);			
			szMessageBody[140] = (char) 0;
		}
		else
		{
			strcpy(szMessageBody, lpszMessageBody);
		}
		
		strcat(szMessageBody, "\x1A");

		if (!m_bMessageFormat)
			SetCMGF(true);

		if (m_bMessageFormat)
		{
			CleanUp();
			m_nExpectedResultsCount = 2;
			m_pExpectedResults[0] = ">";
			m_pExpectedResults[1] = "Error";

			char szATCommand[32];

			sprintf_s(szATCommand, sizeof szATCommand, "AT+CMGS=\"%s\"\r", lpszMobileNumber);

			if (Write(szATCommand, strlen(szATCommand)) == S_OK)
			{
				if (::WaitForSingleObject(m_hExpectedResultEvent, 5000) == WAIT_OBJECT_0)
				{
					if (strcmp(m_pszFoundResult, ">") == 0)
					{
						CleanUp();
						m_nExpectedResultsCount = 2;
						m_pExpectedResults[0] = "OK";
						m_pExpectedResults[1] = "Error";


						if (Write(szMessageBody, strlen(szMessageBody)) == S_OK)
						{
							if (::WaitForSingleObject(m_hExpectedResultEvent, nWaitTimeInSec * 1000)
									== WAIT_OBJECT_0)
							{
								if (strcmp(m_pszFoundResult, "OK") == 0)
								{
									bResult = true;
								}
							}
						}
					}
				}
			}
		}
	}
	catch(...)
	{
	}

	return bResult;
}

char * CSMSEngine::SwapBuffer(char *lpBuffer, int nBuffSize)
{	
	char c;
	int j;
	
	for (int i = 0; i < nBuffSize / 2; i++)
	{
		j = i * 2;
		c = lpBuffer[j];
		lpBuffer[j] = lpBuffer[j + 1];
		lpBuffer[j + 1] = c;
	}

	return lpBuffer;
}

LPSTR CSMSEngine::SwapMobileNumber(LPSTR lpszBuffer)
{
	int nLen = strlen(lpszBuffer);
	
	if (nLen % 2 == 1)
	{
		strcat(lpszBuffer, "F");
		nLen++;
	}
	
	return SwapBuffer(lpszBuffer, nLen);
}

int CSMSEngine::StrToPDU(LPSTR lpszBuffer)
{
	int nPDULen = 0;	

	if (lpszBuffer)
	{
		int nStrLen = strlen(lpszBuffer);
		int i = 0;

		do
		{
			lpszBuffer[nPDULen] = (BYTE) (lpszBuffer[i] >> (i % 8)) +
				(BYTE) (lpszBuffer[i + 1] << (8 - (i + 1) % 8));
			nPDULen++;
			
			if ((i + 2) % 8 == 0)
				i = i + 2;
			else
				i++;
		} while ((i + 1) < nStrLen);

		if ((i + 1) == nStrLen)
		{
			lpszBuffer[nPDULen] = (BYTE) (lpszBuffer[i] >> (i % 8));
			nPDULen++;
		}
	}

	  return nPDULen;
}

bool CSMSEngine::SendPDUMessage(LPCSTR lpszMobileNumber, LPCSTR lpszMessageBody, int nWaitTimeInSec)
{
	bool bResult = false;

	try
	{
		char szMessageBody[161];
		char szPDUBuff[512];
		char szLen[3];
		char szSwapBuff[16];
		CStrConverter strConv;
		char *pHexStr = NULL;

		if (strlen(lpszMessageBody) > 160)
		{
			strncpy(szMessageBody, lpszMessageBody, 160);			
			szMessageBody[160] = (char) 0;
		}
		else
		{
			strcpy(szMessageBody, lpszMessageBody);
		}
		
		strcpy_s(szSwapBuff, sizeof szSwapBuff, lpszMobileNumber);

		strcpy_s(szPDUBuff, sizeof szPDUBuff, "001100");
		sprintf(szLen, "%02X", (BYTE) strlen(lpszMobileNumber));
		strcat_s(szPDUBuff, sizeof szPDUBuff, szLen);
		strcat_s(szPDUBuff, sizeof szPDUBuff, "91");		
		strcat_s(szPDUBuff, sizeof szPDUBuff, SwapMobileNumber(szSwapBuff));
		strcat_s(szPDUBuff, sizeof szPDUBuff, "0000AA");
		sprintf(szLen, "%02X", strlen(szMessageBody));
		strcat_s(szPDUBuff, sizeof szPDUBuff, szLen);
		int nPDULen = StrToPDU(szMessageBody);
		strConv.ByteArrayToHexStr((const unsigned char *) szMessageBody, nPDULen, &pHexStr);
		strcat_s(szPDUBuff, sizeof szPDUBuff, pHexStr);
		free(pHexStr);

		strcat_s(szPDUBuff, sizeof szPDUBuff, "\x1A");

		if (m_bMessageFormat)
			SetCMGF(false);

		if (!m_bMessageFormat)
		{
			CleanUp();
			m_nExpectedResultsCount = 2;
			m_pExpectedResults[0] = ">";
			m_pExpectedResults[1] = "Error";

			char szATCommand[16];

			sprintf_s(szATCommand, sizeof szATCommand, "AT+CMGS=%d\r", strlen(szPDUBuff) / 2 - 1);

			if (Write(szATCommand, strlen(szATCommand)) == S_OK)
			{
				if (::WaitForSingleObject(m_hExpectedResultEvent, 5000) == WAIT_OBJECT_0)
				{
					if (strcmp(m_pszFoundResult, ">") == 0)
					{
						CleanUp();
						m_nExpectedResultsCount = 2;
						m_pExpectedResults[0] = "OK";
						m_pExpectedResults[1] = "Error";

						if (Write(szPDUBuff, strlen(szPDUBuff)) == S_OK)
						{
							if (::WaitForSingleObject(m_hExpectedResultEvent, nWaitTimeInSec * 1000)
									== WAIT_OBJECT_0)
							{
								if (strcmp(m_pszFoundResult, "OK") == 0)
								{
									bResult = true;
								}
							}
						}
					}
				}
			}
		}
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::SendUnicodeMessage(LPCSTR lpszMobileNumber, LPCWSTR lpwcsMessageBody, int nWaitTimeInSec)
{
	bool bResult = false;

	try
	{
		wchar_t wcsMessageBody[71];
		char szPDUBuff[512];
		char szLen[3];
		char szSwapBuff[16];
		CStrConverter strConv;
		char *pHexStr = NULL;

		if (wcslen(lpwcsMessageBody) > 70)
		{
			wcsncpy(wcsMessageBody, lpwcsMessageBody, 70);			
			wcsMessageBody[70] = (wchar_t) 0;
		}
		else
		{
			wcscpy(wcsMessageBody, lpwcsMessageBody);
		}
		
		strcpy_s(szSwapBuff, sizeof szSwapBuff, lpszMobileNumber);

		strcpy_s(szPDUBuff, sizeof szPDUBuff, "001100");
		sprintf(szLen, "%02X", (BYTE) strlen(lpszMobileNumber));
		strcat_s(szPDUBuff, sizeof szPDUBuff, szLen);
		strcat_s(szPDUBuff, sizeof szPDUBuff, "91");		
		strcat_s(szPDUBuff, sizeof szPDUBuff, SwapMobileNumber(szSwapBuff));
		strcat_s(szPDUBuff, sizeof szPDUBuff, "0008AA");
		sprintf(szLen, "%02X", wcslen(wcsMessageBody) * 2);
		strcat_s(szPDUBuff, sizeof szPDUBuff, szLen);
		int nPDULen = wcslen(wcsMessageBody) * 2;
		SwapBuffer((char *) wcsMessageBody, nPDULen);		
		strConv.ByteArrayToHexStr((const unsigned char *) wcsMessageBody, nPDULen, &pHexStr);
		strcat_s(szPDUBuff, sizeof szPDUBuff, pHexStr);
		free(pHexStr);

		strcat_s(szPDUBuff, sizeof szPDUBuff, "\x1A");

		if (m_bMessageFormat)
			SetCMGF(false);

		if (!m_bMessageFormat)
		{
			CleanUp();
			m_nExpectedResultsCount = 2;
			m_pExpectedResults[0] = ">";
			m_pExpectedResults[1] = "Error";

			char szATCommand[16];

			sprintf_s(szATCommand, sizeof szATCommand, "AT+CMGS=%d\r", strlen(szPDUBuff) / 2 - 1);

			if (Write(szATCommand, strlen(szATCommand)) == S_OK)
			{
				if (::WaitForSingleObject(m_hExpectedResultEvent, 5000) == WAIT_OBJECT_0)
				{
					if (strcmp(m_pszFoundResult, ">") == 0)
					{
						CleanUp();
						m_nExpectedResultsCount = 2;
						m_pExpectedResults[0] = "OK";
						m_pExpectedResults[1] = "Error";

						if (Write(szPDUBuff, strlen(szPDUBuff)) == S_OK)
						{
							if (::WaitForSingleObject(m_hExpectedResultEvent, nWaitTimeInSec * 1000)
									== WAIT_OBJECT_0)
							{
								if (strcmp(m_pszFoundResult, "OK") == 0)
								{
									bResult = true;
								}
							}
						}
					}
				}
			}
		}
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::GetIMEI(LPSTR lpszIMEIBuff)
{
	bool bResult = false;

	try
	{
		CleanUp();
		m_nExpectedResultsCount = 2;
		m_pExpectedResults[0] = "OK";
		m_pExpectedResults[1] = "Error";


		if (Write("AT+CGSN\r", 8) == S_OK)
		{
			if (::WaitForSingleObject(m_hExpectedResultEvent, 5000) == WAIT_OBJECT_0)
			{
				if (strcmp(m_pszFoundResult, "OK") == 0)
				{
					if (lpszIMEIBuff)
					{
						strncpy(lpszIMEIBuff, strstr((LPCSTR) m_pReadBuff, "AT+CGSN") + 10, 15);
						bResult = true;
					}
				}
			}
		}
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::DeleteMessage(LPCSTR lpszMessageIndex)
{
	bool bResult = false;

	try
	{
		CleanUp();
		m_nExpectedResultsCount = 2;
		m_pExpectedResults[0] = "OK";
		m_pExpectedResults[1] = "Error";

		char szATCommand[24];

		sprintf_s(szATCommand, sizeof szATCommand, "AT+CMGD=%s\r", lpszMessageIndex);

		if (Write(szATCommand, strlen(szATCommand)) == S_OK)
			if (::WaitForSingleObject(m_hExpectedResultEvent, 3000) == WAIT_OBJECT_0)
				if (strcmp(m_pszFoundResult, "OK") == 0)
					bResult = true;
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::SendVCard(LPCSTR lpszMobileNumber,
	LPCSTR lpszVCardName, LPCSTR lpszVCardNumber, int nWaitTimeInSec)
{
	bool bResult = false;

	try
	{
		char szMessageBody[70];
		char szPDUBuff[MAX_PATH];
		char szLen[3];
		char szSwapBuff[16];
		CStrConverter strConv;
		char *pHexStr = NULL;

		sprintf(szMessageBody, "BEGIN:VCARD\r\nVERSION:2.1\r\nN:%s\r\nTEL:%s\r\nEND:VCARD\r\n",
			lpszVCardName, lpszVCardNumber);
		
		strcpy_s(szSwapBuff, sizeof szSwapBuff, lpszMobileNumber);

		strcpy_s(szPDUBuff, sizeof szPDUBuff, "005100");
		sprintf(szLen, "%02X", (BYTE) strlen(lpszMobileNumber));
		strcat_s(szPDUBuff, sizeof szPDUBuff, szLen);
		strcat_s(szPDUBuff, sizeof szPDUBuff, "91");		
		strcat_s(szPDUBuff, sizeof szPDUBuff, SwapMobileNumber(szSwapBuff));
		strcat_s(szPDUBuff, sizeof szPDUBuff, "0004AA");
		sprintf(szLen, "%02X", strlen(szMessageBody) + 5);
		strcat_s(szPDUBuff, sizeof szPDUBuff, szLen);
		strcat_s(szPDUBuff, sizeof szPDUBuff, "040402E200");	
		strConv.ByteArrayToHexStr((const unsigned char *) szMessageBody, strlen(szMessageBody), &pHexStr);
		strcat_s(szPDUBuff, sizeof szPDUBuff, pHexStr);
		free(pHexStr);

		strcat_s(szPDUBuff, sizeof szPDUBuff, "\x1A");

		if (m_bMessageFormat)
			SetCMGF(false);

		if (!m_bMessageFormat)
		{
			CleanUp();
			m_nExpectedResultsCount = 2;
			m_pExpectedResults[0] = ">";
			m_pExpectedResults[1] = "Error";

			char szATCommand[16];

			sprintf_s(szATCommand, sizeof szATCommand, "AT+CMGS=%d\r", strlen(szPDUBuff) / 2 - 1);

			if (Write(szATCommand, strlen(szATCommand)) == S_OK)
			{
				if (::WaitForSingleObject(m_hExpectedResultEvent, 5000) == WAIT_OBJECT_0)
				{
					if (strcmp(m_pszFoundResult, ">") == 0)
					{
						CleanUp();
						m_nExpectedResultsCount = 2;
						m_pExpectedResults[0] = "OK";
						m_pExpectedResults[1] = "Error";

						if (Write(szPDUBuff, strlen(szPDUBuff)) == S_OK)
						{
							if (::WaitForSingleObject(m_hExpectedResultEvent, nWaitTimeInSec * 1000)
									== WAIT_OBJECT_0)
							{
								if (strcmp(m_pszFoundResult, "OK") == 0)
								{
									bResult = true;
								}
							}
						}
					}
				}
			}
		}
	}
	catch(...)
	{
	}

	return bResult;
}

int CSMSEngine::PDUToStr(LPBYTE lpPDUBuffer, int nPDULen, LPSTR lpszStrBuffer)
{
	int nStrLen = nPDULen + (nPDULen / 7);

	for (int i = 0; i < nStrLen; i++)
	{
		if (i % 8 == 0)
			lpszStrBuffer[i] = (BYTE) ((BYTE) lpPDUBuffer[i - (i / 8)] << 1) >> 1;
		else
			lpszStrBuffer[i] = (BYTE) ((BYTE) ((BYTE) lpPDUBuffer[i - (i / 8)] << i % 8 + 1) >> 1) |
				(BYTE) ((BYTE) lpPDUBuffer[i - 1 - (i / 8)] >> (8 - i % 8));
	}

	return nStrLen;
}


bool CSMSEngine::GetReadMessageList(ReadMessageList *lpReadMessageList,
	int nMessageType, int nWaitTimeInSec)
{
	bool bResult = false;

	try
	{
		if (m_bMessageFormat)
			SetCMGF(false);

		if (!m_bMessageFormat)
		{
			CleanUp();
			m_nExpectedResultsCount = 2;
			m_pExpectedResults[0] = "OK";
			m_pExpectedResults[1] = "Error";

			char szATCommand[16];

			sprintf_s(szATCommand, sizeof szATCommand, "AT+CMGL=%d\r", nMessageType);

			if (Write(szATCommand, strlen(szATCommand)) == S_OK)
			{
				if (::WaitForSingleObject(m_hExpectedResultEvent, nWaitTimeInSec * 1000) == WAIT_OBJECT_0)
				{
					if (strcmp(m_pszFoundResult, "OK") == 0)
					{
						bResult = true;

						ReadMessage readMessage;
						char szMessageBody[324];
						char szMobileNumber[15];
						char szMessageTime[20];
						char szMessageCenter[15];
						int nMessageSegmentCount;
						int nMessageSegmentNumber;
						int nMessageIndex;
						int nMessageNestNumber;
						int nMessageType;
						bool bHasUDH;
						char szTemp[16];
						int nTemp;
						CStrConverter strConv;
						char *pTemp = NULL;

						char *pBuff = (char *) m_pReadBuff;

						while (true)
						{							
							memset(szMessageBody, 0, sizeof szMessageBody);
							memset(szMobileNumber, 0, sizeof szMobileNumber);
							memset(szMessageTime, 0, sizeof szMessageTime);
							nMessageSegmentCount = -1;
							nMessageSegmentNumber = -1;
							nMessageIndex = -1;
							nMessageType = -1;

							memset(szMessageCenter, 0, sizeof szMessageCenter);
							memset(szTemp, 0, sizeof szTemp);
							nTemp = 0;							

							if (strstr(pBuff, "+CMGL:") == 0)
								break;

							pBuff = strstr(pBuff, "+CMGL:");
							strncpy(szTemp, pBuff + 6, (strstr(pBuff, ",") - pBuff) - 6);
							nMessageNestNumber = atoi(szTemp);
							pBuff = strstr(pBuff, "\r\n") + 2;
							sscanf(pBuff, "%02X", &nTemp);
							strncpy(szMessageCenter, pBuff + 4, 2 * (nTemp - 1));
							SwapBuffer(szMessageCenter, sizeof szMessageCenter);

							if (strlen(szMessageCenter))
								if (szMessageCenter[strlen(szMessageCenter)] == 'F')
									szMessageCenter[strlen(szMessageCenter)] = (char) 0;

							pBuff = pBuff + 2 * nTemp + 2;
							sscanf(pBuff, "%02X", &nTemp);
							bHasUDH = (nTemp & 64) ? true : false;

							if ((nTemp & 3) == 0)
							{
								sscanf(pBuff + 2, "%02X", &nTemp);
								strncpy(szMobileNumber, pBuff + 6, nTemp);
								SwapBuffer(szMobileNumber, sizeof szMobileNumber);

								if (strlen(szMobileNumber))
									if (szMobileNumber[strlen(szMobileNumber)] == 'F')
										szMobileNumber[strlen(szMobileNumber)] = (char) 0;

								pBuff = pBuff + nTemp + 8;
								sscanf(pBuff, "%02X", &nMessageType);

								pBuff += 2;
								strncpy(szMessageTime, pBuff, 14);
								SwapBuffer(szMessageTime, sizeof szMessageTime);
								pBuff += 16;								

								if (!bHasUDH)
								{
									strncpy(szMessageBody, pBuff, strstr(pBuff, "\r\n") - pBuff);									

									if (nMessageType == 0)
									{
										strConv.HexStrToByteArray(szMessageBody, (unsigned char **) &pTemp,
											(UINT *) &nTemp);
										memset(szMessageBody, 0, sizeof szMessageBody);
										PDUToStr((LPBYTE) pTemp, nTemp, szMessageBody);
										free(pTemp);
									}
									else if (nMessageType == 8)
									{
										strConv.HexStrToByteArray(szMessageBody, (unsigned char **) &pTemp,
											(UINT *) &nTemp);		
										SwapBuffer((char *) pTemp, nTemp);
										memset(szMessageBody, 0, sizeof szMessageBody);
										::WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR | WC_SEPCHARS,
												(LPCWSTR) pTemp, nTemp / 2,
												szMessageBody, sizeof szMessageBody, " ", NULL);
										free(pTemp);
									}
								}
								else
								{
									if (strncmp(pBuff, "050003", 6) == 0)
									{
										sscanf(pBuff + 6, "%02X", &nMessageIndex);
										sscanf(pBuff + 8, "%02X", &nMessageSegmentCount);
										sscanf(pBuff + 10, "%02X", &nMessageSegmentNumber);
										pBuff += 12;										

										if (nMessageType == 0)
										{
											sscanf(pBuff, "%02X", &nTemp);
											nTemp >>= 1;
											szMessageBody[0] = (char) nTemp;
											pBuff += 2;
											strncpy(szMessageBody + 1, pBuff, strstr(pBuff, "\r\n") - pBuff);
											strConv.HexStrToByteArray(szMessageBody + 1, (unsigned char **) &pTemp,
												(UINT *) &nTemp);
											memset(szMessageBody + 1, 0, sizeof szMessageBody - 1);
											PDUToStr((LPBYTE) pTemp, nTemp, szMessageBody + 1);
											free(pTemp);
										}
										else if (nMessageType == 8)
										{
											strncpy(szMessageBody, pBuff, strstr(pBuff, "\r\n") - pBuff);
											strConv.HexStrToByteArray(szMessageBody, (unsigned char **) &pTemp,
												(UINT *) &nTemp);		
											SwapBuffer((char *) pTemp, nTemp);
											memset(szMessageBody, 0, sizeof szMessageBody);
											::WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR | WC_SEPCHARS,
												(LPCWSTR) pTemp, nTemp / 2,
												szMessageBody, sizeof szMessageBody, " ", NULL);
											free(pTemp);
										}
									}
								}
							}							

							strcpy(readMessage.szMessageBody, szMessageBody);
							strcpy(readMessage.szMobileNumber, szMobileNumber);
							strcpy(readMessage.szMessageTime, szMessageTime);
							strcpy(readMessage.szMessageCenter, szMessageCenter);
							readMessage.nMessageSegmentCount = nMessageSegmentCount;
							readMessage.nMessageSegmentNumber = nMessageSegmentNumber;
							readMessage.nMessageIndex = nMessageIndex;
							readMessage.nMessageNestNumber = nMessageNestNumber;
							readMessage.nMessageType = nMessageType;

							lpReadMessageList->push_back(readMessage);
						}
					}
				}
			}
		}
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::SendLongPDUMessage(LPCSTR lpszMobileNumber, LPCSTR lpszMessageBody, int nWaitTimeInSec)
{
	bool bResult = false;

	try
	{
		char szMessageBody[154];
		char szPDUBuff[512];
		char szTemp[8];
		char szSwapBuff[16];
		CStrConverter strConv;
		char *pHexStr = NULL;
		int nMessageIndex, nMessageSegCount, nMessageSegNum;

		nMessageIndex = m_nRefNum;
		m_nRefNum++;

		if (strlen(lpszMessageBody) % 153 == 0)
		  nMessageSegCount = strlen(lpszMessageBody) / 153;
		else
		  nMessageSegCount = strlen(lpszMessageBody) / 153 + 1;

		for (nMessageSegNum = 1; nMessageSegNum <= nMessageSegCount; nMessageSegNum++)
		{
			bResult = false;

			if (strlen(lpszMessageBody) > 153)
			{
				strncpy(szMessageBody, lpszMessageBody, 153);			
				szMessageBody[153] = (char) 0;
				lpszMessageBody += 153;
			}
			else
			{
				strcpy(szMessageBody, lpszMessageBody);
			}
			
			strcpy_s(szSwapBuff, sizeof szSwapBuff, lpszMobileNumber);

			strcpy_s(szPDUBuff, sizeof szPDUBuff, "005100");
			sprintf(szTemp, "%02X", (BYTE) strlen(lpszMobileNumber));
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			strcat_s(szPDUBuff, sizeof szPDUBuff, "91");		
			strcat_s(szPDUBuff, sizeof szPDUBuff, SwapMobileNumber(szSwapBuff));
			strcat_s(szPDUBuff, sizeof szPDUBuff, "0000AA");
			sprintf(szTemp, "%02X", strlen(szMessageBody) + 7);
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			strcat_s(szPDUBuff, sizeof szPDUBuff, "050003");
			sprintf(szTemp, "%02X", nMessageIndex);
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			sprintf(szTemp, "%02X", nMessageSegCount);
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			sprintf(szTemp, "%02X", nMessageSegNum);
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			sprintf(szTemp, "%02X", szMessageBody[0] << 1);
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			int nPDULen = StrToPDU(szMessageBody + 1);
			strConv.ByteArrayToHexStr((const unsigned char *) szMessageBody + 1, nPDULen, &pHexStr);
			strcat_s(szPDUBuff, sizeof szPDUBuff, pHexStr);
			free(pHexStr);

			strcat_s(szPDUBuff, sizeof szPDUBuff, "\x1A");

			if (m_bMessageFormat)
				SetCMGF(false);

			if (!m_bMessageFormat)
			{
				CleanUp();
				m_nExpectedResultsCount = 2;
				m_pExpectedResults[0] = ">";
				m_pExpectedResults[1] = "Error";

				char szATCommand[16];

				sprintf_s(szATCommand, sizeof szATCommand, "AT+CMGS=%d\r", strlen(szPDUBuff) / 2 - 1);

				if (Write(szATCommand, strlen(szATCommand)) == S_OK)
				{
					if (::WaitForSingleObject(m_hExpectedResultEvent, 5000) == WAIT_OBJECT_0)
					{
						if (strcmp(m_pszFoundResult, ">") == 0)
						{
							CleanUp();
							m_nExpectedResultsCount = 2;
							m_pExpectedResults[0] = "OK";
							m_pExpectedResults[1] = "Error";

							if (Write(szPDUBuff, strlen(szPDUBuff)) == S_OK)
							{
								if (::WaitForSingleObject(m_hExpectedResultEvent, nWaitTimeInSec * 1000)
										== WAIT_OBJECT_0)
								{
									if (strcmp(m_pszFoundResult, "OK") == 0)
									{
										bResult = true;
									}
								}
							}
						}
					}
				}
			}

			if (!bResult)
				break;
		}
	}
	catch(...)
	{
	}

	return bResult;
}

bool CSMSEngine::SendLongUnicodeMessage(LPCSTR lpszMobileNumber, LPCWSTR lpwcsMessageBody, int nWaitTimeInSec)
{
	bool bResult = false;

	try
	{
		wchar_t wcsMessageBody[68];
		char szPDUBuff[512];
		char szTemp[8];
		char szSwapBuff[16];
		CStrConverter strConv;
		char *pHexStr = NULL;
		int nMessageIndex, nMessageSegCount, nMessageSegNum;

		nMessageIndex = m_nRefNum;
		m_nRefNum++;

		if (wcslen(lpwcsMessageBody) % 67 == 0)
		  nMessageSegCount = wcslen(lpwcsMessageBody) / 67;
		else
		  nMessageSegCount = wcslen(lpwcsMessageBody) / 67 + 1;

		for (nMessageSegNum = 1; nMessageSegNum <= nMessageSegCount; nMessageSegNum++)
		{
			bResult = false;

			if (wcslen(lpwcsMessageBody) > 67)
			{
				wcsncpy(wcsMessageBody, lpwcsMessageBody, 67);			
				wcsMessageBody[67] = (wchar_t) 0;
				lpwcsMessageBody += 67;
			}
			else
			{
				wcscpy(wcsMessageBody, lpwcsMessageBody);
			}		
		
			strcpy_s(szSwapBuff, sizeof szSwapBuff, lpszMobileNumber);

			strcpy_s(szPDUBuff, sizeof szPDUBuff, "005100");
			sprintf(szTemp, "%02X", (BYTE) strlen(lpszMobileNumber));
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			strcat_s(szPDUBuff, sizeof szPDUBuff, "91");		
			strcat_s(szPDUBuff, sizeof szPDUBuff, SwapMobileNumber(szSwapBuff));
			strcat_s(szPDUBuff, sizeof szPDUBuff, "0008AA");
			sprintf(szTemp, "%02X", wcslen(wcsMessageBody) * 2 + 6);
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			strcat_s(szPDUBuff, sizeof szPDUBuff, "050003");
			sprintf(szTemp, "%02X", nMessageIndex);
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			sprintf(szTemp, "%02X", nMessageSegCount);
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			sprintf(szTemp, "%02X", nMessageSegNum);
			strcat_s(szPDUBuff, sizeof szPDUBuff, szTemp);
			sprintf(szTemp, "%02X", wcslen(wcsMessageBody) * 2);
			int nPDULen = wcslen(wcsMessageBody) * 2;
			SwapBuffer((char *) wcsMessageBody, nPDULen);		
			strConv.ByteArrayToHexStr((const unsigned char *) wcsMessageBody, nPDULen, &pHexStr);
			strcat_s(szPDUBuff, sizeof szPDUBuff, pHexStr);
			free(pHexStr);

			strcat_s(szPDUBuff, sizeof szPDUBuff, "\x1A");

			if (m_bMessageFormat)
				SetCMGF(false);

			if (!m_bMessageFormat)
			{
				CleanUp();
				m_nExpectedResultsCount = 2;
				m_pExpectedResults[0] = ">";
				m_pExpectedResults[1] = "Error";

				char szATCommand[16];

				sprintf_s(szATCommand, sizeof szATCommand, "AT+CMGS=%d\r", strlen(szPDUBuff) / 2 - 1);

				if (Write(szATCommand, strlen(szATCommand)) == S_OK)
				{
					if (::WaitForSingleObject(m_hExpectedResultEvent, 5000) == WAIT_OBJECT_0)
					{
						if (strcmp(m_pszFoundResult, ">") == 0)
						{
							CleanUp();
							m_nExpectedResultsCount = 2;
							m_pExpectedResults[0] = "OK";
							m_pExpectedResults[1] = "Error";

							if (Write(szPDUBuff, strlen(szPDUBuff)) == S_OK)
							{
								if (::WaitForSingleObject(m_hExpectedResultEvent, nWaitTimeInSec * 1000)
										== WAIT_OBJECT_0)
								{
									if (strcmp(m_pszFoundResult, "OK") == 0)
									{
										bResult = true;
									}
								}
							}
						}
					}
				}
			}

			if (!bResult)
				break;
		}
	}
	catch(...)
	{
	}

	return bResult;
}