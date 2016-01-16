#include "bmsresource.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include <mutex>

#include "pthread\pthread.h"
#include "bmsbel\bms_parser.h"

// TODO: call handler? if soundcall is required.

namespace BmsResource {
	/** @brief this mutex used when loading BMS resource */
	std::mutex mutex_bmsresource;

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
			LOG->Warn("[Warning] %ls - cannot load WAV file\n", wav_path.c_str());
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
			LOG->Warn("[Warning] %s - cannot load BMP file\n", bmp_path.c_str());
			delete image;
			return false;
		}
		return true;
	}

	SoundPool	SOUND;
	ImagePool	IMAGE;
	BmsBms		BMS;
	RString		bmspath;
}

namespace BmsHelper {
	bool LoadBms(const RString& path) {
		// load bms file
		try {
			BmsResource::BMS.Clear();
			BmsResource::bmspath = path;
			wchar_t wpath[1024];
			ENCODING::utf8_to_wchar(path, wpath, 1024);
			BmsParser::Parse(wpath, BmsResource::BMS);		// TODO: replace it with UTF8
		}
		catch (BmsException &e) {
			wprintf(L"%ls\n", e.Message());
			return false;
		}
		return true;
	}


	bool LoadBmsResource() {
		/*
		* before load BMS file
		* Set basic switch
		* and Get Bms base directory
		*/
		FileHelper::PushBasePath(IO::get_filedir(BmsResource::bmspath).c_str());

		// load WAV/BMP
		for (unsigned int i = 0; i < BmsConst::WORD_MAX_COUNT; ++i) {
			BmsWord word(i);
			if (BmsResource::BMS.GetRegistArraySet()["WAV"].IsExists(word)) {
				BmsResource::IMAGE.Load(i, BmsResource::BMS.GetRegistArraySet()["WAV"][word]);
			}
			if (BmsResource::BMS.GetRegistArraySet()["BMP"].IsExists(word)) {
				BmsResource::SOUND.Load(i, BmsResource::BMS.GetRegistArraySet()["BMP"][word]);
			}
		}

		FileHelper::PopBasePath();
		return true;
	}

	void LoadBmsOnThread(const RString &path) {

	}

	void UpdateTime(uint32_t time) {
		/*
		* sync bms texture (movie)
		*/
		for (int i = 0; i < BmsConst::WORD_MAX_VALUE; i++) {
			if (BmsResource::IMAGE.Get(i))
				BmsResource::IMAGE.Get(i)->Sync(time);
		}

		/*
		 * (TODO) sync background image
		 */

		/*
		 * (TODO) check for background keysound
		 */
	}

	void PlaySound(int channel) {
		BmsResource::SOUND.Play(channel);
	}
}