#include "skintexturefont.h"
#include <sstream>

char _buffer[10240];

SkinTextureFont::SkinTextureFont() {
	Clear();
}
void SkinTextureFont::Clear() {
	glyphs.clear();
	imgcnt = 0;
	cycle = 0;
}

void SkinTextureFont::AddImageSrc(const std::string& imagepath) {
	this->imagepath[imgcnt++] = imagepath;
}

void SkinTextureFont::AddGlyph(Glyphs &gs, uint8_t image, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	gs.glyphs[gs.glyphcnt++] = { image, x, y, w, h };
}

void SkinTextureFont::AddGlyph(uint32_t unicode, uint8_t image, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	Glyphs gs;
	gs.glyphcnt = 0;
	AddGlyph(gs, image, x, y, w, h);
	glyphs.insert(std::pair<uint32_t, Glyphs>(unicode, gs));
}
void SkinTextureFont::SetCycle(int cycle) {
	cycle = 0;
}
bool SkinTextureFont::LoadFromFile(const char *filepath) {
	FILE *f = fopen(filepath, "r");
	if (!f)
		return false;
	fgets(_buffer, 10240, f);
	fclose(f);

	LoadFromText(_buffer);
	return true;
}
bool SkinTextureFont::LoadFromLR2File(const char *filepath) {
	// TODO
	return false;
}
void SkinTextureFont::LoadFromText(const char *text) {
	char buf[10240];
	char arg_str[1024];
	strcpy(buf, text);

	Clear();

	/*
	 * mode: means parsing mode
	 * 0: [resource]
	 * 1: [glyph]
	 */
	int mode = 0;	// basically resource
	int args[10];
	char *p = buf;
	char *np;
	while ((np = strchr(p, '\n')) != 0) {
		*np = 0;
		if (strcmp(p, "[resource]") == 0)
			mode = 0;
		else if (strcmp(p, "[glyphs]") == 0)
			mode = 1;

		else if (mode == 0) {
			if (sscanf(p, "imagecnt=%d", &args[0]))
				imgcnt = args[0];
			else if (sscanf(p, "image%d=%s", &args[0], arg_str) == 2) {
				imagepath[args[0]] = arg_str;
			}
		}
		else if (mode == 1) {
			if (sscanf(p, "%d=%s", &args[0], _buffer) == 2) {
				Glyphs gs;
				char *p2 = _buffer;
				do {
					if (sscanf(p2, "%d,%d,%d,%d,%d", &args[1], &args[2], &args[3], &args[4], &args[5]) == 5) {
						AddGlyph(gs, args[1], args[2], args[3], args[4], args[5]);
					}
					p2 = strchr(p2, ';');
					if (p2) p2++;
				} while (p2 > 0);
				glyphs.insert(std::pair<uint32_t, Glyphs>(args[0], gs));
			}
		}
		p = np + 1;
	}
}
bool SkinTextureFont::SaveToFile(const char* filepath) {
	FILE *f = fopen(filepath, "w");
	if (!f)
		return false;

	std::string out;
	SaveToText(out);
	fputs(out.c_str(), f);
	fclose(f);
	return true;
}
void SkinTextureFont::SaveToText(std::string& out) {
	std::ostringstream ss;
	ss << "[resource]\n";
	for (int i = 0; i < imgcnt; i++) {
		ss << "image" << i << "=" << imagepath[i];
		ss << "\n";
	}
	ss << "imagecnt=" << imgcnt << "\n";

	ss << "[glyphs]\n";
	for (auto it = glyphs.begin(); it != glyphs.end(); ++it) {
		ss << it->first << "=";
		Glyphs& gs = it->second;
		for (int i = 0; i < gs.glyphcnt; i++) {
			sprintf_s(_buffer, "%d,%d,%d,%d,%d;", gs.glyphs[i].image,
				gs.glyphs[i].x, gs.glyphs[i].y, gs.glyphs[i].w, gs.glyphs[i].h);
			if (i == gs.glyphcnt - 1)
				_buffer[strlen(_buffer) - 1] = 0;
			ss << _buffer;
		}
		ss << "\n";
	}

	out = ss.str();
}
SkinTextureFont::Glyph* SkinTextureFont::GetGlyph(uint32_t unicode, uint32_t time) {
	if (glyphs.find(unicode) == glyphs.end())
		return 0;
	else {
		int frame_num = 0;
		if (cycle > 0)
			frame_num = (time * glyphs[unicode].glyphcnt / cycle) % glyphs[unicode].glyphcnt;
		return &glyphs[unicode].glyphs[frame_num];
	}
}