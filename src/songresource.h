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

class SongResource {
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
		Surface *mov_table[BmsConst::WORD_MAX_COUNT];	// only store movie surface
	public:
		ImagePool();
		BmsBmp* Get(BmsWord channel);
		bool Load(BmsWord channel, const RString &path);
		void UnloadAll();
	};

	SoundPool m_Sound;
	ImagePool m_Image;
	BmsBms m_Bms;
public:
	bool LoadBms(const RString &bmspath);
	void Cleanup();
};

// load bms only once at a time
extern SongResource SONGRESOURCE;

//
// TODO:	BmsHelper isn't an independent module, but kind of a tool of BmsResource.
//			maybe I need to refactor this.
//
namespace BmsHelper {
	/** @brief only loads Bms data. call LoadBmsResource() to load related resources. */
	bool LoadBms(const RString &bmspath);
	/** @brief load Bms resource from loaded Bms. must call LoadBms first. */
	bool LoadBmsResource();
	/** @brief LoadBms() on multithread. callback by OnBmsLoadingEnd */
	void LoadBmsOnThread(const RString &bmspath);
	/** @brief LoadBmsResource() on multithread. callback by OnBmsLoadingEnd */
	void LoadBmsResourceOnThread();
	void SetLoadOption(int ms, int me, int r);
	/** @brief Release all bms resources & automatically stops bms loading. include note data. */
	void ReleaseAll();

	/** @brief Update time. BGA/BGM is automatically setted by progressed time. */
	void Update(uint32_t time);
	/** @brief Reset time. BGA/BGM is resetted to pointing time. */
	void Reset(uint32_t barindex = 0);

	struct BGAInformation {
		BmsWord missbga;
		BmsWord mainbga;
		BmsWord layer1bga;
		BmsWord layer2bga;
	};

	Display::Texture* GetMissBGA();
	Display::Texture* GetMainBGA();
	Display::Texture* GetLayer1BGA();
	Display::Texture* GetLayer2BGA();

	/** @brief get cached scroll bar value. */
	double GetCurrentBar();
	/** @brief get cached scroll pos value. */
	double GetCurrentPos();

	double GetCurrentBPM();
	/*
	 * @brief get end time of song. 
	 * different from BmsBms::GetEndTime(). this method considers audio length.
	 */
	double GetEndTime();
	/** MUST call AFTER load bms & BEFORE load bms audios */
	void SetRate(double length);



	void StopAllSound();
}

typedef struct {
	double*			songloadprogress;
	Timer*			OnSongLoading;
	Timer*			OnSongLoadingEnd;

	double*			PlayProgress;
	int*			PlayBPM;
	int*			PlayMin;
	int*			PlaySec;
	int*			PlayRemainMin;
	int*			PlayRemainSec;

	Timer*			OnBeat;
	Timer*			OnBgaMain;
	Timer*			OnBgaLayer1;
	Timer*			OnBgaLayer2;
} BmsValue;

extern BmsValue BMSVALUE;