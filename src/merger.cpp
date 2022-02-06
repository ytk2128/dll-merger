#include <iostream>
#include <string>
#include <vector>
#include "PE/PEBase.h"
#include "PE/Exception.h"
#include "Assembler.h"

using namespace std;
using namespace pe32;

DWORD hashGenerate(const string& str) {
	DWORD hash = 0;
	DWORD poly = 0xEDB88320;

	for (auto i = 0u; i <= str.size(); i++) {
		poly = (poly << 1) | (poly >> 31);
		hash = (DWORD)(poly * hash + str[i]);
	}

	return hash;
}

int main(int argc, char** argv) {
	if (argc < 3) {
		cout << "merger.exe [exe name] [dll name 1] [dll name 2] ... [dll name N]\n";
		return 1;
	}

	string input(argv[1]);
	string output(input + "_out.exe");
	vector<string> dlls(&argv[2], &argv[argc]);
	vector<PEFile> pedlls;
	size_t totalSize = 0;

	try {
		for (auto& i : dlls) {
			PEFile dll(i);
			pedlls.push_back(dll);
			totalSize += dll.getFileSize();
		}

		PEFile file(input);
		file.createNewSection(".dlls", totalSize, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE);
		auto dataSection = file.rvaToAddr(file.getLastSection().VirtualAddress);

		file.createNewSection(".ldr", dlls.size() * 8 + 8 + 0x372, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE);
		file.setPos(file.getLastSection().PointerToRawData);
		auto pdlls = file.getPos();

		for (auto& dll : pedlls) {
			file.copyMemory(dataSection.raw, dll.data(), dll.getFileSize());
			file << dataSection.rva << (DWORD)dll.getFileSize();
			dataSection += dll.getFileSize();
		}
		file << 0ul << 0ul;
		
		Assembler assembler;

#pragma region Kernel32 Parser
		assembler.setScript(R"(
			push ebp
			mov ebp, esp
			sub esp, 0x10
			push ecx
			push edx
			push ebx
			push esi
			push edi
			xor eax, eax
			mov dword ptr ss:[ebp-0x4], eax
			mov eax, dword ptr fs:[0x00000030]
			mov eax, dword ptr ds:[eax+0xC]
			mov eax, dword ptr ds:[eax+0xC]
			mov eax, dword ptr ds:[eax]
			mov eax, dword ptr ds:[eax]
			mov eax, dword ptr ds:[eax+0x18]
			mov ecx, dword ptr ds:[eax+0x3C]
			add ecx, 0x78
			mov edx, dword ptr ds:[eax+ecx*1]
			add edx, eax
			xor ebx, ebx
			add edx, 0x20
			mov ebx, dword ptr ds:[edx]
			add ebx, eax
			mov edi, eax
		label2:
			add ebx, 0x4
			inc dword ptr ss:[ebp-0x4]
			mov esi, dword ptr ds:[ebx]
			add esi, edi
			xor ecx, ecx
			mov eax, 0xEDB88320
			mov dword ptr ss:[ebp-0x8], eax
			mov dword ptr ss:[ebp-0x10], ecx
			xor eax, eax
		label1:
			lodsb
			mov ecx, dword ptr ss:[ebp-0x8]
			shl ecx, 0x1
			mov dword ptr ss:[ebp-0xC], ecx
			mov ecx, dword ptr ss:[ebp-0x8]
			shr ecx, 0x1F
			or ecx, dword ptr ss:[ebp-0xC]
			mov dword ptr ss:[ebp-0x8], ecx
			mov ecx, dword ptr ss:[ebp-0x10]
			push eax
			push edx
			mov eax, dword ptr ss:[ebp-0x8]
			mul ecx
			mov ecx, eax
			pop edx
			pop eax
			add ecx, eax
			mov dword ptr ss:[ebp-0x10], ecx
			test al, al
			jne label1
			cmp ecx, dword ptr ss:[ebp+0x8]
			jne label2
			xor ebx, ebx
			add edx, 0x4
			mov ebx, dword ptr ds:[edx]
			add ebx, edi
			xor eax, eax
			mov al, 0x2
			mov esi, edx
			mul word ptr ds:[ebp-0x4]
			mov dword ptr ss:[ebp-0x4], eax
			xor eax, eax
			add ebx, dword ptr ss:[ebp-0x4]
			mov ax, word ptr ds:[ebx]
			sub esi, 0x8
			mov ecx, dword ptr ds:[esi]
			add ecx, edi
			xor ebx, ebx
			mov ebx, eax
			mov eax, 0x4
			mul ebx
			add ecx, eax
			mov ecx, dword ptr ds:[ecx]
			add ecx, edi
			mov eax, ecx
			pop edi
			pop esi
			pop ebx
			pop edx
			pop ecx
			mov esp, ebp
			pop ebp
			ret 0x4
		)");
		if (assembler.build() == false) {
			throw Exception("main", "failed to build Kernel32 Parser");
		}
		auto pKernel32Parser = file.getPos();
		file << assembler.getVector();
#pragma endregion

#pragma region Copy Memory
		assembler.setScript(R"(
			push ebp
			mov ebp, esp
			push esi
			push edi
			push ecx
			mov edi, dword ptr ss:[esp+0x14]
			mov esi, dword ptr ss:[esp+0x18]
			mov ecx, dword ptr ss:[esp+0x1C]
		label1:
			mov al, byte ptr ds:[esi]
			mov byte ptr ds:[edi], al
			inc esi
			inc edi
			loop short label1
			pop ecx
			pop edi
			pop esi
			mov esp, ebp
			pop ebp
			ret 0xC
		)");
		if (assembler.build() == false) {
			throw Exception("main", "failed to build Copy Memory");
		}
		auto pCopyMemory = file.getPos();
		file << assembler.getVector();
#pragma endregion

#pragma region Zero Memory
		assembler.setScript(R"(
			push ebp
			mov ebp, esp
			push edi
			push ecx
			mov edi, dword ptr ss:[esp+0x10]
			mov ecx, dword ptr ss:[esp+0x14]
		label1:
			mov byte ptr ds:[edi], 0x0
			inc edi
			loop short label1
			pop ecx
			pop edi
			mov esp, ebp
			pop ebp
			ret 0x8
		)");
		if (assembler.build() == false) {
			throw Exception("main", "failed to build Zero Memory");
		}
		auto pZeroMemory = file.getPos();
		file << assembler.getVector();
#pragma endregion
		
#pragma region Thread Proc
		assembler.setScript(R"(
			push ebp
			mov ebp, esp
			mov edx, dword ptr ss:[ebp+0x8]
			mov ecx, dword ptr ds:[edx]
			mov ebx, dword ptr ds:[edx+0x4]
			push ecx
			push edx
			push ebx
			mov eax, dword ptr ds:[edx+0x8]
			push MEM_RELEASE
			push 0x0
			push edx
			call eax
			pop ebx
			pop edx
			pop ecx
			push 0x0
			push DLL_PROCESS_ATTACH
			push ebx
			call ecx
			mov esp, ebp
			pop ebp
			ret 0x4
		)")
		.setSymbol("MEM_RELEASE", MEM_RELEASE)
		.setSymbol("DLL_PROCESS_ATTACH", DLL_PROCESS_ATTACH);
		if (assembler.build() == false) {
			throw Exception("main", "failed to build Thread Proc");
		}
		auto pThreadProc = file.getPos();
		file << assembler.getVector();
#pragma endregion

#pragma region Entry Point
		assembler.setScript(R"(
			pushad
			push ebp
			mov ebp, esp
			sub esp, 0x50
			mov eax, dword ptr fs:[0x00000030]
			mov eax, dword ptr ds:[eax+0x8]
			mov dword ptr ss:[ebp-0x4], eax
			mov ebx, eax
			lea edi, ds:[ebx+pKernel32Parser]
			push VirtualAlloc
			call edi
			mov dword ptr ss:[ebp-0xC], eax
			push LoadLibraryA
			call edi
			mov dword ptr ss:[ebp-0x14], eax
			push GetProcAddress
			call edi
			mov dword ptr ss:[ebp-0x18], eax

		; Mapping memory
			lea eax, ds:[ebx+pdlls]
			mov dword ptr ss:[ebp-0x8], eax

		@L00000001:
			mov ecx, dword ptr ds:[eax]
			test ecx, ecx
			je @L00000017
			mov edx, dword ptr ds:[eax+0x4]
			test edx, edx
			je @L00000017
			lea ecx, ds:[ecx+ebx*1]
			mov eax, ecx
			mov eax, dword ptr ds:[eax+0x3C]
			lea eax, ds:[eax+ecx*1]
			lea eax, ds:[eax+0x18]
			mov eax, dword ptr ds:[eax+0x38]
			mov esi, eax
			push ecx
			push edx
			push 0x40
			push 0x3000
			push esi
			push 0x0
			call dword ptr ss:[ebp-0xC]
			pop edx
			pop ecx
			mov esi, eax
			mov eax, ecx
			mov eax, dword ptr ds:[eax+0x3C]
			lea eax, ds:[eax+ecx*1]
			mov edi, eax
			lea eax, ds:[eax+0x18]
			mov eax, dword ptr ds:[eax+0x3C]
			push eax
			push ecx
			push esi
			lea eax, ds:[ebx+pCopyMemory]
			call eax
			lea eax, ds:[edi+0xF8]
			mov dword ptr ss:[ebp-0x10], eax

		@L00000002:
			lea eax, ds:[eax+0xC]
			cmp dword ptr ds:[eax], 0x0
			je short @L00000004
			push ebx
			push edx
			mov ebx, esi
			add ebx, dword ptr ds:[eax]
			mov edx, ecx
			push eax
			mov eax, dword ptr ds:[eax+0x8]
			test eax, eax
			je short @L00000003
			add edx, eax
			pop eax
			push eax
			mov eax, dword ptr ds:[eax+0x4]
			test eax, eax
			je short @L00000003
			push eax
			push edx
			push ebx
			mov eax, dword ptr ss:[ebp-0x4]
			lea eax, ds:[eax+pCopyMemory]
			call eax

		@L00000003:
			pop eax
			pop edx
			pop ebx
			add dword ptr ss:[ebp-0x10], 0x28
			mov eax, dword ptr ss:[ebp-0x10]
			jmp short @L00000002

		@L00000004:
			push edx
			push ecx
			lea eax, ds:[ebx+pZeroMemory]
			call eax

		; Base Relocation
			mov eax, dword ptr ds:[esi+0x3C]
			lea eax, ds:[eax+esi*1]
			mov edi, eax
			lea eax, ds:[eax+0x18]
			mov ecx, dword ptr ds:[eax+0x1C]
			mov dword ptr ss:[ebp-0x1C], esi
			sub dword ptr ss:[ebp-0x1C], ecx
			lea eax, ds:[eax+0x60]
			mov eax, dword ptr ds:[eax+0x28]
			lea eax, ds:[eax+esi*1]
			mov ecx, eax

		@L00000005:
			mov eax, dword ptr ds:[ecx]
			test eax, eax
			je short @L00000009
			mov edx, dword ptr ds:[ecx+0x4]
			test edx, edx
			je short @L00000008
			sub edx, 0x8
			mov eax, edx
			push ebx
			xor edx, edx
			mov ebx, 0x2
			div ebx
			pop ebx
			mov edx, eax
			xor eax, eax

		@L00000006:
			cmp eax, edx
			je short @L00000008
			cmp word ptr ds:[ecx+eax*2+0x8], 0x0
			je short @L00000007
			push edx
			xor edx, edx
			mov dx, word ptr ds:[ecx+eax*2+0x8]
			and dx, 0xFFF
			add edx, dword ptr ds:[ecx]
			mov dword ptr ss:[ebp-0x20], edx
			add dword ptr ss:[ebp-0x20], esi
			mov edx, dword ptr ss:[ebp-0x20]
			push ebx
			mov ebx, dword ptr ss:[ebp-0x1C]
			add dword ptr ds:[edx], ebx
			pop ebx
			pop edx

		@L00000007:
			inc eax
			jmp short @L00000006

		@L00000008:
			mov eax, dword ptr ds:[ecx+0x4]
			lea ecx, ds:[eax+ecx*1]
			jmp short @L00000005

		@L00000009:
			lea eax, ds:[edi+0x18]
			lea eax, ds:[eax+0x60]
			mov eax, dword ptr ds:[eax+0x8]
			lea eax, ds:[eax+esi*1]

		; Recovering import directory
			push ebx
			mov ebx, eax

		@L00000010:
			mov ecx, dword ptr ds:[ebx+0x10]
			test ecx, ecx
			je short @L00000016
			lea ecx, ds:[ecx+esi*1]
			mov edx, dword ptr ds:[ebx]
			lea edx, ds:[edx+esi*1]
			mov eax, dword ptr ds:[ebx+0xC]
			lea eax, ds:[eax+esi*1]
			push ecx
			push edx
			push ebx
			push eax
			call dword ptr ss:[ebp-0x14]
			pop ebx
			pop edx
			pop ecx
			test eax, eax
			je short @L00000015
			mov dword ptr ss:[ebp-0x24], eax

		@L00000011:
			mov eax, dword ptr ds:[edx]
			test eax, eax
			je short @L00000015
			mov eax, dword ptr ds:[edx]
			and eax, 0x80000000
			test eax, eax
			je short @L00000012
			mov eax, dword ptr ds:[edx]
			and eax, 0xFFFF
			jmp short @L00000013

		@L00000012:
			mov eax, dword ptr ds:[edx]
			lea eax, ds:[eax+esi*1]
			lea eax, ds:[eax+0x2]

		@L00000013:
			push ecx
			push edx
			push ebx
			push eax
			push dword ptr ss:[ebp-0x24]
			call dword ptr ss:[ebp-0x18]
			pop ebx
			pop edx
			pop ecx
			test eax, eax
			je short @L00000014
			mov dword ptr ds:[ecx], eax

		@L00000014:
			lea edx, ds:[edx+0x4]
			lea ecx, ds:[ecx+0x4]
			jmp short @L00000011

		@L00000015:
			lea ebx, ds:[ebx+0x14]
			jmp short @L00000010

		@L00000016:
			pop ebx

		; Calling Thread Proc
			push 0x4
			push 0x3000
			push 0xC
			push 0x0
			call dword ptr ss:[ebp-0xC]
			mov edx, eax
			lea ecx, ds:[edi+0x18]
			mov ecx, dword ptr ds:[ecx+0x10]
			lea ecx, ds:[ecx+esi*1]
			mov dword ptr ds:[edx], ecx
			mov dword ptr ds:[edx+0x4], esi
			push edi
			lea edi, ds:[ebx+pKernel32Parser]
			push VirtualFree
			call edi
			mov dword ptr ds:[edx+0x8], eax
			push CreateThread
			call edi
			pop edi
			mov ecx, eax
			push 0x0
			push 0x0
			push edx
			lea eax, ds:[ebx+pThreadProc]
			push eax
			push 0x0
			push 0x0
			call ecx
			add dword ptr ss:[ebp-0x8], 0x8
			mov eax, dword ptr ss:[ebp-0x8]
			jmp @L00000001

		@L00000017:
		; Jump to the original entry point
			add esp, 0x50
			mov esp, ebp
			pop ebp
			lea eax, ds:[ebx+OEP]
			mov dword ptr ss:[esp], eax
			popad
			jmp dword ptr ss:[esp-0x20]
		)")
		.setSymbol("pKernel32Parser", pKernel32Parser.rva)
		.setSymbol("VirtualAlloc", hashGenerate("VirtualAlloc"))
		.setSymbol("LoadLibraryA", hashGenerate("LoadLibraryA"))
		.setSymbol("GetProcAddress", hashGenerate("GetProcAddress"))
		.setSymbol("pdlls", pdlls.rva)
		.setSymbol("pCopyMemory", pCopyMemory.rva)
		.setSymbol("pZeroMemory", pZeroMemory.rva)
		.setSymbol("VirtualFree", hashGenerate("VirtualFree"))
		.setSymbol("CreateThread", hashGenerate("CreateThread"))
		.setSymbol("pThreadProc", pThreadProc.rva)
		.setSymbol("OEP", *file.AddressOfEntryPoint);
		if (assembler.build() == false) {
			throw Exception("main", "failed to build Entry Point");
		}
		auto pEntryPoint = file.getPos();
		file << assembler.getVector();
#pragma endregion

		*file.AddressOfEntryPoint = pEntryPoint.rva;
		file.save(output);

		cout << "DLLs are successfully merged.\n\n";
	}
	catch (Exception& ex) {
		cerr << ex.get() << "\n";
	}

	return 0;
}