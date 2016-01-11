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
		uint8_t image;
		uint16_t x, y, w, h;
	} Glyph;

	std::string imagepath[100];			// up to 100 images for glyph
	Glyph imagesrc[100];				// src for text glyph
	int imgcnt;
	std::map<uint32_t, Glyph> glyphs;	// stores glyph (up to 4 bytes; UTF32)
public:
	SkinTextureFont();
	void Clear();
	void AddImageSrc(const std::string& imagepath);
	void AddImageSrc(const std::string& imagepath, uint16_t srcx, uint16_t srcy, uint16_t srcw, uint16_t srch);
	void AddGlyph(uint32_t unicode, uint8_t image, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
	bool LoadFromFile(const char *filepath);
	bool LoadFromLR2File(const char *filepath);		// for compatibility
	void LoadFromText(const char *text);
	bool SaveToFile(const char* filepath);
	void SaveToText(std::string& out);
	Glyph* GetGlyph(uint32_t unicode);
};