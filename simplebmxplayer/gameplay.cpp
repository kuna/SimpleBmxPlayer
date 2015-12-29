#include "gameplay.h"
#include "skin.h"
#include "handler.h"
#include "bmsbel/bms_bms.h"
#include "bmsbel/bms_parser.h"
#include "bmsresource.h"
#include "audio.h"
#include "image.h"
#include "game.h"
#include "handler.h"

namespace GamePlay {
	// bms skin related
	Skin playskin;
	SkinRenderData skin_note[20];
	SkinRenderData skin_lnstart[20];
	SkinRenderData skin_lnbody[20];
	SkinRenderData skin_lnend[20];

	// bms play related
	BmsBms bms;
	BmsResource<Audio, Image> bmsresource;
	std::wstring bmspath;
	Player player[2];	// player available up to 2
	int lanestart;
	int laneheight;

	double speed_multiply;
	Timer gametimer;

	SDL_Texture* temptexture;	// only for test purpose
}


/*
* BMS Player part
*/
BmsWord bgavalue(0);
void OnBmsBga(void *p) {
	OnGamePlayBgaArg *arg = (OnGamePlayBgaArg*)p;
	if (arg->bgatype == BmsChannelType::BGA)
		bgavalue = arg->value;
}
void OnBmsSound(void *p) {
	OnGamePlaySoundArg *arg = (OnGamePlaySoundArg *)p;
	int channel = arg->value.ToInteger();
	if (GamePlay::bmsresource.IsWAVLoaded(channel)) {
		GamePlay::bmsresource.GetWAV(channel)->Stop();
		if (arg->on) GamePlay::bmsresource.GetWAV(channel)->Play();
	}
}

void GamePlay::Init() {
	// register events
	Handler::AddHandler(HANDLER::OnGamePlaySound, OnBmsSound);

	// temp resource
	int pitch;
	Uint32 *p;
	temptexture = SDL_CreateTexture(Game::GetRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 20, 10);
	SDL_LockTexture(temptexture, 0, (void**)&p, &pitch);
	for (int i = 0; i < 20 * 10; i++)
		p[i] = (255 << 24 | 255 << 16 | 120 << 8 | 120);
	SDL_UnlockTexture(temptexture);
}

bool GamePlay::LoadSkin(const char* path) {
	// load play skin
	if (!playskin.Parse(path))
	{
		printf("Failed to load skin %s\n", path);
		return false;
	}
	else {
		printf("Loaded Bms Skin Successfully\n");
	}

	// prefetch note render information
	playskin.note[0].GetRenderData(skin_note[1]);
	playskin.note[1].GetRenderData(skin_note[2]);
	playskin.note[2].GetRenderData(skin_note[3]);
	playskin.note[3].GetRenderData(skin_note[4]);
	playskin.note[4].GetRenderData(skin_note[5]);
	playskin.note[5].GetRenderData(skin_note[8]);
	playskin.note[6].GetRenderData(skin_note[9]);
	playskin.note[7].GetRenderData(skin_note[6]);
	playskin.lnbody[0].GetRenderData(skin_lnbody[1]);
	playskin.lnbody[1].GetRenderData(skin_lnbody[2]);
	playskin.lnbody[2].GetRenderData(skin_lnbody[3]);
	playskin.lnbody[3].GetRenderData(skin_lnbody[4]);
	playskin.lnbody[4].GetRenderData(skin_lnbody[5]);
	playskin.lnbody[5].GetRenderData(skin_lnbody[8]);
	playskin.lnbody[6].GetRenderData(skin_lnbody[9]);
	playskin.lnbody[7].GetRenderData(skin_lnbody[6]);
	playskin.lnstart[0].GetRenderData(skin_lnstart[1]);
	playskin.lnstart[1].GetRenderData(skin_lnstart[2]);
	playskin.lnstart[2].GetRenderData(skin_lnstart[3]);
	playskin.lnstart[3].GetRenderData(skin_lnstart[4]);
	playskin.lnstart[4].GetRenderData(skin_lnstart[5]);
	playskin.lnstart[5].GetRenderData(skin_lnstart[8]);
	playskin.lnstart[6].GetRenderData(skin_lnstart[9]);
	playskin.lnstart[7].GetRenderData(skin_lnstart[6]);
	playskin.lnend[0].GetRenderData(skin_lnend[1]);
	playskin.lnend[1].GetRenderData(skin_lnend[2]);
	playskin.lnend[2].GetRenderData(skin_lnend[3]);
	playskin.lnend[3].GetRenderData(skin_lnend[4]);
	playskin.lnend[4].GetRenderData(skin_lnend[5]);
	playskin.lnend[5].GetRenderData(skin_lnend[8]);
	playskin.lnend[6].GetRenderData(skin_lnend[9]);
	playskin.lnend[7].GetRenderData(skin_lnend[6]);

	lanestart = -skin_note[1].dst.h;
	laneheight = skin_note[1].dst.y;
	laneheight += skin_note[1].dst.h;

	return true;
}

bool GamePlay::LoadBms(std::wstring& path) {
	// load bms file
	try {
		bms.Clear();
		bmspath = path;
		BmsParser::Parse(path, bms);
	}
	catch (BmsException &e) {
		wprintf(L"%ls\n", e.Message());
		return false;
	}
	return true;
}

void GamePlay::SetPlayer(const PlayerSetting& playersetting, int playernum) {
	// set player
	player[playernum].SetPlayerSetting(playersetting);	// TODO: is this have any meaning?
	player[playernum].Prepare(&bms, 0, true, 0);

	// do some option change from player
	SetSpeed(playersetting.speed);
}

void GamePlay::Start() {
	gametimer.Start();
}

bool GamePlay::LoadBmsResource() {
	/*
	 * before load BMS file
	 * Set basic switch
	 * and Get Bms base directory
	 */
	std::wstring bms_dir = IO::get_filedir(bmspath) + PATH_SEPARATOR;
	SkinDST::On(33);		// autoplay on
	SkinDST::On(41);		// BGA on


	// load WAV/BMP
	for (unsigned int i = 0; i < BmsConst::WORD_MAX_COUNT; ++i) {
		BmsWord word(i);
		if (bms.GetRegistArraySet()[L"WAV"].IsExists(word)) {
			std::wstring wav_path = bms_dir + bms.GetRegistArraySet()[L"WAV"][word];
			std::wstring alter_ogg_path = bms_dir + IO::substitute_extension(bms.GetRegistArraySet()[L"WAV"][word], L".ogg");
			Audio *audio = NULL;
			if (IO::is_file_exists(alter_ogg_path))
				audio = new Audio(alter_ogg_path, word.ToInteger());
			else if (IO::is_file_exists(wav_path))
				audio = new Audio(wav_path, word.ToInteger());

			if (audio && audio->IsLoaded()) {
				bmsresource.SetWAV(word.ToInteger(), audio);
			}
			else {
				wprintf(L"[Warning] %ls - cannot load WAV file\n", wav_path.c_str());
				if (audio) delete audio;
			}
		}
		if (bms.GetRegistArraySet()[L"BMP"].IsExists(word)) {
			std::wstring bmp_path = bms_dir + bms.GetRegistArraySet()[L"BMP"][word];
			Image *image = new Image(bmp_path);
			if (image->IsLoaded()) {
				bmsresource.SetBMP(word.ToInteger(), image);
			}
			else {
				wprintf(L"[Warning] %ls - cannot load BMP file\n", bmp_path.c_str());
				delete image;
			}
		}
	}
	return true;
}

void GamePlay::Render() {
	/*
	 * preparation for skin/note rendering
	 */
	SkinRenderData renderdata;
	SDL_Rect src, dest;
	SDL_Rect src_lnbody, src_lnstart, src_lnend;

	/*
	 * draw basic skin elements
	 */
	for (auto it = playskin.begin(); it != playskin.end(); ++it) {
		(*it).GetRenderData(renderdata);
		if (!renderdata.img)
			continue;
		src.x = renderdata.src.x;
		src.y = renderdata.src.y;
		src.w = renderdata.src.w;
		src.h = renderdata.src.h;
		dest.x = renderdata.dst.x;
		dest.y = renderdata.dst.y;
		dest.w = renderdata.dst.w;
		dest.h = renderdata.dst.h;
		switch (renderdata.blend) {
		case 0:
			SDL_BlendMode(SDL_BLENDMODE_NONE);
			break;
		case 1:
			SDL_BlendMode(SDL_BLENDMODE_BLEND);
			break;
		case 2:
			SDL_BlendMode(SDL_BLENDMODE_ADD);
			break;
		case 3:
			break;
		case 4:
			SDL_BlendMode(SDL_BLENDMODE_MOD);
			break;
		}
		SDL_RenderCopy(Game::GetRenderer(), renderdata.img->GetPtr(), &src, &dest);
	}

	/*
	 * BGA rendering
	 */
	SDL_Rect bga_dst;
	playskin.bga.ToRect(bga_dst);
	if (bmsresource.IsBMPLoaded(bgavalue.ToInteger()))
		SDL_RenderCopy(Game::GetRenderer(), bmsresource.GetBMP(bgavalue.ToInteger())->GetPtr(), 0, &bga_dst);

	/*
	 * note rendering
	 */
	for (int pidx = 0; pidx < 2; pidx++) {
		// only for prepared player
		if (player[pidx].IsFinished())
			continue;

		// set player time(play sound) and ...
		player[pidx].SetTime(gametimer.GetTick());
		// get note pos
		double notepos = player[pidx].GetCurrentPos();
		// render notes
		int lnstartpos[20];
		bool lnstart[20];
		BmsTimeManager& bmstime = player[pidx].GetBmsTimeManager();
		BmsNoteContainer& bmsnote = player[pidx].GetBmsNotes();
		for (int i = 0; i < 20; i++) {
			// init lnstart
			dest.x = skin_note[i].dst.x;
			dest.w = skin_note[i].dst.w;
			src.x = skin_note[i].src.x;		src.y = skin_note[i].src.y;
			src.w = skin_note[i].src.w;		src.h = skin_note[i].src.h;
			src_lnbody.x = skin_lnbody[i].src.x;		src_lnbody.y = skin_lnbody[i].src.y;
			src_lnbody.w = skin_lnbody[i].src.w;		src_lnbody.h = 1;
			src_lnstart.x = skin_lnstart[i].src.x;		src_lnstart.y = skin_lnstart[i].src.y;
			src_lnstart.w = skin_lnstart[i].src.w;		src_lnstart.h = skin_lnstart[i].src.h;
			src_lnend.x = skin_lnend[i].src.x;		src_lnend.y = skin_lnend[i].src.y;
			src_lnend.w = skin_lnend[i].src.w;		src_lnend.h = skin_lnend[i].src.h;
			lnstartpos[i] = laneheight + lanestart;
			lnstart[i] = false;
			for (int nidx = player[pidx].GetCurrentNoteBar(i);
				nidx >= 0 && nidx < bmstime.GetSize();
				nidx = player[pidx].GetAvailableNoteIndex(i, nidx + 1)) {
				double ypos = laneheight - (bmstime[nidx].absbeat - notepos) * speed_multiply + lanestart;
				switch (bmsnote[i][nidx].type){
				case BmsNote::NOTE_NORMAL:
					dest.y = ypos;		dest.h = skin_note[i].dst.h;
					SDL_RenderCopy(Game::GetRenderer(), skin_note[i].img->GetPtr(), &src, &dest);
					break;
				case BmsNote::NOTE_LNSTART:
					lnstartpos[i] = ypos;
					lnstart[i] = true;
					break;
				case BmsNote::NOTE_LNEND:
					dest.y = ypos + skin_lnstart[i].dst.h;		dest.h = lnstartpos[i] - ypos - skin_lnstart[i].dst.h;
					SDL_RenderCopy(Game::GetRenderer(), skin_lnbody[i].img->GetPtr(), &src_lnbody, &dest);
					dest.y = lnstartpos[i];						dest.h = skin_lnstart[i].dst.h;
					SDL_RenderCopy(Game::GetRenderer(), skin_lnstart[i].img->GetPtr(), &src_lnstart, &dest);
					dest.y = ypos;								dest.h = skin_lnend[i].dst.h;
					SDL_RenderCopy(Game::GetRenderer(), skin_lnend[i].img->GetPtr(), &src_lnend, &dest);
					lnstart[i] = false;
					break;
				}
				// off the screen -> exit loop
				if (ypos < -lanestart)
					break;
			}
			// if LN_end hasn't found
			// then draw it to end of the screen
			if (lnstart[i]) {
				dest.y = lanestart + skin_lnstart[i].dst.h;		dest.h = lnstartpos[i] - 0 - skin_lnstart[i].dst.h;
				SDL_RenderCopy(Game::GetRenderer(), skin_lnbody[i].img->GetPtr(), &src_lnbody, &dest);
				dest.y = lnstartpos[i] + lanestart;				dest.h = skin_lnstart[i].dst.h;
				SDL_RenderCopy(Game::GetRenderer(), skin_lnstart[i].img->GetPtr(), &src_lnstart, &dest);
				dest.y = lanestart;								dest.h = skin_lnend[i].dst.h;
				SDL_RenderCopy(Game::GetRenderer(), skin_lnend[i].img->GetPtr(), &src_lnend, &dest);
			}
		}
	}
}

void GamePlay::Release() {
	// skin, bmsresource clear
	playskin.Release();
	bmsresource.Clear();
	bms.Clear();

	// temp resource
	if (temptexture) SDL_DestroyTexture(temptexture);
}

// ---------------------------

void GamePlay::SetSpeed(double speed) {
	//speed_multiply = 900.0 / speed * 1.0;								// normal multiply (1x: show 4 beat in a screen)
	speed_multiply = (double)laneheight / speed * 1000 * (120 / bms.GetBaseBPM());	// if you use constant `green` speed ... (= 1 measure per 310ms)
}