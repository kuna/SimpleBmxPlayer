#include "bmsresource.h"
#include "globalresources.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "handlerargs.h"
#include <mutex>

#include "pthread\pthread.h"
#include "bmsbel\bms_parser.h"

BmsValue			BMSVALUE;

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
			wav_table[channel.ToInteger()]->Stop();
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

	void SoundPool::UnloadAll() {
		for (int i = 0; i < BmsConst::WORD_MAX_COUNT; i++)
			SAFE_DELETE(wav_table[i]);
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

	void ImagePool::UnloadAll() {
		for (int i = 0; i < BmsConst::WORD_MAX_COUNT; i++)
			SAFE_DELETE(bmp_table[i]);
	}

	SoundPool			SOUND;
	ImagePool			IMAGE;
	BmsBms				BMS;
	RString				bmspath;
}

namespace BmsHelper {
	/** @brief this mutex used when loading BMS resource */
	std::mutex		mutex_bmsresource;
	/** @brief POSIX thread used when loading BMS file */
	pthread_t		t_bmsresource;

	/* privates */
	barindex		currentbar;		// current bar's index
	double			currentpos;		// current bar's position
	BGAInformation	currentbga;
	Uint32			bmslength;		// bms length in msec
	Uint32			bmsbar_index;	// only for `OnBeat`

	// current iter
	int bgm_channel_cnt_;
	BmsBuffer::Iterator	bgm_iter_[100];
	BmsBuffer::Iterator	bga_iter_;
	BmsBuffer::Iterator	bga1_iter_;
	BmsBuffer::Iterator	bga2_iter_;
	BmsBuffer::Iterator	bga_miss_iter_;

	// cache
	BmsBuffer::Iterator	bgm_iter_end_[100];
	BmsBuffer::Iterator	bga_iter_end_;
	BmsBuffer::Iterator	bga1_iter_end_;
	BmsBuffer::Iterator	bga2_iter_end_;
	BmsBuffer::Iterator	bga_miss_iter_end_;

	void Reset(Uint32 barindex) {
		// just reset bar position & bgm/bga iterator
		currentbar = barindex;
		BmsChannel& bgmchannel = BmsResource::BMS.GetChannelManager()[BmsWord(1)];
		BmsBuffer& missbgachannel = BmsResource::BMS.GetChannelManager()[BmsWord(4)].GetBuffer();
		BmsBuffer& bgachannel = BmsResource::BMS.GetChannelManager()[BmsWord(4)].GetBuffer();
		BmsBuffer& bgachannel2 = BmsResource::BMS.GetChannelManager()[BmsWord(7)].GetBuffer();
		BmsBuffer& bgachannel3 = BmsResource::BMS.GetChannelManager()[BmsWord(10)].GetBuffer();
		bgm_channel_cnt_ = bgmchannel.GetBufferCount();
		for (int i = 0; i < bgm_channel_cnt_; i++) {
			bgm_iter_end_[i] = bgmchannel[i].End();
			bgm_iter_[i] = bgmchannel[i].Begin(barindex);
		}
		bga_iter_ = bgachannel.Begin(barindex);
		bga1_iter_ = bgachannel2.Begin(barindex);
		bga2_iter_ = bgachannel3.Begin(barindex);
		bga_miss_iter_ = missbgachannel.Begin(barindex);
		bga_iter_end_ = bgachannel.End();
		bga1_iter_end_ = bgachannel2.End();
		bga2_iter_end_ = bgachannel3.End();
		bga_miss_iter_end_ = missbgachannel.End();
	}

	/*
	 * Loading thread should not interrupted but joined.
	 * If this value is false, loading thread will be stopped by exiting loading loop.
	 * So, to stop BMS loading, make this value false and join loading thread.
	 * (or call ReleaseAll() method)
	 */
	bool isbmsloading = false;

	bool LoadBms(const RString& path) {
		bool succeed = true;
		// lock first, so LoadBmsResource() can wait 
		// until BMS is fully loaded.
		mutex_bmsresource.lock();

		// clear instance
		BmsResource::BMS.Clear();
		BmsResource::bmspath = path;
		currentbar = 0;

		// load bms file & create time table
		if (!BmsResource::BMS.LoadBmsFile(path)) {
			succeed = false;
		}

		mutex_bmsresource.unlock();

		// reset all values
		Reset();

		return succeed;
	}


	bool LoadBmsResource() {
		using namespace BmsResource;
		/*
		* before load BMS file
		* Set basic switch
		* and Get Bms base directory
		*/
		mutex_bmsresource.lock();
		isbmsloading = true;
		BMSVALUE.OnSongLoadingEnd->Stop();

		// load WAV/BMP
		FileHelper::PushBasePath(IO::get_filedir(bmspath).c_str());
		for (unsigned int i = 0; i < BmsConst::WORD_MAX_COUNT && isbmsloading; i++) {
			BmsWord word(i);
			if (BMS.GetRegistArraySet()["WAV"].IsExists(word)) {
				SOUND.Load(i, BMS.GetRegistArraySet()["WAV"][word]);
				//BmsResource::SOUND.Get(word)->Resample(0.7);
			}
			if (BMS.GetRegistArraySet()["BMP"].IsExists(word)) {
				IMAGE.Load(i, BMS.GetRegistArraySet()["BMP"][word]);
			}
			*BMSVALUE.songloadprogress = (double)i / BmsConst::WORD_MAX_COUNT;
		}
		*BMSVALUE.songloadprogress = 1;
		FileHelper::PopBasePath();

		// get bms duration (include audio length)
		BmsNoteManager *note = new BmsNoteManager();
		BMS.GetNoteData(*note);
		bmslength = 0;
		int audiolen = 0;
		for (auto itc = BMS.GetChannelManager().Begin(); itc != BMS.GetChannelManager().End(); ++itc) {
			if (!itc->second->IsShouldPlayWavChannel()) continue;
			for (auto it = itc->second->Begin(); it != itc->second->End(); ++it) {
				BmsBuffer *buf = (*it);
				for (auto itobj = buf->Begin(); itobj != buf->End(); ++itobj) {
					Uint32 t = BMS.GetTimeManager().GetTimeFromBar(itobj->first) * 1000;
					Audio *audio = SOUND.Get(itobj->second);
					if (audio)
						audiolen = audio->GetLength();
					else
						audiolen = 0;
					bmslength = max(bmslength, t + audiolen);
				}
			}
		}
		delete note;

		// end
		BMSVALUE.OnSongLoadingEnd->Start();
		isbmsloading = false;
		mutex_bmsresource.unlock();
		LOG->Info("Bms resource loading done.");
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

	void ReleaseAll() {
		// first check is bms loading
		if (isbmsloading) {
			isbmsloading = false;
			pthread_join(t_bmsresource, 0);
		}
		// and release all resources
		BmsResource::SOUND.UnloadAll();
		BmsResource::IMAGE.UnloadAll();
	}

#define FOREACH_CHANNEL
	void Update(uint32_t time) {
		using namespace BmsResource;
		/*
		* BMS related timer/value update
		*/
		int remaintime = GetEndTime() - time;
		if (remaintime < 0) remaintime = 0;
		*BMSVALUE.PlayProgress
			= (double)time / GetEndTime();
		*BMSVALUE.PlayBPM = GetCurrentBPM();
		*BMSVALUE.PlayMin = time / 1000 / 60;
		*BMSVALUE.PlaySec = time / 1000 % 60;
		*BMSVALUE.PlayRemainMin = remaintime / 1000 / 60;
		*BMSVALUE.PlayRemainSec = remaintime / 1000 % 60;

		/*
		 * get new bar position
		 */
		double bar_ = BMS.GetTimeManager().GetBarFromTime(time / 1000.0);
		currentbar = bar_;
		currentpos = BMS.GetBarManager().GetPosByBar(bar_);

		/*
		 * check for background keysound & background image
		 * If there's bar change, then it'll check for keysound
		 */
		// OnMeasure
		if (BMS.GetBarManager().GetMeasureByBar(bar_) * 4 >= bmsbar_index) {
			bmsbar_index++;
			BMSVALUE.OnBeat->Start();
		}
			
		for (int i = 0; i < bgm_channel_cnt_; i++) {
			for (; bgm_iter_[i] != bgm_iter_end_[i]; ++bgm_iter_[i]) {
				if (currentbar < bgm_iter_[i]->first)
					break;
				BmsWord current_word(bgm_iter_[i]->second);
				if (current_word == BmsWord::MIN)
					continue;
				BmsResource::SOUND.Play(current_word.ToInteger());
			}
		}
		for (; bga_iter_ != bga_iter_end_; ++bga_iter_) {
			if (currentbar < bga_iter_->first)
				break;
			BmsWord current_word(bga_iter_->second);
			if (current_word == BmsWord::MIN)
				continue;
			BMSVALUE.OnBgaMain->Start();
			currentbga.mainbga = current_word;
		}
		for (; bga1_iter_ != bga1_iter_end_; ++bga1_iter_) {
			if (currentbar < bga1_iter_->first)
				break;
			BmsWord current_word(bga1_iter_->second);
			if (current_word == BmsWord::MIN)
				continue;
			BMSVALUE.OnBgaLayer1->Start();
			currentbga.layer1bga = current_word;
		}
		for (; bga2_iter_ != bga2_iter_end_; ++bga2_iter_) {
			if (currentbar < bga2_iter_->first)
				break;
			BmsWord current_word(bga2_iter_->second);
			if (current_word == BmsWord::MIN)
				continue;
			BMSVALUE.OnBgaLayer2->Start();
			currentbga.layer2bga = current_word;
		}
		for (; bga_miss_iter_ != bga_miss_iter_end_; ++bga_miss_iter_) {
			if (currentbar < bga_miss_iter_->first)
				break;
			BmsWord current_word(bga_miss_iter_->second);
			if (current_word == BmsWord::MIN)
				continue;
			currentbga.missbga = current_word;
		}
	}

	double GetCurrentBar() {
		return currentbar;
	}

	double GetCurrentPos() {
		return currentpos;
	}

	double GetEndTime() {
		return bmslength;
	}

	double GetCurrentBPM() {
		return BmsResource::BMS.GetTimeManager().GetBPMFromBar(currentbar);
	}

	double GetMaxBPM() {
		return BmsResource::BMS.GetTimeManager().GetMaxBPM();
	}

	double GetMinBPM() {
		return BmsResource::BMS.GetTimeManager().GetMinBPM();
	}

	double GetMediumBPM() {
		return BmsResource::BMS.GetTimeManager().GetMediumBPM();
	}

	Image* GetMainBGA() {
		/*
		 * Only sync when requires
		 */
		if (currentbga.mainbga != 0) {
			Image* img = BmsResource::IMAGE.Get(currentbga.mainbga);
			if (img) img->Sync(BMSVALUE.OnBgaMain->GetTick());
			return img;
		}
		else {
			return 0;
		}
	}

	Image *GetMissBGA() {
		return BmsResource::IMAGE.Get(currentbga.missbga);
	}

	Image *GetLayer1BGA() {
		/*
		* Only sync when requires
		*/
		if (currentbga.layer1bga != 0) {
			Image* img = BmsResource::IMAGE.Get(currentbga.layer1bga);
			if (img) img->Sync(BMSVALUE.OnBgaLayer1->GetTick());
			return img;
		}
		else {
			return 0;
		}
	}

	Image *GetLayer2BGA() {
		/*
		* Only sync when requires
		*/
		if (currentbga.layer2bga != 0) {
			Image* img = BmsResource::IMAGE.Get(currentbga.layer2bga);
			if (img) img->Sync(BMSVALUE.OnBgaLayer2->GetTick());
			return img;
		}
		else {
			return 0;
		}
	}
}