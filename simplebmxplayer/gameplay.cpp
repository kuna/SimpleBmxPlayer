#include "gameplay.h"
#include "skin.h"
#include "skinrendertree.h"
#include "handler.h"
#include "bmsbel/bms_bms.h"
#include "bmsbel/bms_parser.h"
#include "bmsresource.h"
#include "audio.h"
#include "image.h"
#include "game.h"
#include "util.h"
#include "file.h"

SkinRenderTree rtree;
SkinOption skinoption;

namespace GamePlay {
	// bms skin related
	// skin resource will be loaded in GlobalResource
	Skin playskin;

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
	Handler::AddHandler(HANDLER::OnGamePlayBga, OnBmsBga);

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
	//SkinDST::On(33);		// autoplay on
	//SkinDST::On(41);		// BGA on

	// load play skin (lr2)
	// and create render tree
	bool r = false;
	if (strstr(path, ".lr2skin") == 0) {
		r = playskin.Parse(path);
	}
	else {
		_LR2SkinParser *lr2skin = new _LR2SkinParser();
		r = lr2skin->ParseLR2Skin(path, &playskin);
		delete lr2skin;
	}
	if (!r)
	{
		printf("Failed to load skin %s\n", path);
		return false;
	}

	// load skin option, and set Environment.
	playskin.GetDefaultOption(&skinoption);
	skinoption.SetEnvironmentFromOption();

	// load skin resource
	// - skin resources are relative to .xml file.
	RString skindirpath = IO::get_filedir(path);
	FileHelper::ConvertPathToAbsolute(skindirpath);
	FileHelper::PushBasePath(skindirpath);
	SkinRenderHelper::LoadResourceFromSkin(rtree, playskin);
	SkinRenderHelper::ConstructTreeFromSkin(rtree, playskin);
	FileHelper::PopBasePath();

	printf("Loaded Bms Skin Successfully\n");

	// prefetch note render information
	// TODO

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
	/*
	 * tick once again
	 * before scene start
	 */
	GameTimer::Tick();
	GameTimer::Start(41);	// Game Start
	gametimer.Start();
}

bool GamePlay::LoadBmsResource() {
	/*
	 * before load BMS file
	 * Set basic switch
	 * and Get Bms base directory
	 */
	std::wstring bms_dir = IO::get_filedir(bmspath) + PATH_SEPARATORW;

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
	SDL_Rect src, dest;
	SDL_Rect src_lnbody, src_lnstart, src_lnend;

	/*
	 * sync bms texture (movie)
	 */
	for (int i = 0; i < BmsConst::WORD_MAX_VALUE; i++) {
		if (bmsresource.GetBMP(i))
			bmsresource.GetBMP(i)->Sync(gametimer.GetTick());
	}

	/*
	 * draw basic skin elements
	 * - we'll going to comment it until Lua part is finished.
	 */
#if 0
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
			SDL_SetTextureBlendMode(renderdata.img->GetPtr(), SDL_BLENDMODE_NONE);
			break;
		case 1:
			SDL_SetTextureBlendMode(renderdata.img->GetPtr(), SDL_BLENDMODE_BLEND);
			break;
		case 2:
			SDL_SetTextureBlendMode(renderdata.img->GetPtr(), SDL_BLENDMODE_ADD);
			break;
		case 3:
			break;
		case 4:
			SDL_SetTextureBlendMode(renderdata.img->GetPtr(), SDL_BLENDMODE_MOD);
			break;
		}
		//SDL_SetTextureAlphaMod(renderdata.img->GetPtr(), 120);
		SDL_RenderCopy(Game::GetRenderer(), renderdata.img->GetPtr(), &src, &dest);
	}
#endif

	/*
	 * we're currently working with skin parsing,
	 * so we'll going to block under these codes currently (related with playing)
	 */
#if 0

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
#endif
}

void GamePlay::Release() {
	// skin clear (COMMENT: we don't need to release skin in real. just in beta version.)
	// we don't need to clear BMS data until next BMS is loaded
	rtree.ReleaseAll();
	rtree.ReleaseAllResources();
	playskin.Release();
	//bmsresource.Clear();
	//bms.Clear();

	// temp resource
	if (temptexture) SDL_DestroyTexture(temptexture);
}

// ---------------------------

void GamePlay::SetSpeed(double speed) {
	//speed_multiply = 900.0 / speed * 1.0;								// normal multiply (1x: show 4 beat in a screen)
	speed_multiply = (double)laneheight / speed * 1000 * (120 / bms.GetBaseBPM());	// if you use constant `green` speed ... (= 1 measure per 310ms)
}