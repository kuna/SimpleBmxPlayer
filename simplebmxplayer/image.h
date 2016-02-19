#pragma once

#include "global.h"
#include "file.h"
// bye-bye, dudes! now I use SOIL/GLEW.
//#include "SDL/SDL_image.h"
//#include "SDL/SDL_video.h"
#include "GL/glew.h"

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
	// texture info
	int width, height;
	GLuint texid;
	Uint32 colorkey;
	bool usecolorkey;

	// movie info
	AVFormatContext *moviectx;			// movie context (handle)
	AVStream *stream;					// current movie stream
	int moviestream;
	AVCodecContext *codecctxorig;		// codec context (orig)
	AVCodecContext *codecctx;			// codec context
	AVCodec *codec;						// codec
	AVFrame *frame;						// rendering buffer
	SwsContext *sws_ctx;				// converter (image data -> RGB24)
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
	GLuint GetTexID();
};
