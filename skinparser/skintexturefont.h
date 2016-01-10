/*
 * Loads/Saves Unpacked Texturefont for skin.
 * Renderer-independent module.
 * Author: @lazykuna
 */

#pragma once

#include <string>
#include <map>
#include <stdint.h>

class SkinTextureFont {
private:
	typedef struct {
		uint16_t x, y, w, h;
	} Glyph;

	std::string imagepath[100];			// up to 100 images for glyph
	Glyph imagesrc;						// src for text glyph
	std::map<uint32_t, Glyph> glyphs;	// stores glyph (up to 4 bytes; UTF32)
public:
	void Clear();
	void SetTextSrc(const std::string& imagepath);
	void SetTextSrc(const std::string& imagepath, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
	void AddGlyph(uint32_t unicode, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
	bool LoadFromFile(const char *filepath);
	bool LoadFromLR2File(const char *filepath);		// for compatibility
	void LoadFromText(const char *text);
	bool SaveToFile(const char* filepath);
	void SaveToText(const char* text);
};