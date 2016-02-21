#pragma once

#include "global.h"
#include "file.h"
#include "GL/glew.h"
#include "Display.h"





/*
 * @description
 * stopped, static surface object
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
	virtual bool LoadFromMemory(const unsigned char* fileptr, int len, int width, int height);
	void Create(int width, int height, uint32_t color = 0);
	void SetPixel(int x, int y, uint32_t color);
	uint32_t GetPixel(int x, int y);
	bool RemoveColor(uint32_t clr);		// sets specific color's alpha to zero

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
	double movielength;

public:
	SurfaceMovie();
	~SurfaceMovie();
	virtual void Release();

	virtual bool Load(const char* path);
	virtual bool LoadFromMemory(const unsigned char* fileptr, int len, int width, int height);

	bool LoadMovie(const char* path);
	void ReleaseMovie();
	void UpdateSurface(unsigned msec);
};







/*
 * @description
 * very basic element of renderable object
 * COMMENT: making/deleting sprite is independent with texture.
 *          this acts like static SDL_RenderCopy() method.
 */

class Sprite {
protected:
	// be careful to don't have invalid texID during rendering
	Display::Texture tex;
	virtual void Update();
	// matrixes (quad-triangle, basically)
public:
	Sprite() { tex.id = 0; tex.width = 1; tex.height = 1; };
	Sprite(Display::Texture tex) { this->tex = tex; };
	~Sprite() {};
	void SetTexture(Display::Texture tex) { this->tex = tex; }

	void SetSrc(const Display::Rect *r);
	void SetDest(const Display::Rect *r);
	void SetTilt(float sx, float sy);
	void SetRotateCenter(int x, int y);
	void SetRotateX(float r);
	void SetRotateY(float r);
	void SetRotateZ(float r);
	void SetBlending(int blend);
	virtual void Render();
};

namespace RenderHelper {
	void Render(Display::Texture* tex, const Display::Rect* src, const Display::Rect* dst);
}

