#include "image.h"
#include "game.h"
#include "util.h"

extern "C" {
#include "ffmpeg/libavutil/avutil.h"
}

#define ISLOADED(v) (v != 0)
//
// ffmpeg initalize part
//

bool Image::_initalized = false;
bool Image::_movie_available = false;	// if initalize failed, then false

// from: http://dranger.com/ffmpeg/
void Image::_init() {
	// attempt initalize only once
	if (_initalized)
		return;
	_initalized = true;

	// initalize library
	// This registers all available file formats and codecs
	av_register_all();

	// if successfully initalized, then movie is available
	_movie_available = true;
}

Image::Image() : sdltex(0), moviectx(0),
usecolorkey(false), colorkey(0xFF000000) {
	// initalize ffmpeg first
	try {
		_init();
	}
	catch (...) {
	}

	frame = 0;
	uPlane = 0;
	vPlane = 0;
	yPlane = 0;
	codecctxorig = 0;
	codecctx = 0;
	moviectx = 0;
}

#ifdef _WIN32
Image::Image(const std::wstring& filepath, bool loop) : Image() {
	Load(filepath.c_str(), loop);
}

bool Image::Load(const std::wstring& filepath, bool loop) {
	RString path_utf8 = WStringToRString(filepath);
	return Load(path_utf8, loop);
}
#endif

Image::Image(const std::string& filepath, bool loop) : Image() {
	Load(filepath.c_str(), loop);
}

#define R(x) ((x) >> 16 & 0x000000FF)
#define G(x) ((x) >> 8 & 0x000000FF)
#define B(x) ((x) & 0x000000FF)

bool Image::Load(const std::string& filepath, bool loop) {
	RString abspath = filepath;
	FileHelper::ConvertPathToAbsolute(abspath);

	// check is it movie or image
	std::string ext = get_fileext(filepath);	MakeLower(ext);
	if (ext == ".mpg" || ext == ".avi" || ext == ".mpeg"
		|| ext == ".m1v") {
		if (_movie_available) {
			if (!LoadMovie(abspath)) {
				ReleaseMovie();
				return false;
			}
		}
	}
	else {
		// MUST new context, or block renderer.
		//SDL_GLContext c = SDL_GL_CreateContext(Game::WINDOW);
		SDL_Surface *surf = IMG_Load(abspath);
		if (!surf) return false;
		if (usecolorkey) SDL_SetColorKey(surf, SDL_TRUE, 
			SDL_MapRGB(surf->format, R(colorkey), G(colorkey), B(colorkey)));
		Game::RMUTEX.lock();
		sdltex = SDL_CreateTextureFromSurface(Game::RENDERER, surf);
		Game::RMUTEX.unlock();
		SDL_FreeSurface(surf);
		if (!sdltex)
			return false;
	}
	this->loop = loop;
	return true;
}

bool Image::Load(FileBasic* f, bool loop) {
	// don't support movie in this case.
	// TODO: support movie!
	SDL_Surface *surf = IMG_Load_RW(f->GetSDLRW(), 1);
	if (!surf) return false;
	if (usecolorkey) SDL_SetColorKey(surf, SDL_TRUE,
		SDL_MapRGB(surf->format, R(colorkey), G(colorkey), B(colorkey)));
	Game::RMUTEX.lock();
	sdltex = SDL_CreateTextureFromSurface(Game::RENDERER, surf);
	Game::RMUTEX.unlock();
	SDL_FreeSurface(surf);
	if (!sdltex) return false; else return true;
}

bool Image::LoadMovie(const char *path) {
	// load movie
	if (avformat_open_input(&moviectx, path, NULL, 0) != 0) {
		// failed to load movie
		return false;
	}
	if (avformat_find_stream_info(moviectx, 0) < 0) {
		// failed to retrieve stream information
		return false;
	}
	// find first video stream
	moviestream = -1;
	for (int i = 0; i < moviectx->nb_streams; i++) if (moviectx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		moviestream = i;
	if (moviestream < 0) {
		return false;
	}
	stream = moviectx->streams[moviestream];

	/*
	* codec start
	*/
	// get codec context pointer
	codecctxorig = stream->codec;
	// Find the decoder for the video stream
	codec = avcodec_find_decoder(codecctxorig->codec_id);
	if (codec == NULL) {
		// unsupported codec!
		return false;
	}
	// copy codec context
	codecctx = avcodec_alloc_context3(codec);
	if (avcodec_copy_context(codecctx, codecctxorig) != 0) {
		// failed to copy codec context
		return false;
	}
	// open codec
	if (avcodec_open2(codecctx, codec, NULL) < 0) {
		return false;
	}
	/*
	* codec end
	*/

	// allocate video frame
	frame = av_frame_alloc();

	// create basic texture
	sdltex = SDL_CreateTexture(Game::RENDERER, SDL_PIXELFORMAT_YV12,
		SDL_TEXTUREACCESS_STREAMING, codecctx->width, codecctx->height);
	if (sdltex == 0) {
		return false;
	}

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(codecctx->width, codecctx->height,
		codecctx->pix_fmt, codecctx->width, codecctx->height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL);
	// set up YV12 pixel array (12 bits per pixel)
	size_t yPlaneSz, uvPlaneSz;
	yPlaneSz = codecctx->width * codecctx->height;
	uvPlaneSz = codecctx->width * codecctx->height / 4;
	yPlane = (Uint8*)malloc(yPlaneSz);
	uPlane = (Uint8*)malloc(uvPlaneSz);
	vPlane = (Uint8*)malloc(uvPlaneSz);

	// set video pos to first & render first scene
	Reset();
	Sync(0);

	return true;
}

void Image::Sync(Uint32 t) {
	if (!ISLOADED(moviectx))
		return;

	if (t < moviepts)
		return;


	int uvPitch = codecctx->width / 2;
	AVPacket *packet;
	packet = av_packet_alloc();
	int frameFinished;
	while (av_read_frame(moviectx, packet) >= 0) {
		// Is this a packet from the video stream? (not audio stream!)
		if (packet->stream_index == moviestream) {
			// Decode video frame
			avcodec_decode_video2(codecctx, frame, &frameFinished, packet);

			/*
			* we need to get pts; video clock from presentation clock.
			*/
			int64_t pts;
			if (packet->dts != AV_NOPTS_VALUE) {
				pts = av_frame_get_best_effort_timestamp(frame);
			}
			else {
				pts = 0;
			}
			if (pts < 0) pts = 0;
			moviepts = pts * 1000.0 * av_q2d(stream->time_base);

			// Did we get a video frame?
			if (frameFinished) {
				AVPicture pict;
				pict.data[0] = yPlane;
				pict.data[1] = uPlane;
				pict.data[2] = vPlane;
				pict.linesize[0] = codecctx->width;
				pict.linesize[1] = uvPitch;
				pict.linesize[2] = uvPitch;

				// Convert the image into YUV format that SDL uses
				sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
					frame->linesize, 0, codecctx->height, pict.data,
					pict.linesize);
				
				SDL_UpdateYUVTexture(
					sdltex,
					NULL,
					yPlane,
					codecctx->width,
					uPlane,
					uvPitch,
					vPlane,
					uvPitch
					);
			}
			break;
		}
	}
	av_packet_free(&packet);
}

void Image::Reset() {
	// set pts to zero
	// and reset movie ctx
	moviepts = 0;
	// TODO: reset movie ctx
}

void Image::ReleaseMovie() {
	// we don't release texture, it'll automatically destroyed in ~Image();
	if (frame) { av_frame_free(&frame); frame = 0; }
	if (yPlane) { free(yPlane); yPlane = 0; }
	if (uPlane) { free(uPlane); uPlane = 0; }
	if (vPlane) { free(vPlane); vPlane = 0; }
	if (codecctxorig) { avcodec_close(codecctxorig); codecctxorig = 0; }
	if (codecctx) { avcodec_close(codecctx); codecctx = 0; }
	if (moviectx) { avformat_close_input(&moviectx); moviectx = 0; }
}

int Image::GetWidth() {
	if (!IsLoaded())
		return 0;
	int w;
	SDL_QueryTexture(sdltex, 0, 0, &w, 0);
	return w;
}

int Image::GetHeight() {
	if (!IsLoaded())
		return 0;
	int h;
	SDL_QueryTexture(sdltex, 0, 0, 0, &h);
	return h;
}

void Image::SetColorKey(bool use, Uint32 clr) {
	usecolorkey = use;
	colorkey = clr;
}

Image::~Image() {
	Release();
}

void Image::Release() {
	if (ISLOADED(moviectx))
		ReleaseMovie();
	if (ISLOADED(sdltex)) {
		SDL_DestroyTexture(sdltex);
		sdltex = 0;
	}
}

bool Image::IsLoaded() {
	return ISLOADED(sdltex);
}

SDL_Texture* Image::GetPtr() {
	return sdltex;
}

#define COLOR_ARGB(a, r, g, b) ((a) << 24 | (r) << 16 | (g) << 8 | (b))

ImageColor::ImageColor(uint32_t color, int w, int h) {
	int pitch;
	Uint32 *p;
	sdltex = SDL_CreateTexture(Game::RENDERER, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	SDL_LockTexture(sdltex, 0, (void**)&p, &pitch);
	for (int i = 0; i < w * h; i++)
		p[i] = color;
	SDL_UnlockTexture(sdltex);
}