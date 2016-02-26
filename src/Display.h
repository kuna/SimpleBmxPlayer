/* *********************************************************************
 * @description
 * related to very basic modules about graphic things.
 * collection of definitions/objects, which are dependent to renderer.
 * (Renderer / Texture / Sprite)
 *
 * COMMENT: better to change object name to renderer?
   (as this object don't initalize display, in fact. SDL window object is doing that thing.)
 * *********************************************************************/

// type declaration

#pragma once
#include <stdint.h>

class Surface;
namespace Display {
	typedef struct Texture {
		unsigned id;
		unsigned width;
		unsigned height;
	};

	struct Rect {
		int x, y, w, h;
	};

	struct Point {
		int x, y;
	};

	struct PointF {
		float x, y;
	};

	struct Color {
		unsigned char r, g, b, a;
	};

	struct Rotate {
		float x, y, z;
	};
}




// interface of display object
// (2.5D)
class IDisplay {
public:
	virtual void BeginRender() = 0;
	virtual void EndRender() = 0;
	virtual bool Initialize(int width, int height) = 0;
	virtual void Release() = 0;
	virtual void SetResolution(int width, int height) = 0;

	virtual const char* GetInfo() = 0;
	virtual int GetErrorCode() = 0;
	virtual const char* GetErrorStr() = 0;
	virtual Display::Texture* CreateTexture(const Surface* surf) = 0;
	virtual Display::Texture* CreateEmptyTexture(int width, int height) = 0;
	virtual void UpdateTexture(Display::Texture* tex, const Surface* surf, int x = 0, int y = 0) = 0;
	virtual void DeleteTexture(Display::Texture* tex) = 0;

	// used when start drawing object
	virtual void PushState() = 0;
	// used when end drawing object
	virtual void PopState() = 0;

	//virtual void LoadShader(const char* vertex_fpath, const char* frag_fpath);
	virtual void SetBlendMode(int blend) = 0;
	virtual void SetTexture(const Display::Texture* tex) = 0;
	virtual void SetColorMod(unsigned char r, unsigned char g, unsigned char b, unsigned char a) = 0;
	virtual void SetSRC(const Display::Rect* r) = 0;
	virtual void SetDST(const Display::Rect* r) = 0;
	virtual void SetZPos(float z) = 0;
	virtual void SetCenter(int x, int y) = 0;
	virtual void SetRotateX(float angle) = 0;
	virtual void SetRotateY(float angle) = 0;
	virtual void SetRotateZ(float angle) = 0;
	virtual void SetTilt(float sx, float sy) = 0;
	virtual void SetOffset(int x, int y) = 0;
	virtual void SetClip(const Display::Rect* r) = 0;
	virtual void SetZoom(float zx, float zy) = 0;
	virtual void ResetDrawing() = 0;
	virtual void DrawPrimitives() = 0;
};

// ////////////////////////////////////////////
// interface declaration end
// ////////////////////////////////////////////









#include "SDL/SDL.h"
#include "GL/glew.h"

// this object can be generated at each thread
// (needs to be generated at each thread if we want to load texture)
class DisplaySDLGlew : public IDisplay {
protected:
	SDL_Window *m_SDL;
	SDL_GLContext m_glCtx;

	// under these two are necessary?
	int m_width;
	int m_height;

	// current rendering status
	Display::Texture m_Texture;
	Display::Color m_ColorMod;
	Display::Rotate m_Rotation;
	Display::Point m_Center;
	Display::Rect m_SRC, m_DST;
	Display::PointF m_Tilt;
	Display::PointF m_Zoom;
	float m_ZPos;

	// COMMENT: should it better to make these static?
	float v_Color[4 * 3];		// colormod
	float v_Texture[4 * 2];		// texture
	float v_Vertex[4 * 4];		// vertex
	void SetupVertices();
public:
	DisplaySDLGlew(SDL_Window *win) { m_SDL = win; ResetDrawing(); };
	~DisplaySDLGlew() { Release(); }

	virtual void BeginRender();
	virtual void EndRender();
	virtual bool Initialize(int width, int height);
	virtual void Release();
	virtual void SetResolution(int width, int height);

	virtual const char* GetInfo();
	virtual int GetErrorCode();
	virtual const char* GetErrorStr();
	virtual Display::Texture* CreateTexture(const Surface* surf);
	virtual Display::Texture* CreateEmptyTexture(int width, int height);
	virtual void UpdateTexture(Display::Texture* tex, const Surface* surf, int x = 0, int y = 0);
	virtual void DeleteTexture(Display::Texture* tex);

	virtual void PushState();
	virtual void PopState();

	virtual void SetBlendMode(int blend);
	virtual void SetTexture(const Display::Texture* tex);
	virtual void SetColorMod(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
	virtual void SetSRC(const Display::Rect* r);
	virtual void SetDST(const Display::Rect* r);
	virtual void SetZPos(float z);
	virtual void SetCenter(int x, int y);
	virtual void SetRotateX(float angle);
	virtual void SetRotateY(float angle);
	virtual void SetRotateZ(float angle);
	virtual void SetTilt(float sx, float sy);
	virtual void SetOffset(int x, int y);
	virtual void SetZoom(float zx, float zy);
	virtual void SetClip(const Display::Rect* r);
	virtual void ResetDrawing();
	virtual void DrawPrimitives();
};






namespace RenderHelper {
	void Render(IDisplay *d, Display::Texture* tex, const Display::Rect* src, const Display::Rect* dst);
}

