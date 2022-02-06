#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdarg>

using namespace std;

namespace pe32 {
	class Log {
	public:
		Log()
			: _logs()
		{}

		void add(const char* format, ...) {
			time_t t;
			struct tm lt;
			time(&t);
			localtime_s(&lt, &t);

			char time[80] = {};
			strftime(time, 80, "%m/%d/%Y %H:%M:%S", &lt);

			char log[1024] = {};
			va_list aptr;
			va_start(aptr, format);
			vsnprintf(log, 1024, format, aptr);
			va_end(aptr);

			ostringstream line;
			line << "[" << time << "] " << log;
			_logs.push_back(line.str().c_str());
		}

		string getLogs() const {
			ostringstream logs;
			for (auto& i : _logs) {
				logs << i << endl;
			}
			return logs.str();
		}

		void print(bool all = false) {
			if (all) {
				cout << getLogs() << endl;
			}
			else {
				cout << _logs.back() << endl;
			}
		}

	private:
		vector<string> _logs;
	};
}