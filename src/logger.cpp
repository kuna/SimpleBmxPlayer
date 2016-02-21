#include "logger.h"
#include "util.h"
#ifdef _WIN32
#include <Windows.h>
#endif

Logger *LOG = 0;

void Logger::Info(const char *fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	RString msg = vssprintf(fmt, vl);
	PrintToScreen(msg, 0);
}

void Logger::Warn(const char *fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	RString msg = vssprintf(fmt, vl);
	PrintToScreen(msg, 1);
}

void Logger::Critical(const char *fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	RString msg = vssprintf(fmt, vl);
	PrintToScreen(msg, 2);
}

void Logger::PrintToScreen(const char *msg, int type) {
#ifdef _WIN32
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	switch (type) {
	case 0:
		// default
		SetConsoleTextAttribute(hConsole, 7);
		break;
	case 1:
		// yellow
		SetConsoleTextAttribute(hConsole, 14);
		break;
	case 2:
		// red
		SetConsoleTextAttribute(hConsole, 12);
		break;
	}
#endif
	printf(msg);
	printf("\n");

	// set to default again
	SetConsoleTextAttribute(hConsole, 7);
}