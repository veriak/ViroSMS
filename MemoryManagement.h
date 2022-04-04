#include <windows.h>

void SetLength(LPVOID* lppBuff, UINT nElementSize, UINT nCount);

BOOL StrToWideStr(char* lpStr, wchar_t** lppWideStr);
BOOL WideStrToStr(wchar_t* lpWideStr, char** lppStr);
void FreeWideStr(wchar_t** lppWideStr);