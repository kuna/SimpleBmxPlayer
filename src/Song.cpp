#include "Song.h"
#include "File.h"
#include "Logger.h"
#include "Pool.h"
#include "bmsbel/bms_parser.h"


namespace BmsHelper {
	int m_BmsStart = 0;
	int m_BmsEnd = 1000;
	int m_BmsRepeat = 1;
	bool m_BmsShowBga = true;

	void SetLoadOption(int start, int end, int repeat, bool bga) {
		m_BmsStart = start;
		m_BmsEnd = end;
		m_BmsRepeat = repeat;
		m_BmsShowBga = bga;
	}

	bool LoadBms(const RString& path, BmsBms& bms) {
		FileBasic *f = FILEMANAGER->LoadFile(path);
		if (!f) {
			LOG->Critical("Failed to open Bms File(%s)..", path.c_str());
			return false;
		}

		BmsParser::Parser *p = new BmsParser::Parser(bms);
		RString bmstext;
		f->ReadAll(bmstext);
		bool succeed = p->Parse(bmstext);
		delete p;
		delete f;

		if (!succeed) {
			LOG->Critical("Failed to parse Bms File(%s)..", path.c_str());
			return false;
		}

#if 0
		if (!bms.LoadBmsFile(path))
			return false;
#endif

		//
		// modify bms if necessary
		//
		if (m_BmsStart != 0 || m_BmsEnd != 1000) {
			bms.Cut(m_BmsStart, m_BmsEnd);
		}
		if (m_BmsRepeat > 1) {
			bms.Repeat(m_BmsRepeat);
		}
		if (bms.GetObjectExistsFirstMeasure() == 0) {	// <- TODO: bug fix
			// object at very first measure is too fast for us
			// so push 1 measure for player
			bms.Push(bms.GetBarManager().GetResolution());
		}

		//
		// Bga channel?
		//
		if (!m_BmsShowBga) {
			bms.GetChannelManager().DeleteChannel(BmsWord("01"));
			SWITCH_OFF("IsBGA");
		}

		LOG->Info("Bms File loading done.");
		return true;
	}

	int GetInsaneDiffFromHash(const RString& hash) {
		// TODO
		return 0;
	}

	bool ImplictSubtitle(RString& title, RString& subtitle, char s, char e) {
		int ei = title.find_last_of(e);
		if (ei == std::string::npos) return false;
		int si = title.find_last_of(s, ei - 1);
		if (si == std::string::npos) return false;
		subtitle = title.substr(si, ei - si + 1);
		title = title.substr(0, si - 1);
	}

	void GetBmsMetadata(BmsBms& bms, SongInfo &info) {
		info.iKeyCount = bms.GetKey();
		info.iLevel = 0;
		info.iDifficulty = 0;
		info.iBpm = 0;
		info.iRank = 0;
		info.iTotal = 0;
		info.iPlayer = 0;
		// TODO: difficulty sometimes automatically set by filename/subtitle...?
		bms.GetHeaders().Query("DIFFICULTY", &info.iDifficulty);
		bms.GetHeaders().Query("PLAYLEVEL", &info.iLevel);
		bms.GetHeaders().Query("BPM", &info.iLevel);
		bms.GetHeaders().Query("RANK", &info.iRank);
		bms.GetHeaders().Query("TOTAL", &info.iTotal);

		// if player has no information, then automatically figure out
		if (!bms.GetHeaders().Query("PLAYER", &info.iTotal)) {
			if (info.iKeyCount >= 10) info.iPlayer = 3;
			else info.iPlayer = 1;
		}

		// TODO: trim title
		info.sTitle.empty();
		info.sSubTitle.empty();
		info.sGenre.empty();
		info.sArtist.empty();
		info.sSubArtist.empty();
		bms.GetHeaders().Query("TITLE", info.sTitle);
		if (!bms.GetHeaders().Query("SUBTITLE", info.sSubTitle)) {
			// process 'implict subtitle' http://hitkey.nekokan.dyndns.info/cmds.htm#TITLE
			bool r = false;
			if (!r) r = ImplictSubtitle(info.sTitle, info.sSubTitle, '-', '-');
			if (!r) r = ImplictSubtitle(info.sTitle, info.sSubTitle, '~', '~');		// TODO: full-width tidal (¢¦)??
			if (!r) r = ImplictSubtitle(info.sTitle, info.sSubTitle, '(', ')');
			if (!r) r = ImplictSubtitle(info.sTitle, info.sSubTitle, '[', ']');
			if (!r) r = ImplictSubtitle(info.sTitle, info.sSubTitle, '<', '>');
			if (!r) r = ImplictSubtitle(info.sTitle, info.sSubTitle, '\"', '\"');

		}
		bms.GetHeaders().Query("ARTIST", info.sArtist);
		bms.GetHeaders().Query("SUBARTIST", info.sSubArtist);
		bms.GetHeaders().Query("GENRE", info.sGenre);

		// backbmp/banner
		info.sBackBmp.empty();
		info.sBanner.empty();
		bms.GetHeaders().Query("BACKBMP", info.sBackBmp);
		bms.GetHeaders().Query("BANNER", info.sBanner);
	}
}

SongManager*		SONGMANAGER;