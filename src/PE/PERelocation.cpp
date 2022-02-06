#include <iostream>
#include <vector>
#include "PERelocation.h"

namespace pe32 {
	PERelocation::PERelocation(PEFile& file)
		: _file(file)
		, _data()
	{}

	void PERelocation::push_rva(DWORD rva) {
		_data.emplace_back(rva, vector<WORD>());
	}

	void PERelocation::push_data(DWORD rva) {
		_data.back().second.push_back((WORD)((rva - _data.back().first) ^ 0x3000));
	}

	void PERelocation::build() {
		auto reloc = _file.getPos();
		_file.RelocationDirectory->VirtualAddress = reloc.rva;
		_file.RelocationDirectory->Size = 0;

		for (auto& i : _data) {
			_file << i.first << (DWORD)(i.second.size() * 2 + sizeof(IMAGE_BASE_RELOCATION));

			for (auto& j : i.second) {
				_file << j;
			}

			_file.RelocationDirectory->Size += i.second.size() * 2 + sizeof(IMAGE_BASE_RELOCATION);
		}
	}
}