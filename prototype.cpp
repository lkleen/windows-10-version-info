// lkleen 10/2018
#include <windows.h>
#include <strsafe.h>

class VersionLibrary
{
public:
	VersionLibrary ()
	{
		//Load the dll and keep the handle to it
		dllHandle = LoadLibrary ("version.dll");
	}

	~VersionLibrary ()
	{
		// If the handle is valid, try to get the function address. 
		// (joda style fresh from msdn library) -> https://msdn.microsoft.com/en-us/library/ms810279.aspx
		if (NULL != dllHandle)
		{

			//Free the library:
			freeResult = FreeLibrary (dllHandle);
		}
	}

	DWORD GetFileVersionInfoSizeA (
		LPCSTR  lptstrFilename,
		LPDWORD lpdwHandle
	)
	{
		typedef DWORD (CALLBACK* GetFileVersionInfoSizeA)(LPCSTR, LPDWORD);

		GetFileVersionInfoSizeA GetFileVersionInfoSizeAPtr =
			(GetFileVersionInfoSizeA) GetProcAddress (dllHandle, "GetFileVersionInfoSizeA");

		return GetFileVersionInfoSizeAPtr (lptstrFilename, lpdwHandle);
	}

	BOOL GetFileVersionInfoA (
		LPCSTR lptstrFilename,
		DWORD  dwHandle,
		DWORD  dwLen,
		LPVOID lpData
	)
	{
		typedef BOOL (CALLBACK* GetFileVersionInfoA)(
			LPCSTR,
			DWORD,
			DWORD,
			LPVOID
		);

		GetFileVersionInfoA GetFileVersionInfoAPtr = 
			(GetFileVersionInfoA) GetProcAddress (dllHandle, "GetFileVersionInfoA");

		return GetFileVersionInfoAPtr (lptstrFilename, dwHandle, dwLen, lpData);
	}

	BOOL VerQueryValueA (
		LPCVOID pBlock,
		LPCSTR  lpSubBlock,
		LPVOID  *lplpBuffer,
		PUINT   puLen
	)
	{
		typedef BOOL (CALLBACK* VerQueryValueA)(
			LPCVOID,
			LPCSTR,
			LPVOID*,
			PUINT
			);

		VerQueryValueA VerQueryValueAPtr =
			(VerQueryValueA) GetProcAddress (dllHandle, "VerQueryValueA");

		return VerQueryValueAPtr (pBlock, lpSubBlock, lplpBuffer, puLen);
	}

private:
	BOOL freeResult, runTimeLinkSuccess = FALSE;
	HINSTANCE dllHandle = NULL;
};

std::string readOSVersion () const
{
	VersionLibrary lib;

	// https://docs.microsoft.com/en-us/windows/desktop/sysinfo/getting-the-system-version
	// https://docs.microsoft.com/en-us/windows/desktop/api/winver/nf-winver-getfileversioninfoa

	LPCSTR  lptstrFilename = "kernel32.dll";
	LPDWORD lpdwHandle = NULL;
	DWORD dwLen = 0;

	dwLen = lib.GetFileVersionInfoSizeA (lptstrFilename, lpdwHandle);
	
	LPVOID lpData = malloc (dwLen);
	
	lib.GetFileVersionInfoA (lptstrFilename, NULL, dwLen, lpData);
	
	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	// Read the list of languages and code pages.

	unsigned int cbTranslate = 0;

	lib.VerQueryValueA (lpData,
		TEXT ("\\VarFileInfo\\Translation"),
		(LPVOID*)&lpTranslate,
		&cbTranslate);

	HRESULT hr;

	char SubBlock[2048];

	for (int i = 0; i < (cbTranslate / sizeof (struct LANGANDCODEPAGE)); i++)
	{
		hr = StringCchPrintf (SubBlock, 50,
			TEXT ("\\StringFileInfo\\%04x%04x\\ProductVersion"),
			lpTranslate[i].wLanguage,
			lpTranslate[i].wCodePage);
		if (FAILED (hr))
		{
			continue;
		}

		void* buffer;
		unsigned int dwBytes;

		// Retrieve product version for language and code page "i". 
		lib.VerQueryValueA (lpData,
			SubBlock,
			(LPVOID*) &buffer,
			&dwBytes);

		char* version = (char*)buffer;
		return version;
	}

	free (lpData);

	return "failed to retrieve version number";
}