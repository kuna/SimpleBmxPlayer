#include "skin.h"
#include "skinconverter.h"
#include "skinutil.h"
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
		"Scene",		// this timer is automatically started when scene started, actually.
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

	const char* SkintypeCode[] = {
		"7Key", "5Key", "14Key", "10Key", "9Key",
		"Select", "Decide", "Result", "KeyConfig", "SkinSelect",
		"SoundSelect", "", "5KeyDouble", "7KeyDouble", "", "CourseResult",
		"",
	};
}

#pragma endregion
//
// translator end
//

// utility macros
#define ADDCHILD(base, name)\
	((XMLElement*)(base)->LinkEndChild((base)->GetDocument()->NewElement(name)))
#define ADDCHILDFRONT(base, name)\
	((XMLElement*)(base)->InsertFirstChild((base)->GetDocument()->NewElement(name)))
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

	// after we read all lines, skin parse start
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

	// load skin line
	line_total = LoadSkin(filepath);
	if (!line_total) {
		printf("LR2Skin Warning - Cannot find file (%s)\n", filepath);
		return false;
	}

	// after we read all lines, skin parse start
	currentline = 0;
	while (currentline >= 0 && currentline < line_total) {
		ParseSkinCSVLine();
	}

	// after structure parsing finished,
	// add resource lua code
	// (MUST add element at very first position)
	std::string lua_resource = m_Res.ToString();
	s->CreateElement("lua")->SetText(lua_resource.c_str());
	s->GetBaseElement()->InsertFirstChild(s->GetCurrentElement());

	// end
	return true;
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

/* a simple private macro for PLAYLANE (reset position) - DEPRECIATED */
#if 0
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
#endif

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
		s->PopParent();
		s->PopParent();
		cond = true;
	}
	else if (CMD_IS("#ELSE")) {
		s->PopParent();
		s->SetCurrentParent(s->CreateElement("if"));
		s->PushParent();
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			cls.AddCondition(TranslateOPs(INT(args[i])));
		}
		s->SetAttribute("condition", cls.ToString());
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
		s->CreateElement("condition");
		s->PushParent();
		s->CreateElement("if");
		s->PushParent();
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			cls.AddCondition(TranslateOPs(INT(args[i])));
		}
		s->SetAttribute("condition", cls.ToString());
		cond = true;
	}
	else if (CMD_IS("#ELSEIF")) {
		s->PopParent();
		s->CreateElement("elseif");
		s->PushParent();
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			cls.AddCondition(TranslateOPs(INT(args[i])));
		}
		s->SetAttribute("condition", cls.ToString());
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

/*
 * about options or info that related to skin
 */
bool _LR2SkinParser::ProcessMetadata(const args_read_& args) {
	if (CMD_IS("#CUSTOMOPTION")) {
		std::string name_safe = args[1];
		ReplaceString(name_safe, " ", "_");
		int option_intvalue = atoi(args[2]);

		SkinMetric::CustomValue val;
		val.optionname = name_safe;
		int i = 0;
		for (const char **p = args + 3; *p != 0 && strlen(*p) > 0; p++) {
			char t_[10];
			itoa(option_intvalue++, t_, 10);

			SkinMetric::Option option;
			option.desc = *p;
			option.eventname = t_;
			option.value = i++;	// meaningless
			val.options.push_back(option);
#if 0
			desc_txt += *p;
			desc_txt.push_back(';');
			val_txt += t_;
			val_txt.push_back(';');
#endif
		}

		sm->values.push_back(val);
		return true;
	}
	else if (CMD_IS("#CUSTOMFILE")) {
		std::string name_safe = args[1];
		ReplaceString(name_safe, " ", "_");

		// decide file type
		std::string path_safe = args[2];
#if 0
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
#endif
		//ReplaceString(path, "*", args[3]);
		std::string path = args[2];
		ConvertLR2PathToRelativePath(path);

		SkinMetric::CustomFile f;
		f.optionname = name_safe;
		f.path = path;
		f.path_default = args[3];	// default value for star
		sm->files.push_back(f);
		return true;
	}
	else if (CMD_IS("#INFORMATION")) {
		// set skin's metadata
		sm->info.width = 1280;
		sm->info.height = 720;
		sm->info.title = args[2];
		sm->info.artist = args[3];
		sm->info.type = SkintypeCode[INT(args[1])];
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
		// check for optionname path
		std::string path_converted = args[1];
		ConvertLR2PathToRelativePath(path_converted);
		for (auto it = sm->files.begin(); it != sm->files.end(); ++it) {
			if (path_converted == it->path) {
				// replace filtered path to reserved metrics
				ReplaceString(path_converted, "*", "$(" + it->optionname + ")");
				break;
			}
		}
		// convert path to relative
		ConvertLR2PathToRelativePath(path_converted);

		m_Res.AddImage(path_converted.c_str());

		return true;
	}
	else if (CMD_IS("#LR2FONT")) {
		// if lr2font isn't exists or converted or something else,
		// dont worry, game program will automatically use fallback ttf font.
		std::string path_converted = args[1];
		ConvertLR2PathToRelativePath(path_converted);
		m_Res.AddText(path_converted.c_str());
		return true;
	}
	return false;
}

bool _LR2SkinParser::ProcessDepreciated(const args_read_& args) {
	if (CMD_IS("#FONT")) {
		// it's depreciated, but I'm going to add size attribute
		m_Res.AddTextAttr(atoi(args[1]), atoi(args[2]));
		return true;
	} else if (
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

_LR2SkinParser::OP _LR2SkinParser::ProcessSRC(const args_read_& args) {
	OP srcop;	memset(&srcop, 0, sizeof(OP));
	srcop.op[0] = INT(args[11]);
	srcop.op[1] = INT(args[12]);
	srcop.op[2] = INT(args[13]);
	srcop.op[3] = INT(args[14]);
	int resid = INT(args[2]);	// COMMENT: check out for pre-occupied resid
	switch (resid) {
	case 100:
		s->SetAttribute("file", "_stagefile");
		break;
	case 101:
		s->SetAttribute("file", "_backbmp");
		break;
	case 102:
		s->SetAttribute("file", "_banner");
		break;
	case 110:
		s->SetAttribute("file", "_black");
		break;
	case 111:
		s->SetAttribute("file", "_white");
		break;
	default:
		s->SetAttribute("file", resid);
		break;
	}
	s->SetAttribute("sx", INT(args[3]));
	s->SetAttribute("sy", INT(args[4]));
	if (INT(args[5]) > 0) {
		s->SetAttribute("sw", INT(args[5]));
		s->SetAttribute("sh", INT(args[6]));
	}
	if (INT(args[7]) > 1 || INT(args[8]) > 1) {
		s->SetAttribute("divx", INT(args[7]));
		s->SetAttribute("divy", INT(args[8]));
	}
	if (INT(args[9]))
		s->SetAttribute("cycle", INT(args[9]));
	int sop1 = 0, sop2 = 0, sop3 = 0, sop4 = 0;		// sop4 used in scratch rotation
	// COMMENT: SRC timer is necessary?
	if (INT(args[10]))
		s->SetAttribute("timer", TranslateTimer(INT(args[10])));
	// COMMENT: SRC loop is necessary?
	
	// we just processed 1 line
	currentline++;

	return srcop;
}

/* call this after parent object is focused with ... you ... */
_LR2SkinParser::OP _LR2SkinParser::ProcessDST(const args_read_& args) {
	int looptime = 0, blend = 0, timer = 0, rotatecenter = -1, acc = 0;
	OP dstop;	memset(&dstop, 0, sizeof(OP));
	int time = 0;
	TweenCommand cmd;
	{
		int a_ = 255, r_ = 255, g_ = 255, b_ = 255;
		int x_ = 0, y_ = 0, w_ = 0, h_ = 0;
		int angle_ = 0;
		for (int nl = currentline; nl < line_total; nl++) {
			args_read_ args = line_args_[nl].args__;
			if (strcmp(args[0], "") == 0) continue;
			if (CMD_IS("#ENDIF"))
				continue;			// we can ignore #ENDIF command, maybe
			if (!CMD_STARTSWITH("#DST_", 5))
				break;
			// if it's very first line (parse basic element)
			if (rotatecenter < 0) {
				looptime = INT(args[16]);
				blend = INT(args[12]);
				rotatecenter = INT(args[15]);
				dstop.op[3] = timer = INT(args[17]);
				acc = INT(args[7]);
				dstop.op[0] = INT(args[18]);
				dstop.op[1] = INT(args[19]);
				dstop.op[2] = INT(args[20]);

				// acc, blend, rotatecenter, condition are MUST defined at the very first of the actions.
				if (acc)
					cmd.Add("acc", acc);
				if (blend > 0)
					cmd.Add("blend", blend);
				if (rotatecenter > 0)
					cmd.Add("center", rotatecenter);
			}
			/*
			* frame attribute
			* only add attribute if value is different from previous(default) one.
			*/
			time = atoi(args[2]);
			s->CreateElement("time")->SetAttribute("v", time);
			if (INT(args[3]) != x_) {
				x_ = INT(args[3]);
				cmd.Add("x", x_);
			}
			if (INT(args[4]) != y_) {
				y_ = INT(args[4]);
				cmd.Add("y", y_);
			}
			if (INT(args[5]) != w_) {
				w_ = INT(args[5]);
				cmd.Add("w", w_);
			}
			if (INT(args[6]) != h_) {
				h_ = INT(args[6]);
				cmd.Add("h", h_);
			}
			if (INT(args[8]) != a_) {
				a_ = INT(args[8]);
				cmd.Add("a", a_);
			}
			if (INT(args[9]) != r_) {
				r_ = INT(args[9]);
				cmd.Add("r", r_);
			}
			if (INT(args[10]) != g_) {
				g_ = INT(args[10]);
				cmd.Add("g", g_);
			}
			if (INT(args[11]) != b_) {
				b_ = INT(args[11]);
				cmd.Add("b", b_);
			}
			if (INT(args[14]) != angle_) {
				angle_ = INT(args[14]);
				cmd.Add("angle", angle_);
			}
		}
	}

	// set common draw attribute
	const char* handlername = TranslateTimer(timer);
	if (handlername && *handlername) {
		char buf[128] = "On";
		strcat(buf, handlername);
		s->SetName(buf);
	}

	// condition of DST means `display or not`, not using alpha or visible command.
	ConditionAttribute cls;
	const char *c;
	c = dstop.op[0] ? TranslateOPs(dstop.op[0]) : 0;
	if (c) cls.AddCondition(c);
	c = dstop.op[1] ? TranslateOPs(dstop.op[1]) : 0;
	if (c) cls.AddCondition(c);
	c = dstop.op[2] ? TranslateOPs(dstop.op[2]) : 0;
	if (c) cls.AddCondition(c);
	if (cls.GetConditionNumber())
		s->SetAttribute("visible", cls.ToString());

	// loop MUST be declared at the very last of the actions.
	// and looptime should be different from last action time.
	if (looptime >= 0 && looptime != time)
		cmd.Add("loop", looptime);

	s->SetAttribute("cmd", cmd.ToString());
	return dstop;
}

int GetDSTAttrFromObject(XMLElement *e, const char* name) {
	for (XMLElement *c = e->FirstChildElement(); c; c = c->NextSiblingElement()) {
		if (strnicmp(c->Name(), "On", 2) == 0) {
			TweenCommand cmd;
			cmd.Parse(e->Attribute("cmd"));
			for (auto it = cmd.Begin(); it != cmd.End(); ++it) {
				if (stricmp((*it).c_str(), name) == 0) {
					return atoi((*it).c_str() + strlen(name) + 1);
				}
			}
		}
	}

	// cannot find
	return 0;
}

int GetSRCAttrFromObject(XMLElement *e, const char* name) {/*
	for (XMLElement *c = e->FirstChildElement(); c; c = c->NextSiblingElement()) {
		if (stricmp(c->Name(), "SRC") == 0) {
			return c->IntAttribute(name);
		}
	}*/
	return e->IntAttribute(name);

	// cannot find
	//return 0;
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
		std::string relpath = args[1];
		std::string basepath = filepath;
		ConvertLR2PathToRelativePath(relpath);

		s->CreateElement("include");
		s->SetAttribute("path", relpath);
	}
	else if (CMD_IS("#SETOPTION")) {
		// this clause is translated during render tree construction
		std::ostringstream luacode;
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			const char *c = INT(args[i]) ? TranslateOPs(INT(args[i])) : 0;
			if (c)
				luacode << "SetSwitch(\"" << c << "\");\n";
		}

		s->CreateElement("lua");
		s->SetText("\n" + luacode.str());
	}
	else if (CMD_STARTSWITH("#SRC_", 4)){
		// get ingeneral attributes first
		int objidx = INT(args[2]);		// note index, etc ...
		OP dstop, srcop;

		// process SRC.
		// SRC may have condition (attribute condition; normally not used)
		s->CreateElement("sprite");
		srcop = ProcessSRC(args);

		/*
		 * process NOT-general-objects first
		 * these objects doesn't have #DST object directly
		 */
		// check for play area
		int isPlayElement = ProcessLane(args, objidx);
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
		//s->PushParent();
		dstop = ProcessDST(args);
		//s->PopParent();


		/*
		 * just add each panel to frame
		 * for ease of skin coding ...
		 * (op4 for dst is `timer` code, in fact.)
		 * 
		 * (NOT USED; we don't need to use modern style in old style LR2 skin structure)
		 */
#if 0
#define CHECK_PANEL(v) (dstop.op[3] == (v))		// dstop.op[0] == (v) || dstop.op[1] == (v) || dstop.op[2] == (v) || 
		if (CHECK_PANEL(21) || CHECK_PANEL(31))
			FindElementWithAttribute(cur_e, "frame", "id", "panel1", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(22) || CHECK_PANEL(32))
			FindElementWithAttribute(cur_e, "frame", "id", "panel2", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(23) || CHECK_PANEL(33))
			FindElementWithAttribute(cur_e, "frame", "id", "panel3", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(24) || CHECK_PANEL(34))
			FindElementWithAttribute(cur_e, "frame", "id", "panel4", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(25) || CHECK_PANEL(35))
			FindElementWithAttribute(cur_e, "frame", "id", "panel5", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(26) || CHECK_PANEL(36))
			FindElementWithAttribute(cur_e, "frame", "id", "panel6", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(27) || CHECK_PANEL(37))
			FindElementWithAttribute(cur_e, "frame", "id", "panel7", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(28) || CHECK_PANEL(38))
			FindElementWithAttribute(cur_e, "frame", "id", "panel8", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(29) || CHECK_PANEL(39))
			FindElementWithAttribute(cur_e, "frame", "id", "panel9", &s->skinlayout)->LinkEndChild(obj);
		else
			cur_e->LinkEndChild(obj);
#endif
		

		/*
		 * Check out for some special object (which requires #DST object)
		 * COMMENT: most of them behaves like #IMAGE object.
		 */
		// combo (play)
		int isComboElement = ProcessCombo(args);
		if (isComboElement) {
			currentline += isComboElement;
			return;
		}

		// select menu (select)
		int isSelectBar = ProcessSelectBar(args);
		if (isSelectBar) {
			currentline += isSelectBar;
			return;
		}

		/* 
		 * under these are general individual object
		 */
		if (OBJTYPE_IS("IMAGE")) {
			/*
			 * nothing to do (general object) basically,
			 * but some special objects (BOMB/HOLD/ONBEAT) may need special care.
			 */
			int timer = dstop.op[3];
			XMLElement *obj = s->GetCurrentElement();
			if (!(GetDSTAttrFromObject(obj, "w") < 100
				&& GetDSTAttrFromObject(obj, "h") < 100))
			{
				if ((timer >= 50 && timer < 60) ||
					(timer >= 70 && timer < 80) ||
					(timer >= 100 && timer < 110) ||
					(timer >= 120 && timer < 130)) {
					// P1
					XMLElement *notefield = s->FindElementWithAttr("notefield", "player", "0");
					notefield->LinkEndChild(obj);
					// relocate notefield
					s->GetCurrentParent()->InsertEndChild(notefield);
				}
				else if ((timer >= 60 && timer < 70) ||
					(timer >= 80 && timer < 90) ||
					(timer >= 110 && timer < 120) ||
					(timer >= 130 && timer < 140)) {
					// P2
					XMLElement *notefield = s->FindElementWithAttr("notefield", "player", "1");
					notefield->LinkEndChild(obj);
					// relocate notefield
					s->GetCurrentParent()->InsertEndChild(notefield);
				}
			}
			// if sop4 == 1, then change object to scratch
			if (srcop.op[3]) {
				obj->SetName("scratch");
			}
			// BOMB SRC effect(SRC loop) MUST TURN OFF; don't loop.
			if ((timer >= 50 && timer < 60) || (timer >= 60 && timer < 70))
				obj->SetAttribute("loop", 0);
		}
		else if (OBJTYPE_IS("BGA")) {
			// change tag to BGA and remove SRC tag
			s->SetName("bga");
			// set bga side & remove redundant tag
			// (LR2 doesn't support 'real' battle mode, so no side attribute.)
			s->SetAttribute("player", 0);
			s->GetCurrentElement()->DeleteAttribute("file");
		}
		else if (OBJTYPE_IS("NUMBER")) {
			s->SetName("number");
			ProcessNumber(srcop.op[0], srcop.op[1], srcop.op[2]);
		}
		else if (OBJTYPE_IS("SLIDER")) {
			// change tag to slider and add attr
			XMLElement *obj = s->GetCurrentElement();
			obj->SetName("slider");
			obj->SetAttribute("direction", srcop.op[0]);
			obj->SetAttribute("range", srcop.op[1]);
			if (TranslateSlider(srcop.op[2]))
				obj->SetAttribute("value", TranslateSlider(srcop.op[2]));
			//obj->SetAttribute("range", sop2); - disable option is ignored
		}
		else if (OBJTYPE_IS("TEXT")) {
			// delete src attr and change to font/st/align/edit
			XMLElement *obj = s->GetCurrentElement();
			obj->DeleteAttribute("x");
			obj->DeleteAttribute("y");
			obj->DeleteAttribute("w");
			obj->DeleteAttribute("h");
			obj->DeleteAttribute("cycle");
			obj->DeleteAttribute("divx");
			obj->DeleteAttribute("divy");
			obj->SetName("text");
			if (TranslateText(INT(args[3])))
				s->SetAttribute("value", TranslateText(INT(args[3])));
			s->SetAttribute("align", args[4]);
			s->SetAttribute("edit", args[5]);
		}
		else if (OBJTYPE_IS("BARGRAPH")) {
			s->SetName("graph");
			if (TranslateGraph(srcop.op[0]))
				s->SetAttribute("value", TranslateGraph(srcop.op[0]));
			s->SetAttribute("direction", srcop.op[1]);
		}
		else if (OBJTYPE_IS("BUTTON")) {
			// TODO: onclick event
			s->SetName("button");
		}
		/* 
		 * some special object (PLAY lane object) 
		 */
		else if (OBJTYPE_IS("GROOVEGAUGE")) {
			// TODO: make each gauge for different type?
			int side = INT(args[1]);
			int addx = srcop.op[0];
			int addy = srcop.op[1];
			s->SetAttribute("player", side);
			s->SetAttribute("addx", addx);
			s->SetAttribute("addy", addy);
			// oldtype - for LR2 compatibility
			// (cycle related information is automatically set)
			s->SetAttribute("oldtype", 1);	
			// process SRC and make new SRC elements
#if 0
			int x = GetDSTAttrFromObject(obj, "x");
			int y = GetDSTAttrFromObject(obj, "y");
			int w = GetDSTAttrFromObject(obj, "w");
			int h = GetDSTAttrFromObject(obj, "h");
			int divx = GetSRCAttrFromObject(obj, "divy");
			int divy = GetSRCAttrFromObject(obj, "divx");
			int c = divx * divy;
			int dw = w / divx;
			int dh = h / divy;
			s->SetAttribute("SRC_GROOVE_ACTIVE", CreateSRC(
				x + dw * (1 % divx), y + dh * (1 / divx % divy), dw, dh));
			s->SetAttribute("SRC_GROOVE_INACTIVE", CreateSRC(
				x + dw * (3 % divx), y + dh * (3 / divx % divy), dw, dh));
			s->SetAttribute("SRC_HARD_ACTIVE", CreateSRC(
				x, y, dw, dh));
			s->SetAttribute("SRC_HARD_INACTIVE", CreateSRC(
				x + dw * (2 % divx), y + dh * (2 / divx % divy), dw, dh));
			s->SetAttribute("SRC_HARD_ACTIVE", CreateSRC(
				x, y, dw, dh));
			s->SetAttribute("SRC_HARD_INACTIVE", CreateSRC(
				x + dw * (2 % divx), y + dh * (2 / divx % divy), dw, dh));

			// remove obsolete SRC
			s->DeleteAttribute("x");
			s->DeleteAttribute("y");
			s->DeleteAttribute("w");
			s->DeleteAttribute("h");
			s->DeleteAttribute("divx");
			s->DeleteAttribute("divy");
#endif
			s->SetName("groovegauge");
		}
		else if (OBJTYPE_IS("JUDGELINE")) {
			s->SetName("judgeline");
			XMLElement *playarea = s->FindElementWithAttr("notefield", "player", objectid, true);
			playarea->LinkEndChild(s->GetCurrentElement());
		}
		else if (OBJTYPE_IS("LINE")) {
			s->SetName("line");
			XMLElement *playarea = s->FindElementWithAttr("notefield", "player", objectid, true);
			playarea->LinkEndChild(s->GetCurrentElement());
		}
		else if (OBJTYPE_IS("ONMOUSE")) {
			// depreciated/ignore
			// TODO: support this by SRC_HOVER tag?
			printf("#XXX_ONMOUSE command is depreciated, ignore. (%dL) \n", currentline);
		}
		else {
			printf("Unknown General Object (%s), consider as IMAGE. (%dL)\n", args[0] + 5, currentline);
		}

		// we already processed line in SRC/DST, so don't do anything in here
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
		XMLElement *musiclist = s->FindElement("musiclist", true);
		musiclist->SetAttribute("center", args[1]);
	}
	else if (CMD_IS("#BAR_AVAILABLE")) {
		// depreciated, not parse
		printf("#BAR_AVAILABLE - depreciated, Ignore.\n");
	}
	/*
	 * PLAY part 
	 * use notefield for ease of coding & lift supporting.
	 */
	else if (CMD_STARTSWITH("#DST_NOTE", 9)) {
		int objectid = INT(args[1]);
		XMLElement *playarea = s->FindElementWithAttr("notefield", "player", objectid / 10, true);
		s->PushParent();
		s->SetCurrentElement(playarea);
		XMLElement *note = s->FindElementWithAttr("note_dumy", "index", objectid, true);
		// Actually, we can implement this without considering relative pos.
		note->SetAttribute("x", INT(args[3]));
		note->SetAttribute("y", INT(args[4]));
		note->SetAttribute("w", INT(args[5]));
		note->SetAttribute("h", INT(args[6]));
		std::ostringstream ss;
		ss << "x:" << INT(args[3])
			<< ",y:" << INT(args[4])
			<< ",w:" << INT(args[5])
			<< ",h:" << INT(args[6]);
		note->SetAttribute("OnInit", ss.str().c_str());
#if 0
		// COMMENT: we should make it in OnInit(), but I think supporting this method seems okay.
		note->SetAttribute("x", INT(args[3]) - playarea->IntAttribute("x"));
		note->SetAttribute("y", INT(args[4]) - playarea->IntAttribute("y"));
		note->SetAttribute("w", INT(args[5]));
		note->SetAttribute("h", INT(args[6]));
#endif
		s->PopParent();
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

void _LR2SkinParser::ProcessNumber(int sop1, int sop2, int sop3) {
#if 0
	// just convert SRC to texturefont ...
	ConvertToTextureFont(obj);
#endif
	// don't need to create texturefont (don't going to use it!)
	std::string glyphs;
	int c = s->GetAttribute<int>("divx") * s->GetAttribute<int>("divy");
	if (c % 10 == 0)
		glyphs = "0123456789";
	else if (c % 11 == 0)
		glyphs = "0123456789*";
	else // 24
		glyphs = "0123456789*+ABCDEFGHIJ#-";
	s->SetAttribute("glyphs", glyphs.c_str());

	/*
	* Number object will act just like a extended-string object.
	* If no value, then it'll just show '0' value.
	*/
	if (TranslateNumber(sop1))
		s->SetAttribute("value", TranslateNumber(sop1));
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
	s->SetAttribute("align", sop2);
	/*
	* if type == 11 or 24, then set length
	* if type == 24, then set '24mode' (only for LR2 - depreciated supportance)
	* (If you want to implement LR2-like font, then you may have to make 2 type of texturefont SRC -
	* plus and minus - with proper condition.)
	*/
	if (sop3)
		s->SetAttribute("length", sop3);
#if 0
	// remove some attrs
	s->DeleteAttribute("x");
	s->DeleteAttribute("y");
	s->DeleteAttribute("w");
	s->DeleteAttribute("h");
	s->DeleteAttribute("divx");
	s->DeleteAttribute("divy");
	s->DeleteAttribute("cycle");
#endif
}

/*
 * process commands about lane
 * if not lane, return 0
 * if lane, return next parsed line
 */
int _LR2SkinParser::ProcessLane(const args_read_& args, int resid) {
	int objectid = INT(args[1]);

#define SETNOTE(name)\
	s->PushParent(false);\
	XMLElement *playarea = s->FindElementWithAttr("notefield", "player", objectid / 10, true);\
	s->SetCurrentParent(playarea);\
	XMLElement *note = s->FindElementWithAttr("note_dummy", "index", objectid, true);	/* for copying OnInit */\
	s->SetName(name);\
	s->SetAttribute("file", note->Attribute("file"));\
	s->SetAttribute("OnInit", note->Attribute("OnInit"));\
	s->PopParent();\
	return 1;

	if (OBJTYPE_IS("NOTE")) {
		SETNOTE("NOTE");
	}
	else if (OBJTYPE_IS("LN_END")) {
		SETNOTE("LN_END");
	}
	else if (OBJTYPE_IS("LN_BODY")) {
		SETNOTE("LN_BODY");
	}
	else if (OBJTYPE_IS("LN_START")) {
		SETNOTE("LN_START");
	}
	else if (OBJTYPE_IS("MINE")) {
		SETNOTE("MINE");
	}
	if (OBJTYPE_IS("AUTO_NOTE")) {
		SETNOTE("AUTO_NOTE");
	}
	else if (OBJTYPE_IS("AUTO_LN_END")) {
		SETNOTE("AUTO_LN_END");
	}
	else if (OBJTYPE_IS("AUTO_LN_BODY")) {
		SETNOTE("AUTO_LN_BODY");
	}
	else if (OBJTYPE_IS("AUTO_LN_START")) {
		SETNOTE("AUTO_LN_START");
	}
	else if (OBJTYPE_IS("AUTO_MINE")) {
		SETNOTE("AUTO_MINE");
	}
	else if (OBJTYPE_IS("JUDGELINE")) {
		XMLElement *playarea = s->FindElementWithAttr("notefield", "player", objectid, true);
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
	sprintf(buf, "P%d", player);

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
int _LR2SkinParser::ProcessCombo(const args_read_& args) {
	int objectid = INT(args[1]);
	int sop1 = 0, sop2 = 0, sop3 = 0;
	if (args[11]) sop1 = INT(args[11]);
	if (args[12]) sop2 = INT(args[12]);
	if (args[13]) sop3 = INT(args[13]);

#define GETCOMBOOBJ(side)\
	XMLElement *playcombo = s->FindElementWithAttr("judge", "type", objectid, true);

	/*
	 * only judge object has information about pos
	 * (combo object has relative pos to judge object)
	 */
	if (OBJTYPE_IS("NOWJUDGE_1P")) {
		s->SetName("judge");
		s->SetAttribute("type", objectid);
		s->SetAttribute("player", 0);
		return 1;
	}
	else if (OBJTYPE_IS("NOWCOMBO_1P")) {
		GETCOMBOOBJ(0);
		s->SetName("number");
		ProcessNumber(0, 0, 0);
		s->SetAttribute("value", "P1Combo");
		s->SetAttribute("align", 1);
		playcombo->LinkEndChild(s->GetCurrentElement());
		return 1;
	}
	else if (OBJTYPE_IS("NOWJUDGE_2P")) {
		s->SetName("judge");
		s->SetAttribute("type", objectid);
		// if playtype is DP -> set side as 1
		// TODO
		s->SetAttribute("player", 1);
		return 1;
	}
	else if (OBJTYPE_IS("NOWCOMBO_2P")) {
		GETCOMBOOBJ(1);
		s->SetName("number");
		ProcessNumber(0, 0, 0);
		s->SetAttribute("value", "P2Combo");
		s->SetAttribute("align", 1);
		playcombo->LinkEndChild(s->GetCurrentElement());
		return 1;
	}

	// not a combo object
	return 0;
}

int _LR2SkinParser::ProcessSelectBar(const args_read_& args) {
	int objectid = INT(args[1]);

	// select menu part
	if (!OBJTYPE_IS("BARGRAPH") && CMD_STARTSWITH("#SRC_BAR", 8)) {
		XMLElement *musiclist = s->FindElement("musiclist", true);
		if (OBJTYPE_IS("BAR_BODY")) {
			// only copy src attribute from element
			s->FindElement("bar")->SetAttribute("SRC", s->GetAttribute<const char*>("SRC"));
			s->DeleteNode(s->GetCurrentElement());
		}
		else if (OBJTYPE_IS("BAR_FLASH")) {
			s->SetName("flash");
			musiclist->LinkEndChild(s->GetCurrentElement());
		}
		else if (OBJTYPE_IS("BAR_TITLE")) {
			s->SetName("title");
			s->SetAttribute("type", objectid);	// new: 1
			musiclist->LinkEndChild(s->GetCurrentElement());
		}
		else if (OBJTYPE_IS("BAR_LEVEL")) {
			s->SetName("level");
			s->SetAttribute("type", objectid);	// difficulty
			musiclist->LinkEndChild(s->GetCurrentElement());
		}
		else if (OBJTYPE_IS("BAR_LAMP")) {
			s->SetName("lamp");
			s->SetAttribute("type", objectid);	// clear
		}
		else if (OBJTYPE_IS("BAR_MY_LAMP")) {
			s->SetName("mylamp");
			s->SetAttribute("type", objectid);	// clear
			musiclist->LinkEndChild(s->GetCurrentElement());
		}
		else if (OBJTYPE_IS("BAR_RIVAL_LAMP")) {
			s->SetName("rivallamp");
			s->SetAttribute("type", objectid);	// clear
			musiclist->LinkEndChild(s->GetCurrentElement());
		}
		else if (OBJTYPE_IS("BAR_RIVAL")) {
			// ignore
			s->DeleteNode(s->GetCurrentElement());
		}
		else if (OBJTYPE_IS("BAR_RANK")) {
			// ignore
			s->DeleteNode(s->GetCurrentElement());
		}
		return 1;
	}

	// not a select bar object
	return 0;
}

/*
 * it's not firmly decided yet ...
 */
int _LR2SkinParser::ProcessSelectBar_DST(const args_read_& args) {
	int objectid = INT(args[1]);
#define CMD_IS(v) (strcmp(args[0], (v)) == 0)
	if (CMD_IS("#DST_BAR_BODY_ON")) {
		/*
		 * cannot set exact value - it's too hard!
		 * but I think I can set addx property ...
		 */
		s->PushParent(false);
		XMLElement *list = s->FindElement("musiclist", true);
		XMLElement *bar = s->FindElementWithAttr("bar", "index", INT(args[1]), true);
		XMLElement *dst = s->FindElement("OnFocus", true);
		ADDCHILD(dst, "time")->SetAttribute("v", INT(args[2]));
		ADDCHILD(dst, "x")->SetAttribute("v", INT(args[3]));
		ADDCHILD(dst, "y")->SetAttribute("v", INT(args[4]));
		ADDCHILD(dst, "w")->SetAttribute("v", INT(args[5]));
		ADDCHILD(dst, "h")->SetAttribute("v", INT(args[6]));
		ADDCHILD(dst, "a")->SetAttribute("v", INT(args[8]));
		s->PopParent();
		return 1;
	}
	else if (CMD_IS("#DST_BAR_BODY_OFF")) {
		/*
		 * default position of BAR_BODY
		 */
		s->PushParent(false);
		XMLElement *list = s->FindElement("musiclist", true);	s->SetCurrentElement(list);
		XMLElement *bar = s->FindElementWithAttr("bar", "index", INT(args[1]), true);
		XMLElement *dst = s->FindElement("OnInit", true);
		ADDCHILD(dst, "time")->SetAttribute("v", INT(args[2]));
		ADDCHILD(dst, "x")->SetAttribute("v", INT(args[3]));
		ADDCHILD(dst, "y")->SetAttribute("v", INT(args[4]));
		ADDCHILD(dst, "w")->SetAttribute("v", INT(args[5]));
		ADDCHILD(dst, "h")->SetAttribute("v", INT(args[6]));
		ADDCHILD(dst, "a")->SetAttribute("v", INT(args[8]));
		s->PopParent();
		// TODO: a, r, g, b, angle
		// TODO: set first attribute: acc, center, ...
		// TODO: add func like DST
		// TODO: compare with previous one?
		return 1;
	}
	else {
		// not a select bar object
		return 0;
	}
}

#if 0
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
	obj->SetAttribute("file", tfont_idx);
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
	XMLElement *img = FindElementWithAttribute(resource, "image", "name", obj->Attribute("file"));
	// create font data
	SkinTextureFont tfont;
	tfont.AddImageSrc(img->Attribute("path"));
	tfont.SetCycle(obj->IntAttribute("cycle"));
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
#endif

void _LR2SkinParser::Clear() {
	s = 0;
	line_total = 0;
	image_cnt = 0;
	font_cnt = 0;
	lines_.clear();
	line_args_.clear();
	m_Res.Clear();
}

// ----------------------- LR2Skin part end ------------------------






void _LR2SkinParser::Resource::AddImage(const char* path) {
	image.push_back(path);
}

void _LR2SkinParser::Resource::AddTFont(const char* data) {
	tfont.push_back(data);
}

void _LR2SkinParser::Resource::AddText(const char* path) {
	if (font.size() > fntpath_idx) {
		font[fntpath_idx].path = path;
	}
	else {
		Font fnt;
		fnt.size = 10;
		fnt.thick = 1;
		fnt.path = path;
		font.push_back(fnt);
	}
	fntpath_idx++;
}

void _LR2SkinParser::Resource::AddTextAttr(int size, int thick) {
	if (font.size() > fntattr_idx) {
		font[fntattr_idx].size = size;
		font[fntattr_idx].thick = thick;
	}
	else {
		Font fnt;
		fnt.size = size;
		fnt.thick = thick;
		font.push_back(fnt);
	}
	fntattr_idx++;
}

std::string _LR2SkinParser::Resource::ToString() {
	std::ostringstream ss;
	ss << "\n";

	// image
	for (int i = 0; i < image.size(); i++) {
		ss << "LoadImage{path=\"" << image[i] << "\", id=\"" << i << "\"}\n";
	}

	// font (ttf or bitmap)
	for (int i = 0; i < font.size(); i++) {
		ss << "LoadFont{path=\"" << font[i].path <<
			"\", size=\"" << font[i].size <<
			"\", thick=\"" << font[i].thick <<
			"\", id=\"" << i << "\"}\n";
	}

	// tfont
	// (in fact, tfont is same as font, only it has data on program)
	for (int i = 0; i < tfont.size(); i++) {
		ss << "LoadFont{raw=\"" << tfont[i] <<
			"\", id=\"" << font.size() + i + 1 << "\"}\n";
	}

	return ss.str();
}

void _LR2SkinParser::Resource::Clear() {
	font.clear();
	tfont.clear();
	image.clear();
	fntpath_idx = fntattr_idx = 0;
}