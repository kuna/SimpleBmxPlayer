#include "game.h"
#include "font.h"
#include "util.h"
#include "file.h"
#include "Pool.h"
#include "skintexturefont.h"
#include "logger.h"
#include <assert.h>
using namespace Display;

// refer freetype
// http://www.freetype.org/freetype2/docs/tutorial/step2.html

/*
 * it's system dependent,
 * so remove this from here.
 *
#define FALLBACK_FONT			"../system/resource/NanumGothicExtraBold.ttf"
#define FALLBACK_FONT_TEXTURE_L	"../system/resource/fontbackground.png"
#define FALLBACK_FONT_TEXTURE_M	"../system/resource/fontbackground_medium.png"
#define FALLBACK_FONT_TEXTURE_S	"../system/resource/fontbackground_small.png"
 */

namespace {
	char _buffer[10240];
	int ftLibRefCnt = 0;
	FT_Library ftLib;
	FT_Stroker ftStroker;

	void InitalizeFT() {
		if (ftLibRefCnt == 0) {
			if (FT_Init_FreeType(&ftLib) != 0) {
				LOG->Critical("freetype initalization failed\n");
				return;
			}
			FT_Stroker_New(ftLib, &ftStroker);
		}
		ftLibRefCnt++;
	}

	void ReleaseFT() {
		assert(ftLibRefCnt > 0);
		ftLibRefCnt--;
		if (ftLibRefCnt == 0) {
			FT_Stroker_Done(ftStroker);
			FT_Done_FreeType(ftLib);
		}
	}

	/* from SDL_FontCache library, MIT License */
	Uint32 FC_GetCodepointFromUTF8(const char** c, Uint8 advance_pointer)
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

Font::Font() {
	// clear basic status
	::InitalizeFT();
	m_TexCnt = 0;
	m_TexX = m_TexY = 0;
	m_TTF = 0;
	m_FntProperty.m_Baseline = 0;
	m_FntProperty.m_DefaultWidth = 1;
	m_FntProperty.m_LetterSpacing = 0;
	m_FntProperty.m_LineSpacing = 0;
	m_FntProperty.m_Height = 0;
	m_FntProperty.m_MaxHeight = 0;
	// clear all status
	m_Status = 0;
	m_ScaleX = m_ScaleY = 1;
}

Font::~Font() {
	Release();
}

void Font::Release() {
	// close TTF
	if (m_TTF) {
		FT_Done_Face(m_TTF);
		m_TTF = 0;
	}
	::ReleaseFT();
	// remove all glyphs / textures
	// TODO
}

/*
 * Font loading should be always successful. (incorrect font may be placed)
 * So, use fallback font/texture when cannot found...
 */
bool Font::LoadTTFFont(const RString& path, const TTFFontArgs& args) {
	Release();
	FT_Error r = FT_New_Face(ftLib, path, 0, &m_TTF);
	if (r != 0)
		return false;
	m_TTFArgs = args;

	// set default properties
	FT_Set_Pixel_Sizes(m_TTF, 0, args.size);
	FT_Select_Charmap(m_TTF, FT_ENCODING_UNICODE);
	// TODO
	//style_outline = (style->style | TTF_STYLE_OUTLINE);
	//TTF_SetFontStyle(ttf, style->style);
	//TTF_SetFontOutline(ttf, style->thickness + style->outline);

	// get default properties
	// with global glyph metrics
	m_FntProperty.m_MaxHeight = m_TTF->max_advance_height;
	m_FntProperty.m_Baseline = m_TTF->ascender;
	m_FntProperty.m_DefaultWidth = m_TTF->max_advance_width;
	m_FntProperty.m_LineSpacing = m_TTF->height;	// not for caching but drawing
	return true;
}

bool Font::LoadBitmapFont(const RString& path) {
	RString t;
	if (GetFileContents(path, t))
	{
		return LoadBitmapFontByText(t);
	} else return false;
}

bool Font::LoadBitmapFontByText(const RString& textdata) {
	Release();
	// parser is provided with SkintextureFont, so use it
	SkinTextureFont *tf = new SkinTextureFont();
	tf->LoadFromText(textdata);
	// load surface and upload it at once
	for (int i = 0; i < tf->GetImageCount(); i++) {
		Surface *surf_original = new Surface();
		if (!surf_original->Load(tf->GetImagePath(i))) {
			LOG->Warn("Font - Cannot load Bitmap resource - %s", tf->GetImagePath(i));
			delete surf_original;
			continue;
		}
		// we won't check failure on creating texture
		m_Tex[i] = DISPLAY->CreateTexture(surf_original);
		delete surf_original;
	}
	// add glyphs
	for (auto gi = tf->GlyphBegin(); gi != tf->GlyphEnd(); ++gi) {
		for (int i = 0; i < gi->second.glyphcnt; i++) {
			FontGlyph fg;
			fg.m_src.x = gi->second.glyphs[i].x;
			fg.m_src.y = gi->second.glyphs[i].y;
			fg.m_src.w = gi->second.glyphs[i].w;
			fg.m_src.h = gi->second.glyphs[i].h;
			fg.m_Texidx = gi->second.glyphs[i].image;
			m_FntGlyphs[gi->first][i] = fg;
		}
	}
	// done-
	delete tf;
	return true;
}

FontGlyph* Font::GetGlyph(uint32_t code) {
	if (m_FntGlyphs[m_Status].find(code) == m_FntGlyphs[m_Status].end())
		return 0;
	else
		return &m_FntGlyphs[m_Status][code];
}

int Font::GetWidth(const char* text) {
	unsigned start = 0;
	const char* p = text;
	uint32_t code;
	int w = 0;
	while (code = FC_GetCodepointFromUTF8(&p, true)) {
		FontGlyph* g = GetGlyph(code);
		if (!g) w += m_FntProperty.m_DefaultWidth;
		else w += g->m_src.w;
	}
	// scale-dependent
	return w * m_ScaleX;
}

//
// COMMENT: this function MUST be done in singleton,
//          as this function uses ftStroker object globally.
//
bool Font::CacheGlyph(uint32_t code) {
	// only available on TTF
	if (!IsTTFFont())
		return false;
	// if glyph provided?
	FT_UInt ftcode = FT_Get_Char_Index(m_TTF, code);
	if (!ftcode)
		return false;
	// load basic glyph first
	FT_Load_Glyph(m_TTF, ftcode, FT_LOAD_DEFAULT);
	// create surface
	Surface *surf = new Surface();
	surf->Create(m_TTF->glyph->metrics.width, m_FntProperty.m_MaxHeight);
	// render border
	if (m_TTFArgs.border) {
		FT_Glyph glyph;
		FT_Get_Glyph(m_TTF->glyph, &glyph);
		FT_Stroker_Set(ftStroker,
			m_TTFArgs.border,
			FT_STROKER_LINECAP_ROUND,
			FT_STROKER_LINEJOIN_ROUND,
			0);
		FT_Glyph_StrokeBorder(&glyph, ftStroker, 0, 1);
		FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, true);
		FT_BitmapGlyph bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
		SetSurfaceFromGlyph(surf, &bitmapGlyph->bitmap,
			0, 0,
			Font::GetFontForegroundColor);
		FT_Done_Glyph(glyph);
	}
	// render thickness: foreground bitmap
	if (m_TTFArgs.thickness) {
		FT_Glyph glyph;
		FT_Get_Glyph(m_TTF->glyph, &glyph);
		FT_Stroker_Set(ftStroker,
			m_TTFArgs.thickness,
			FT_STROKER_LINECAP_ROUND,
			FT_STROKER_LINEJOIN_ROUND,
			0);
		FT_Glyph_StrokeBorder(&glyph, ftStroker, 0, 1);
		FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, true);
		FT_BitmapGlyph bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
		SetSurfaceFromGlyph(surf, &bitmapGlyph->bitmap,
			m_TTFArgs.thickness,
			m_TTFArgs.thickness,
			Font::GetFontForegroundColor);
	}
	// render text: foreground bitmap
	FT_Render_Glyph(m_TTF->glyph, FT_RENDER_MODE_NORMAL);
	SetSurfaceFromGlyph(surf, &m_TTF->glyph->bitmap, 
		m_TTFArgs.border + m_TTFArgs.thickness, 
		m_TTFArgs.border + m_TTFArgs.thickness, 
		Font::GetFontForegroundColor);
	// upload to texture/glyph
	bool r = UploadGlyph(code, surf);
	// cleanup & return
	delete surf;
	return r;
}

void Font::SetSurfaceFromGlyph(Surface *surf, const FT_Bitmap *bitmap, 
	int offsetx, int offsety, uint32_t(Font::*ColorFunc)(int, int))
{
	for (int y = 0; y < bitmap->rows; ++y) {
		int outy = bitmap->rows - m_TTF->glyph->bitmap_top + y + offsety;
		for (int x = 0; x < bitmap->width; ++x) {
			int outx = m_TTF->glyph->bitmap_left + x + offsetx;
			unsigned char alpha = bitmap->buffer[bitmap->width * y + x];
			uint32_t orgclr = GetFontForegroundColor(outx, outy);
			uint32_t color = (alpha * (orgclr & 0x000000FF) / 255) 
				| (orgclr & 0xFFFFFF00);
			surf->SetPixel(outx, outy, color);
		}
	}
}

bool Font::UploadGlyph(uint32_t code, Surface* surf) {
	// find new position for uploading texture, or create new texture
	bool makenewtex = false;
	if (m_TexCnt == 0) {
		makenewtex = true;
	}
	if (m_TexX + surf->GetWidth() > FONT_TEX_SIZE) {
		m_TexY += m_FntProperty.m_MaxHeight;
		m_TexX = 0;
	}
	if (m_TexY + surf->GetHeight() > FONT_TEX_SIZE) {
		makenewtex = true;
	}
	if (makenewtex) {
		if (m_TexCnt > MAX_SURFACE_COUNT) {
			LOG->Warn("Font: too many glyphs, cannot make new one");
			return false;
		}
		CreateNewSurface();
		m_TexX = m_TexY = 0;
	}
	// ok, now upload texture to glyph
	DISPLAY->UpdateTexture(&m_Tex[m_TexCnt - 1], surf, m_TexX, m_TexY);
	// add new glyph
	FontGlyph g;
	g.m_src = { m_TexX, m_TexY, surf->GetWidth(), surf->GetHeight()};
	g.m_Texidx = m_TexCnt - 1;
	m_FntGlyphs[m_Status][code] = g;
	return true;
}

void Font::CreateNewSurface() {
	m_Tex[m_TexCnt] = DISPLAY->CreateEmptyTexture(FONT_TEX_SIZE, FONT_TEX_SIZE);
	m_TexCnt++;
}

uint32_t Font::GetFontForegroundColor(int x, int y) {
	if (m_TTFArgs.foretexture) {
		int w = m_TTFArgs.foretexture->GetWidth();
		int h = m_TTFArgs.foretexture->GetHeight();
		return m_TTFArgs.foretexture->GetPixel(x % w, y % h);
	}
	else {
		return m_TTFArgs.color;
	}
}

uint32_t Font::GetFontBorderColor(int x, int y) {
	// a little dummy function
	return m_TTFArgs.bordercolor;
}

void Font::Render(const char* text, int x, int y) {
	unsigned start = 0;
	int cx = 0;
	const char* p = text;
	uint32_t code;
	while (code = FC_GetCodepointFromUTF8(&p, true)) {
		FontGlyph* g = GetGlyph(code);
		if (!g) {
			// attempt to render glyph
			if (!CacheGlyph(code))
				cx += m_FntProperty.m_DefaultWidth;
			else
				// glyph caching success, maybe
				g = GetGlyph(code);
		}
		if (g) {
			// draws from TexRect
			// scale-dependent
			Rect dst;
			dst.x = x + cx * m_ScaleX;
			dst.y = y * m_ScaleY;
			dst.w = g->m_src.w;
			dst.h = g->m_src.h;
			spr.SetTexture(m_Tex[g->m_Texidx]);
			spr.SetSrc(&g->m_src);
			spr.SetDest(&dst);
			spr.Render();

			cx += g->m_src.w;
		}
	}
}

void Font::Render(const char* text, int x, int y, int width) {
	// resize font if it's bigger then width
	int textwidth = GetWidth(text);
	float backup_sx = m_ScaleX;
	if (textwidth > width)
		SetScale(m_ScaleX * width / textwidth, m_ScaleY);
	Render(text, x, y);
	SetScale(backup_sx, m_ScaleY);
}

void Font::SetScale(float sx, float sy) {
	m_ScaleX = sx;
	m_ScaleY = sy;
}
