/*
 * @description
 * Global definitions & structures for game
 */

#pragma once

#include <stdio.h>
#include <stdint.h>
#include "StdString.h"

#define ASSERT _ASSERT
#define SAFE_DELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; }
typedef StdString::CStdStringA RString;

/* Branch optimizations: */
#if defined(__GNUC__)
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

// http://www.powa-asso.fr/forum/viewtopic.php?f=26&t=824
namespace BmsJudgeTiming {
	enum BMSJUDGETIMING {
		PGREAT = 20,
		GREAT = 41,
		GOOD = 125,
		BAD = 173,
		POOR = 350,
	};
}

// TODO
namespace BmsHealthRecover {
	enum BMSHEALTHRECOVER {
		PGREAT = 20,
	};
}

#ifdef _WIN32
#define USE_MBCS	// uses wchar_t/wstring - for windows
#endif