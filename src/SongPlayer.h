/*
 * @description loads/manages Bms file & Bms resources. (only single object)
 * Contains: Bms / Bms Resource / Bms Progress(Includes BGA/BGM)
 * - we don't need to have multiple-bms-files-loaded states. only one file is enough.
 *   so we have only one bms global object. if multi file is necessary, then we may have to refactor it ...
 */

#pragma once

#include "audio.h"
#include "timer.h"
#include "Surface.h"

#include "bmsbel\bms_bms.h"
#include "bmsbel\bms_define.h"
#include <vector>

/*
 * CLAIM:
 * This class loads bms only for playing, not gather metadata.
 * To get bms metadata, use SongManager class.
 */
class SongPlayer {
protected:
	typedef Audio BmsWav;
	typedef Display::Texture BmsBmp;

	class SoundPool {
	private:
		BmsWav *wav_table[BmsConst::WORD_MAX_COUNT];
	public:
		SoundPool();
		BmsWav* Get(BmsWord channel);
		bool Play(BmsWord channel);
		bool Stop(BmsWord channel);
		bool Load(BmsWord channel, const RString &path);
		void UnloadAll();
	};

	class ImagePool {
	private:
		BmsBmp *bmp_table[BmsConst::WORD_MAX_COUNT];
	public:
		ImagePool();
		BmsBmp* Get(BmsWord channel);
		bool Load(BmsWord channel, const RString &path);
		void UnloadAll();
		void Update(uint32_t msec);
	};

	// bms related core resources
	SoundPool m_Sound;
	ImagePool m_Image;
	BmsBms m_Bms;

	// loading related variables
	double m_Rate;
	bool m_BmsLoading;
	int m_BmsEnd;
	int m_BmsStart;
	int m_BmsRepeat;

	// playing related variables
	// current iter
	int bgm_channel_cnt_;
	BmsBuffer::Iterator	bgm_iter_[100];
	BmsBuffer::Iterator	bga_iter_;
	BmsBuffer::Iterator	bga1_iter_;
	BmsBuffer::Iterator	bga2_iter_;
	BmsBuffer::Iterator	bga_miss_iter_;
	struct {
		BmsWord mainbga;
		BmsWord layer1bga, layer2bga;
		BmsWord missbga;
	} currentbga;
	int currenttime;
	double currentbar;
	int bmsbar_index;
	uint32_t bmsduration;
public:
	SongPlayer();
	~SongPlayer();

	void LoadBmsResource(BmsBms& bms);
	void Cleanup();
	bool IsBmsLoading() { return m_BmsLoading; }
	void SetRate(double v) { m_Rate = v; }
	double GetRate() { return m_Rate; }

	/** @brief update time, so it triggers timers & sounds & background changes. */
	void Update(uint32_t msec);
	/** @brief reset iterator - MUST be called if you don't start game from 0 msec */
	void Reset(uint32_t msec);

	// some utilites
	BmsBms* GetBmsObject();
	void PlayKeySound(BmsWord v);
	void StopAllSound();
	double GetEndTime();
	double GetCurrentBar();
	double GetCurrentPos();
	double GetCurrentConstPos();
	double GetCurrentBpm();
	double GetMediumBPM();
	double GetMaxBPM();
	double GetMinBPM();

	Display::Texture* GetMissBGA();
	Display::Texture* GetMainBGA();
	Display::Texture* GetLayer1BGA();
	Display::Texture* GetLayer2BGA();
};

// load/player bms only once at a time
extern SongPlayer*	SONGPLAYER;





namespace BmsHelper {
	void LoadBmsOnThread(BmsBms& bms);
	void StopBmsLoadingOnThread();
	void ReleaseAll();
	void Update();
}