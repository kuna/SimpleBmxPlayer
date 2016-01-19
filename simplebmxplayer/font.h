#pragma once

#include "SDL/SDL_ttf.h"
#include "SDL/SDL_FontCache.h"
#include "SDL/SDL_image.h"
#include "skintexturefont.h"
#include "image.h"
#include "global.h"

class TextureFont {
private:
	SkinTextureFont stf;
	Image* imgs[_MAX_TEXTUREFONT_IMAGES];
	// scale
	double sx, sy;
public:
	~TextureFont();
	void Release();

	void SetFont(const RString& textdata);
	void SetScale(double sx, double sy);
	int GetSingleWidth(uint32_t code);
	int GetWidth(const RString& text);
	void Render(const RString& text, int x, int y);
};

/*
* Nah, it time for a font. quite tedious...
* @description
* stores font and it's texture related information
* Two types of font are supported:
* - TTF font uses SDL_TTF / SDL_Fontcache library.
*/
class Font {
private:
	TextureFont* texturefont;
	FC_Font* ttffont;
public:
	Font();
	~Font();
	bool LoadTTFFont(const RString& path, int size, SDL_Color color,
		int border, SDL_Color bordercolor = FC_MakeColor(0, 0, 0, 255),
		int style = TTF_STYLE_NORMAL, int thickness = 0, const char* texturepath = 0);
	bool LoadTextureFont(const RString& path);
	void LoadTextureFontByText(const RString& textdata);

	bool IsLoaded();
	int GetWidth(const char* text);
	/** @brief just renders text at x, y pos */
	void Render(const char* text, int x, int y);
	/** @brief render text, but stretch if it's longer then width size. */
	void Render(const char* text, int x, int y, int width);
	void Release();
};