#include "image.h"
#include "game.h"
#include "util.h"
#include "SOIL.h"
#include <assert.h>

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

Image::Image() : texid(0), moviectx(0),
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

	width = height = 0;
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
namespace {
	void DoColorkey(unsigned char* ptr, int width, int height, Uint32 colorkey) {
		// channel must be 4 channels - RGBA
		for (int i = 0; i < width*height*4; i += 4)
		{
			if (ptr[i] == R(colorkey) && ptr[i + 1] == G(colorkey) && ptr[i + 2] == B(colorkey))
				ptr[i + 3] = 0;
		}
	}

	void LoadTexture(const unsigned char* imgdata, GLuint *texid, int width, int height) {
		Game::RMUTEX.lock();
		glGenTextures(1, texid);
		// maybe we don't going to deal with multiple texture rendering
		// so don't use it
		//glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, *texid);
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			width,
			height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			imgdata);
		Game::RMUTEX.unlock();
	}
}

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
		//
		//glTex = SOIL_load_OGL_texture(abspath, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
		//
		// although we can make easily new texture with upper code,
		// we need to make colorkey in case, so iterate each pixels.
		//
		int channels;
		unsigned char *imgdata = SOIL_load_image
			(
			abspath,
			&width, &height, &channels,
			SOIL_LOAD_RGBA			// SOIL_LOAD_AUTO ?
			);
		if (!imgdata) return false;
		assert(channels == 4);		// need alpha channel MUST

		DoColorkey(imgdata, width, height, colorkey);
		LoadTexture(imgdata, &texid, width, height);
		SOIL_free_image_data(imgdata);
		if (!texid)
			return false;
	}
	this->loop = loop;
	return true;
}

bool Image::Load(FileBasic* f, bool loop) {
	// don't support movie in this case.
	// TODO: support movie!
	size_t s = f->GetSize();
	char* imgrawdata = (char*)malloc(s);
	f->Read(imgrawdata, s);

	int channels;
	unsigned char* imgdata = 
		SOIL_load_image_from_memory((unsigned char*)imgrawdata, s, &width, &height, &channels, SOIL_LOAD_RGBA);
	free(imgrawdata);
	if (!imgdata) return false;
	assert(channels == 4);		// need alpha channel MUST

	DoColorkey(imgdata, width, height, colorkey);
	LoadTexture(imgdata, &texid, width, height);
	SOIL_free_image_data(imgdata);
	if (!texid)
		return false;
	this->loop = loop;
	return true;
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
	Game::RMUTEX.lock();
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	const size_t _tsize = codecctx->width * codecctx->height * 3;
	unsigned char *_tmp = (unsigned char*)malloc(_tsize);
	memset(_tmp, 0, _tsize);
	glTexImage2D(GL_TEXTURE_2D,              //Always GL_TEXTURE_2D
		0,                                   //0 for now
		GL_RGB,                              //Format OpenGL uses for image
		codecctx->width, codecctx->height,   //Width and height
		0,                                   //The border of the image
		GL_RGB,                              //GL_RGB, because pixels are stored in RGB format
		GL_UNSIGNED_BYTE,                    //GL_UNSIGNED_BYTE, because pixels are stored as unsigned numbers
		_tmp);                               //The actual pixel data
	free(_tmp);
	Game::RMUTEX.unlock();

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(codecctx->width, codecctx->height,
		codecctx->pix_fmt, codecctx->width, codecctx->height,
		AV_PIX_FMT_RGB24,
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
				static AVPicture pict;
				pict.data[0] = yPlane;
				pict.data[1] = uPlane;
				pict.data[2] = vPlane;
				pict.linesize[0] = codecctx->width;
				pict.linesize[1] = uvPitch;
				pict.linesize[2] = uvPitch;

				// Convert the image into RGB format that SDL uses
				sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
					frame->linesize, 0, codecctx->height, pict.data,
					pict.linesize);
				
				// glTexSubImage2D is a way faster
				glBindTexture(GL_TEXTURE_2D, texid);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, codecctx->width, codecctx->height, 
					GL_RGB, GL_UNSIGNED_BYTE, frame->data);
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
	return width;
}

int Image::GetHeight() {
	return height;
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
	if (ISLOADED(texid)) {
		glDeleteTextures(1, &texid);
		texid = 0;
	}
}

bool Image::IsLoaded() {
	return ISLOADED(texid);
}

GLuint Image::GetTexID() {
	return texid;
}
