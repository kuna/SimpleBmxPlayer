#pragma once

#include "global.h"

/** @brief logs program status here. accessible from anywhere. */
class Logger {
public:
	void Info(const char* fmt, ...);
	void Warn(const char* fmt, ...);
	void Critical(const char* fmt, ...);
private:
	void PrintToScreen(const char *msg, int type);
	void PrintToFile(const char *msg, int type);
};

/* Accessible from anywhere. */
extern Logger *LOG;