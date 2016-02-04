#include "game.h"
#include "font.h"
#include "util.h"
#include "file.h"
#include "globalresources.h"

#define FALLBACK_FONT			"../system/resource/NanumGothicExtraBold.ttf"
#define FALLBACK_FONT_TEXTURE_L	"../system/resource/fontbackground.png"
#define FALLBACK_FONT_TEXTURE_M	"../system/resource/fontbackground_medium.png"
#define FALLBACK_FONT_TEXTURE_S	"../system/resource/fontbackground_small.png"

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
	if (ttffont) FC_ClearFont(ttffont), ttffont=0;
}

/*
 * Font loading should be always successful. (incorrect font may be placed)
 * So, use fallback font/texture when cannot found...
 */
bool Font::LoadTTFFont(const RString& path, int size, SDL_Color color, int thickness, 
	SDL_Color bordercolor, int border, int fontstyle, const char* texturepath) {
	ttffont = FC_CreateFont();
	if (!ttffont)
		return false;

	RString path_ = path;
	if (!FileHelper::IsFile(path_)) {
		path_ = FALLBACK_FONT;
		FileHelper::ConvertPathToSystem(path_);
	}

	FC_Style style;
	memset(&style, 0, sizeof(FC_Style));

	Uint32 *texture = 0;
	if (texturepath) {
		RString texturepath_ = texturepath;
		if (!FileHelper::IsFile(texturepath_)) {
			if (size < 70) texturepath_ = FALLBACK_FONT_TEXTURE_S;
			else if (size < 120) texturepath_ = FALLBACK_FONT_TEXTURE_M;
			else texturepath_ = FALLBACK_FONT_TEXTURE_L;
			FileHelper::ConvertPathToSystem(texturepath_);
		}
		// load texture
		SDL_Surface *surf = IMG_Load(texturepath_);
		SDL_LockSurface(surf);
		style.textureWidth = surf->w;
		style.textureHeight = surf->h;
		texture = new Uint32[surf->w * surf->h];
		memcpy(texture, surf->pixels, sizeof(Uint32) * surf->w * surf->h);
		SDL_UnlockSurface(surf);
		SDL_FreeSurface(surf);
	}
	style.texture = texture;

	style.fontSize = size;		style.color = color;				style.style = fontstyle;
	style.outline = border;		style.outline_color = bordercolor;	style.thickness = thickness;
	FC_LoadFontFromStyle(ttffont, Game::RENDERER, path_, &style);
	FC_SetSpacing(ttffont, -border);
	SAFE_DELETE(texture);
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
		return FC_GetWidth(ttffont, text);
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

void Font::SetAlphaMod(uint8_t a) {
	if (ttffont) {
		FC_SetAlphaMod(ttffont, a);
	}
	else if (texturefont) {
		texturefont->SetAlphaMod(a);
	}
}

void Font::SetColorMod(uint8_t r, uint8_t g, uint8_t b) {
	if (ttffont) {
		FC_SetColorMod(ttffont, r, g, b);
	}
	else if (texturefont) {
		texturefont->SetColorMod(r, g, b);
	}
}

// -------------------------------------------------

TextureFont::TextureFont()
	: imgs_cnt(0), sx(1), sy(1), t(0), a(255), r(255), g(255), b(255) {}

TextureFont::~TextureFont() { Release(); }

void TextureFont::Release() {
	// in fact, it doesn't release anything
	// but reduce image texture count reference
	for (int i = 0; i < imgs_cnt; i++) {
		IMAGEPOOL->Release(imgs[i]);
	}
	imgs_cnt = 0;
	t = 0;
}

void TextureFont::SetFont(const RString& textdata) {
	ASSERT(imgs_cnt == 0);
	stf.LoadFromText(textdata);
	// loads image
	imgs_cnt = stf.GetImageCount();
	if (stf.GetTimer())
		t = TIMERPOOL->Get(stf.GetTimer());
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
	SkinTextureFont::Glyph *g = stf.GetGlyph(code, t?t->GetTick():0);
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
		SkinTextureFont::Glyph* gl = GetGlyph(glyphcode);
		if (gl) {
			SDL_Rect src = { gl->x, gl->y, gl->w, gl->h };
			SDL_Rect dst = { leftpos, y, gl->w, gl->h };
			SDL_Texture *t = imgs[gl->image] ? imgs[gl->image]->GetPtr() : 0;
			if (t) {
				SDL_SetTextureAlphaMod(t, a);
				SDL_SetTextureColorMod(t, r, g, b);
				SDL_RenderCopy(Game::RENDERER, t, &src, &dst);
			}
			leftpos += gl->w;
		} else leftpos += stf.GetFallbackWidth();
		p++;
	}
}

void TextureFont::SetAlphaMod(uint8_t a) { this->a = a; }

void TextureFont::SetColorMod(uint8_t r, uint8_t g, uint8_t b) {
	this->r = r;
	this->g = g;
	this->b = b;
}