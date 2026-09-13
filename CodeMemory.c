#include "packet.h"

typedef LONG NTSTATUS;
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#define NTAPI __stdcall

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG           Length;
    HANDLE          RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG           Attributes;
    PVOID           SecurityDescriptor;
    PVOID           SecurityQualityOfService;
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, * PCLIENT_ID;

typedef NTSTATUS(NTAPI* fnNtOpenProcess)(
    PHANDLE            ProcessHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PCLIENT_ID         ClientId
);

typedef NTSTATUS(NTAPI* NtClose_t)(
    HANDLE Handle
);

typedef NTSTATUS(NTAPI* pNtWriteProcessMemory)(
    HANDLE Handle, PVOID Address, PVOID Buffer, SIZE_T NumberOfWriteToBytes, PSIZE_T NumbersOfWritten
);

typedef NTSTATUS(NTAPI* pNtReadProcessMemory)(
    HANDLE Handle, PVOID Address, PVOID Buffer, SIZE_T BufferSize, PSIZE_T NumbersOfRead
);

#define InitializeObjectAttributes(p, n, a, r, s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES);          \
    (p)->RootDirectory = r;                           \
    (p)->Attributes = a;                              \
    (p)->ObjectName = n;                              \
    (p)->SecurityDescriptor = s;                      \
    (p)->SecurityQualityOfService = NULL;             \
}


EXPORT uintptr_t read_memory(DWORD pid, uintptr_t memory_to_read) {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return 0;

    fnNtOpenProcess NtOpenProcess = (fnNtOpenProcess)GetProcAddress(hNtdll, "NtOpenProcess");
    NtClose_t NtClose = (NtClose_t)GetProcAddress(hNtdll, "NtClose");
    pNtReadProcessMemory NtReadMemory = (pNtReadProcessMemory)GetProcAddress(hNtdll, "NtReadVirtualMemory");

    if (!NtOpenProcess || !NtReadMemory || !NtClose) return 0;

    OBJECT_ATTRIBUTES objAttr;
    CLIENT_ID clientId;
    InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);

    clientId.UniqueProcess = (HANDLE)(ULONG_PTR)pid;
    clientId.UniqueThread = 0;

    HANDLE hProcess = NULL;
    NTSTATUS status = NtOpenProcess(&hProcess, PROCESS_VM_READ, &objAttr, &clientId);

    if (!NT_SUCCESS(status)) return 0;

    uintptr_t read_value = 0;
    SIZE_T bytesRead = 0;
    
    NTSTATUS readStatus = NtReadMemory(hProcess, (PVOID)memory_to_read, &read_value, sizeof(read_value), &bytesRead);


    NtClose(hProcess);

    return NT_SUCCESS(readStatus) ? read_value : 0;
}

EXPORT bool write_memory(DWORD pid, uintptr_t memory_to_write, float new_value) {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return false;
    DWORD oldprotect;
    fnNtOpenProcess NtOpenProcess = (fnNtOpenProcess)GetProcAddress(hNtdll, "NtOpenProcess");
    NtClose_t NtClose = (NtClose_t)GetProcAddress(hNtdll, "NtClose");
    pNtWriteProcessMemory NtWriteMemory = (pNtWriteProcessMemory)GetProcAddress(hNtdll, "NtWriteVirtualMemory");

    if (!NtOpenProcess || !NtWriteMemory || !NtClose) return false;

    OBJECT_ATTRIBUTES objAttr;
    CLIENT_ID clientId;
    InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);

    clientId.UniqueProcess = (HANDLE)(ULONG_PTR)pid;
    clientId.UniqueThread = 0;

    HANDLE hProcess = NULL;
    NTSTATUS status = NtOpenProcess(&hProcess, PROCESS_VM_WRITE | PROCESS_VM_OPERATION, &objAttr, &clientId);

    if (!NT_SUCCESS(status)) return false;

    SIZE_T bytesWrite = 0;
    NTSTATUS writeStatus = NtWriteMemory(hProcess, (PVOID)memory_to_write, &new_value, sizeof(new_value), &bytesWrite);


    NtClose(hProcess);

    return NT_SUCCESS(writeStatus);
}
