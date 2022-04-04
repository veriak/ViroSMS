/////////////////////////////////////////////////////////////////////////////
// SerialCommPort.cpp
//
// Copyright (C) 2013 Veria Kalantari
// veria.kalantary@gmail.com
// https://github.com/veriak
//

#include "stdafx.h"
#include "SerialCommPort.h"

using namespace SerialCommPort;


BOOL CSerialCommPort::CloseAndCleanHandle(HANDLE &hHandle)
{
	BOOL bResult = ::CloseHandle(hHandle);

	if (!bResult)
	{
		::MessageBox(NULL, _T("Handle is not closed"), _T("Handle Error"), MB_ICONSTOP);
		bResult = E_FAIL;
	}

	hHandle = INVALID_HANDLE_VALUE;

	return bResult;
}

CSerialCommPort::CSerialCommPort()
{	
	m_pReadBuff = NULL;
	m_nReadBuffSize = 0;
	m_nTotalReadSize = 0;
	m_hWnd = NULL;
	m_uReadMsg = 0;
	m_pCallBackFunc = NULL;
	m_hCommPort = INVALID_HANDLE_VALUE;
	m_hThreadTerm = INVALID_HANDLE_VALUE;
	m_hThread = INVALID_HANDLE_VALUE;

	InitLock();
}

CSerialCommPort::~CSerialCommPort()
{
	DelLock();
}

void CSerialCommPort::LockThis()
{
	::EnterCriticalSection(&m_csLock);
}

void CSerialCommPort::UnLockThis()
{
	::LeaveCriticalSection(&m_csLock);
}

void CSerialCommPort::InitLock()
{
	::InitializeCriticalSection(&m_csLock);
}

void CSerialCommPort::DelLock()
{
	::DeleteCriticalSection(&m_csLock);
}

void CSerialCommPort::SetDataReadEvent()
{
	::SetEvent(m_hDataRx);
}

HRESULT CSerialCommPort::Init(LPCTSTR lptcsPortName, DWORD dwBaudRate, BYTE byParity,
	BYTE byStopBits, BYTE byByteSize)
{
	HRESULT hr = S_OK;

	try
	{
		m_hCommPort = ::CreateFile(lptcsPortName, GENERIC_READ | GENERIC_WRITE, 0, 0,
			OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
		
		DWORD dwError = ::GetLastError();

		if (dwError == ERROR_FILE_NOT_FOUND)
		{
			::MessageBox(NULL, _T("The COM Port not found"), lptcsPortName, MB_ICONSTOP);
			return E_FAIL;
		}
		else if (dwError == ERROR_ACCESS_DENIED)
		{
			::MessageBox(NULL, _T("The COM Port access denied"), lptcsPortName, MB_ICONSTOP);
			return E_FAIL;
		}

		if (m_hCommPort == INVALID_HANDLE_VALUE)
		{
			::MessageBox(NULL, _T("Failed to open the COM Port"), lptcsPortName, MB_ICONSTOP);
			return E_FAIL;
		}
		
		if (!::SetCommMask(m_hCommPort, EV_RXCHAR))
		{
			::MessageBox(NULL, _T("Failed to set COM Mask"), lptcsPortName, MB_ICONSTOP);
			return E_FAIL;
		}
		
		DCB dcb = {0};
		dcb.DCBlength = sizeof(DCB);

		if (!::GetCommState(m_hCommPort, &dcb))
		{
			::MessageBox(NULL, _T("Failed to get COM State"), lptcsPortName, MB_ICONSTOP);
			return E_FAIL;
		}
		
		dcb.BaudRate = dwBaudRate;
		dcb.ByteSize = byByteSize;
		dcb.Parity = byParity;

		if (byStopBits == 1)
			dcb.StopBits = ONESTOPBIT;
		else if (byStopBits == 2) 
			dcb.StopBits = TWOSTOPBITS;
		else 
			dcb.StopBits = ONE5STOPBITS;

		dcb.fDsrSensitivity = 0;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		dcb.fOutxDsrFlow = 0;
		dcb.XonChar = 0x17;
		dcb.XoffChar = 0x19;
		dcb.ErrorChar = 0x0;
		dcb.EofChar = 0x0;
		dcb.EvtChar = 0x0;
		dcb.XonLim = 1024;
		dcb.XoffLim = 1024;

		if (!::SetCommState(m_hCommPort, &dcb))
		{
			::MessageBox(NULL, _T("Failed to set COM State"), lptcsPortName, MB_ICONSTOP);
			return E_FAIL;
		}
		
		COMMTIMEOUTS timeouts;

		timeouts.ReadIntervalTimeout = MAXDWORD; 
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		timeouts.WriteTotalTimeoutConstant = 0;

		if (!::SetCommTimeouts(m_hCommPort, &timeouts))
		{
			::MessageBox(NULL, _T("Failed to set COM Timeouts"), lptcsPortName, MB_ICONSTOP);
			return E_FAIL;
		}
	}
	catch(...)
	{
		hr = E_FAIL;
	}

	return hr;
}	

HRESULT CSerialCommPort::Init(HWND hWnd, UINT uReadMsg, LPCTSTR lptcsPortName, DWORD dwBaudRate,
	BYTE byParity, BYTE byStopBits, BYTE byByteSize)
{
	HRESULT hr = Init(lptcsPortName, dwBaudRate, byParity, byStopBits, byByteSize);

	SetCommRead(hWnd, uReadMsg);
	StartCommRead();

	return hr;
}

HRESULT CSerialCommPort::Init(LRESULT CALLBACK lpCallBackFunc, LPCTSTR lptcsPortName, DWORD dwBaudRate,
	BYTE byParity, BYTE byStopBits, BYTE byByteSize)
{
	HRESULT hr = Init(lptcsPortName, dwBaudRate, byParity, byStopBits, byByteSize);

	SetCommRead(lpCallBackFunc);
	StartCommRead();

	return hr;
}

HRESULT CSerialCommPort::UnInit()
{
	HRESULT hr = S_OK;

	try
	{
		if (m_hCommPort != INVALID_HANDLE_VALUE)
		{
			CloseAndCleanHandle(m_hCommPort);

			if (m_hThread != INVALID_HANDLE_VALUE)
			{
				::SignalObjectAndWait(m_hThreadTerm, m_hThread, INFINITE, FALSE);
				CloseAndCleanHandle(m_hThreadTerm);
				CloseAndCleanHandle(m_hThread);
			}
		}

	}
	catch(...)
	{
		hr = E_FAIL;
	}

	return hr;
}

void CSerialCommPort::StartCommRead()
{
	m_hThreadTerm = ::CreateEvent(0, TRUE, FALSE, 0);
	m_hThread =	(HANDLE) _beginthreadex(0, 0, CSerialCommPort::ThreadFn, (void *) this, 0, 0);
}

void CSerialCommPort::SetCommRead(HWND hWnd, UINT uReadMsg)
{
	m_hWnd = hWnd;
	m_uReadMsg = uReadMsg; 
}

void CSerialCommPort::SetCommRead(LRESULT CALLBACK lpCallBackFunc)
{
	m_pCallBackFunc = lpCallBackFunc;
}

HRESULT CSerialCommPort::Write(const void *lpBuffer, DWORD dwBuffSize)
{
	OVERLAPPED ov;
	DWORD dwBytesWritten = 0;

	memset(&ov, 0, sizeof(ov));	
	ov.hEvent = ::CreateEvent(0, TRUE, FALSE, 0);

	::WriteFile(m_hCommPort, lpBuffer, dwBuffSize, &dwBytesWritten, &ov);

	if (::WaitForSingleObject(ov.hEvent, 1000) == WAIT_OBJECT_0)
	{
		::GetOverlappedResult(m_hCommPort, &ov, &dwBytesWritten, 0);
		::CloseHandle(ov.hEvent);

		return S_OK;
	}
	else
	{
		::CloseHandle(ov.hEvent);
		return E_FAIL;
	}
}

unsigned __stdcall CSerialCommPort::ThreadFn(void *pvParam)
{
	CSerialCommPort *pThis = (CSerialCommPort *) pvParam;
	bool bContinue = true;
	DWORD dwEventMask = 0;
	COMSTAT comStat;
    DWORD dwErrors;
	HANDLE hReadEvent;	
	OVERLAPPED ovCommRead;
	OVERLAPPED ov;
	HANDLE arHandles[2];
	DWORD dwWait;

	memset(&ovCommRead, 0, sizeof(ovCommRead));
	hReadEvent = ::CreateEvent(0, TRUE, FALSE, 0);
	ovCommRead.hEvent = hReadEvent;	

	memset(&ov, 0, sizeof(ov));
	ov.hEvent = ::CreateEvent(0, TRUE, FALSE, 0);	
	arHandles[0] = pThis->m_hThreadTerm;	

	while (bContinue)
	{
		BOOL WaitEventResult = ::WaitCommEvent(pThis->m_hCommPort, &dwEventMask, &ov) ;
		
		if (::GetLastError() == ERROR_IO_PENDING)
		{
			if (::WaitForSingleObject(ov.hEvent, INFINITE) == WAIT_OBJECT_0)				
				WaitEventResult = 1;			
			else
				WaitEventResult = 0;
		}

		if (WaitEventResult)
		{
			//arHandles[1] = ov.hEvent;
			//dwWait = ::WaitForMultipleObjects(2, arHandles, FALSE, INFINITE);

			//switch (dwWait)
			//{
			//	case WAIT_OBJECT_0:
			//		SetLength(&pThis->m_pReadBuff, 0, 0);
			//		_endthreadex(1);
			//	break;

			//	case WAIT_OBJECT_0 + 1:
					if (::ClearCommError(pThis->m_hCommPort, &dwErrors, &comStat) > 0)
					{
						if (comStat.cbInQue)
						{	
							DWORD dwBytesRead = 0;

							pThis->LockThis();

							if (pThis->m_nTotalReadSize + comStat.cbInQue > pThis->m_nReadBuffSize)
							{
								pThis->m_nReadBuffSize += comStat.cbInQue;
								SetLength(&pThis->m_pReadBuff, sizeof(BYTE), pThis->m_nReadBuffSize);
							}

							try 
							{																											
								::ReadFile(pThis->m_hCommPort,
									(BYTE *) pThis->m_pReadBuff + pThis->m_nTotalReadSize,
									comStat.cbInQue, &dwBytesRead, &ovCommRead);

								if (::WaitForSingleObject(hReadEvent, 1000) != WAIT_OBJECT_0)
									dwBytesRead = 0;
								else
								{
									::GetOverlappedResult(pThis->m_hCommPort, &ovCommRead,
										&dwBytesRead, 0);
									pThis->m_nTotalReadSize += dwBytesRead;
									::ResetEvent(hReadEvent);
								}
							}
							catch(...)
							{
							}

							pThis->UnLockThis();

							if (pThis->m_hWnd)
							{
								::SendMessage(pThis->m_hWnd, pThis->m_uReadMsg, 0, dwBytesRead);
							}
							else if (pThis->m_pCallBackFunc)
							{
								DWORD dwThis = (DWORD) pThis;

								__asm
								{
									push dwBytesRead
									push dwThis
									mov eax, pThis
									call [eax].m_pCallBackFunc									
								}
							}
						}						
					}

					::ResetEvent(ov.hEvent);
			//	break;
			//}
		}
	}

	::CloseHandle(hReadEvent);
	::CloseHandle(ov.hEvent);	

	return 0;
}

BOOL CSerialCommPort::ReadExact(void *lpReadBuff, UINT nBuffSize, UINT nReadSize, UINT *lpNeededSize)
{
	if (nReadSize > nBuffSize)
	{
		*lpNeededSize = nReadSize;

		return false;
	}

	LockThis();

	memcpy(lpReadBuff, m_pReadBuff, nReadSize);
	m_nTotalReadSize -= nReadSize;
	memcpy(m_pReadBuff, (BYTE *) m_pReadBuff + nReadSize, m_nTotalReadSize);
	*((BYTE *) m_pReadBuff + m_nTotalReadSize) = 0;

	if ((m_nTotalReadSize < SIZEOFCOMMBUFF) && (m_nReadBuffSize > SIZEOFCOMMBUFF))
	{
		m_nReadBuffSize = SIZEOFCOMMBUFF;
		SetLength(&m_pReadBuff, sizeof(BYTE), m_nReadBuffSize);
	}

	UnLockThis();

	return true;
}

BOOL CSerialCommPort::ReadAll(void *lpReadBuff, UINT nBuffSize, UINT *lpNeededSize)
{
	return (ReadExact(lpReadBuff, nBuffSize, m_nTotalReadSize, lpNeededSize));
}