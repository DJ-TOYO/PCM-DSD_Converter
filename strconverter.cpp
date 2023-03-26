#include "stdafx.h"
#include "strconverter.h"

TCHAR* convert_from_utf8(const char* utf8_str)
{//utf8 => tchar*
	int utf16_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, 0, 0);
	WCHAR *utf16_str = (WCHAR*)malloc((utf16_len+1)*sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, utf16_str, utf16_len);
    utf16_str[utf16_len] = 0;
#ifdef _UNICODE
    return utf16_str;
#else
    int ansi_len = WideCharToMultiByte(CP_ACP, 0, utf16_str, -1,0,0,NULL,NULL);
    char *ansi_str = (char*)malloc(ansi_len + 1);
    WideCharToMultiByte(CP_ACP, 0, utf16_str, -1, ansi_str, ansi_len, NULL, NULL);
    free(utf16_str);
	return ansi_str;
#endif
}

char* convert_to_utf8(const TCHAR* t_str)
{//tchar* =>utf8
#ifndef _UNICODE
	int utf16_len = MultiByteToWideChar(CP_ACP, 0, t_str, -1, 0, 0);
	WCHAR *utf16_str = (WCHAR*)malloc(utf16_len*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, t_str, -1, utf16_str, utf16_len);
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, 0, 0, NULL, NULL);
    char *utf8_str = (char*)malloc(utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, utf8_str, utf8_len, NULL, NULL);
    free(utf16_str);
    return utf8_str;
#else
    const WCHAR *utf16_str = t_str;
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, 0, 0, NULL, NULL);
	char *utf8_str = new char[utf8_len];	// デバッグ時にメモリリークが分かるようにnewに変更。Unicode版しかexe作らないから上のロジックは放置する。
	memset(utf8_str, 0xFF, utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, utf8_str, utf8_len, NULL, NULL);
    return utf8_str;
#endif
}
