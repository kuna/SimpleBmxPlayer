#include "bmsresource.h"
#include "globalresources.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "handlerargs.h"
#include <mutex>

#include "pthread\pthread.h"
#include "bmsbel\bms_parser.h"

// TODO: call handler? if soundcall is required.

namespace BmsResource {
	SoundPool::SoundPool() {
		memset(wav_table, 0, sizeof(wav_table));
	}

	Audio* SoundPool::Get(BmsWord channel) {
		return wav_table[channel.ToInteger()];
	}

	bool SoundPool::Play(BmsWord channel) {
		if (wav_table[channel.ToInteger()]) {
			wav_table[channel.ToInteger()]->Play();
			return true;
		}
		return false;
	}

	bool SoundPool::Stop(BmsWord channel) {
		if (wav_table[channel.ToInteger()]) {
			wav_table[channel.ToInteger()]->Stop();
			return true;
		}
		return false;
	}

	bool SoundPool::Load(BmsWord channel, const RString &path) {
		if (BMS.GetRegistArraySet()["WAV"].IsNotExists(channel))
			return false;
		RString wav_path = BMS.GetRegistArraySet()["WAV"][channel];
		RString alter_ogg_path = IO::substitute_extension(BMS.GetRegistArraySet()["WAV"][channel], ".ogg");
		FileHelper::ConvertPathToAbsolute(wav_path);
		FileHelper::ConvertPathToAbsolute(alter_ogg_path);
		Audio *audio = NULL;
		if (IO::is_file_exists(alter_ogg_path))
			audio = new Audio(alter_ogg_path, channel.ToInteger());
		else if (IO::is_file_exists(wav_path))
			audio = new Audio(wav_path, channel.ToInteger());

		if (audio && audio->IsLoaded()) {
			wav_table[channel.ToInteger()] = audio;
		}
		else {
			LOG->Warn("[Warning] %s - cannot load WAV file", wav_path.c_str());
			if (audio) delete audio;
			return false;
		}
		return true;
	}

	ImagePool::ImagePool() {
		memset(bmp_table, 0, sizeof(bmp_table));
	}

	Image* ImagePool::Get(BmsWord channel) {
		return bmp_table[channel.ToInteger()];
	}

	bool ImagePool::Load(BmsWord channel, const RString &path) {
		if (BMS.GetRegistArraySet()["BMP"].IsNotExists(channel))
			return false;
		RString bmp_path = BMS.GetRegistArraySet()["BMP"][channel];
		FileHelper::ConvertPathToAbsolute(bmp_path);
		Image *image = new Image(bmp_path);
		if (image->IsLoaded()) {
			bmp_table[channel.ToInteger()] = image;
		}
		else {
			LOG->Warn("[Warning] %s - cannot load BMP file", bmp_path.c_str());
			delete image;
			return false;
		}
		return true;
	}

	SoundPool			SOUND;
	ImagePool			IMAGE;
	BmsBms				BMS;
	BmsTimeManager		BMSTIME;
	RString				bmspath;
}

namespace BmsHelper {
	/** @brief this mutex used when loading BMS resource */
	std::mutex		mutex_bmsresource;
	/** @brief POSIX thread used when loading BMS file */
	pthread_t		t_bmsresource;

	/* privates */
	Uint32			currentbar;
	BGAInformation	currentbga;

	bool LoadBms(const RString& path) {
		bool succeed = false;
		// lock first, so LoadBmsResource() can wait 
		// until BMS is fully loaded.
		mutex_bmsresource.lock();

		// clear instance
		BmsResource::BMS.Clear();
		BmsResource::BMSTIME.Clear();
		BmsResource::bmspath = path;

		// load bms file & create time table
		try {
			BmsParser::Parse(path, BmsResource::BMS);
			BmsResource::BMS.CalculateTime(BmsResource::BMSTIME);
			succeed = true;
		}
		catch (BmsException &e) {
			wprintf(L"%ls\n", e.Message());
		}

		mutex_bmsresource.unlock();
		return succeed;
	}


	bool LoadBmsResource() {
		/*
		* before load BMS file
		* Set basic switch
		* and Get Bms base directory
		*/
		mutex_bmsresource.lock();
		FileHelper::PushBasePath(IO::get_filedir(BmsResource::bmspath).c_str());

		// load WAV/BMP
		for (unsigned int i = 0; i < BmsConst::WORD_MAX_COUNT; ++i) {
			BmsWord word(i);
			if (BmsResource::BMS.GetRegistArraySet()["WAV"].IsExists(word)) {
				BmsResource::SOUND.Load(i, BmsResource::BMS.GetRegistArraySet()["WAV"][word]);
			}
			if (BmsResource::BMS.GetRegistArraySet()["BMP"].IsExists(word)) {
				BmsResource::IMAGE.Load(i, BmsResource::BMS.GetRegistArraySet()["BMP"][word]);
			}
			DOUBLEPOOL->Set("SongLoadProgress", (double)i / BmsConst::WORD_MAX_COUNT);
		}
		DOUBLEPOOL->Set("SongLoadProgress", 1);

		FileHelper::PopBasePath();
		TIMERPOOL->Set("OnSongLoadingEnd");
		mutex_bmsresource.unlock();
		return true;
	}

	void* _LoadBms(void*) { return 0; }
	void LoadBmsOnThread(const RString &path) {
		// TODO
	}

	void* _LoadBmsResource(void*) { int r = LoadBmsResource() ? 1 : 0; return (void*)r; }
	void LoadBmsResourceOnThread() {
		pthread_create(&t_bmsresource, 0, _LoadBmsResource, 0);
	}

#define FOREACH_CHANNEL
	void Update(uint32_t time) {
		/*
		* BMS related timer/value update
		*/
		DOUBLEPOOL->Set("PlayProgress", time / 1000.0 / BmsResource::BMSTIME.GetEndTime());

		/*
		* sync bms texture (movie)
		*/
		for (int i = 0; i < BmsConst::WORD_MAX_VALUE; i++) {
			if (BmsResource::IMAGE.Get(i))
				BmsResource::IMAGE.Get(i)->Sync(time);
		}

		/*
		 * get new bar position
		 * if bar has no update, then exit
		 */
		Uint32 newbar = BmsResource::BMSTIME.GetBarIndexFromTime(time / 1000.0);

		/*
		 * check for background keysound & background image
		 * If there's bar change, then it'll check for keysound
		 */
		if (currentbar == newbar) return;
		for (; currentbar <= newbar; currentbar++)
		{
			// OnBeat
			if (BmsResource::BMSTIME[currentbar].measure)
				TIMERPOOL->Set("OnBeat");

			// call event handler for BGA/BGM
			BmsChannel& bgmchannel		= BmsResource::BMS.GetChannelManager()[BmsWord(1)];
			BmsChannel& missbgachannel	= BmsResource::BMS.GetChannelManager()[BmsWord(4)];
			BmsChannel& bgachannel		= BmsResource::BMS.GetChannelManager()[BmsWord(4)];
			BmsChannel& bgachannel2		= BmsResource::BMS.GetChannelManager()[BmsWord(7)];
			BmsChannel& bgachannel3		= BmsResource::BMS.GetChannelManager()[BmsWord(10)];
			for (auto it = bgmchannel.Begin(); it != bgmchannel.End(); ++it) {
				// TODO: fix bmsbuffer structure
				// only BGM can have multiple channel...?
				if (currentbar > (**it).GetLength())
					continue;
				BmsWord current_word((**it)[currentbar]);
				if (current_word == BmsWord::MIN)
					continue;
				BmsHelper::PlaySound(current_word.ToInteger());
			}
			for (auto it = bgachannel.Begin(); it != bgachannel.End(); it++) {
				if (currentbar > (**it).GetLength())
					continue;
				BmsWord current_word((**it)[currentbar]);
				if (current_word == BmsWord::MIN)
					continue;
				currentbga.mainbga = current_word;
			}
			for (auto it = bgachannel2.Begin(); it != bgachannel2.End(); it++) {
				if (currentbar > (**it).GetLength())
					continue;
				BmsWord current_word((**it)[currentbar]);
				if (current_word == BmsWord::MIN)
					continue;
				currentbga.layer1bga = current_word;
			}
			for (auto it = bgachannel3.Begin(); it != bgachannel3.End(); it++) {
				if (currentbar > (**it).GetLength())
					continue;
				BmsWord current_word((**it)[currentbar]);
				if (current_word == BmsWord::MIN)
					continue;
				currentbga.layer2bga = current_word;
			}
			for (auto it = missbgachannel.Begin(); it != missbgachannel.End(); it++) {
				if (currentbar > (**it).GetLength())
					continue;
				BmsWord current_word((**it)[currentbar]);
				if (current_word == BmsWord::MIN)
					continue;
				currentbga.missbga = current_word;
			}
		}
		currentbar = newbar;
	}

	bool IsFinished(uint32_t time) {
		return (time > BmsResource::BMSTIME.GetEndTime() * 1000.0);
	}

	void ResetTime(uint32_t time) {
		// just reset bar position
		// and check for keysound
		currentbar = BmsResource::BMSTIME.GetBarIndexFromTime(time);
		Update(time);
	}

	Uint32 GetCurrentBar() {
		return currentbar;
	}

	void PlaySound(int channel) {
		BmsResource::SOUND.Stop(channel);
		BmsResource::SOUND.Play(channel);
	}

	double GetCurrentPosFromTime(double time_sec) {
		return BmsResource::BMSTIME.GetAbsBeatFromTime(time_sec);
	}

	double GetCurrentPosFromBar(int bar) {
		return BmsResource::BMSTIME[bar].absbeat;
	}

	double GetCurrentTimeFromBar(int bar) {
		return BmsResource::BMSTIME[bar].time;
	}

	Image* GetMainBGA() {
		return currentbga.mainbga != 0 ? BmsResource::IMAGE.Get(currentbga.mainbga) : 0;
	}

	Image *GetMissBGA() {
		return BmsResource::IMAGE.Get(currentbga.missbga);
	}

	Image *GetLayer1BGA() {
		return currentbga.layer1bga != 0 ? BmsResource::IMAGE.Get(currentbga.layer1bga) : 0;
	}

	Image *GetLayer2BGA() {
		return currentbga.layer2bga != 0 ? BmsResource::IMAGE.Get(currentbga.layer2bga) : 0;
	}
}