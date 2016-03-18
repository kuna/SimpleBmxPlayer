#pragma once

/*
 * @description
 * simple TTF/Bitmap font loader/renderer.
 *
 */

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_STROKER_H
#include "Surface.h"
#include "Display.h"
#include <stdint.h>
#include <map>

struct TTFFontArgs {
	int size;
	uint32_t color;
	int border;
	uint32_t bordercolor;
	int style;
	int thickness;
	Surface* foretexture;
};

struct FontGlyph {
	// index of texture
	int m_Texidx;
	// src rect of texture
	Display::Rect m_src;
};

struct FontProperty {
	int m_Height;			// generally bigger then ascend + descend
	int m_MaxHeight;
	int m_DefaultWidth;		// when glyph isn't found
	int m_LineSpacing;		// plus/minus relative to height
	int m_LetterSpacing;
	int m_Baseline;
};

//
// glyph is stored in real texture
//
#define MAX_SURFACE_COUNT 10
#define MAX_FONT_STATE 5
#define FONT_TEX_SIZE 4096

class Font {
protected:
	// base font properties
	std::map<uint32_t, FontGlyph> m_FntGlyphs[MAX_FONT_STATE];
	FontProperty m_FntProperty;
	TTFFontArgs m_TTFArgs;
	FT_Face m_TTF;
	bool IsTTFLoaded;

	// font object status
	int m_Status;
	float m_ScaleX, m_ScaleY;

	// managing glyphs
	Display::Texture* m_Tex[MAX_SURFACE_COUNT];
	int m_TexCnt;
	int m_TexY;
	int m_TexX;
	FontGlyph* GetGlyph(uint32_t text);
	bool CacheGlyph(uint32_t code);
	bool UploadGlyph(uint32_t code, Surface* surf);
	void CreateNewSurface();
	void SetSurfaceFromGlyph(Surface *surf, const FT_Bitmap *bitmap, 
		int offsetx, int offsety, uint32_t (Font::*ColorFunc)(int, int));
	uint32_t GetFontForegroundColor(int x, int y);
	uint32_t GetFontBorderColor(int x, int y);
public:
	Font();
	~Font();
	void Release();

	bool LoadTTFFont(const RString& path, const TTFFontArgs& args);
	bool LoadBitmapFont(const RString& path);
	bool LoadBitmapFontByText(const RString& textdata);
	bool IsTTFFont() { return m_TTF != 0; }

	int GetWidth(const char* text);
	void SetScale(float sx, float sy);
	/** @brief just renders text at x, y pos */
	void Render(const char* text, int x, int y);
	/** @brief render text, but stretch if it's longer then width size. */
	void Render(const char* text, int x, int y, int width);
};

// --------------- old code, maybe ---------------------

#include <stdint.h>

#define _MAX_TEXTUREFONT_CYCLE 10
#define _MAX_TEXTUREFONT_IMAGES 20

class SkinTextureFont {
public:
	typedef struct {
		uint8_t image;
		uint16_t x, y, w, h;
	} Glyph;
private:
	typedef struct {
		Glyph glyphs[_MAX_TEXTUREFONT_CYCLE];
		int glyphcnt;
	} Glyphs;
	std::string imagepath[_MAX_TEXTUREFONT_IMAGES];		// up to 100 images for glyph
	std::string timer;									// used if cycle exists
	int imgcnt;
	int cycle;
	std::map<uint32_t, Glyphs> glyphs;	// stores glyph (up to 4 bytes; UTF32)
	int fallback_width;					// when no glyph(even '?') found, use this as empty space

	void AddGlyph(Glyphs& glyphs, uint8_t imageidx, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
public:
	SkinTextureFont();
	void Clear();
	void AddImageSrc(const std::string& imagepath);
	void AddGlyph(uint32_t unicode, uint8_t imageidx, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
	void SetFallbackWidth(int w);
	void SetCycle(int cycle);
	int GetFallbackWidth();
	int GetCycle();
	const char* GetImagePath(int imgno);
	int GetImageCount();

	bool LoadFromFile(const char *filepath);
	bool LoadFromLR2File(const char *filepath);		// for compatibility
	void LoadFromText(const char *text);
	bool SaveToFile(const char* filepath);
	void SaveToText(std::string& out);

	/** @brief use this method to get Texture SRC */
	Glyph* GetGlyph(uint32_t unicode, uint32_t time = 0);
	typedef std::map<uint32_t, Glyphs>::const_iterator GIter;
	GIter GlyphBegin() { return glyphs.begin(); }
	GIter GlyphEnd() { return glyphs.end(); }
};