#pragma once
#include "PEBase.h"

using namespace std;

namespace pe32 {
	class PERelocation {
	public:
		PERelocation(PEFile& file);
		void push_rva(DWORD rva);
		void push_data(DWORD rva);
		void build();

	private:
		PEFile& _file;
		vector<pair<DWORD, vector<WORD>>> _data;
	};
}