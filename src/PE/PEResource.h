#pragma once
#include <tuple>
#include "PEBase.h"

using namespace std;

namespace pe32 {
	class PEResource {
	public:
		PEResource(PEFile& file);
		void push_entry(DWORD name);
		void push_data(DWORD id, DWORD name, BYTE* data, size_t size);
		void build();
		size_t size() const;

	private:


	private:
		using ResourceEntry = vector<tuple<DWORD, DWORD, vector<BYTE>>>;
		using ResourceData = pair<DWORD, ResourceEntry>;
		PEFile& _file;
		vector<ResourceData> _rData;
		size_t _baseDirSize;
		size_t _subDirSize;
		size_t _finalDirSize;
		size_t _dataEntrySize;
		size_t _dataSize;
	};
}