#pragma once

/*
 * @description
 * loads, and provides song(bms) data/information.
 */

#include "global.h"
#include "bmsbel/bms_bms.h"
#include "Theme.h"

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

	int iType;		// 0: Song, 1: Course
};

/*
 * @description caches, loads song information from db.
 * But this is just a light-weight(?) player, so there's no caching function in here!
 * (Only exists for singleton theme metrics)
 */
class SongManager {
protected:
	//
	Value<int>		m_diff;
	Value<RString>	m_maintitle;

public:
	//
	bool LoadInfoFromCache(SongInfo& info, const RString& bmshash);
	void SetCurrentSong(SongInfo& info);
};

extern SongManager*		SONGMANAGER;


namespace BmsHelper {
	bool LoadBms(const RString& path, BmsBms& bms);
	void SetLoadOption(int start, int end, int repeat, bool bga);
	void GetBmsMetadata(BmsBms& bms, SongInfo &info);
	int GetInsaneDiffFromHash(const RString& hash);
}