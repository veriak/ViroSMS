# ViroSMS
Use this library for work with Comm port on windows and using AT Commands to Sending and Receiving one or multipart SMS(PDU, Unicode)

CSMSEngine *pSMSEngine = new CSMSEngine(_T("com1"), 9600);

	if (pSMSEngine->CheckGSMModemActivity()) {
		char szIMEI[MAX_PATH];

		if (pSMSEngine->GetIMEI(szIMEI)) {
			if (pSMSEngine->SendUnicodeMessage("989122222222", _T("Test SMS"))) {
				ReadMessageList ml;

				if (pSMSEngine->GetReadMessageList(&ml)) {
				}
			}
		}
	}

	delete pSMSEngine;	
}
