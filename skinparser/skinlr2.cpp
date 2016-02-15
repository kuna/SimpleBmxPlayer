#include "skin.h"
#include "skinutil.h"
#include "skintexturefont.h"
using namespace SkinUtil;
using namespace tinyxml2;
#include <sstream>

// functions/values commonly used here
namespace {
	char translated[1024];
	const char empty_str_[] = "";
	const char unknown_[] = "UNKNOWN";

	char* Trim(char *p) {
		char *r = p;
		while (*p == ' ' || *p == '\t')
			p++;
		r = p;
		while (*p != 0)
			p++;
		p--;
		while (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != 0)
			p--;
		*p = 0;
		return r;
	}
}

//
// translator starts
//
#pragma region TRANSLATECODE

//
// OP code converter
//
// these code are supported by system/scripts/lr2.lua
// all status are calculated before objects are created.
//
namespace {
	const char* op_code_[1000] = { 
		//
		// 0
		// code 0 is constant value - but should NOT given as an argument, I suspect...
		//
		"true",						"IsSelectBarFolder",
		"IsSelectBarSong",			"IsSelectBarCourse",
		"IsSelectBarNewCourse",		"IsSelectBarPlayable",
		empty_str_,					empty_str_,
		empty_str_,					empty_str_,
		// 10
		// 12: this includes battle
		// 13: this includes ghost battle
		"IsDoublePlay",             "IsBattlePlay",
		"IsDoublePlay",             "IsBattlePlay",
		empty_str_,                 empty_str_,
		empty_str_,                 empty_str_,
		empty_str_,                 empty_str_,
		// 20
		"Panel", "Panel1",
		"Panel2", "Panel3",
		"Panel4", "Panel5",
		"Panel6", "Panel7",
		"Panel8", "Panel9",
		// 30
		"IsBGANormal", "IsBGA",
		"!IsAutoPlay", "IsAutoPlay",
		"IsGhostOff", "IsGhostA",
		"IsGhostB", "IsGhostC",
		"!IsScoreGraph", "IsScoreGraph",
		// 40
		"!IsBGA", "IsBGA",
		"IsP1GrooveGauge", "IsP1HardGauge",
		"IsP2GrooveGauge", "IsP2HardGauge",
		"IsDifficultyFilter", "!IsDifficultyFilter",
		empty_str_, empty_str_,
		// 50
		"!IsOnline", "IsOnline",
		"!IsExtraMode", "IsExtraMode",
		"!IsP1AutoSC", "IsP1AutoSC",
		"!IsP2AutoSC", "IsP2AutoSC",
		empty_str_, empty_str_,
		// 60
		"!IsRecordable", "IsRecordable", "!IsRecordable", 
		"IsEasyClear", "IsGrooveClear", "IsHardClear", "IsFCClear",
		empty_str_, empty_str_, empty_str_,
		// 70
		"IsBeginnerSparkle", "IsNormalSparkle", "IsHyperSparkle", "IsAnotherSparkle", "IsInsaneSparkle",
		"!IsBeginnerSparkle", "!IsNormalSparkle", "!IsHyperSparkle", "!IsAnotherSparkle", "!IsInsaneSparkle",
		// 80
		"OnSongLoading", "OnSongLoadingEnd", 
		empty_str_, empty_str_,
		"OnSongReplay", empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		// 90
		"OnResultClear", "OnResultFail",
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		// 100 ~ 150: empty
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 150
		"OnDiffNone", "OnDiffBeginner", "OnDiffNormal", "OnDiffHyper", "OnDiffAnother", "OnDiffInsane",
		empty_str_, empty_str_, empty_str_, empty_str_, 
		"Is7Keys", "Is5Keys", "Is14Keys", "Is10Keys", "Is9Keys",
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 170
		"IsBGA", "!IsBGA",
		"IsLongNote", "!IsLongNote",
		"IsBmsReadme", "!IsBmsReadme",
		"IsBpmChange", "!IsBpmChange",
		"IsBmsRandomCommand", "!IsBmsRandomCommand",
		// 180
		"IsJudgeVERYHARD", "IsJudgeHARD", "IsJudgeNORMAL", "IsJudgeEASY",
		"IsLevelSparkle", "!IsLevelSparkle", "IsLevelSparkle",
		empty_str_, empty_str_, empty_str_,
		// 190
		"!IsStageFile", "IsStageFile",
		"!IsBANNER", "IsBANNER",
		"!IsBACKBMP", "IsBACKBMP",
		"!IsReplayable", "!IsReplayable",
		empty_str_, empty_str_,
		//
		// 200
		// - during play
		//
		"IsP1AAA", "IsP1AA", "IsP1A", "IsP1B", "IsP1C", "IsP1D", "IsP1E", "IsP1F",
		empty_str_, empty_str_,
		"IsP2AAA", "IsP2AA", "IsP2A", "IsP2B", "IsP2C", "IsP2D", "IsP2E", "IsP2F",
		empty_str_, empty_str_,
		"IsP1ReachAAA", "IsP1ReachAA", "IsP1ReachA", "IsP1ReachB", "IsP1ReachC", "IsP1ReachD", "IsP1ReachE", "IsP1ReachF",
		empty_str_, empty_str_,
		"IsP2ReachAAA", "IsP2ReachAA", "IsP2ReachA", "IsP2ReachB", "IsP2ReachC", "IsP2ReachD", "IsP2ReachE", "IsP2ReachF",
		empty_str_, empty_str_,
		// 240
		empty_str_, "P1JudgePerfect",
		"P1JudgeGreat", "P1JudgeGood",
		"P1JudgeBad", "P1JudgePoor",
		"P1JudgeNPoor", "P1Miss",
		"!P1Miss", empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		//  260
		empty_str_, "P2JudgePerfect",
		"P2JudgeGreat", "P2JudgeGood",
		"P2JudgeBad", "P2JudgePoor",
		"P2JudgeNPoor", "P2Miss",
		"!P2Miss", empty_str_,
		// 270
		"OnP1SuddenChange", "OnP2SuddenChange",
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 280
		"IsCourse1Stage", "IsCourse2Stage",
		"IsCourse3Stage", "IsCourse4Stage",
		"IsCourse5Stage", "IsCourse6Stage",
		"IsCourse7Stage", "IsCourse8Stage",
		"IsCourse9Stage", "IsCourseFinal",
		// 290
		// 291: ӫ������
		"IsCourse", "IsGrading", "IsExpertCourse", "IsClassCourse",
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		//
		// 300
		// result screen
		//
		"IsP1AAA", "IsP1AA", "IsP1A", "IsP1B", "IsP1C", "IsP1D", "IsP1E", "IsP1F",
		empty_str_, empty_str_,
		"IsP2AAA", "IsP2AA", "IsP2A", "IsP2B", "IsP2C", "IsP2D", "IsP2E", "IsP2F",
		empty_str_, empty_str_,
		"IsP1BeforeAAA", "IsP1BeforeAA", "IsP1BeforeA", "IsP1BeforeB", "IsP1BeforeC", "IsP1BeforeD", "IsP1BeforeE", "IsP1BeforeF",
		empty_str_, empty_str_,
		"IsP2BeforeAAA", "IsP2BeforeAA", "IsP2BeforeA", "IsP2BeforeB", "IsP2BeforeC", "IsP2BeforeD", "IsP2BeforeE", "IsP2BeforeF",
		empty_str_, empty_str_,
		"IsP1AfterAAA", "IsP1AfterAA", "IsP1AfterA", "IsP1AfterB", "IsP1AfterC", "IsP1AfterD", "IsP1AfterE", "IsP1AfterF",
		empty_str_, empty_str_,
		"IsP2AfterAAA", "IsP2AfterAA", "IsP2AfterA", "IsP2AfterB", "IsP2AfterC", "IsP2AfterD", "IsP2AfterE", "IsP2AfterF",
		empty_str_, empty_str_,
		// 350
		"IsResultUpdated", "IsMaxcomboUpdated", "IsMinBPUpdated", "IsResultUpdated", "IsIRRankUpdated", "IsIRRankUpdated",
		empty_str_, empty_str_, empty_str_, empty_str_,
		// 360 ~ 400: empty
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		//
		// 400
		//
		"Is714Key",
		"Is9Key",
		"Is510Key",
		//
		// end
		//
		empty_str_,
	};
}

//
// Timer code converter
// - Well, these should be converted into handler!
//
namespace {
	const char* timer_code_[1000] = { 
		// 0
		empty_str_, 
		"StartInput",
		"FadeOut",
		"ShutDown",
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 20
		empty_str_, "Panel1", "Panel2", "Panel3", "Panel4", "Panel5", "Panel6", "Panel7", "Panel8", "Panel9",
		empty_str_, "Panel1Close", "Panel2Close", "Panel3Close", "Panel4Close",
		"Panel5Close", "Panel6Close", "Panel7Close", "Panel8Close", "Panel9Close",
		// 40
		// - basic handler is P1GaugeChanged / P2GaugeChanged
		//   we should make helper code out of there.
		"Ready", "GameStart", 
		"P1GaugeUp", "P2GaugeUp", 
		"P1GaugeMax", "P2GaugeMax",
		"P1Combo", "P2Combo", 
		"P1FullCombo", "P2FullCombo",
		// 50
		"P1JudgeSCOkay", "P1Judge1Okay",
		"P1Judge2Okay", "P1Judge3Okay",
		"P1Judge4Okay", "P1Judge5Okay",
		"P1Judge6Okay", "P1Judge7Okay",
		"P1Judge8Okay", "P1Judge9Okay",
		"P2JudgeSCOkay", "P2Judge1Okay",
		"P2Judge2Okay", "P2Judge3Okay",
		"P2Judge4Okay", "P2Judge5Okay",
		"P2Judge6Okay", "P2Judge7Okay",
		"P2Judge8Okay", "P2Judge9Okay",
		// 70
		"P1JudgeSCHold", "P1Judge1Hold",
		"P1Judge2Hold", "P1Judge3Hold",
		"P1Judge4Hold", "P1Judge5Hold",
		"P1Judge6Hold", "P1Judge7Hold",
		"P1Judge8Hold", "P1Judge9Hold",
		"P2JudgeSCHold", "P2Judge1Hold",
		"P2Judge2Hold", "P2Judge3Hold",
		"P2Judge4Hold", "P2Judge5Hold",
		"P2Judge6Hold", "P2Judge7Hold",
		"P2Judge8Hold", "P2Judge9Hold",
		// 90
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		// 100
		"P1KeySCPress", "P1Key1Press",
		"P1Key2Press", "P1Key3Press",
		"P1Key4Press", "P1Key5Press",
		"P1Key6Press", "P1Key7Press",
		"P1Key8Press", "P1Key9Press",
		"P2KeySCPress", "P2Key1Press",
		"P2Key2Press", "P2Key3Press",
		"P2Key4Press", "P2Key5Press",
		"P2Key6Press", "P2Key7Press",
		"P2Key8Press", "P2Key9Press",
		// 120
		"P1KeySCUp", "P1Key1Up",
		"P1Key2Up", "P1Key3Up",
		"P1Key4Up", "P1Key5Up",
		"P1Key6Up", "P1Key7Up",
		"P1Key8Up", "P1Key9Up",
		"P2KeySCUp", "P2Key1Up",
		"P2Key2Up", "P2Key3Up",
		"P2Key4Up", "P2Key5Up",
		"P2Key6Up", "P2Key7Up",
		"P2Key8Up", "P2Key9Up",
		// 140
		"Measure",
		empty_str_,
		empty_str_,
		"P1LastNote",
		"P2LastNote",
		empty_str_,
		empty_str_,
		empty_str_,
		empty_str_,
		empty_str_,
		// 150
		"Result",
		// end
		empty_str_,
	};
}

//
// Number / Text / Double value
// These values are calculated mostly at -
// - selected song changed
// - song decided
// -
// -
// -
//
// so keep track with these event handler.
//
namespace {
	const char* number_code_[300] = {
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 10
		"P1Speed", "P2Speed",
		"JudgeTiming", "TargetRate",
		"P1Sudden", "P2Sudden",
		"P1Lift", "P2Lift",			// actually, this doesn't supported in LR2
		empty_str_, empty_str_,
		// 20
		/* these attrivute will updated per OnTick */
		"FPS", "Year",
		"Month", "Day",
		"Hour", "Minute",
		"Second", empty_str_,
		empty_str_, empty_str_,
		// 30
		"PlayerPlayCount", "PlayerClearCount", "PlayerFailCount",
		empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_,
		// 40
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		"BeginnerLevel", "NormalLevel", "HyperLevel", "AnotherLevel", "InsaneLevel",
		// 50
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 70
		"Score", "ExScore",
		"ExScore", "Rate",
		"TotalNotes", "MaxCombo",
		"MinBP", "PlayCount",
		"ClearCount", "FailCount",
		// 80
		"Perfect", "Great", "Good", "Bad", "Poor",
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 90
		"BPMMax", "BPMMin", "IRRank", "IRTotal", "IRRate", "RivalDiff",
		empty_str_, empty_str_, empty_str_, empty_str_,
		// 100
		"P1Score", "P1ExScore",
		"P1Rate", "P1Rate_d",
		"P1Combo", "P1MaxCombo",
		"P1TotalNotes", "P1Gauge",
		"P1RivalDiff", empty_str_,
		"P1Perfect", "P1Great",
		"P1Good", "P1Bad", "P1Poor",
		"P1TotalRate", "P1TotalRate_d", empty_str_,
		empty_str_, empty_str_,
		// 120
		"P2Score", "P2ExScore",
		"P2Rate", "P2Rate_d",
		"P2Combo", "P2MaxCombo",
		"P2TotalNotes", "P2Gauge",
		"P2RivalDiff", empty_str_,
		"P2Perfect", "P2Great",
		"P2Good", "P2Bad", "P2Poor",
		"P2TotalRate", "P2TotalRate_d", empty_str_,
		empty_str_, empty_str_,
		// 140: mybest (useless, so ignore ...?)
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 160
		// - information related to play
		"PlayBPM", "PlayMinute", "PlaySecond", "PlayRemainMinute", "PlayRemainSecond", "PlayProgress",
		empty_str_, empty_str_, empty_str_, empty_str_,
		// 170
		"ResultExScoreBefore", "ResultExScoreNow", "ResultExScoreDiff",
		"ResultMaxComboBefore", "ResultMaxComboNow", "ResultMaxComboDiff",
		"ResultMinBPBefore", "ResultMinBPNow", "ResultMinBPDiff",
		"ResultIRNow", "ResultIRTotal", "ResultIRRate", "ResultIRBefore",
		"ResultRate", "ResultRate_d",
		// 185
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 190 ~ 270: none
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 270 (Rival)
		"P2Score", "P2ExScore",
		"P2Rate", "P2Rate_d",
		"P2Combo", "P2MaxCombo",
		"P2TotalNotes", "P2Gauge",
		"P2RivalDiff", empty_str_,
		"P2Perfect", "P2Great",
		"P2Good", "P2Bad", "P2Poor",
		"P2TotalRate", "P2TotalRate_d", empty_str_,
		empty_str_, empty_str_,
		// 290 ~ depreciated
		empty_str_,
	};

	const char* text_code_[300] = {
		empty_str_, "RivalName", "PlayerName",
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 10
		"Title", "SubTitle", "MainTitle",
		"Genre", "Artist", "SubArtist",
		"SearchTag", "PlayLevel", "PlayDifficulty", "TagLevel",
		// 20 ~ 30: for editing
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_, empty_str_,
		// 40
		"KeySlot0", "KeySlot1",
		"KeySlot2", "KeySlot3",
		"KeySlot4", "KeySlot5",
		"KeySlot6", "KeySlot7",
		empty_str_, empty_str_,
		// 50
		"SkinName", "SkinAuthor",
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		// 60
		// - play option related
		//   most of them are *depreciated*
		"PlayMode", "PlaySort", "PlayDifficulty", 
		"RandomP1", "RandomP2", "GaugeP1", "GaugeP2",
		"AssistP1", "AssistP2", "Battle", 
		// 70
		"Flip", "ScoreGraph", "Ghost", empty_str_, "ScrollType",
		"BGASize", "IsBGA", "ScreenColor", "VSync", "ScreenMode",
		// 80
		"AutoJudge", "ReplaySave",
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		// 90 ~ end
		empty_str_,
	};

	const char* graph_code_[300] = {
		empty_str_,
		"PlayProgress",
		"SongLoadProgress",
		"SongLoadProgress",
		"BeginnerLevel",
		"NormalLevel",
		"HyperLevel",
		"AnotherLevel",
		"InsaneLevel",
		"P1ExScore",
		"P1ExScoreEsti",
		"P1HighScore",
		"P1HighScoreEsti",
		"P2ExScore",
		"P2ExScoreEsti",
		"P2HighScore",
		"P2HighScoreEsti",
		empty_str_, empty_str_, empty_str_, empty_str_,
		// 20
		"P1Perfect",
		"P1Great",
		"P1Good",
		"P1Bad",
		"P1Poor",
		"P1MaxCombo",
		"P1Score",
		"P1ExScore",
		empty_str_, empty_str_,
		// 30
		"P2Perfect",
		"P2Great",
		"P2Good",
		"P2Bad",
		"P2Poor",
		"P2MaxCombo",
		"P2Score",
		"P2ExScore",
		empty_str_, empty_str_,
		// 40 - depreciated (maybe this is mybest player)
		empty_str_,
	};

	const char* slider_code_[100] = {
		empty_str_,
		"SelectBar",
		"P1HighSpeed",
		"P2HighSpeed",
		"P1Sudden",
		"P2Sudden",
		"PlayProgress",
		empty_str_, empty_str_, empty_str_,
		// 10
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, "Volume",
		empty_str_, empty_str_,
		// 20
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		empty_str_, empty_str_,
		"Pitch", empty_str_,
		empty_str_, empty_str_,
		// 30 - end
		empty_str_,
	};
}

//
// Button code
// - well ... button should decide what to execute,
//   but also decide what value it has. (in LR2)
//   so it's actually responsive image -
//   in Stepmania, ... (TODO)
//

namespace {
	const char* button_code_[100] = {
		empty_str_,
		"TogglePanel1()",
		"TogglePanel2()",
		"TogglePanel3()",
		"TogglePanel4()",
		"TogglePanel5()",
		"TogglePanel6()",
		"TogglePanel7()",
		"TogglePanel8()",
		"TogglePanel9()",
		// 10
		"ChangeDiff()",
		"ChangeMode()",
		"ChangeSort()",
		// 13
		"StartKeyConfig()",
		"StartSkinSetting()",
		"StartPlay()",
		"StartPlay()",			// this(autoplay) is depreciated. autoplay will be set in settings.
		"StartTextView()",
		empty_str_,
		"StartReplay()",
		// 20
		// TODO: keyconfig, skinselect
		empty_str_,
	};
}

// this really does translation.
namespace {
	// TODO: exception / buffer overflow process
	const char* TranslateOPs(int op) {
		/*
		 * In Rhythmus, there's no object called OP(condition) code. but, all conditions have timer code, and that does OP code's work.
		 * So this function will translate OP code into a valid condition code.
		 */
		int negative = 0;
		if (op < 0) {
			strcpy(translated, "!");
			op *= -1;
			negative = 1;
		}
		else {
			translated[0] = 0;
		}

		if (op == 0) {
			return "";
		}
		else if (op == 999) {
			strcpy(translated + negative, "false");
		}
		else if (op >= 900) {
			// #CUSTOMOPTION code
			itoa(op, translated + negative, 10);
		}
		else {
			const char* code = op_code_[op];
			if (!code) code = empty_str_;
			strcpy(translated + negative, code);
		}

		// remove double negative expression
		if (strncmp(translated, "!!", 2) == 0) {
			for (int i = 2; i <= strlen(translated); i++) {
				translated[i - 2] = translated[i];
			}
		}
		return translated;
	}

	const char* TranslateTimer(int timer) {
		const char* code = timer_code_[timer];
		if (!code) code = empty_str_;
		return code;
	}

	const char* TranslateButton(int code) {
		const char* ret = button_code_[code];
		if (!ret) ret = empty_str_;
		return ret;
	}

	const char* TranslateSlider(int code) {
		const char* ret = slider_code_[code];
		if (!ret) ret = empty_str_;
		return ret;
	}

	const char* TranslateGraph(int code) {
		const char* ret = graph_code_[code];
		if (!ret) ret = empty_str_;
		return ret;
	}

	const char* TranslateNumber(int code) {
		const char* ret = number_code_[code];
		if (!ret) ret = empty_str_;
		return ret;
	}

	const char* TranslateText(int code) {
		const char* ret = text_code_[code];
		if (!ret) ret = empty_str_;
		return ret;
	}
}

#pragma endregion
//
// translator end
//

// utility macros
#define ADDCHILD(base, name)\
	(base)->LinkEndChild((base)->GetDocument()->NewElement(name))
#define ADDTEXT(base, name, val)\
	((XMLElement*)ADDCHILD(base, name))->SetText(val);

// ---------------------------------------------------------------

bool _LR2SkinParser::ParseLR2Skin(const char *filepath, SkinMetric *s) {
	// fill basic information to skin
	Clear();
	this->sm = s;

	// load skin line
	line_total = LoadSkin(filepath);
	if (!line_total) {
		printf("LR2Skin Warning - Cannot find file (%s)\n", filepath);
		return false;
	}

	XMLElement *skin = s->tree.NewElement("skin");
	s->tree.LinkEndChild(skin);
	ADDCHILD(skin, "option");

	// after we read all lines, skin parse start
	filter_to_optionname.clear();			// clear filter_to_optionname in here.
	condition_element[0] = cur_e = skin;
	condition_level = 0;
	currentline = 0;
	while (currentline >= 0 && currentline < line_total) {
		ParseSkinMetricLine();
	}

	// don't check condition here
	return true;
}

bool _LR2SkinParser::ParseCSV(const char *filepath, Skin *s) {
	// fill basic information to skin
	Clear();
	this->s = s;

	strcpy(s->filepath, filepath);
	XMLComment *cmt = s->skinlayout.NewComment("Auto-generated code.\n"
		"Converted from LR2Skin.\n"
		"LR2 project origins from lavalse, All rights reserved.");
	s->skinlayout.LinkEndChild(cmt);
	/* 4 elements are necessary */
	XMLElement *skin = s->skinlayout.NewElement("skin");
	s->skinlayout.LinkEndChild(skin);

	// load skin line
	line_total = LoadSkin(filepath);
	if (!line_total) {
		printf("LR2Skin Warning - Cannot find file (%s)\n", filepath);
		return false;
	}

	// after we read all lines, skin parse start
	condition_element[0] = cur_e = skin;
	condition_level = 0;
	currentline = 0;
	while (currentline >= 0 && currentline < line_total) {
		ParseSkinCSVLine();
	}


	if (condition_level == 0) {
		printf("lr2skin(%s) parsing successfully done\n", filepath);
		return true;
	}
	else {
		// if this value is the final output,
		// then there might be some #ENDIF was leaked.
		printf("LR2Skin Warning - invalid #IF clause seems existing (not closed)\n");
		return false;
	}
}

int _LR2SkinParser::LoadSkin(const char *filepath) {
	int lines = 0;
	FILE *f = fopen(filepath, "r");
	if (!f) {
		printf("LR2Skin - Cannot find Skin file %s - ignore\n", filepath);
		return 0;
	}
	strcpy(this->filepath, filepath);

	char line[MAX_LINE_CHARACTER_];
	char *p;
	int current_line = 0;		// current file's reading line
	while (!feof(f)) {
		current_line++;
		if (!fgets(line, 1024, f))
			break;
		p = Trim(line);

		// ignore comment
		// (or push the line as comment?)
		if (strncmp("//", p, 2) == 0 || !strlen(p))
			continue;

		line_v_ line__;
		strcpy(line__.line__, line);
		lines_.push_back(line__);
		lines++;
	}
	fclose(f);

	// parse argument
	for (int i = 0; i < lines_.size(); i++) {
		args_v_ line_args__;
		line_v_* line__stored_ = &lines_[i];
		SplitCSVLine(line__stored_->line__, line_args__.args__);
		line_args_.push_back(line_args__);
	}

	// returns how much lines we read
	return lines;
}

void _LR2SkinParser::SplitCSVLine(char *p, const char **args) {
	// first element is element name
	args[0] = p;
	int i;
	for (i = 1; i < MAX_ARGS_COUNT_; i++) {
		p = strchr(p, ',');
		if (!p)
			break;
		*p = 0;
		args[i] = (++p);
	}
	// for safety, pack left argument as ""
	for (; i < MAX_ARGS_COUNT_; i++) {
		args[i] = empty_str_;
	}
}

/* a simple private macro for PLAYLANE (reset position) */
void MakeFrameRelative(int x, int y, XMLElement *frame) {
	frame->SetAttribute("x", frame->IntAttribute("x") - x);
	frame->SetAttribute("y", frame->IntAttribute("y") - y);
}
void MakeRelative(int x, int y, XMLElement *e) {
	XMLElement *dst = e->FirstChildElement("DST");
	while (dst) {
		XMLElement *frame = dst->FirstChildElement("frame");
		while (frame) {
			MakeFrameRelative(x, y, frame);
			frame = frame->NextSiblingElement("frame");
		}
		dst = e->NextSiblingElement("DST");
	}
}

/* *********************************************************
 * LR2 csv parsing start
 * *********************************************************/

#define CMD_IS(v) (strcmp(args[0], (v)) == 0)
#define OBJTYPE_IS(v) (strcmp(args[0]+5, (v)) == 0)
#define CMD_STARTSWITH(v,l) (strncmp(args[0], (v), (l)) == 0)
#define INT(v) (atoi(v))
bool _LR2SkinParser::ProcessCondition(const args_read_& args) {
	bool cond = false;
	if (CMD_IS("#ENDIF")) {
		// we just ignore this statement, so only 1st-level parsing is enabled.
		// but if previous #IF clause exists, then close it
		if (condition_level > 0)
			cur_e = condition_element[--condition_level];
		else
			printf("LR2Skin - Invalid #ENDIF (%d)\n", currentline);
		cond = true;
	}
	else if (CMD_IS("#ELSE")) {
		XMLElement *cond_ = cur_e->Parent()->ToElement();
		XMLElement *obj_ = (XMLElement*)ADDCHILD(cond_, "else");
		cur_e = obj_;
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			cls.AddCondition(TranslateOPs(INT(args[i])));
		}
		cond = true;
	}
	else if (CMD_IS("#IF")) {
		//
		// xml structure
		//
		// condition
		//   if
		//     ...
		//   /if
		//   elseif
		//     ...
		//   /elseif
		//   else
		//   ...
		//   /else
		// /condition
		//
		condition_element[condition_level] = cur_e;
		XMLElement *cond_ = (XMLElement*)ADDCHILD(cur_e, "condition");
		XMLElement *obj_ = (XMLElement*)ADDCHILD(cond_, "if");
		cur_e = obj_;
		condition_level++;
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			cls.AddCondition(TranslateOPs(INT(args[i])));
		}
		obj_->SetAttribute("condition", cls.ToString());
		cond = true;
	}
	else if (CMD_IS("#ELSEIF")) {
		XMLElement *cond_ = cur_e->Parent()->ToElement();
		XMLElement *obj_ = (XMLElement*)ADDCHILD(cond_, "elseif");
		cur_e = obj_;
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			cls.AddCondition(TranslateOPs(INT(args[i])));
		}
		obj_->SetAttribute("condition", cls.ToString());
		cond = true; 
	}

	return cond;
}

#define CMD_IS_(v) (strcmp(cmd, (v)) == 0)
bool _LR2SkinParser::IsMetadata(const char* cmd) {
	if (CMD_IS_("#CUSTOMOPTION") ||
		CMD_IS_("#CUSTOMFILE") ||
		CMD_IS_("#INFORMATION"))
		return true;
	else
		return false;
}

bool _LR2SkinParser::ProcessMetadata(const args_read_& args) {
	if (CMD_IS("#CUSTOMOPTION")) {
		XMLElement *option = FindElement(cur_e, "option", &s->skinlayout);
		XMLElement *customoption = ADDCHILD(option, "customswitch")->ToElement();
		option->LinkEndChild(customoption);

		std::string name_safe = args[1];
		ReplaceString(name_safe, " ", "_");
		customoption->SetAttribute("name", name_safe.c_str());
		int option_intvalue = atoi(args[2]);
		std::string desc_txt = "";
		std::string val_txt = "";
		for (const char **p = args + 3; *p != 0 && strlen(*p) > 0; p++) {
			desc_txt += *p;
			desc_txt.push_back(';');
			char t_[10];
			itoa(option_intvalue, t_, 10);
			val_txt += t_;
			val_txt.push_back(';');
		}
		desc_txt.pop_back();
		val_txt.pop_back();
		customoption->SetAttribute("valuename", desc_txt.c_str());
		customoption->SetAttribute("value", val_txt.c_str());
		return true;
	}
	else if (CMD_IS("#CUSTOMFILE")) {
		XMLElement *option = FindElement(cur_e, "option", &s->skinlayout);
		XMLElement *customfile = ADDCHILD(option, "customfile")->ToElement();

		std::string name_safe = args[1];
		ReplaceString(name_safe, " ", "_");
		customfile->SetAttribute("name", name_safe.c_str());
		// decide file type
		std::string path_safe = args[2];
		ReplaceString(path_safe, "\\", "/");
		if (FindString(path_safe.c_str(), "*.jpg") || FindString(path_safe.c_str(), "*.png"))
			customfile->SetAttribute("type", "file/image");
		else if (FindString(path_safe.c_str(), "*.mpg") || FindString(path_safe.c_str(), "*.mpeg") || FindString(path_safe.c_str(), "*.avi"))
			customfile->SetAttribute("type", "file/video");
		else if (FindString(path_safe.c_str(), "*.ttf"))
			customfile->SetAttribute("type", "file/ttf");
		else if (FindString(path_safe.c_str(), "/*"))
			customfile->SetAttribute("type", "folder");		// it's somewhat ambiguous...
		else
			customfile->SetAttribute("type", "file");
		std::string path = args[2];
		ReplaceString(path, "*", args[3]);
		ConvertLR2PathToRelativePath(path);
		customfile->SetAttribute("path", path.c_str());		// means default path

		// register to filter_to_optionname for image change
		AddPathToOption(args[2], name_safe);
		XMLComment *cmt = sm->tree.NewComment(args[2]);
		option->LinkEndChild(cmt);
		return true;
	}
	else if (CMD_IS("#INFORMATION")) {
		// set skin's metadata
		XMLElement *info = (XMLElement*)ADDCHILD(cur_e, "info");
		ADDTEXT(info, "width", 1280);
		ADDTEXT(info, "height", 720);
		int type_ = INT(args[1]);
		if (type_ == 5) {
			ADDTEXT(info, "type", "Select");
		}
		else if (type_ == 6) {
			ADDTEXT(info, "type", "Decide");
		}
		else if (type_ == 7) {
			ADDTEXT(info, "type", "Result");
		}
		else if (type_ == 8) {
			ADDTEXT(info, "type", "KeyConfig");
		}
		/** @comment skinselect / soundselect are all depreciated, integrated into option. */
		else if (type_ == 9) {
			ADDTEXT(info, "type", "SkinSelect");
		}
		else if (type_ == 10) {
			ADDTEXT(info, "type", "SoundSelect");
		}
		/** @comment end */
		else if (type_ == 12) {
			ADDTEXT(info, "type", "Play");
			ADDTEXT(info, "key", 15);
		}
		else if (type_ == 13) {
			ADDTEXT(info, "type", "Play");
			ADDTEXT(info, "key", 17);
		}
		else if (type_ == 15) {
			ADDTEXT(info, "type", "CourseResult");
		}
		else if (type_ < 5) {
			ADDTEXT(info, "type", "Play");
			int key_ = 7;
			switch (type_) {
			case 0:
				// 7key
				key_ = 7;
				break;
			case 1:
				// 9key
				key_ = 5;
				break;
			case 2:
				// 14key
				key_ = 14;
				break;
			case 3:
				key_ = 10;
				break;
			case 4:
				key_ = 9;
				break;
			}
			ADDTEXT(info, "key", key_);
		}
		else {
			printf("LR2Skin error: unknown type of lr2skin(%d). consider as 7Key Play.\n", type_);
			type_ = 0;
		}

		ADDTEXT(info, "name", args[2]);
		ADDTEXT(info, "author", args[3]);
		return true;
	}
	return false;
}

bool _LR2SkinParser::ProcessResource(const args_read_& args) {
	//
	// all resources are splited into 
	// - id
	// - resource path
	// LR2 has not so modern style skin structure :p
	//
	if (CMD_IS("#IMAGE")) {
		XMLElement *resource = s->skinlayout.FirstChildElement("skin");
		XMLElement *image = s->skinlayout.NewElement("image");
		image->SetAttribute("name", image_cnt++);
		// check for optionname path
		std::string path_converted = args[1];
		for (auto it = filter_to_optionname.begin(); it != filter_to_optionname.end(); ++it) {
			if (FindString(args[1], it->first.c_str())) {
				// replace filtered path to reserved name
				ReplaceString(path_converted, it->first, "$(" + it->second + ")");
				break;
			}
		}
		// convert path to relative
		ReplaceString(path_converted, "\\", "/");
		ConvertLR2PathToRelativePath(path_converted);
		image->SetAttribute("path", path_converted.c_str());

		resource->InsertFirstChild(image);

		return true;
	}
	else if (CMD_IS("#LR2FONT")) {
		// we don't use bitmap fonts
		// So if cannot found, we'll going to use default font/texture.
		// current font won't support TTF, so basically we're going to use default font.
		XMLElement *resource = s->skinlayout.FirstChildElement("skin");
		XMLElement *font = s->skinlayout.NewElement("font");
		font->SetAttribute("name", font_cnt++);
		font->SetAttribute("path", "default");
		int size = 18;
		if (strstr(args[1], "small")) size = 15;
		if (strstr(args[1], "title")
			|| strstr(args[1], "big")
			|| strstr(args[1], "large"))
			size = 42;
		if (size > 20)
			font->SetAttribute("texturepath", "default");
		font->SetAttribute("size", size);
#if 0
		/*
		* these are available in #FONT, not #LR2FONT
		*/
		switch (INT(args[3])) {
		case 0:
			// normal
			font->SetAttribute("style", "normal");
			break;
		case 1:
			// italic
			font->SetAttribute("style", "italic");
			break;
		case 2:
			// bold
			font->SetAttribute("style", "bold");
			break;
		}
		font->SetAttribute("thickness", INT(args[2]));
#endif
		font->SetAttribute("border", size / 20 + 1);
		resource->InsertFirstChild(font);
		return true;
	}
	return false;
}

bool _LR2SkinParser::ProcessDepreciated(const args_read_& args) {
	if (CMD_IS("#FONT") ||
		CMD_IS("#ENDOFHEADER") ||
		CMD_IS("#FLIPRESULT") ||
		CMD_IS("TRANSCLOLR")) 
	{
		return true;
	}
	return false;
}

void _LR2SkinParser::ParseSkinMetricLine() {
	args_read_ args;			// contains linebuffer's address
	args = line_args_[currentline].args__;
	int objectid = INT(args[1]);

	/*
	 * depreciated ones
	 */
	if (ProcessDepreciated(args)) {
		printf("LR2Skin - %s is depreciated command, ignore.\n", args[0]);
		currentline++;
		return;
	}

	/*
	 * header/metadata parsing
	 */
	if (ProcessMetadata(args)) {
		currentline++;
		return;
	}

	// ignore all other elements

	currentline++;
}

void _LR2SkinParser::ParseSkinCSVLine() {
	// get current line's string & argument
	args_read_ args;			// contains linebuffer's address
	args = line_args_[currentline].args__;
	int objectid = INT(args[1]);

	/*
	 * depreciated ones
	 */
	if (ProcessDepreciated(args)) {
		printf("LR2Skin - %s is depreciated command, ignore.\n", args[0]);
		currentline++;
		return;
	}

	/*
	 * Is it conditional clause?
	 */
	if (ProcessCondition(args)) {
		currentline++;
		return;
	}

	/*
	 * header/metadata parsing
	 * - ignore metadata here
	 */
	if (IsMetadata(args[0])) {
		currentline++;
		return;
	}

	/*
	 * resource parsing
	 */
	if (ProcessResource(args)) {
		currentline++;
		return;
	}


	if (CMD_IS("#INCLUDE")) {
		XMLElement *e = (XMLElement*)ADDCHILD(cur_e, "include");

		std::string relpath = args[1];
		std::string basepath = filepath;
		ConvertLR2PathToRelativePath(relpath);
		//GetParentDirectory(basepath);
		//ConvertRelativePathToAbsPath(relpath, basepath);

		e->SetAttribute("path", relpath.c_str());
	}
	else if (CMD_IS("#SETOPTION")) {
		// this clause is translated during render tree construction
		XMLElement *setoption = s->skinlayout.NewElement("lua");
		std::ostringstream luacode;
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			const char *c = INT(args[i]) ? TranslateOPs(INT(args[i])) : 0;
			if (c)
				luacode << "SetTimer(\"" << c << "\")\n";
		}
		setoption->SetText(("\n" + luacode.str()).c_str());
		cur_e->LinkEndChild(setoption);
	}
	else if (CMD_STARTSWITH("#SRC_", 4)){
		// we parse #DST with #SRC.
		// process SRC
		// SRC may have condition (attribute condition; normally not used)
		XMLElement *obj;
		obj = s->skinlayout.NewElement("sprite");
		int resid = INT(args[2]);	// COMMENT: check out for pre-occupied resid
		switch (resid) {
		case 100:
			obj->SetAttribute("resid", "_stagefile");
			break;
		case 101:
			obj->SetAttribute("resid", "_backbmp");
			break;
		case 102:
			obj->SetAttribute("resid", "_banner");
			break;
		case 110:
			obj->SetAttribute("resid", "_black");
			break;
		case 111:
			obj->SetAttribute("resid", "_white");
			break;
		default:
			obj->SetAttribute("resid", resid);
			break;
		}
		obj->SetAttribute("x", INT(args[3]));
		obj->SetAttribute("y", INT(args[4]));
		if (INT(args[5]) > 0) {
			obj->SetAttribute("w", INT(args[5]));
			obj->SetAttribute("h", INT(args[6]));
		}
		if (INT(args[7]) > 1 || INT(args[8]) > 1) {
			obj->SetAttribute("divx", INT(args[7]));
			obj->SetAttribute("divy", INT(args[8]));
		}
		if (INT(args[9]))
			obj->SetAttribute("cycle", INT(args[9]));
		int sop1 = 0, sop2 = 0, sop3 = 0, sop4 = 0;		// sop4 used in scratch rotation
		if (INT(args[10]))
			obj->SetAttribute("timer", TranslateTimer(INT(args[10])));
		sop1 = INT(args[11]);
		sop2 = INT(args[12]);
		sop3 = INT(args[13]);
		sop4 = INT(args[14]);

		/*
		 * process NOT-general-objects first
		 * these objects doesn't have #DST object directly
		 * (bad-syntax >:( )
		 */
		// check for play area
		int isPlayElement = ProcessLane(args, obj, resid);
		if (isPlayElement) {
			currentline += isPlayElement;
			return;
		}

		/*
		 * under these are objects which requires #DST object directly
		 * (good syntax)
		 * we have to make object now to parse #DST
		 * and, if unable to figure out what this element is, it'll be considered as Image object.
		 */

		int looptime = 0, blend = 0, timer = 0, rotatecenter = -1, acc = 0;
		int op1 = 0, op2 = 0, op3 = 0;
		XMLElement *dst = s->skinlayout.NewElement("DST");
		obj->LinkEndChild(dst);
		for (int nl = currentline + 1; nl < line_total; nl++) {
			args_read_ args = line_args_[nl].args__;
			if (!args[0]) continue;
			if (CMD_IS("#ENDIF"))
				continue;			// we can ignore #ENDIF command, maybe
			if (!CMD_STARTSWITH("#DST_", 5))
				break;
			// if it's very first line (parse basic element)
			if (rotatecenter < 0) {
				looptime = INT(args[16]);
				blend = INT(args[12]);
				rotatecenter = INT(args[15]);
				timer = INT(args[17]);
				acc = INT(args[7]);
				if (args[18]) op1 = INT(args[18]);
				if (args[19]) op2 = INT(args[19]);
				if (args[20]) op3 = INT(args[20]);
			}
			XMLElement *frame = s->skinlayout.NewElement("frame");
			frame->SetAttribute("x", args[3]);
			frame->SetAttribute("y", args[4]);
			frame->SetAttribute("w", args[5]);
			frame->SetAttribute("h", args[6]);
			frame->SetAttribute("time", args[2]);
			if (!(INT(args[8]) == 255))
				frame->SetAttribute("a", args[8]);
			if (!(INT(args[9]) == 255 && INT(args[10]) == 255 && INT(args[11]) == 255)) {
				frame->SetAttribute("r", args[9]);
				frame->SetAttribute("g", args[10]);
				frame->SetAttribute("b", args[11]);
			}
			if (INT(args[14]))
				frame->SetAttribute("angle", args[14]);
			dst->LinkEndChild(frame);
		}
		// set common draw attribute
		dst->SetAttribute("acc", acc);
		if (blend > 1)
			dst->SetAttribute("blend", blend);
		if (looptime >= 0)
			dst->SetAttribute("loop", looptime);
		if (rotatecenter > 0)
			dst->SetAttribute(
			"rotatecenter", rotatecenter);
		if (TranslateTimer(timer))
			dst->SetAttribute("timer", TranslateTimer(timer));
		ConditionAttribute cls;
		const char *c;
		c = op1?TranslateOPs(op1):0;
		if (c) cls.AddCondition(c);
		c = op2?TranslateOPs(op2):0;
		if (c) cls.AddCondition(c);
		c = op3?TranslateOPs(op3):0;
		if (c) cls.AddCondition(c);
		if (cls.GetConditionNumber())
			obj->SetAttribute("condition", cls.ToString());

		/*
		* If object is select screen panel dependent(timer/op code 2x),
		* then add object to there (ease of control)
		* TODO: take care of 3x objects (OnPanelClose)
		*/
#define CHECK_PANEL(v) (op1 == (v) || op2 == (v) || op3 == (v) || timer == (v))
		if (CHECK_PANEL(21))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel1", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(22))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel2", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(23))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel3", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(24))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel4", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(25))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel5", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(26))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel6", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(27))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel7", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(28))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel8", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(29))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel9", &s->skinlayout)->LinkEndChild(obj);
		else
			cur_e->LinkEndChild(obj);
		

		/*
		 * Check out for some special object (which requires #DST object)
		 * COMMENT: most of them behaves like #IMAGE object.
		 */
		// combo (play)
		int isComboElement = ProcessCombo(args, obj);
		if (isComboElement) {
			currentline += isComboElement;
			return;
		}

		// select menu (select)
		int isSelectBar = ProcessSelectBar(args, obj);
		if (isSelectBar) {
			currentline += isSelectBar;
			return;
		}

		/* 
		 * under these are general individual object
		 */
		if (OBJTYPE_IS("IMAGE")) {
			// nothing to do (general object)
			// but it's not in some special OP/Timer code
			// - include BOMB/LANE effect into PLAYLANE object
			// (we may want to include BOMB/OnBeat, but it's programs's limit. Can't do it now.)
			// (do it yourself)
			if (!(obj->IntAttribute("w") < 100 && obj->IntAttribute("h") < 100)) {
				if (//(timer >= 50 && timer < 60) ||
					(timer >= 70 && timer < 80) ||
					(timer >= 100 && timer < 110) ||
					(timer >= 120 && timer < 130)) {
					// P1
					XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", 0, &s->skinlayout);
					MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
					playarea->LinkEndChild(obj);
				}
				else if (//(timer >= 60 && timer < 70) ||
					(timer >= 80 && timer < 90) ||
					(timer >= 110 && timer < 120) ||
					(timer >= 130 && timer < 140)) {
					// P2
					XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", 1, &s->skinlayout);
					MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
					playarea->LinkEndChild(obj);
				}
			}
			// if sop4 == 1, then change object to scratch
			if (sop4) {
				obj->SetName("scratch");
			}
			// BOMB SRC effect(SRC loop) MUST TURN OFF; don't loop.
			if ((timer >= 50 && timer < 60) || (timer >= 60 && timer < 70))
				obj->SetAttribute("loop", 0);
		}
		else if (OBJTYPE_IS("BGA")) {
			// change tag to BGA and remove SRC tag
			obj->SetName("bga");
			// set bga side & remove redundant tag
			// (LR2 doesn't support 'real' battle mode, so no side attribute.)
			obj->SetAttribute("side", 0);
			obj->DeleteAttribute("resid");
		}
		else if (OBJTYPE_IS("NUMBER")) {
			obj->SetName("number");
			ProcessNumber(obj, sop1, sop2, sop3);
		}
		else if (OBJTYPE_IS("SLIDER")) {
			// change tag to slider and add attr
			obj->SetName("slider");
			obj->SetAttribute("direction", sop1);
			obj->SetAttribute("range", sop2);
			if (TranslateSlider(sop3))
				obj->SetAttribute("value", TranslateSlider(sop3));
			//obj->SetAttribute("range", sop2); - disable option is ignored
		}
		else if (OBJTYPE_IS("TEXT")) {
			// delete src attr and change to font/st/align/edit
			obj->DeleteAttribute("x");
			obj->DeleteAttribute("y");
			obj->DeleteAttribute("w");
			obj->DeleteAttribute("h");
			obj->DeleteAttribute("cycle");
			obj->DeleteAttribute("divx");
			obj->DeleteAttribute("divy");
			obj->SetName("text");
			if (TranslateText(INT(args[3])))
				obj->SetAttribute("value", TranslateText(INT(args[3])));
			obj->SetAttribute("align", args[4]);
			obj->SetAttribute("edit", args[5]);
		}
		else if (OBJTYPE_IS("BARGRAPH")) {
			obj->SetName("graph");
			if (TranslateGraph(sop1))
				obj->SetAttribute("value", TranslateGraph(sop1));
			obj->SetAttribute("direction", sop2);
		}
		else if (OBJTYPE_IS("BUTTON")) {
			// TODO: onclick event
			obj->SetName("button");
		}
		/* 
		 * some special object (PLAY lane object) 
		 */
		else if (OBJTYPE_IS("GROOVEGAUGE")) {
			int side = INT(args[1]);
			int addx = sop1;
			int addy = sop2;
			obj->SetAttribute("side", side);
			obj->SetAttribute("addx", addx);
			obj->SetAttribute("addy", addy);
			// process SRC and make new SRC elements
			int x = obj->IntAttribute("x");
			int y = obj->IntAttribute("y");
			int w = obj->IntAttribute("w");
			int h = obj->IntAttribute("h");
			int divx = obj->IntAttribute("divx");
			int divy = obj->IntAttribute("divy");
			int c = divx * divy;
			int dw = w / divx;
			int dh = h / divy;
			XMLElement *active, *inactive;
			active = s->skinlayout.NewElement("SRC_GROOVE_ACTIVE");
			inactive = s->skinlayout.NewElement("SRC_GROOVE_INACTIVE");
			active->SetAttribute("x", x + dw * (1 % divx));
			active->SetAttribute("y", y + dh * (1 / divx % divy));
			active->SetAttribute("w", dw);
			active->SetAttribute("h", dh);
			inactive->SetAttribute("x", x + dw * (3 % divx));
			inactive->SetAttribute("y", y + dh * (3 / divx % divy));
			inactive->SetAttribute("w", dw);
			inactive->SetAttribute("h", dh);
			obj->LinkEndChild(active);
			obj->LinkEndChild(inactive);

			active = s->skinlayout.NewElement("SRC_HARD_ACTIVE");
			inactive = s->skinlayout.NewElement("SRC_HARD_INACTIVE");
			active->SetAttribute("x", x);
			active->SetAttribute("y", y);
			active->SetAttribute("w", dw);
			active->SetAttribute("h", dh);
			inactive->SetAttribute("x", x + dw * (2 % divx));
			inactive->SetAttribute("y", y + dh * (2 / divx % divy));
			inactive->SetAttribute("w", dw);
			inactive->SetAttribute("h", dh);
			obj->LinkEndChild(active);
			obj->LinkEndChild(inactive);

			active = s->skinlayout.NewElement("SRC_EX_ACTIVE");
			inactive = s->skinlayout.NewElement("SRC_EX_INACTIVE");
			active->SetAttribute("x", x);
			active->SetAttribute("y", y);
			active->SetAttribute("w", dw);
			active->SetAttribute("h", dh);
			inactive->SetAttribute("x", x + dw * (2 % divx));
			inactive->SetAttribute("y", y + dh * (2 / divx % divy));
			inactive->SetAttribute("w", dw);
			inactive->SetAttribute("h", dh);
			obj->LinkEndChild(active);
			obj->LinkEndChild(inactive);

			// remove obsolete SRC
			obj->DeleteAttribute("x");
			obj->DeleteAttribute("y");
			obj->DeleteAttribute("w");
			obj->DeleteAttribute("h");
			obj->DeleteAttribute("divx");
			obj->DeleteAttribute("divy");
			obj->SetName("groovegauge");
		}
		else if (OBJTYPE_IS("JUDGELINE")) {
			obj->SetName("judgeline");
			XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid, &s->skinlayout);
			MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
			playarea->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("LINE")) {
			obj->SetName("line");
			XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid, &s->skinlayout);
			MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
			playarea->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("ONMOUSE")) {
			// depreciated/ignore
			// TODO: support this by SRC_HOVER tag.
			printf("#XXX_ONMOUSE command is depreciated, ignore. (%dL) \n", currentline);
		}
		else {
			printf("Unknown General Object (%s), consider as IMAGE. (%dL)\n", args[0] + 5, currentline);
		}

		// return new line
		currentline+=1;
		return;
	}
	/*
	 * SELECT part 
	 */
	else if (CMD_STARTSWITH("#DST_BAR_BODY", 13)) {
		// select menu part
		int isProcessSelectBarDST = ProcessSelectBar_DST(args);
		if (isProcessSelectBarDST) {
			currentline += isProcessSelectBarDST;
			return;
		}
	}
	else if (CMD_IS("#BAR_CENTER")) {
		// set center and property ...
		XMLElement *selectmenu = FindElement(cur_e, "selectmenu");
		selectmenu->SetAttribute("center", args[1]);
	}
	else if (CMD_IS("#BAR_AVAILABLE")) {
		// depreciated, not parse
		printf("#BAR_AVAILABLE - depreciated, Ignore.\n");
	}
	/*
	 * PLAY part 
	 */
	else if (CMD_STARTSWITH("#DST_NOTE", 9)) {
		int objectid = INT(args[1]);
		XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "note", "index", objectid, &s->skinlayout);
		lane->SetAttribute("x", INT(args[3]) - playarea->IntAttribute("x"));
		lane->SetAttribute("y", INT(args[4]) - playarea->IntAttribute("y"));
		lane->SetAttribute("w", INT(args[5]));
		lane->SetAttribute("h", INT(args[6]));
	}
	/*
	 * etc
	 */
	else if (CMD_STARTSWITH("#DST_", 5)) {
		// just ignore
	}
	else {
		printf("LR2Skin - Unknown Type: %s (%dL) - Ignore.\n", args[0], currentline);
	}

	// parse next line
	currentline++;
}

// comment: maybe I need to process it with namespace ...?
void _LR2SkinParser::ProcessNumber(XMLElement *obj, int sop1, int sop2, int sop3) {
	// just convert SRC to texturefont ...
	ConvertToTextureFont(obj);
	/*
	* Number object will act just like a extended-string object.
	* If no value, then it'll just show '0' value.
	*/
	if (TranslateNumber(sop1))
		obj->SetAttribute("value", TranslateNumber(sop1));
	/*
	* LR2's NUMBER alignment is a bit strange ...
	* why is it different from string's alignment ... fix it
	*/
	if (sop2 == 2)
		sop2 = 1;
	else if (sop2 == 1)
		sop2 = 2;
	else if (sop2 == 0)
		sop2 = 0;
	obj->SetAttribute("align", sop2);
	/*
	* if type == 11 or 24, then set length
	* if type == 24, then set '24mode' (only for LR2 - depreciated supportance)
	* (If you want to implement LR2-like font, then you may have to make 2 type of texturefont SRC -
	* plus and minus - with proper condition.)
	*/
	if (sop3)
		obj->SetAttribute("length", sop3);

	// remove some attrs
	obj->DeleteAttribute("x");
	obj->DeleteAttribute("y");
	obj->DeleteAttribute("w");
	obj->DeleteAttribute("h");
	obj->DeleteAttribute("divx");
	obj->DeleteAttribute("divy");
	obj->DeleteAttribute("cycle");
}

/*
 * process commands about lane
 * if not lane, return 0
 * if lane, return next parsed line
 */
int _LR2SkinParser::ProcessLane(const args_read_& args, XMLElement *src, int resid) {
	int objectid = INT(args[1]);

#define SETNOTE(name)\
	XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid / 10, &s->skinlayout);\
	XMLElement *lane = FindElementWithAttribute(playarea, "note", "index", objectid, &s->skinlayout);\
	lane->SetAttribute("resid", src->Attribute("resid"));\
	src->DeleteAttribute("resid");\
	src->SetName(name);\
	lane->LinkEndChild(src);\
	return 1;
	if (OBJTYPE_IS("NOTE")) {
		SETNOTE("SRC_NOTE");
	}
	else if (OBJTYPE_IS("LN_END")) {
		SETNOTE("SRC_LN_END");
	}
	else if (OBJTYPE_IS("LN_BODY")) {
		SETNOTE("SRC_LN_BODY");
	}
	else if (OBJTYPE_IS("LN_START")) {
		SETNOTE("SRC_LN_START");
	}
	else if (OBJTYPE_IS("MINE")) {
		SETNOTE("SRC_MINE");
	}
	if (OBJTYPE_IS("AUTO_NOTE")) {
		SETNOTE("SRC_AUTO_NOTE");
	}
	else if (OBJTYPE_IS("AUTO_LN_END")) {
		SETNOTE("SRC_AUTO_LN_END");
	}
	else if (OBJTYPE_IS("AUTO_LN_BODY")) {
		SETNOTE("SRC_AUTO_LN_BODY");
	}
	else if (OBJTYPE_IS("AUTO_LN_START")) {
		SETNOTE("SRC_AUTO_LN_START");
	}
	else if (OBJTYPE_IS("AUTO_MINE")) {
		SETNOTE("SRC_AUTO_MINE");
	}
	else if (OBJTYPE_IS("JUDGELINE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid, &s->skinlayout);
		// find DST object to set Lane attribute
		for (int _l = currentline; _l < line_total; _l++) {
			if (strcmp(line_args_[_l].args__[0], "#DST_JUDGELINE") == 0 &&
				INT(line_args_[_l].args__[1]) == objectid) {
				int x = INT(line_args_[_l].args__[3]);
				int y = 0;
				int w = INT(line_args_[_l].args__[5]);
				int h = INT(line_args_[_l].args__[4]);
				playarea->SetAttribute("x", x);
				playarea->SetAttribute("y", y);
				playarea->SetAttribute("w", w);
				playarea->SetAttribute("h", h);
				XMLElement *dst = FindElement(playarea, "DST", &s->skinlayout);
				XMLElement *frame = FindElement(dst, "frame", &s->skinlayout);
				frame->SetAttribute("x", x);
				frame->SetAttribute("y", y);
				frame->SetAttribute("w", w);
				frame->SetAttribute("h", h);
				break;
			}
		}
		// but don't add SRC; we'll add it to PlayArea
		return 0;
	}

	// not an play object
	return 0;
}

std::string _getcomboconditionstring(int player, int level) {
	char buf[256];
	sprintf(buf, "OnP%dJudge", player);

	switch (level) {
	case 0:
		return std::string(buf) + "NPoor";
		break;
	case 1:
		return std::string(buf) + "Poor";
		break;
	case 2:
		return std::string(buf) + "Bad";
		break;
	case 3:
		return std::string(buf) + "Good";
		break;
	case 4:
		return std::string(buf) + "Great";
		break;
	case 5:
		return std::string(buf) + "Perfect";
		break;
	default:
		/* this shouldn't be happened */
		return "";
		break;
	}
}
int __comboy = 0;
int __combox = 0;
int _LR2SkinParser::ProcessCombo(const args_read_& args, XMLElement *obj) {
	int objectid = INT(args[1]);
	int sop1 = 0, sop2 = 0, sop3 = 0;
	if (args[11]) sop1 = INT(args[11]);
	if (args[12]) sop2 = INT(args[12]);
	if (args[13]) sop3 = INT(args[13]);

#define GETCOMBOOBJ(side)\
	std::string cond = _getcomboconditionstring(side, objectid);\
	XMLElement *playcombo = FindElementWithAttribute(cur_e, "combo", "condition", cond.c_str(), &s->skinlayout);
	if (OBJTYPE_IS("NOWJUDGE_1P")) {
		GETCOMBOOBJ(1);
		obj->SetName("sprite");
		playcombo->LinkEndChild(obj);
		__comboy = obj->FirstChildElement("DST")->FirstChildElement("frame")->IntAttribute("y");
		__combox = obj->FirstChildElement("DST")->FirstChildElement("frame")->IntAttribute("x");
		return 1;
	}
	else if (OBJTYPE_IS("NOWCOMBO_1P")) {
		GETCOMBOOBJ(1);
		obj->SetName("number");
		ProcessNumber(obj, 0, 0, 0);
		obj->SetAttribute("value", "P1Combo");
		obj->SetAttribute("align", 1);
		for (XMLElement *e = obj->FirstChildElement("DST")->FirstChildElement("frame"); e;) {
			e->SetAttribute("y", __comboy);
			e->SetAttribute("x", e->IntAttribute("x") + __combox);
			e->SetAttribute("w", 0);
			e = e->NextSiblingElement("frame");
		}
		playcombo->LinkEndChild(obj);
		return 1;
	}
	else if (OBJTYPE_IS("NOWJUDGE_2P")) {
		GETCOMBOOBJ(2);
		obj->SetName("sprite");
		playcombo->LinkEndChild(obj);
		__comboy = obj->FirstChildElement("DST")->FirstChildElement("frame")->IntAttribute("y");
		__combox = obj->FirstChildElement("DST")->FirstChildElement("frame")->IntAttribute("x");
		return 1;
	}
	else if (OBJTYPE_IS("NOWCOMBO_2P")) {
		GETCOMBOOBJ(2);
		obj->SetName("number");
		ProcessNumber(obj, 0, 0, 0);
		obj->SetAttribute("value", "P2Combo");
		obj->SetAttribute("align", 1);
		for (XMLElement *e = obj->FirstChildElement("DST")->FirstChildElement("frame"); e;) {
			e->SetAttribute("y", __comboy);
			e->SetAttribute("x", e->IntAttribute("x") + __combox);
			e->SetAttribute("w", 0);
			e = e->NextSiblingElement("frame");
		}
		playcombo->LinkEndChild(obj);
		return 1;
	}

	// not a combo object
	return 0;
}

int _LR2SkinParser::ProcessSelectBar(const args_read_& args, XMLElement *obj) {
	int objectid = INT(args[1]);

	// select menu part
	if (!OBJTYPE_IS("BARGRAPH") && CMD_STARTSWITH("#SRC_BAR", 8)) {
		XMLElement *selectmenu = FindElement(cur_e, "selectmenu", &s->skinlayout);
		if (OBJTYPE_IS("BAR_BODY")) {
			// only register SRC object
			XMLElement *src = obj->FirstChildElement("SRC");
			src->SetName("SRC_BODY");
			src->SetAttribute("type", objectid);	// foldertype
			selectmenu->LinkEndChild(src);
			// should remove parent object
			obj->DeleteChildren();
			s->skinlayout.DeleteNode(obj);
		}
		else if (OBJTYPE_IS("BAR_FLASH")) {
			obj->SetName("flash");
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_TITLE")) {
			obj->SetName("title");
			obj->SetAttribute("type", objectid);	// new: 1
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_LEVEL")) {
			obj->SetName("level");
			obj->SetAttribute("type", objectid);	// difficulty
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_LAMP")) {
			obj->SetName("lamp");
			obj->SetAttribute("type", objectid);	// clear
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_MY_LAMP")) {
			obj->SetName("mylamp");
			obj->SetAttribute("type", objectid);	// clear
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_RIVAL_LAMP")) {
			obj->SetName("rivallamp");
			obj->SetAttribute("type", objectid);	// clear
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_RIVAL")) {
			// ignore
			s->skinlayout.DeleteNode(obj);
		}
		else if (OBJTYPE_IS("BAR_RANK")) {
			// ignore
			s->skinlayout.DeleteNode(obj);
		}
		return 1;
	}

	// not a select bar object
	return 0;
}

int _LR2SkinParser::ProcessSelectBar_DST(const args_read_& args) {
	int objectid = INT(args[1]);
#define CMD_IS(v) (strcmp(args[0], (v)) == 0)
	if (CMD_IS("#DST_BAR_BODY_ON")) {
		XMLElement *selectmenu = FindElement(cur_e, "selectmenu", &s->skinlayout);
		XMLElement *position = FindElement(selectmenu, "position", &s->skinlayout);
		XMLElement *bodyoff = FindElement(position, "bar");
		if (bodyoff) {
			/*
			* DST_SELECTED: only have delta_x, delta_y value
			*/
			XMLElement *frame = FindElement(bodyoff, "frame");
			position->SetAttribute("delta_x", INT(args[3]) - frame->IntAttribute("x"));
		}
		return 1;
	}
	else if (CMD_IS("#DST_BAR_BODY_OFF")) {
		XMLElement *selectmenu = FindElement(cur_e, "selectmenu", &s->skinlayout);
		XMLElement *position = FindElement(selectmenu, "position", &s->skinlayout);
		XMLElement *bodyoff = FindElementWithAttribute(position, "bar", "index", INT(args[1]), &s->skinlayout);
		XMLElement *frame = s->skinlayout.NewElement("frame");
		frame->SetAttribute("time", INT(args[2]));
		frame->SetAttribute("x", INT(args[3]));
		frame->SetAttribute("y", INT(args[4]));
		frame->SetAttribute("w", INT(args[5]));
		frame->SetAttribute("h", INT(args[6]));
		bodyoff->LinkEndChild(frame);
		return 1;
	}
	else {
		// not a select bar object
		return 0;
	}
}

void _LR2SkinParser::ConvertToTextureFont(XMLElement *obj) {
	XMLElement *dst = obj->FirstChildElement("DST");

	/*
	* Number uses Texturefont
	* (Number object itself is quite depreciated, so we'll going to convert it)
	* - Each number creates one texturefont
	* - texturefont syntax: *.ini file, basically.
	*   so, we have to convert Shift_JIS code into UTF-8 code.
	*   (this would be a difficult job, hmm.)
	*
	* if type == 10, then it's a just normal texture font
	* if type == 11,
	* if type == 22, convert like 11 (+ add new SRC for negative value)
	* if SRC is timer-dependent, then make multiple fonts and add condition
	* (maybe somewhat sophisticated condition)
	*/
	int glyphcnt = obj->IntAttribute("divx") * obj->IntAttribute("divy");
	int fonttype = 10;
	if (glyphcnt % 11 == 0)
		fonttype = 11;	// '*' glyph for empty space
	else if (glyphcnt % 24 == 0)
		fonttype = 24;	// +/- font included (so, 2 fonts will be created)

	/*
	 * Create font  set it
	 */
	int tfont_idx;
	tfont_idx = GenerateTexturefontString(obj);
	obj->SetAttribute("resid", tfont_idx);
	/*
	 * Processed little differently if it's 24mode.
	 * only for LR2, LEGACY attribute.
	 * negative number will be matched with Alphabet.
	 * Don't use this for general purpose, negative number will drawn incorrect.
	 * if you want, use Lua condition. that'll be helpful.
	 */
	if (fonttype == 24)
		obj->SetAttribute("mode24", true);
}

// returns new(or previous) number
int _LR2SkinParser::GenerateTexturefontString(XMLElement *obj) {
	char _temp[1024];
	std::string out;
	// get all attributes first
	int x = obj->IntAttribute("x");
	int y = obj->IntAttribute("y");
	int w = obj->IntAttribute("w");
	int h = obj->IntAttribute("h");
	int divx = obj->IntAttribute("divx");
	int divy = obj->IntAttribute("divy");
	const char* timer = obj->Attribute("timer");
	int dw = w / divx;
	int dh = h / divy;
	int glyphcnt = obj->IntAttribute("divx") * obj->IntAttribute("divy");
	int fonttype = 10;
	if (glyphcnt % 11 == 0)
		fonttype = 11;
	else if (glyphcnt % 24 == 0)
		fonttype = 24;
	int repcnt = glyphcnt / fonttype;

	// oh we're too tired to make every new number
	// so, we're going to reuse previous number if it exists
	int id = w | h << 12 | obj->IntAttribute("name") << 24;	//	kind of id
	if (texturefont_id.find(id) != texturefont_id.end()) {
		return texturefont_id[id];
	}
	texturefont_id.insert(std::pair<int, int>(id, font_cnt++));

	// get image file path from resource
	XMLElement *resource = s->skinlayout.FirstChildElement("skin");
	XMLElement *img = FindElementWithAttribute(resource, "image", "name", obj->Attribute("resid"));
	// create font data
	SkinTextureFont tfont;
	tfont.AddImageSrc(img->Attribute("path"));
	tfont.SetCycle(obj->IntAttribute("cycle"));
	if (timer) tfont.SetTimer(timer);
	tfont.SetFallbackWidth(dw);
	char glyphs[] = "0123456789*+ABCDEFGHIJ#-";
	for (int r = 0; r < repcnt; r++) {
		for (int i = 0; i < fonttype; i++) {
			int gi = i + r * fonttype;
			int cx = gi % divx;
			int cy = gi / divx;
			tfont.AddGlyph(glyphs[i], 0, x + dw * cx, y + dh * cy, dw, dh);
		}
	}
	tfont.SaveToText(out);
	out = "\n# Auto-generated texture font data by SkinParser\n" + out;

	// register to LR2 resource
	XMLElement *res = s->skinlayout.FirstChildElement("skin");
	XMLElement *restfont = s->skinlayout.NewElement("texturefont");
	restfont->SetAttribute("name", font_cnt-1);		// create new texture font
	restfont->SetAttribute("type", "1");			// for LR2 font type. (but not decided for other format, yet.)
	restfont->SetText(out.c_str());
	res->InsertFirstChild(restfont);

	return font_cnt-1;
}

void _LR2SkinParser::Clear() {
	s = 0;
	line_total = 0;
	cur_e = 0;
	image_cnt = 0;
	font_cnt = 0;
	texturefont_id.clear();
	lines_.clear();
	line_args_.clear();
}

// ----------------------- LR2Skin part end ------------------------

//
// skin converter
//


namespace SkinConverter {
	// private function; for searching including file
	void SearchInclude(std::vector<std::string>& inc, XMLElement* node) {
		for (XMLElement* n = node;
			n;
			n = n->NextSiblingElement())
		{
			if (strcmp(n->Name(), "include") == 0) {
				inc.push_back(SkinUtil::GetAbsolutePath(n->Attribute("path")));
			}
			SearchInclude(inc, n->FirstChildElement());
		}
	}

	// depreciated; only for basic test, or 
	bool ConvertLR2SkinToXml(const char* lr2skinpath) {
		_LR2SkinParser *parser = new _LR2SkinParser();
		SkinMetric* skinmetric = new SkinMetric();
		Skin* skinmetric_code = new Skin();
		// skin include path is relative to csvskin path,
		// so need to register basepath
		std::string basedir = SkinUtil::GetParentDirectory(lr2skinpath);
		SkinUtil::SetBasePath(basedir);
		
		bool r = parser->ParseLR2Skin(lr2skinpath, skinmetric);
		r &= parser->ParseCSV(lr2skinpath, skinmetric_code);
		if (r) {
			std::string dest_xml = SkinUtil::ReplaceExtension(lr2skinpath, ".skin.xml");
			std::string dest_lua = SkinUtil::ReplaceExtension(lr2skinpath, ".xml");
			// save metric data first
			skinmetric->Save(dest_xml.c_str());
			skinmetric_code->Save(dest_lua.c_str());
			// check for included files
			std::vector<std::string> include_csvs;
			SearchInclude(include_csvs, skinmetric_code->skinlayout.FirstChildElement());
			// convert all included files
			for (int i = 0; i < include_csvs.size(); i++) {
				Skin* csvskin = new Skin();
				if (parser->ParseCSV(include_csvs[i].c_str(), csvskin)) {
					std::string dest_path = SkinUtil::ReplaceExtension(include_csvs[i], ".xml");
					// TODO: depart texturefont files?
					csvskin->Save(dest_path.c_str());
				}
				delete csvskin;
			}
		}
		delete skinmetric;
		delete parser;

		return r;
	}

	bool ConvertLR2SkinToLua(const char* lr2skinpath) {
		_LR2SkinParser *parser = new _LR2SkinParser();
		SkinMetric* skinmetric = new SkinMetric();
		Skin* skinmetric_code = new Skin();
		// skin include path is relative to csvskin path,
		// so need to register basepath
		std::string basedir = SkinUtil::GetParentDirectory(lr2skinpath);
		SkinUtil::SetBasePath(basedir);

		bool r = parser->ParseLR2Skin(lr2skinpath, skinmetric);
		r &= parser->ParseCSV(lr2skinpath, skinmetric_code);
		if (r) {
			std::string dest_xml = SkinUtil::ReplaceExtension(lr2skinpath, ".skin.xml");
			std::string dest_lua = SkinUtil::ReplaceExtension(lr2skinpath, ".lua");
			// save metric data first
			skinmetric->Save(dest_xml.c_str());
			skinmetric_code->SaveToLua(dest_lua.c_str());
			// check for included files
			std::vector<std::string> include_csvs;
			SearchInclude(include_csvs, skinmetric_code->skinlayout.FirstChildElement());
			// convert all included files
			for (int i = 0; i < include_csvs.size(); i++) {
				Skin* csvskin = new Skin();
				if (parser->ParseCSV(include_csvs[i].c_str(), csvskin)) {
					std::string dest_path = SkinUtil::ReplaceExtension(include_csvs[i], ".lua");
					// TODO: depart texturefont files?
					csvskin->SaveToLua(dest_path.c_str());
				}
				delete csvskin;
			}
		}
		delete skinmetric;
		delete parser;

		return false;
	}
}