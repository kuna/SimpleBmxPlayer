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

/* 
 * game related information
 */

// http://www.powa-asso.fr/forum/viewtopic.php?f=26&t=824
namespace BmsJudgeTiming {
	enum BMSJUDGETIMING {
		PGREAT = 20,
		GREAT = 41,
		GOOD = 125,
		BAD = 173,
		POOR = 350,
		NPOOR = 500,
	};
}

// TODO
namespace BmsHealthRecover {
	enum BMSHEALTHRECOVER {
		PGREAT = 20,
	};
}

namespace JUDGETYPE {
	const int JUDGE_PGREAT = 5;
	const int JUDGE_GREAT = 4;
	const int JUDGE_GOOD = 3;
	const int JUDGE_BAD = 2;
	const int JUDGE_POOR = 1;
	const int JUDGE_NPOOR = 0;
	const int JUDGE_EARLY = 10;	// it's too early, so it should have no effect
	const int JUDGE_LATE = 11;	// it's too late, so it should have no effect
}

namespace GRADETYPE {
	const int GRADE_AAA = 8;
	const int GRADE_AA = 7;
	const int GRADE_A = 6;
	const int GRADE_B = 5;
	const int GRADE_C = 4;
	const int GRADE_D = 3;
	const int GRADE_E = 2;
	const int GRADE_F = 1;
}

namespace PLAYERTYPE {
	const int NORMAL = 0;
	const int AUTO = 1;
	const int REPLAY = 2;
	const int NETWORK = 3;	// not implemented
}

namespace PlayerKeyIndex {
	const int NONE = 0;
	const int P1_BUTTON1 = 1;
	const int P1_BUTTON2 = 2;
	const int P1_BUTTON3 = 3;
	const int P1_BUTTON4 = 4;
	const int P1_BUTTON5 = 5;
	const int P1_BUTTON6 = 6;
	const int P1_BUTTON7 = 7;
	// this means Scratch, mostly
	const int P1_BUTTON8 = 8;
	const int P1_BUTTONSC = 8;
	const int P1_BUTTON9 = 9;
	const int P2_BUTTON1 = 11;
	const int P2_BUTTON2 = 12;
	const int P2_BUTTON3 = 13;
	const int P2_BUTTON4 = 14;
	const int P2_BUTTON5 = 15;
	const int P2_BUTTON6 = 16;
	const int P2_BUTTON7 = 17;
	// this means Scratch, mostly
	const int P2_BUTTON8 = 18;
	const int P2_BUTTONSC = 18;
	const int P2_BUTTON9 = 19;
	const int P1_BUTTONSTART = 20;
	const int P1_BUTTONVEFX = 21;
	const int P1_BUTTONEFFECT = 22;
	const int P2_BUTTONSTART = 30;
	const int P2_BUTTONVEFX = 31;
	const int P2_BUTTONEFFECT = 32;
}

namespace SPEEDTYPE {
	const int NONE = 0;
	const int MEDIUM = 1;
	const int MAXBPM = 2;
	const int MINBPM = 3;
	const int CONSTANT = 4;
}

namespace GAUGETYPE {
	const int GROOVE = 0;
	const int EASY = 1;
	const int HARD = 2;
	const int EXHARD = 3;
	const int HAZARD = 4;
	const int PATTACK = 5;
	const int ASSISTEASY = 6;
}

namespace PACEMAKERTYPE {
	const int PACE0 = 0;
	const int PACE100 = 1;
	const int PACECUSTOM = 2;
	const int PACEMYBEST = 3;
	const int PACEA = 4;
	const int PACEAA = 5;
	const int PACEAAA = 6;
	const int PACEIRNEXT = 7;
	const int PACEIRBEST = 8;
	const int PACERIVAL1 = 9;
	const int PACERIVAL2 = 10;
	const int PACERIVAL3 = 11;
	const int PACERIVAL4 = 12;
	const int PACERIVAL5 = 13;
}

namespace PLAYTYPE {
	const int KEY5 = 5;
	const int KEY7 = 7;
	const int KEY9 = 9;
	const int KEY10 = 10;
	const int KEY14 = 14;
	const int KEY18 = 18;
}

namespace SCENETYPE {

}


#ifdef _WIN32
#define USE_MBCS	// uses wchar_t/wstring - for windows
#endif