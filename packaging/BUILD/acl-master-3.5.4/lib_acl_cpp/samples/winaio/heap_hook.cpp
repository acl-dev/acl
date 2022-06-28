#include "stdafx.h"
#include <assert.h>
#include "dbghelpapi.h"

#define BLOCKMAPRESERVE     64  // This should strike a balance between memory use and a desire to minimize heap hits.
#define HEAPMAPRESERVE      2   // Usually there won't be more than a few heaps in the process, so this should be small.
#define MAXSYMBOLNAMELENGTH 256 // Maximum symbol name length that we will allow. Longer names will be truncated.
#define MODULESETRESERVE    16  // There are likely to be several modules loaded in the process.

typedef struct patchentry_s 
{
	LPCSTR  exportmodulename; // The name of the module exporting the patched API.
	LPCSTR  importname;       // The name (or ordinal) of the imported API being patched.
	LPSTR   modulepath;       // (Optional) module path. If the module is loaded, then VLD will fill this in at startup.
	LPCVOID replacement;      // Pointer to the function to which the imported API should be patched through to.
} patchentry_t;

static HMODULE __dbghelp;
//////////////////////////////////////////////////////////////////////////

static BOOL linkdebughelplibrary (void)
{
	TCHAR   dbghelppath [MAX_PATH] = { 0 };
	LPCSTR  functionname;
	DWORD   length = MAX_PATH;

	strncpy(dbghelppath, "dbghelp.dll", MAX_PATH);

	// Load the copy of dbghelp.dll installed by Visual Leak Detector.
	__dbghelp = LoadLibrary(dbghelppath);
	if (__dbghelp == NULL)
		abort();

	// Obtain pointers to the exported functions that we will be using.
	functionname = "EnumerateLoadedModules";
	if ((pEnumerateLoadedModules = (EnumerateLoadedModules_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "ImageDirectoryEntryToDataEx";
	if ((pImageDirectoryEntryToDataEx = (ImageDirectoryEntryToDataEx_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
			goto getprocaddressfailure;
	}
	functionname = "StackWalk";
	if ((pStackWalk = (StackWalk_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymCleanup";
	if ((pSymCleanup = (SymCleanup_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymFromAddr";
	if ((pSymFromAddr = (SymFromAddr_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymFunctionTableAccess";
	if ((pSymFunctionTableAccess = (SymFunctionTableAccess_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymGetLineFromAddr";
	if ((pSymGetLineFromAddr = (SymGetLineFromAddr_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymGetModuleBase";
	if ((pSymGetModuleBase = (SymGetModuleBase_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymInitialize";
	if ((pSymInitialize = (SymInitialize_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymLoadModule";
	if ((pSymLoadModule = (SymLoadModule_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymGetModuleInfo";
	if ((pSymGetModuleInfo = (SymGetModuleInfo_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymSetOptions";
	if ((pSymSetOptions = (SymSetOptions_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	functionname = "SymUnloadModule";
	if ((pSymUnloadModule = (SymUnloadModule_t)
		GetProcAddress(__dbghelp, functionname)) == NULL)
	{
		goto getprocaddressfailure;
	}
	return TRUE;

getprocaddressfailure:
	// One of the required exports was not found.
	abort();
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////

typedef LONG (__stdcall *LdrLoadDll_t) (LPSTR, PDWORD, LPSTR, PHANDLE);
typedef LPVOID (__stdcall *RtlAllocateHeap_t) (HANDLE, DWORD, SIZE_T);
typedef BOOL (__stdcall *RtlFreeHeap_t) (HANDLE, DWORD, LPVOID);
typedef LPVOID (__stdcall *RtlReAllocateHeap_t) (HANDLE, DWORD, LPVOID, SIZE_T);

static HANDLE currentprocess; // Pseudo-handle for the current process.
static HANDLE currentthread;  // Pseudo-handle for the current thread.
static HANDLE processheap;    // Handle to the process's heap (COM allocations come from here).

static LdrLoadDll_t        LdrLoadDll;
static RtlAllocateHeap_t   RtlAllocateHeap;
static RtlFreeHeap_t       RtlFreeHeap;
static RtlReAllocateHeap_t RtlReAllocateHeap;

typedef void* (__cdecl *_calloc_dbg_t) (size_t, size_t, int, const char*, int);
typedef void* (__cdecl *_malloc_dbg_t) (size_t, int, const char *, int);
typedef void* (__cdecl *_realloc_dbg_t) (void *, size_t, int, const char *, int);
typedef void* (__cdecl *calloc_t) (size_t, size_t);
typedef void* (__cdecl *malloc_t) (size_t);
typedef void* (__cdecl *realloc_t) (void *, size_t);

static _calloc_dbg_t      pcrtd__calloc_dbg       = NULL;
static _malloc_dbg_t      pcrtd__malloc_dbg       = NULL;
static _realloc_dbg_t     pcrtd__realloc_dbg      = NULL;

static CRITICAL_SECTION   loadLock;
//////////////////////////////////////////////////////////////////////////

#define R2VA(modulebase, rva)  (((PBYTE)modulebase) + rva) // Relative Virtual Address to Virtual Address conversion.

static BOOL patchimport (HMODULE importmodule, LPCSTR exportmodulename,
	LPCSTR exportmodulepath, LPCSTR importname, LPCVOID replacement)
{
	HMODULE                  exportmodule;
	IMAGE_THUNK_DATA        *iate;
	IMAGE_IMPORT_DESCRIPTOR *idte;
	FARPROC                  import;
	DWORD                    protect;
	IMAGE_SECTION_HEADER    *section;
	ULONG                    size;

	assert(exportmodulename != NULL);

	// Locate the importing module's Import Directory Table (IDT) entry for the
	// exporting module. The importing module actually can have several IATs --
	// one for each export module that it imports something from. The IDT entry
	// gives us the offset of the IAT for the module we are interested in.
	idte = (IMAGE_IMPORT_DESCRIPTOR*)pImageDirectoryEntryToDataEx(
		(PVOID)importmodule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT,
		&size, &section);
	if (idte == NULL)
	{
		// This module has no IDT (i.e. it imports nothing).
		return FALSE;
	}
	while (idte->OriginalFirstThunk != 0x0)
	{
		if (_stricmp((PCHAR)R2VA(importmodule, idte->Name),
			exportmodulename) == 0)
		{
			// Found the IDT entry for the exporting module.
			break;
		}
		idte++;
	}
	if (idte->OriginalFirstThunk == 0x0)
	{
		// The importing module does not import anything from the exporting
		// module.
		return FALSE;
	}

	// Get the *real* address of the import. If we find this address in the IAT,
	// then we've found the entry that needs to be patched.
	if (exportmodulepath != NULL)
	{
		// Always try to use the full path if available. There seems to be a bug
		// (or is this a new feature of the "side-by-side" kruft?) where
		// GetModuleHandle sometimes fails if the full path is not supplied for
		// DLLs in the side-by-side cache.
		exportmodule = GetModuleHandle(exportmodulepath);
	}
	else {
		// No full path available. Try using just the module name by itself.
		exportmodule = GetModuleHandle(exportmodulename);
	}
	assert(exportmodule != NULL);
	import = GetProcAddress(exportmodule, importname);
	assert(import != NULL); // Perhaps the named export module does not actually export the named import?

	// Locate the import's IAT entry.
	iate = (IMAGE_THUNK_DATA*)R2VA(importmodule, idte->FirstThunk);
	while (iate->u1.Function != 0x0)
	{
		if (iate->u1.Function == (DWORD_PTR)import)
		{
			// Found the IAT entry. Overwrite the address stored in the IAT
			// entry with the address of the replacement. Note that the IAT
			// entry may be write-protected, so we must first ensure that it is
			// writable.
			VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function),
				PAGE_READWRITE, &protect);
			iate->u1.Function = (DWORD_PTR) replacement;
			VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function),
				protect, &protect);

			// The patch has been installed in the import module.
			return TRUE;
		}
		iate++;
	}

	// The import's IAT entry was not found. The importing module does not
	// import the specified import.
	return FALSE;
}

static BOOL patchmodule (HMODULE importmodule, patchentry_t patchtable [], UINT tablesize)
{
	patchentry_t *entry;
	UINT        index;
	BOOL        patched = FALSE;

	// Loop through the import patch table, individually patching each import
	// listed in the table.
	for (index = 0; index < tablesize; index++)
	{
		entry = &patchtable[index];
		if (patchimport(importmodule, entry->exportmodulename,
			entry->modulepath, entry->importname,
			entry->replacement) == TRUE)
		{
			patched = TRUE;
		}
	}

	return patched;
}

//////////////////////////////////////////////////////////////////////////

static HANDLE _HeapCreate (DWORD options, SIZE_T initsize, SIZE_T maxsize)
{
	HANDLE heap;
	// Create the heap.
	heap = HeapCreate(options, initsize, maxsize);
	return heap;
}

static BOOL _HeapDestroy (HANDLE heap)
{
	return HeapDestroy(heap);
}

static LPVOID _RtlAllocateHeap (HANDLE heap, DWORD flags, SIZE_T size)
{
	LPVOID block;
	// Allocate the block.
	//heap = processheap;
	if (heap == 0)
		return 0;
	block = RtlAllocateHeap(heap, flags, size);
	return block;
}

static BOOL _RtlFreeHeap (HANDLE heap, DWORD flags, LPVOID mem)
{
	BOOL status;
	status = RtlFreeHeap(heap, flags, mem);
	return status;
}

static LPVOID _RtlReAllocateHeap (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T size)
{
	LPVOID newmem;
	// Reallocate the block.
	newmem = RtlReAllocateHeap(heap, flags, mem, size);
	return newmem;
}

static patchentry_t __hook_table[] =
{
    // Win32 heap APIs.
    //"kernel32.dll", "GetProcAddress",     NULL, _GetProcAddress, // Not heap related, but can be used to obtain pointers to heap functions.
	{ "kernel32.dll", "HeapAlloc",          NULL, _RtlAllocateHeap },
	{ "kernel32.dll", "HeapCreate",         NULL, _HeapCreate },
	{ "kernel32.dll", "HeapDestroy",        NULL, _HeapDestroy },
	{ "kernel32.dll", "HeapFree",           NULL, _RtlFreeHeap },
	{ "kernel32.dll", "HeapReAlloc",        NULL, _RtlReAllocateHeap },

};

static BOOL CALLBACK attachtomodule(PSTR  modulepath, ULONG modulebase,
	ULONG modulesize, PVOID userContext)
{
	UINT tablesize = sizeof(__hook_table) / sizeof(patchentry_t);

	// Attach to the module.
	patchmodule((HMODULE)modulebase, __hook_table, tablesize);

	return (TRUE);
}

static LONG _LdrLoadDll (LPSTR searchpath, PDWORD flags,
	LPSTR modulename, PHANDLE modulehandle)
{
	LONG status;

	EnterCriticalSection(&loadLock);
	// Load the DLL.
	status = LdrLoadDll(searchpath, flags, modulename, modulehandle);

	if (0 == status)
	{
		// Attach to any newly loaded modules.
		pEnumerateLoadedModules(currentprocess, attachtomodule, NULL);
	}

	LeaveCriticalSection(&loadLock);
	return status;
}

class CHeapHook
{
public:
	CHeapHook()
	{
		HMODULE kernel32  = GetModuleHandle("kernel32.dll");
		HMODULE ntdll     = GetModuleHandle("ntdll.dll");

		// Initialize global variables.
		currentprocess    = GetCurrentProcess();
		currentthread     = GetCurrentThread();
		LdrLoadDll        = (LdrLoadDll_t)GetProcAddress(ntdll, "LdrLoadDll");
		processheap       = GetProcessHeap();
		RtlAllocateHeap   = (RtlAllocateHeap_t)GetProcAddress(ntdll, "RtlAllocateHeap");
		RtlFreeHeap       = (RtlFreeHeap_t)GetProcAddress(ntdll, "RtlFreeHeap");
		RtlReAllocateHeap = (RtlReAllocateHeap_t)GetProcAddress(ntdll, "RtlReAllocateHeap");

		InitializeCriticalSection(&loadLock);
		linkdebughelplibrary();

		// Patch into kernel32.dll's calls to LdrLoadDll so that VLD can
		// dynamically attach to new modules loaded during runtime.
		//patchimport(kernel32, "ntdll.dll", NULL, "LdrLoadDll", _LdrLoadDll);

		pEnumerateLoadedModules(currentprocess, &attachtomodule, NULL);
	}

	~CHeapHook()
	{

	}
};

// The one and only VisualLeakDetector object instance.
//__declspec(dllexport) CHeapHook vld;

//////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include "lib_acl.h"
#include "detours.h"
#include "heap_hook.h"

LPVOID	WINAPI Real_HeapAlloc(	HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
BOOL	WINAPI Real_HeapFree(	HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
LPVOID	WINAPI Real_HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);

DETOUR_TRAMPOLINE(LPVOID WINAPI
		  Real_HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes),
		  HeapAlloc);

DETOUR_TRAMPOLINE(BOOL WINAPI
		  Real_HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem),
		  HeapFree);

DETOUR_TRAMPOLINE(LPVOID WINAPI
		  Real_HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes),
		  HeapReAlloc);

BOOL  WINAPI DetourFunctionWithTrampoline(PBYTE pbTrampoline,
					  PBYTE pbDetour);
LPVOID WINAPI HookHeapAlloc( HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes );
BOOL WINAPI HookHeapFree( HANDLE hHeap, DWORD dwFlags, LPVOID lpMem );
LPVOID WINAPI HookHeapReAlloc( HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes );

LPVOID WINAPI HookHeapAlloc(
			    HANDLE hHeap,   // handle to private heap block
			    DWORD dwFlags,  // heap allocation control
			    SIZE_T dwBytes  // number of bytes to allocate
			    )
{
	const char* myname = "HookHeapAlloc\r\n";
	LPVOID p = Real_HeapAlloc(hHeap, dwFlags, dwBytes);

	//TraceMemoryAllocation( p, dwBytes, fnHeapAlloc );

	//write(1, myname, strlen(myname));
	return p;
}


BOOL WINAPI HookHeapFree(
			 HANDLE hHeap,  // handle to heap
			 DWORD dwFlags, // heap free options
			 LPVOID lpMem   // pointer to memory
			 )
{
	BOOL res = Real_HeapFree(hHeap, dwFlags, lpMem);
	const char* myname = "HookHeapFree\r\n";
	//TraceMemoryRelease( lpMem );

	//write(1, myname, strlen(myname));
	return res;
}


LPVOID WINAPI HookHeapReAlloc(
			      HANDLE hHeap,   // handle to heap block
			      DWORD dwFlags,  // heap reallocation options
			      LPVOID lpMem,   // pointer to memory to reallocate
			      SIZE_T dwBytes  // number of bytes to reallocate
			      )
{
	const char* myname = "HookHeapReAlloc\r\n";
	LPVOID p = Real_HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);

	// the previous block was freed
	//if( !!lpMem )
	//	TraceMemoryRelease( lpMem );

	// new block allocated (may be in same address)
	//TraceMemoryAllocation( p, dwBytes, fnHeapAlloc );

	//write(1, myname, strlen(myname));
	return p;
}

static void* malloc_hook(const char* fname, int lineno, size_t len)
{
	printf(">>>%s\n", __FUNCTION__);

	void* ptr = Real_HeapAlloc(processheap, 0, len);
	if (ptr == NULL)
	{
		char buf[1024];
		int  n = WSAGetLastError();
		printf("error: %d, %s\n", n, acl_strerror(n, buf, sizeof(buf)));
	}

	return ptr;
}

static void *calloc_hook(const char* fname, int lineno, size_t n, size_t len)
{
	printf(">>>%s\n", __FUNCTION__);

	void* ptr = Real_HeapAlloc(processheap, 0, n * len);
	if (ptr == NULL)
		return NULL;
	memset(ptr, 0, n * 140);
	return ptr;
}

static void *realloc_hook(const char* fname, int lineno, void* ptr, size_t len)
{
	printf(">>>%s\n", __FUNCTION__);

	void* newptr = Real_HeapReAlloc(processheap, 0, ptr, len);
	return newptr;
}

static char *strdup_hook(const char* fname, int lineno, const char* ptr)
{
	printf(">>>%s\n", __FUNCTION__);

	size_t len = strlen(ptr);
	char* s = (char*) malloc_hook(fname, lineno, len + 1);

	memcpy(s, (const char*) ptr, len);
	s[len] = 0;
	return s;
}

static char *strndup_hook(const char* fname, int lineno, const char* ptr, size_t n)
{
	printf(">>>%s\n", __FUNCTION__);

	size_t len = strlen(ptr);
	if (len > n)
		len = n;
	char* s = (char*) malloc_hook(fname, lineno, len + 1);
	memcpy(s, (const char*) ptr, len);
	s[len] = 0;
	return s;
}

static void *memdup_hook(const char* fname, int lineno, const void* ptr, size_t n)
{
	printf(">>>%s\n", __FUNCTION__);

	char* p = (char*) malloc_hook(fname, lineno, n);
	memcpy(p, (const char*) ptr, n);
	return p;
}

static void  free_hook(const char* fname, int lineno, void* ptr)
{
	printf(">>>%s\n", __FUNCTION__);

	Real_HeapFree(processheap, 0, ptr);
}

CHeapHook2::CHeapHook2()
{
	processheap       = GetProcessHeap();
	DetourFunctionWithTrampoline((PBYTE)Real_HeapAlloc, (PBYTE)HookHeapAlloc);
	DetourFunctionWithTrampoline((PBYTE)Real_HeapFree, (PBYTE)HookHeapFree);
	DetourFunctionWithTrampoline((PBYTE)Real_HeapReAlloc, (PBYTE)HookHeapReAlloc);

	FILE *fp;
	AllocConsole();
	fp = freopen("CONOUT$","w+t",stdout);

	acl_mem_hook(malloc_hook, calloc_hook, realloc_hook,
		strdup_hook, strndup_hook, memdup_hook, free_hook);
	char* p = acl_mystrdup("hello world!\n");
	printf(">>%s\n", p);
	acl_myfree(p);

	acl_msg_open("heap.log", "test");
}

CHeapHook2::~CHeapHook2()
{
	DetourRemove((PBYTE)Real_HeapAlloc, (PBYTE)HookHeapAlloc);
	DetourRemove((PBYTE)Real_HeapFree, (PBYTE)HookHeapFree);
	DetourRemove((PBYTE)Real_HeapReAlloc, (PBYTE)HookHeapReAlloc);
}

//__declspec(dllexport) CHeapHook2 vld2;
