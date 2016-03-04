#include "Surface.h"
#include "game.h"
#include "util.h"
#include "SOIL.h"
#include <assert.h>
using namespace Display;

extern "C" {
#include "ffmpeg/libavutil/avutil.h"
}






#define R(x) ((x) >> 24 & 0x000000FF)
#define G(x) ((x) >> 16 & 0x000000FF)
#define B(x) ((x) >> 8 & 0x000000FF)

bool Surface::IsLoaded() {
	return pixdata != 0;
}

bool Surface::Load(const char* path) {
	Release();
	int channels;
	pixdata = SOIL_load_image
		(
		path,
		&width, &height, &channels,
		SOIL_LOAD_RGBA			// SOIL_LOAD_AUTO - dont use this, we'll force RGBA.
		);
	assert(channels == 4);
	return pixdata != 0;
}

bool Surface::LoadFromMemory(const unsigned char* ptr, int len) {
	Release();
	int channels;
	pixdata =
		SOIL_load_image_from_memory(ptr, len, &width, &height, &channels, SOIL_LOAD_RGBA);
	assert(channels == 4);
	return pixdata != 0;
}

void Surface::UpdateSurface(Uint32 msec) {
	// do nothing
}

void Surface::Create(int width, int height, uint32_t color) {
	pixdata = (unsigned char*)malloc(width * height * 4);
	for (int i = 0; i < width * height; i++) {
		((uint32_t*)pixdata)[i] = color;
	}
}

void Surface::SetPixel(int x, int y, uint32_t color) {
	if (x > width) return;
	if (y > height) return;
	assert(x >= 0 && y >= 0);
	((uint32_t*)pixdata)[width * y + x] = color;
}

uint32_t Surface::GetPixel(int x, int y) {
	assert(x >= 0 && y >= 0 && x <= width && y <= height);
	return ((uint32_t*)pixdata)[width * y + x];
}

void Surface::RemoveColor(uint32_t clr) {
	unsigned char r_ = R(clr);
	unsigned char g_ = G(clr);
	unsigned char b_ = B(clr);
	for (int i = 0; i < width * height; i++) {
		if (pixdata[i * 4 + 0] == r_ &&
			pixdata[i * 4 + 1] == g_ &&
			pixdata[i * 4 + 2] == b_)
			pixdata[i * 4 + 3] = 0;
	}
}

bool Surface::IsMovie() { return false; }

void Surface::Release() {
	if (pixdata) {
		free(pixdata);
		pixdata = 0;
	}
}







namespace {
	bool init_ffmpeg = false;
	void Initalize_ffmpeg() {
		// initalize library
		// This registers all available file formats and codecs
		if (!init_ffmpeg) {
			av_register_all();
			init_ffmpeg = true;
		}
	}
}

SurfaceMovie::SurfaceMovie() {
	frame = 0;
	uPlane = 0;
	vPlane = 0;
	yPlane = 0;
	codecctxorig = 0;
	codecctx = 0;
	moviectx = 0;

	width = height = 0;
	moviepts = 0;
	duration = 0;
}

SurfaceMovie::~SurfaceMovie() {
	Release();
}

bool SurfaceMovie::Load(const char* path) {
	Initalize_ffmpeg();
	std::string ext = get_fileext(path);	MakeLower(ext);
	// well, much more extension?
	if (ext == ".mpg" || ext == ".avi" || ext == ".mpeg"
		|| ext == ".m1v" || ext == ".wmv" || ext == ".aac") {
		if (!LoadMovie(path)) {
			ReleaseMovie();
			return false;
		} else return true;
	}
	else {
		return ((Surface*)this)->Load(path);
	}
}

bool SurfaceMovie::LoadFromMemory(const unsigned char* data, int len) {
	// not supported; just call Surface::LoadFromMemory()
	// TODO
	return Surface::LoadFromMemory(data, len);
}

bool SurfaceMovie::LoadMovie(const char* path) {
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

	// create texture
	// -> removed, DIY

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

	// set metadata
	double time_base = (double)stream->time_base.num / (double)stream->time_base.den;
	duration = (double)stream->duration * time_base * 1000.0;
	width = codecctx->width;
	height = codecctx->height;

	// set video pos to first & render first scene
	// Reset(0);		<- TODO
	moviepts = 0;
	// just create blank surface
	Create(width, height);
	//UpdateSurface(0);

	return true;
}

void SurfaceMovie::UpdateSurface(Uint32 t) {
	// if not loaded, then return
	if (moviectx == 0)
		return;

	// less then movie time, then return
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

				// Update Surface
				// TODO
				// glTexSubImage2D is a way faster
				//glBindTexture(GL_TEXTURE_2D, );
				//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, codecctx->width, codecctx->height,
				//	GL_RGB, GL_UNSIGNED_BYTE, frame->data);
			}
			break;
		}
	}
	av_packet_free(&packet);;
}

void SurfaceMovie::ReleaseMovie() {
	// we don't release texture, it'll automatically destroyed in ~Image();
	if (frame) { av_frame_free(&frame); frame = 0; }
	if (yPlane) { free(yPlane); yPlane = 0; }
	if (uPlane) { free(uPlane); uPlane = 0; }
	if (vPlane) { free(vPlane); vPlane = 0; }
	if (codecctxorig) { avcodec_close(codecctxorig); codecctxorig = 0; }
	if (codecctx) { avcodec_close(codecctx); codecctx = 0; }
	if (moviectx) { avformat_close_input(&moviectx); moviectx = 0; }
}

bool SurfaceMovie::IsMovie() {
	return (moviectx != 0);
}

void SurfaceMovie::Release() {
	ReleaseMovie();
	Surface::Release();
}


#include "File.h"

namespace SurfaceUtil {
	Surface* LoadSurface(const char* filepath) {
		Surface *surf = new Surface();
		if (!FILEMANAGER->IsMountedFile(filepath)) {
			RString abspath = FILEMANAGER->GetAbsolutePath(filepath);
			if (!surf->Load(abspath)) {
				// attempt to load with surfacemovie
				delete surf;
				surf = new SurfaceMovie();
				surf->Load(abspath);
			}
		}
		else {
			char *p;
			int len;
			if (FILEMANAGER->ReadAllFile(filepath, &p, &len)) {
				surf->LoadFromMemory((unsigned char*)p, len);
				free(p);
			}
		}

		if (!surf->IsLoaded()) {
			delete surf;
			surf = 0;
			return 0;
		}

		return surf;
	}

	Display::Texture* LoadTexture(const char* filepath) {
		Surface *surf = LoadSurface(filepath);
		if (!surf) return 0;

		Texture* tex = DISPLAY->CreateTexture(surf);
		// if movie is surface,
		// then store it to texture for updating texture.
		if (surf->IsMovie()) {
			tex->surf = surf;
		} else delete surf;
		return tex;
	}

	void UpdateTexture(Display::Texture* tex) {
		if (tex && tex->surf) {
			DISPLAY->UpdateTexture(tex, tex->surf);
		}
	}

	Display::Texture* CreateColorTexture(Uint32 clr) {
		Surface *surf = new Surface();
		surf->Create(1, 1, clr);
		Texture* tex = DISPLAY->CreateTexture(surf);
		delete surf;
		return tex;
	}
}