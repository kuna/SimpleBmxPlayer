#include "SongPlayer.h"
#include "Pool.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "Game.h"
#include <mutex>

#include "pthread\pthread.h"
#include "bmsbel\bms_parser.h"

SongPlayer*		SONGPLAYER = new SongPlayer();

// used to interrupt Bms Resource loading
bool m_AllowBmsResourceLoad = 1;




SongPlayer::SoundPool::SoundPool() {
	memset(wav_table, 0, sizeof(wav_table));
}

Audio* SongPlayer::SoundPool::Get(BmsWord channel) {
	return wav_table[channel.ToInteger()];
}

bool SongPlayer::SoundPool::Play(BmsWord channel) {
	if (wav_table[channel.ToInteger()]) {
		wav_table[channel.ToInteger()]->Stop();
		wav_table[channel.ToInteger()]->Play();
		return true;
	}
	return false;
}

bool SongPlayer::SoundPool::Stop(BmsWord channel) {
	if (wav_table[channel.ToInteger()]) {
		wav_table[channel.ToInteger()]->Stop();
		return true;
	}
	return false;
}

void SongPlayer::SoundPool::UnloadAll() {
	for (int i = 0; i < BmsConst::WORD_MAX_COUNT; i++)
		SAFE_DELETE(wav_table[i]);
}

bool SongPlayer::SoundPool::Load(BmsWord channel, const RString &path) {
	RString wav_path = path;
	RString alter_ogg_path = substitute_extension(path, ".ogg");
	// don't use absolute path here
	//FileHelper::ConvertPathToAbsolute(wav_path);
	//FileHelper::ConvertPathToAbsolute(alter_ogg_path);
	Audio *audio = new Audio();
	FileBasic *file = 0;
	if (!FileHelper::LoadFile(alter_ogg_path, &file))
		FileHelper::LoadFile(wav_path, &file);
	if (file) {
		audio->Load(file, channel.ToInteger());
		file->Close();
		delete file;
	}

	if (audio && audio->IsLoaded()) {
		wav_table[channel.ToInteger()] = audio;
	}
	else {
		if (audio) delete audio;
		return false;
	}
	return true;
}

SongPlayer::ImagePool::ImagePool() {
	memset(bmp_table, 0, sizeof(bmp_table));
}

Display::Texture* SongPlayer::ImagePool::Get(BmsWord channel) {
	return bmp_table[channel.ToInteger()];
}

bool SongPlayer::ImagePool::Load(BmsWord channel, const RString &path) {
	RString bmp_path = path;
	// don't use absolute path here
	//FileHelper::ConvertPathToAbsolute(bmp_path);

	SurfaceMovie *surf = new SurfaceMovie();
	/*
	* currently reading from FileBasic doesn't support ffmpeg
	* so we should check if we're reading from memory or not
	*/
	if (FileHelper::CurrentDirectoryIsZipFile()) {
		FileBasic *file = 0;
		FileHelper::LoadFile(bmp_path, &file);
		if (file) {
			unsigned char *iMem = (unsigned char*)malloc(1024000);
			int iSize = file->Read((char*)iMem, 1024000);
			surf->LoadFromMemory(iMem, iSize);
			file->Close();
			free(iMem);
			delete file;
		}
	}
	else {
		surf->Load(bmp_path);
	}
	// colorkey
	surf->RemoveColor(0x000000FF);

	if (surf->IsLoaded()) {
		// make texture from surface
		bmp_table[channel.ToInteger()] = DISPLAY->CreateTexture(surf);
		// if movie, then store surface
		if (surf->IsMovie())
			mov_table[channel.ToInteger()] = surf;
		else
			delete surf;
	}
	else {
		LOG->Warn("[Warning] %s - cannot load BMP file", bmp_path.c_str());
		delete surf;
		return false;
	}
	return true;
}

void SongPlayer::ImagePool::Update(uint32_t msec) {
	for (int i = 0; i < BmsConst::WORD_MAX_COUNT; i++) {
		if (mov_table[i]) {
			mov_table[i]->UpdateSurface(msec);
			DISPLAY->UpdateTexture(bmp_table[i], mov_table[i]);
		}
	}
}

void SongPlayer::ImagePool::UnloadAll() {
	for (int i = 0; i < BmsConst::WORD_MAX_COUNT; i++)
		SAFE_DELETE(bmp_table[i]);
}







SongPlayer::SongPlayer() {
	m_BmsLoading = false;
	m_BmsStart = 0;
	m_BmsEnd = 1000;
	m_BmsRepeat = 1;

	// clear iter & variables
	Reset(0);
}

void SongPlayer::LoadBmsResource(const BmsBms& bms) {
	// automatically cleanup
	Cleanup();

	// copy
	m_Bms = bms;

	// reset iterators
	BmsChannel& bgmchannel = m_Bms.GetChannelManager()[BmsWord(1)];
	bgm_channel_cnt_ = bgmchannel.GetBufferCount();
	Reset(0);
	
	// load bms resource from metadata
	m_BmsLoading = true;
	SWITCH_ON("SongLoading");
	for (int i = 0; i < BmsConst::WORD_MAX_COUNT && m_AllowBmsResourceLoad; i++) {
		if (m_Bms.GetRegistArraySet()["WAV"].IsExists(i)) {
			const RString path = m_Bms.GetRegistArraySet()["WAV"][i];
			if (!m_Sound.Load(i, path))
				LOG->Warn("[BMS Warning] %s - cannot load WAV file", path.c_str());
			else if (m_Rate != 1) {
				// resample
				m_Sound.Get(i)->Resample(m_Rate);
			}
		}
		if (m_Bms.GetRegistArraySet()["BMP"].IsExists(i)) {
			const RString path = m_Bms.GetRegistArraySet()["BMP"][i];
			if (!m_Image.Load(i, path))
				LOG->Warn("[BMS Warning] %s - cannot load BMP file", path.c_str());
		}
		
		// update loading percentage
	}
	m_BmsLoading = false;
	SWITCH_OFF("SongLoading");
	SWITCH_ON("SongLoadingEnd");
}

void SongPlayer::Cleanup() {
	m_Bms.Clear();
	m_Sound.UnloadAll();
	m_Image.UnloadAll();
}

void SongPlayer::Reset(uint32_t msec) {
	barindex bar = m_Bms.GetTimeManager().GetBarFromTime(msec / 1000.0);

	// clear variables
	currenttime = msec;
	currentbar = bar;
	memset(&currentbga, 0, sizeof(currentbga));
	bmsbar_index = 0;

	// set current bpm
	// (TODO)

	// calculate bms duration, as it's static value
	bmsduration = m_Bms.GetEndTime();

	// reset iterator
	BmsChannel& bgmchannel = m_Bms.GetChannelManager()[BmsWord(1)];
	BmsBuffer& missbgachannel = m_Bms.GetChannelManager()[BmsWord(6)].GetBuffer();
	BmsBuffer& bgachannel = m_Bms.GetChannelManager()[BmsWord(4)].GetBuffer();
	BmsBuffer& bgachannel2 = m_Bms.GetChannelManager()[BmsWord(7)].GetBuffer();
	BmsBuffer& bgachannel3 = m_Bms.GetChannelManager()[BmsWord(10)].GetBuffer();

	for (int i = 0; i < bgm_channel_cnt_; i++) {
		bgm_iter_[i] = bgmchannel[i].Begin(bar);
	}
	bga_iter_ = bgachannel.Begin(bar);
	bga1_iter_ = bgachannel2.Begin(bar);
	bga2_iter_ = bgachannel3.Begin(bar);
	bga_miss_iter_ = missbgachannel.Begin(bar);
}

void SongPlayer::Update(uint32_t msec) {
	barindex bar = m_Bms.GetTimeManager().GetBarFromTime(msec * m_Rate / 1000.0);

	/*
	 * BMS related timer/value update
	 */
	int remaintime = GetEndTime() - msec;
	if (remaintime < 0) remaintime = 0;
	*SONGVALUE.PlayProgress
		= (double)msec / GetEndTime();
	*SONGVALUE.PlayBPM = GetCurrentBpm();
	*SONGVALUE.PlayMin = msec / 1000 / 60;
	*SONGVALUE.PlaySec = msec / 1000 % 60;
	*SONGVALUE.PlayRemainMin = remaintime / 1000 / 60;
	*SONGVALUE.PlayRemainSec = remaintime / 1000 % 60;


	// check iterator difference(BGA / BGM) from previous one
	for (int i = 0; i < bgm_channel_cnt_; i++) {
		if (m_Bms.GetBarManager().GetMeasureByBar(bar) * 4 >= bmsbar_index) {
			bmsbar_index++;
			SONGVALUE.OnBeat->Start();
		}

		for (int i = 0; i < bgm_channel_cnt_; i++) {
			for (; bgm_iter_[i] != m_Bms.GetChannelManager()[1][i].End(); ++bgm_iter_[i]) {
				if (currentbar < bgm_iter_[i]->first)
					break;
				BmsWord current_word(bgm_iter_[i]->second);
				if (current_word == BmsWord::MIN)
					continue;
				m_Sound.Play(current_word.ToInteger());
			}
		}
		for (; bga_miss_iter_ != m_Bms.GetChannelManager()[6][0].End(); ++bga_miss_iter_) {
			if (currentbar < bga_miss_iter_->first)
				break;
			BmsWord current_word(bga_miss_iter_->second);
			if (current_word == BmsWord::MIN)
				continue;
			currentbga.missbga = current_word;
		}
		for (; bga_iter_ != m_Bms.GetChannelManager()[4][0].End(); ++bga_iter_) {
			if (currentbar < bga_iter_->first)
				break;
			BmsWord current_word(bga_iter_->second);
			if (current_word == BmsWord::MIN)
				continue;
			SONGVALUE.OnBgaMain->Start();
			currentbga.mainbga = current_word;
		}
		for (; bga1_iter_ != m_Bms.GetChannelManager()[7][0].End(); ++bga1_iter_) {
			if (currentbar < bga1_iter_->first)
				break;
			BmsWord current_word(bga1_iter_->second);
			if (current_word == BmsWord::MIN)
				continue;
			SONGVALUE.OnBgaLayer1->Start();
			currentbga.layer1bga = current_word;
		}
		for (; bga2_iter_ != m_Bms.GetChannelManager()[10][0].End(); ++bga2_iter_) {
			if (currentbar < bga2_iter_->first)
				break;
			BmsWord current_word(bga2_iter_->second);
			if (current_word == BmsWord::MIN)
				continue;
			SONGVALUE.OnBgaLayer2->Start();
			currentbga.layer2bga = current_word;
		}
	}
}







void SongPlayer::PlayKeySound(BmsWord v) {
	m_Sound.Play(v);
}

void SongPlayer::StopAllSound() {
	for (int i = 0; i < BmsConst::WORD_MAX_COUNT; i++)
		m_Sound.Stop(i);
}

double SongPlayer::GetCurrentBar(){
	return currentbar;
}

double SongPlayer::GetCurrentPos() {
	// calculate current scroll pos
	return m_Bms.GetBarManager().GetPosByBar(currentbar);
}

double SongPlayer::GetCurrentConstPos(){
	// if constant, calculate speed from time
	// (Base Speed to 120 BPM)
	// (TODO)
	return currenttime / 1000;
}

double SongPlayer::GetEndTime() {
	return bmsduration;
}

double SongPlayer::GetCurrentBpm(){
	return m_Bms.GetTimeManager().GetBPMFromBar(currentbar);
}

double SongPlayer::GetMaxBPM() {
	return m_Bms.GetTimeManager().GetMaxBPM();
}

double SongPlayer::GetMinBPM(){
	return m_Bms.GetTimeManager().GetMinBPM();
}

double SongPlayer::GetMediumBPM() {
	return m_Bms.GetTimeManager().GetMediumBPM();
}

BmsBms* SongPlayer::GetBmsObject() { return &m_Bms; }

Display::Texture* SongPlayer::GetMainBGA() {
	if (currentbga.mainbga != 0) {
		return m_Image.Get(currentbga.mainbga);
	}
	else {
		return 0;
	}
}

Display::Texture *SongPlayer::GetMissBGA() {
	return m_Image.Get(currentbga.missbga);
}

Display::Texture *SongPlayer::GetLayer1BGA() {
	if (currentbga.layer1bga != 0) {
		return m_Image.Get(currentbga.layer1bga);
	}
	else {
		return 0;
	}
}

Display::Texture *SongPlayer::GetLayer2BGA() {
	if (currentbga.layer2bga != 0) {
		return m_Image.Get(currentbga.layer2bga);
	}
	else {
		return 0;
	}
}







namespace BmsHelper {
	pthread_t		t_bmsresource;
	BmsBms			*m_bms;


	void StopBmsLoadingOnThread() {
		// safely stops bms loading
		if (!SONGPLAYER->IsBmsLoading()) return;
		m_AllowBmsResourceLoad = 0;
		pthread_join(t_bmsresource, 0);
		m_AllowBmsResourceLoad = 1;
	}

	void* _LoadBms(void*) { SONGPLAYER->LoadBmsResource(*m_bms); }
	void LoadBmsOnThread(BmsBms& bms) {
		m_bms = &bms;
		pthread_create(&t_bmsresource, 0, _LoadBms, 0);
	}

	void ReleaseAll() {
		// first check is bms loading
		StopBmsLoadingOnThread();
		// and release all resources
		SONGPLAYER->Cleanup();
	}

	void Update() {
		SONGPLAYER->Update(SONGVALUE.SongTime->GetTick());
	}
}




// TODO: lua