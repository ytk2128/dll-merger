#include <cassert>
#include <memory>
#include "PEBase.h"
#include "Exception.h"

namespace pe32 {
	addr::addr(DWORD _raw, DWORD _rva, DWORD _va)
		: raw(_raw)
		, rva(_rva)
		, va(_va)
	{}

	void addr::reset() {
		raw = rva = va = 0;
	}

	void addr::reset(addr& address) {
		raw = address.raw, rva = address.rva, va = address.va;
	}

	void addr::reset(DWORD _raw, DWORD _rva, DWORD _va) {
		raw = _raw, rva = _rva, va = _va;
	}

	addr addr::operator+(size_t rhs) {
		addr r = *this;
		r += rhs;
		return r;
	}

	addr addr::operator-(size_t rhs) {
		addr r = *this;
		r -= rhs;
		return r;
	}

	addr& addr::operator+=(size_t rhs) {
		this->raw += rhs, this->rva += rhs, this->va += rhs;
		return *this;
	}

	addr& addr::operator-=(size_t rhs) {
		this->raw -= rhs, this->rva -= rhs, this->va -= rhs;
		return *this;
	}

	bool addr::operator==(addr& rhs) const {
		return this->raw == rhs.raw && this->rva == rhs.rva && this->va == rhs.va;
	}

	bool addr::operator!=(addr& rhs) const {
		return this->raw != rhs.raw && this->rva != rhs.rva && this->va != rhs.va;
	}

	PEHeader::~PEHeader() {}

	bool PEHeader::isValidHeader() const {
		return pDosHeader->e_magic == IMAGE_DOS_SIGNATURE && pNtHeader->Signature == IMAGE_NT_SIGNATURE && pNtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386;
	}

	PEFile::PEFile()
		: _mem()
		, _ptr(0)
		, _fileName()
		, _fileSize(0)
		, _retn()
	{}

	PEFile::PEFile(const string& fileName) {
		open(fileName);
	}

	PEFile::PEFile(const size_t fileSize) {
		create(fileSize);
	}

	PEFile::~PEFile() {
		_mem.clear();
	}

	bool PEFile::open(const string& fileName) {
		shared_ptr<void> handle(
			CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr),
			&CloseHandle
		);
		if (handle.get() == INVALID_HANDLE_VALUE) {
			throw Exception("PEFile::open", "Invalid handle value - " + fileName);
		}
		_fileName = fileName;
		_fileSize = GetFileSize(handle.get(), nullptr);
		_mem.resize(_fileSize, 0);

		DWORD dwNumberOfBytesRead;
		if (!ReadFile(handle.get(), _mem.data(), _fileSize, &dwNumberOfBytesRead, nullptr)) {
			throw Exception("PEFile::open", "Cannot read file data - " + fileName);
		}
		setPEHeaders();
		if (!isValidHeader()) {
			throw Exception("PEFile::open", "Invalid PE32 header - " + fileName);
		}
		return true;
	}

	bool PEFile::create(const size_t fileSize) {
		if (!fileSize) {
			throw Exception("PEFile::create", "fileSize is zero");
		}
		_fileSize = fileSize;
		_mem.resize(_fileSize, 0);
		// loadPEHeader with e_lfnew
		return true;
	}

	bool PEFile::save(const string& fileName) {
		shared_ptr<void> handle(
			CreateFile(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr),
			&CloseHandle
		);
		if (handle.get() == INVALID_HANDLE_VALUE) {
			throw Exception("PEFile::save", "Invalid handle value - " + fileName);
		}
		_fileName = fileName;
		DWORD dwNumberOfBytesWritten;
		if (!WriteFile(handle.get(), _mem.data(), _fileSize, &dwNumberOfBytesWritten, nullptr)) {
			throw Exception("PEFile::save", "Cannot write file data - " + fileName);
		}
		return true;
	}

	BYTE* PEFile::data() {
		return _mem.data();
	}

	void PEFile::resize(const size_t newSize) {
		if (_fileSize >= newSize) {
			throw Exception("PEFile::resize", "newSize is less than fileSize");
		}
		_mem.resize(newSize);
		_fileSize = newSize;
		setPEHeaders();
	}

	size_t PEFile::getFileSize() const {
		return _fileSize;
	}

	void PEFile::setFileSize(const size_t fileSize) {
		_fileSize = fileSize;
	}

	string PEFile::getFileName() const {
		return _fileName;
	}

	DWORD PEFile::rvaToRaw(DWORD rva) const {
		for (int i = 0; i != *NumberOfSections; i++) {
			if (rva >= pSecHeader[i].VirtualAddress && rva < pSecHeader[i].VirtualAddress + pSecHeader[i].Misc.VirtualSize) {
				return rva - pSecHeader[i].VirtualAddress + pSecHeader[i].PointerToRawData;
			}
		}
		return 0;
	}

	DWORD PEFile::rawToRva(DWORD raw) const {
		for (int i = 0; i != *NumberOfSections; i++) {
			if (raw >= pSecHeader[i].PointerToRawData && raw < pSecHeader[i].PointerToRawData + pSecHeader[i].SizeOfRawData) {
				return raw - pSecHeader[i].PointerToRawData + pSecHeader[i].VirtualAddress;
			}
		}
		return 0;
	}

	DWORD PEFile::rvaToSectionIdx(DWORD rva) const {
		for (int i = 0; i != *NumberOfSections; i++) {
			if (rva >= pSecHeader[i].VirtualAddress && rva < pSecHeader[i].VirtualAddress + pSecHeader[i].Misc.VirtualSize) {
				return i;
			}
		}
		return -1;
	}

	DWORD PEFile::rawToSectionIdx(DWORD raw) const {
		for (int i = 0; i != *NumberOfSections; i++) {
			if (raw >= pSecHeader[i].PointerToRawData && raw < pSecHeader[i].PointerToRawData + pSecHeader[i].SizeOfRawData) {
				return i;
			}
		}
		return -1;
	}

	addr PEFile::rvaToAddr(DWORD rva) const {
		_retn.rva = rva;
		_retn.raw = rvaToRaw(rva);
		_retn.va = rva + *ImageBase;
		return _retn;
	}

	addr PEFile::rawToAddr(DWORD raw) const {
		_retn.raw = raw;
		_retn.rva = rawToRva(raw);
		_retn.va = _retn.rva + *ImageBase;
		return _retn;
	}

	addr PEFile::vaToAddr(DWORD va) const {
		_retn.va = va;
		_retn.rva = va - *ImageBase;
		_retn.raw = rvaToRaw(_retn.rva);
		return _retn;
	}

	size_t PEFile::getVirtualImageSize() const {
		return align(getLastSection().VirtualAddress + getLastSection().Misc.VirtualSize, *SectionAlignment);
	}

	size_t PEFile::getRawImageSize() const {
		return getLastSection().PointerToRawData + getLastSection().SizeOfRawData;
	}

	IMAGE_SECTION_HEADER& PEFile::getFirstSection() const {
		return pSecHeader[0];
	}

	IMAGE_SECTION_HEADER& PEFile::getLastSection() const {
		return pSecHeader[*NumberOfSections - 1];
	}

	IMAGE_SECTION_HEADER& PEFile::createNewSection(const string& name, const size_t size, DWORD chr) {
		memcpy(pSecHeader[*NumberOfSections].Name, name.c_str(), name.size() > 8 ? 8 : name.size());
		pSecHeader[*NumberOfSections].PointerToRawData = align(getLastSection().PointerToRawData + getLastSection().SizeOfRawData, *FileAlignment);
		pSecHeader[*NumberOfSections].SizeOfRawData = align(size, *FileAlignment);
		pSecHeader[*NumberOfSections].VirtualAddress = align(getLastSection().VirtualAddress + getLastSection().Misc.VirtualSize, *SectionAlignment);
		pSecHeader[*NumberOfSections].Misc.VirtualSize = align(size, *SectionAlignment);
		pSecHeader[*NumberOfSections].Characteristics = chr;
		(*NumberOfSections)++;
		*SizeOfImage = getVirtualImageSize();

		resize(_fileSize + getLastSection().SizeOfRawData);
		memset(_mem.data() + getLastSection().PointerToRawData, 0, getLastSection().SizeOfRawData);

		return getLastSection();
	}

	void PEFile::setPos(DWORD raw) {
		if (raw < 0 || raw >= _fileSize) {
			throw Exception("PEFile::setPos", "raw is out of range");
		}
		_ptr = raw;
	}

	addr PEFile::getPos() {
		return rawToAddr(_ptr);
	}

	void PEFile::setPEHeaders() {
		pDosHeader = (PIMAGE_DOS_HEADER)(_mem.data());
		pNtHeader = (PIMAGE_NT_HEADERS)(_mem.data() + pDosHeader->e_lfanew);
		pSecHeader = (PIMAGE_SECTION_HEADER)(_mem.data() + pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));

		ExportDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		ImportDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		ResourceDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
		SecurityDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
		RelocationDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		DebugDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
		TLSDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
		ConfigurationDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG];
		BoundImportDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
		ImportAddressTableDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT];
		DelayImportDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
		NetMetaDataDirectory = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];

		NumberOfSections = &pNtHeader->FileHeader.NumberOfSections;
		ImageBase = &pNtHeader->OptionalHeader.ImageBase;
		SizeOfImage = &pNtHeader->OptionalHeader.SizeOfImage;
		SizeOfHeaders = &pNtHeader->OptionalHeader.SizeOfHeaders;
		FileAlignment = &pNtHeader->OptionalHeader.FileAlignment;
		SectionAlignment = &pNtHeader->OptionalHeader.SectionAlignment;
		AddressOfEntryPoint = &pNtHeader->OptionalHeader.AddressOfEntryPoint;
		DllCharacteristics = &pNtHeader->OptionalHeader.DllCharacteristics;
	}

	DWORD PEFile::align(DWORD x, DWORD align) const {
		return ((x + align - 1) / align) * align;
	}

	PEFile& PEFile::operator<<(const vector<BYTE>& bytes) {
		// this feature needs to have automatical resizing system someday.
		if (_ptr + bytes.size() >= _fileSize - 1) {
			throw Exception("PEFile::operator<<", "bytes out of range");
		}
		memcpy(_mem.data() + _ptr, bytes.data(), bytes.size());
		_ptr += bytes.size();
		return *this;
	}

	PEFile& PEFile::operator<<(const string& str) {
		if (_ptr + str.size() + 1 >= _fileSize - 1) {
			throw Exception("PEFile::operator<<", "str out of range");
		}
		memcpy(_mem.data() + _ptr, str.data(), str.size() + 1);
		_ptr += str.size() + 1;
		return *this;
	}

	PEFile& PEFile::operator<<(const WORD w) {
		if (_ptr + sizeof(WORD) >= _fileSize - 1) {
			throw Exception("PEFile::operator<<", "word out of range");
		}
		memcpy(_mem.data() + _ptr, &w, sizeof(WORD));
		_ptr += sizeof(WORD);
		return *this;
	}

	PEFile& PEFile::operator<<(const DWORD dw) {
		if (_ptr + sizeof(DWORD) >= _fileSize - 1) {
			throw Exception("PEFile::operator<<", "dword out of range");
		}
		memcpy(_mem.data() + _ptr, &dw, sizeof(DWORD));
		_ptr += sizeof(DWORD);
		return *this;
	}

	PEFile& PEFile::operator+=(const size_t size) {
		setPos(_ptr + size);
		return *this;
	}

	void PEFile::copyMemory(const void* src, size_t size) {
		if (_ptr + size >= _fileSize - 1) {
			throw Exception("PEFile::copyMemory", "src out of range");
		}
		memcpy(_mem.data() + _ptr, src, size);
		_ptr += size;
	}

	void PEFile::copyMemory(DWORD raw, const void* src, size_t size) {
		if (raw + size >= _fileSize - 1) {
			throw Exception("PEFile::copyMemory", "src out of range");
		}
		memcpy(_mem.data() + raw, src, size);
	}
}
