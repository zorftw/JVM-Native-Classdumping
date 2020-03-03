#pragma once

#include <Windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

#include <jni.h>

#include <ImageHlp.h>

#include "minhook/MinHook.h"

#pragma warning(disable:4996)

using java_defineclass_fn = jclass(__stdcall*)(JNIEnv * env,
	jobject loader,
	jstring name,
	jbyteArray data,
	jint offset,
	jint length,
	jobject pd,
	jstring source);

using qword = unsigned long long;	

bool write_to_protected_address(DWORD* dest, DWORD value)
{
	DWORD old_prot;
	VirtualProtect(dest, 4, PAGE_READWRITE, &old_prot);

	*dest = value;

	VirtualProtect(dest, 4, old_prot, nullptr);
	return true;
}

DWORD hook_eat(const char* str_module, DWORD fn_target, DWORD fn_stub)
{
	auto mod = GetModuleHandleA(str_module);
	auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(mod);

	if (!dos_header) {
		printf("bro are you 0 iq?\n");
		return 0;
	}

	/* Somehow doing it like this, the VirtualAddress is 0, must've done something wrong here
	auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(mod + dos_header->e_lfanew);

	
	if (nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0) {
		printf("cmon bruh?\n");
		return 0;
	}

	
	auto export_directory_len = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	auto export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(mod + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	*/

	unsigned long export_directory_len = 0;
	auto export_directory = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData((PVOID)mod, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &export_directory_len);

	/* check if it exists (again) */
	if (!export_directory)
	{
		printf("Unable to allocate Export Table\n");
		return 0;
	}

	/* get necessary stuff */
	auto mod_off = std::ptrdiff_t(mod);
	
	DWORD* address_table = reinterpret_cast<DWORD*>(mod_off + export_directory->AddressOfFunctions);
	DWORD* names = reinterpret_cast<DWORD*>(mod_off + export_directory->AddressOfNames);
	WORD* ordinals = reinterpret_cast<WORD*>(mod_off + export_directory->AddressOfNameOrdinals);

	DWORD* relative = 0;
	DWORD* o_relative = nullptr;
	DWORD old_address = 0;
	for (int j = 0; j < export_directory->NumberOfFunctions; ++j)
	{
		auto name = reinterpret_cast<char*>(mod_off + names[j]);	

		auto o_relative = &address_table[ordinals[j]];
		auto exported = mod_off + *o_relative;

		old_address = exported;

		if (exported == fn_target)
		{
			printf("%s found, offset: 0x%x ", name, o_relative);
			relative = o_relative;
			break;
		}
	}


	if (relative != 0 && fn_stub != 0)
	{
		DWORD off = fn_stub - mod_off;
		printf("new offset: 0x%x\n", off);

		write_to_protected_address(relative, off);
	}
	
	return old_address;
}