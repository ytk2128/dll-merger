#include <iostream>
#include <string>
#include <vector>
#include "PE/PEBase.h"
#include "PE/Exception.h"

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
		cout << "merger.exe [exe name] [dll name 1] ... [dll name N]\n";
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

		file.createNewSection(".ldr", dlls.size() * 8 + 8 + 0x5000, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE);
		file.setPos(file.getLastSection().PointerToRawData);
		auto pdlls = file.getPos();

		for (auto& dll : pedlls) {
			file.copyMemory(dataSection.raw, dll.data(), dll.getFileSize());
			file << dataSection.rva << (DWORD)dll.getFileSize();
			dataSection += dll.getFileSize();
		}
		file << 0ul << 0ul;
		


		file.save(output);
	}
	catch (Exception& ex) {
		cerr << ex.get() << "\n";
	}

	return 0;
}