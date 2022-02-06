#pragma once
#include <string>
#include <vector>
#include <asmjit/asmjit.h>
#include <asmtk/asmtk.h>

using namespace std;
using namespace asmjit;
using namespace asmtk;

class Assembler {
public:
	Assembler()
		: _script()
		, _vec()
	{}

	Assembler& setScript(const string& script) {
		_script = script;
		_vec.clear();
		return *this;
	}

	string getScript() const {
		return _script;
	}

	template <typename T>
	Assembler& setSymbol(string name, T value) {
		_script = replaceAll(_script, name, to_string(value));
		return *this;
	}

	bool build() {
		Environment env(Arch::kX86);
		CodeHolder code;
		
		if (code.init(env, 0)) {
			return false;
		}
		
		x86::Assembler a(&code);
		AsmParser p(&a);

		if (p.parse(_script.c_str())) {
			return false;
		}

		CodeBuffer& buffer = code.sectionById(0)->buffer();
		_vec = vector<uint8_t>(buffer.begin(), buffer.end());
		return true;
	}

	vector<uint8_t> getVector() const {
		return _vec;
	}

private:
	string replaceAll(string str, const string& from, const string& to) {
		size_t startPos = 0;
		while ((startPos = str.find(from, startPos)) != string::npos) {
			str.replace(startPos, from.length(), to);
			startPos += to.length();
		}
		return str;
	}

private:
	string _script;
	vector<uint8_t> _vec;
};