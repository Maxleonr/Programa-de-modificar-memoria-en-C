#ifndef PACKET_H
#define PACKET_H
#include <stdbool.h>

#include <windows.h>

#ifdef __cplusplus
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT __declspec(dllexport)
#endif

EXPORT bool write_memory(DWORD pid, uintptr_t memory, float new_value);
EXPORT uintptr_t read_memory(DWORD pid, uintptr_t memory);

#endif
