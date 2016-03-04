#pragma once

#include "global.h"
#include "file.h"
#include "GL/glew.h"
#include "Display.h"





/*
 * @description
 * constant, static surface object
 */
// suggest to use RGBA format surface
// although it's very basic form of surface module,
// surface object has almost similar type, so I won't make interface
// use RGBA8 format, MUST.
class Surface {
protected:
	unsigned char* pixdata;
	int width;
	int height;
public:
	Surface() : pixdata(0) {}
	// copies surface
	Surface(const Surface& surf, 
		int sx = 0, int sy = 0, int width = 0, int height = 0);
	~Surface() { Release(); }
	virtual void Release();

	bool IsLoaded();
	virtual bool Load(const char* path);
	virtual bool LoadFromMemory(const unsigned char* fileptr, int len);
	virtual void UpdateSurface(Uint32 msec);
	void Create(int width, int height, uint32_t color = 0);
	void SetPixel(int x, int y, uint32_t color);
	uint32_t GetPixel(int x, int y);
	void RemoveColor(uint32_t clr);		// sets specific color's alpha to zero

	virtual bool IsMovie();
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
	const unsigned char* GetPtr() const { return pixdata; }
};





/*
 * @description
 * moving surface object
 * COMMENT: change library to libav?
 */
extern "C" {
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libavformat/avformat.h"
#include "ffmpeg/libavutil/frame.h"
#include "ffmpeg/libswscale/swscale.h"
}

class SurfaceMovie: public Surface {
protected:
	// ffmpeg
	AVFormatContext *moviectx;			// movie context (handle)
	AVStream *stream;					// current movie stream
	int moviestream;
	AVCodecContext *codecctxorig;		// codec context (orig)
	AVCodecContext *codecctx;			// codec context
	AVCodec *codec;						// codec
	AVFrame *frame;						// rendering buffer
	SwsContext *sws_ctx;				// converter (image data -> RGB24)
	Uint8 *yPlane, *uPlane, *vPlane;	// stores YUV data

	double duration;
	int64_t moviepts;
public:
	SurfaceMovie();
	~SurfaceMovie();
	virtual void Release();

	virtual bool Load(const char* path);
	virtual bool LoadFromMemory(const unsigned char* fileptr, int len);
	virtual void UpdateSurface(Uint32 msec);
	
	virtual bool IsMovie();
	bool LoadMovie(const char* path);
	void ReleaseMovie();
};






/*
 * simple util
 */
namespace SurfaceUtil {
	Surface* LoadSurface(const char* filepath);
	Display::Texture* LoadTexture(const char* filepath);
	void UpdateTexture(Display::Texture* tex);
	Display::Texture* CreateColorTexture(Uint32 clr);
}