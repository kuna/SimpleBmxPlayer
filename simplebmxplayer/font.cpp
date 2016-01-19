#include "game.h"
#include "font.h"
#include "util.h"
#include "file.h"

/*
 * TEMP: let's use FC_Font now...
 * implementing all those myself it REALLY hard T_T
 */

namespace {
	char _buffer[10240];
}

Font::Font() 
: texturefont(0), ttffont(0) {}

Font::~Font() { Release(); }

void Font::Release() {
	SAFE_DELETE(texturefont);
	SAFE_DELETE(ttffont);
}

bool Font::LoadTTFFont(const RString& path, int size, SDL_Color color, int thickness, 
	SDL_Color bordercolor, int border, int style, const char* texturepath) {
	ttffont = FC_CreateFont();
	if (!ttffont)
		return false;
	// TODO: we're going to effect these font styles 
	FC_LoadFont(ttffont, Game::RENDERER, "../skin/lazy.ttf", 28, FC_MakeColor(120, 120, 120, 255), style);
	return true;
}

bool Font::LoadTextureFont(const RString& path) {
	RString t;
	if (GetFileContents(path, t))
	{
		LoadTextureFontByText(t);
	} else return false;
}

void Font::LoadTextureFontByText(const RString& textdata) {
	ASSERT(texturefont == 0);
	texturefont = new TextureFont();
	texturefont->SetFont(textdata);
}

bool Font::IsLoaded() {
	return (ttffont || texturefont);
}

int Font::GetWidth(const char* text) {
	if (ttffont) {
		FC_GetWidth(ttffont, text);
	}
	else if (texturefont) {
		return texturefont->GetWidth(text);
	}
	return 0;
}

void Font::Render(const char* text, int x, int y) {
	if (ttffont) {
		FC_Draw(ttffont, Game::RENDERER, x, y, text);
	}
	else if (texturefont) {
		texturefont->Render(text, x, y);
	}
}

void Font::Render(const char* text, int x, int y, int width) {
	// TODO
}

// -------------------------------------------------

TextureFont::TextureFont()
	: imgs_cnt(0), sx(1), sy(1) {}

TextureFont::~TextureFont() { Release(); }

void TextureFont::Release() {
	// in fact, it doesn't release anything
	// but reduce image texture count reference
	for (int i = 0; i < imgs_cnt; i++) {
		IMAGEPOOL->Release(imgs[i]);
	}
	imgs_cnt = 0;
}

void TextureFont::SetFont(const RString& textdata) {
	ASSERT(imgs_cnt == 0);
	stf.LoadFromText(textdata);
	// loads image
	imgs_cnt = stf.GetImageCount();
	for (int i = 0; i < imgs_cnt; i++) {
		imgs[i] = IMAGEPOOL->Load(stf.GetImagePath(i));
	}
}

void TextureFont::SetScale(double sx, double sy) {
	this->sx = sx;
	this->sy = sy;
}

/*
 * copied from SDL_Fontcache project, all rights reserved
 * private func
 * @brief returns next single utf8 pointer
 */
namespace {
	Uint32 GetCodepointFromUTF8String(const char** c, Uint8 advance_pointer)
	{
		Uint32 result = 0;
		const char* str;
		if (c == NULL || *c == NULL)
			return 0;

		str = *c;
		if ((unsigned char)*str <= 0x7F)
			result = *str;
		else if ((unsigned char)*str < 0xE0)
		{
			result |= (unsigned char)(*str) << 8;
			result |= (unsigned char)(*(str + 1));
			if (advance_pointer)
				*c += 1;
		}
		else if ((unsigned char)*str < 0xF0)
		{
			result |= (unsigned char)(*str) << 16;
			result |= (unsigned char)(*(str + 1)) << 8;
			result |= (unsigned char)(*(str + 2));
			if (advance_pointer)
				*c += 2;
		}
		else
		{
			result |= (unsigned char)(*str) << 24;
			result |= (unsigned char)(*(str + 1)) << 16;
			result |= (unsigned char)(*(str + 2)) << 8;
			result |= (unsigned char)(*(str + 3));
			if (advance_pointer)
				*c += 3;
		}
		return result;
	}
}

SkinTextureFont::Glyph* TextureFont::GetGlyph(uint32_t code) {
	SkinTextureFont::Glyph *g = stf.GetGlyph(code);
	if (!g) {
		if (code == '?')
			return 0;
		else
			return GetGlyph('?');
	}
	else return g;
}

int TextureFont::GetWidth(const RString& text) {
	int r = 0;
	const char *p = text.c_str();
	uint32_t glyphcode;
	while ((glyphcode = GetCodepointFromUTF8String(&p, 1)) > 0) {
		SkinTextureFont::Glyph* g = GetGlyph(glyphcode);
		if (g) r += g->w;
		else r += stf.GetFallbackWidth();
		p++;
	}
	return r;
}

void TextureFont::Render(const RString& text, int x, int y) {
	uint32_t leftpos = x;
	const char *p = text.c_str();
	uint32_t glyphcode;
	while ((glyphcode = GetCodepointFromUTF8String(&p, 1)) > 0) {
		SkinTextureFont::Glyph* g = GetGlyph(glyphcode);
		if (g) {
			SDL_Rect src = { g->x, g->y, g->w, g->h };
			SDL_Rect dst = { leftpos, y, g->w, g->h };
			SDL_Texture *t = imgs[g->image]->GetPtr();
			SDL_RenderCopy(Game::RENDERER, t, &src, &dst);
			leftpos += g->w;
		} else leftpos += stf.GetFallbackWidth();
		p++;
	}
}