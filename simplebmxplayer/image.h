#pragma once

#include "global.h"
#include "file.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_video.h"

extern "C" {
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libavformat/avformat.h"
#include "ffmpeg/libavutil/frame.h"
#include "ffmpeg/libswscale/swscale.h"
}

class Image {
private:
	// basics about codecs ...
	// so it must be initalized before you load movie
	static bool _initalized;
	static bool _movie_available;
	static void _init();

protected:
	// also includes movie!
	SDL_Texture *sdltex;
	Uint32 colorkey;
	bool usecolorkey;

	AVFormatContext *moviectx;			// movie context (handle)
	AVStream *stream;					// current movie stream
	int moviestream;
	AVCodecContext *codecctxorig;		// codec context (orig)
	AVCodecContext *codecctx;			// codec context
	AVCodec *codec;						// codec
	AVFrame *frame;						// rendering buffer
	SwsContext *sws_ctx;				// converter (image data -> YUV420)
	Uint8 *yPlane, *uPlane, *vPlane;	// stores YUV data
	double movielength;

	Uint32 moviepts;
	bool loop;

	// processing movie is a little difficult
	bool LoadMovie(const char *path);
	void ReleaseMovie();
public:
#ifdef _WIN32
	Image(const std::wstring& filepath, bool loop = true);
	bool Load(const std::wstring& filepath, bool loop = true);
#endif
	Image(const std::string& filepath, bool loop = true);
	bool Load(const std::string& filepath, bool loop = true);
	bool Load(FileBasic* f, bool loop = true);
	Image();
	~Image();
	void Release();
	int GetWidth();
	int GetHeight();

	void SetColorKey(bool usecolorkey, Uint32 colorkey);

	bool IsLoaded();
	void Reset();				// reset pos to first one
	void Sync(Uint32 t);		// refreshes texture in case of movie (loop forever if it's longer then movie)
	SDL_Texture* GetPtr();
};

/*
 * @description
 * makes a simple texture(1x1) with specificed color
 */
class ImageColor : public Image {
public:
	ImageColor(uint32_t color, int w = 1, int h = 1);
};