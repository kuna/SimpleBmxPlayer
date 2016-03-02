#pragma once

/*
 * @description
 * loads, and provides song(bms) data/information.
 */

#include "global.h"
#include "bmsbel/bms_bms.h"

struct SongInfo {
	RString sMainTitle;
	RString sTitle;
	RString sSubTitle;
	RString sArtist;
	RString sSubArtist;
	RString sGenre;

	RString sBackBmp;
	RString sBanner;

	int iKeyCount;
	int iDifficulty;
	int iLevel;
	int iRank;
	int iBpm;
	int iTotal;
	int iPlayer;
};

namespace BmsHelper {
	bool LoadBms(const RString& path, BmsBms& bms);
	void SetLoadOption(int start, int end, int repeat, bool bga);
	void GetBmsMetadata(BmsBms& bms, SongInfo &info);
	int GetInsaneDiffFromHash(const RString& hash);
}