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

	RString sHash;
	uint32_t iLength;
};

/*
 * @description caches, loads song information from db.
 * But this is just a light-weight(?) player, so there's no caching function in here!
 * (Only exists for singleton theme metrics)
 */
class SongManager {
protected:
	// only prototype
	void UpdateDirectory(const RString& dirpath);
	void UpdateSong(const RString& path);
	// end


	// theme metrics - currently selected song
	Value<RString>		sMainTitle;
	Value<RString>		sTitle;
	Value<RString>		sSubTitle;
	Value<RString>		sGenre;
	Value<RString>		sArtist;
	Value<RString>		sSubArtist;
	Value<int>			iPlayLevel;
	Value<int>			iPlayDifficulty;
	SwitchValue			DiffSwitch[6];

	RString				sBackbmp;
	RString				sBanner;
	Display::Texture*	tex_Backbmp = 0;
	Display::Texture*	tex_Banner = 0;
public:
	SongManager();
	~SongManager();

	// only prototype
	void LoadSongInfoDB(const SongInfo& info);
	void CacheSongInfo(const SongInfo& info);
	void UpdateCache();
	bool IsCacheUpdating();

	bool LoadInfoFromCache(SongInfo& info, const RString& bmshash);
	void SetSelectedIndex(int idx);
	void SetParentDirectory(const RString& dirhash);
	// end

	void SetMetrics(const SongInfo& info);
	void SetMetricsIsCourse(bool v);
};

extern SongManager*		SONGMANAGER;


namespace BmsHelper {
	bool LoadBms(const RString& path, BmsBms& bms);
	void GetBmsMetadata(BmsBms& bms, SongInfo &info);
	int GetInsaneDiffFromHash(const RString& hash);

	void SetTrainingmode(bool v);
	void SetEndMeasure(int v);
	void SetBeginMeasure(int v);
	void SetRepeat(int v);
	void SetBgaChannel(bool v);
	bool IsTrainingmode();
}