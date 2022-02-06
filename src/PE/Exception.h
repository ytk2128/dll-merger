#pragma once
#include <iostream>
#include <string>

using namespace std;

namespace pe32 {
	class Exception {
	public:
		explicit Exception(const string& cls, const string& con)
			: _class(cls.c_str())
			, _content(con.c_str())
		{}

		string get() const {
			return "<Exception occurred   " + _class + " - " + _content + ">";
		}

	private:
		string _class;
		string _content;
	};
}