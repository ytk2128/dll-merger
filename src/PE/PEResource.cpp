#include <iostream>
#include <vector>
#include "PEResource.h"

namespace pe32 {
	PEResource::PEResource(PEFile& file)
		: _file(file)
		, _rData()
		, _baseDirSize(sizeof(IMAGE_RESOURCE_DIRECTORY))
		, _subDirSize(0)
		, _finalDirSize(0)
		, _dataEntrySize(0)
		, _dataSize(0)
	{}

	void PEResource::push_entry(DWORD name) {
		_rData.emplace_back(name, ResourceEntry());
		_baseDirSize += sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
		_subDirSize += sizeof(IMAGE_RESOURCE_DIRECTORY);
	}

	void PEResource::push_data(DWORD id, DWORD name, BYTE* data, size_t size) {
		_rData.back().second.emplace_back(id, name, vector<BYTE>(data, data + size));
		_subDirSize += sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
		_finalDirSize += sizeof(IMAGE_RESOURCE_DIRECTORY) + sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
		_dataEntrySize += sizeof(IMAGE_RESOURCE_DATA_ENTRY);
		_dataSize += size;
	}

	/*
	-------------------------------------------- _baseDirSize
	Base Resource Directory
		Resource Directory Entry ICON
		Resource Directory Entry PNG
		Resource Directory Entry BITMAP
	-------------------------------------------- _subDirSize
	Sub Resource Directory ICON
		Sub Resource Directory Entry: 1
		Sub Resource Directory Entry: 2
	Sub Resource Directory PNG
		Sub Resource Directory Entry: 1
	Sub Resource Directory BITMAP
		Sub Resource Directory Entry: 1
	-------------------------------------------- _finalDirSize
	Sub Sub Resource Directory: 1 of ICON
		Sub Sub Resource Directory Entry
	Sub Sub Resource Directory: 2 of ICON
		Sub Sub Resource Directory Entry
	Sub Sub Resource Directory: 1 of PNG
		Sub Sub Resource Directory Entry
	Sub Sub Resource Directory: 1 of BITMAP
		Sub Sub Resource Directory Entry
	-------------------------------------------- _dataEntrySize
	Resource Data Entry 1
	Resource Data Entry 2
	Resource Data Entry 3
	--------------------------------------------
	Data 1 ...
	Data 2 ...
	Data 3 ...
	--------------------------------------------
	*/

	void PEResource::build() {
		PIMAGE_RESOURCE_DIRECTORY pResDir = 0;
		PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry = 0;
		PIMAGE_RESOURCE_DATA_ENTRY pDataEntry = 0;

		auto baseDir = _file.getPos();
		pResDir = (PIMAGE_RESOURCE_DIRECTORY)(_file.data() + baseDir.raw);
		pResDir->MajorVersion = 4;
		pResDir->NumberOfIdEntries = (WORD)_rData.size();

		auto bDir = baseDir + sizeof(IMAGE_RESOURCE_DIRECTORY);
		auto subDir = baseDir + _baseDirSize;
		auto finalDir = subDir + _subDirSize;
		auto dataEntry = finalDir + _finalDirSize;
		auto dataTable = dataEntry + _dataEntrySize;

		for (auto& i : _rData) {
			pResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(_file.data() + bDir.raw);
			pResEntry->Name = i.first;
			pResEntry->OffsetToData = (subDir.raw - baseDir.raw) ^ 0x80000000;
			bDir += sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);

			pResDir = (PIMAGE_RESOURCE_DIRECTORY)(_file.data() + subDir.raw);
			pResDir->NumberOfIdEntries = (WORD)i.second.size();
			subDir += sizeof(IMAGE_RESOURCE_DIRECTORY);

			for (auto& j : i.second) {
				pResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(_file.data() + subDir.raw);
				pResEntry->Name = get<0>(j);
				pResEntry->OffsetToData = (finalDir.raw - baseDir.raw) ^ 0x80000000;
				subDir += sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);

				pResDir = (PIMAGE_RESOURCE_DIRECTORY)(_file.data() + finalDir.raw);
				pResDir->NumberOfIdEntries = 1;
				finalDir += sizeof(IMAGE_RESOURCE_DIRECTORY);

				pResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(_file.data() + finalDir.raw);
				pResEntry->Name = get<1>(j);
				pResEntry->OffsetToData = dataEntry.raw - baseDir.raw;
				finalDir += sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);

				auto dataPtr = get<2>(j).data();
				auto dataSize = get<2>(j).size();
				pDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)(_file.data() + dataEntry.raw);
				pDataEntry->OffsetToData = dataTable.rva;
				pDataEntry->Size = dataSize;
				dataEntry += sizeof(IMAGE_RESOURCE_DATA_ENTRY);

				memcpy(_file.data() + dataTable.raw, dataPtr, dataSize);
				dataTable += dataSize;
			}
		}

		_file.setPos(dataTable.raw);
		_file.ResourceDirectory->VirtualAddress = baseDir.rva;
		_file.ResourceDirectory->Size = dataTable.raw - baseDir.raw;
	}

	size_t PEResource::size() const {
		return _baseDirSize + _subDirSize + _finalDirSize + _dataEntrySize + _dataSize;
	}
}