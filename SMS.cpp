/////////////////////////////////////////////////////////////////////////////
// SMS.cpp
//
// Copyright (C) 2013 Veria Kalantari
// veria.kalantary@gmail.com
// https://github.com/veriak
//

#include "SMS.h"

using namespace SMS;


/////////////////////////////////////////////////////////////////////////////
// CSMS

CSMS::CSMS(void)
{
	Init();
}

CSMS::CSMS(LPCTSTR lptcsPortName, DWORD dwBaudRate)
{
	Init(lptcsPortName, dwBaudRate);
}

CSMS::~CSMS(void)
{
	Clear();
}

void CSMS::Init(void)
{
	m_hInstance = NULL;
	m_hWnd = NULL;	
	m_ptcsPortName = NULL;	
	m_ptcsErrorMsgBuff = NULL;
	m_dwLastError = 0;
	m_pReadBuff = NULL;
	m_nReadBuffSize = SIZEOFBUFF;
	m_nTotalReadSize = 0;	

	SetLength(&m_pReadBuff, sizeof(BYTE), m_nReadBuffSize);
}

void CSMS::Init(LPCTSTR lptcsPortName, DWORD dwBaudRate)
{
	Init();
	__super::Init((LRESULT CALLBACK) CSMS::OnCommReadEvent, lptcsPortName, dwBaudRate);
}

void CSMS::Clear(void)
{	
	ClearBuffer((LPVOID &) m_ptcsPortName);
	ClearBuffer((LPVOID &) m_ptcsErrorMsgBuff);
	CleanUp();
}

void CSMS::ClearBuffer(LPVOID &lpBuff)
{
	if (lpBuff)
	{
		free(lpBuff);
		lpBuff = NULL;
	}
}

void CSMS::CleanUp(void)
{
}

DWORD CSMS::_GetLastError(void)
{
	return m_dwLastError;
}

void CSMS::SetWnd(HWND hWnd)
{
	m_hWnd = hWnd;
}

LRESULT CALLBACK CSMS::OnCommReadEvent(CSMS *_this, DWORD dwBytesRead)
{
	LRESULT lResult = 0;

	return lResult;
}