/*
 * Loads/Saves Unpacked Texturefont for skin.
 * Renderer-independent module.
 * Author: @lazykuna
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <stdint.h>

#define _MAX_TEXTUREFONT_CYCLE 10
#define _MAX_TEXTUREFONT_IMAGES 20

class SkinTextureFont {
private:
	typedef struct {
		uint8_t image;
		uint16_t x, y, w, h;
	} Glyph;
	typedef struct {
		Glyph glyphs[_MAX_TEXTUREFONT_CYCLE];
		int glyphcnt;
	} Glyphs;

	std::string imagepath[_MAX_TEXTUREFONT_IMAGES];			// up to 100 images for glyph
	int imgcnt;
	int cycle;
	std::map<uint32_t, Glyphs> glyphs;	// stores glyph (up to 4 bytes; UTF32)

	void AddGlyph(Glyphs& glyphs, uint8_t imageidx, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
public:
	SkinTextureFont();
	void Clear();
	void AddImageSrc(const std::string& imagepath);
	void AddGlyph(uint32_t unicode, uint8_t imageidx, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
	void SetCycle(int cycle);

	bool LoadFromFile(const char *filepath);
	bool LoadFromLR2File(const char *filepath);		// for compatibility
	void LoadFromText(const char *text);
	bool SaveToFile(const char* filepath);
	void SaveToText(std::string& out);

	/** @brief use this method to get Texture SRC */
	Glyph* GetGlyph(uint32_t unicode, uint32_t time = 0);
};