/*
 * @description loads/manages Bms file & Bms resources. (only single object)
 * Contains: Bms / Bms Resource / Bms Progress(Includes BGA/BGM)
 * - we don't need to have multiple-bms-files-loaded states. only one file is enough.
 *   so we have only one bms global object. if multi file is necessary, then we may have to refactor it ...
 */

#pragma once

#include "audio.h"
#include "image.h"

#include "bmsbel\bms_bms.h"
#include "bmsbel\bms_define.h"
#include <vector>

namespace BmsResource {
	typedef Audio BmsWav;
	typedef Image BmsBmp;

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
	};

	bool IsBmsResourceLoaded();
	double GetBmsResourceLoadingProgress();

	extern SoundPool	SOUND;
	extern ImagePool	IMAGE;
	extern BmsBms		BMS;
}

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
	/** @brief Release all bms resources. include note data. */
	void ReleaseAll();

	/** @brief plays sound. pauses if that sound(channel) is already playing. */
	void PlaySound(int channel);

	/** @brief Update time. BGA/BGM is automatically setted by progressed time. */
	void Update(uint32_t time);
	/***/
	bool IsFinished(uint32_t time);
	/** @brief Reset time. BGA/BGM is resetted to pointing time. */
	void ResetTime(uint32_t time);

	struct BGAInformation {
		BmsWord missbga;
		BmsWord mainbga;
		BmsWord layer1bga;
		BmsWord layer2bga;
	};

	Image* GetMissBGA();
	Image* GetMainBGA();
	Image* GetLayer1BGA();
	Image* GetLayer2BGA();

	/** @brief get current bar(not measure). not in general use; for Player object. */
	uint32_t GetCurrentBar();
	uint32_t GetEndTime();

	double GetCurrentPosFromTime(double time_sec);

	double GetCurrentPosFromBar(int bar);

	double GetCurrentTimeFromBar(int bar);
}