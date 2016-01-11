#include "skintexturefont.h"
#include <sstream>

char _buffer[10240];

SkinTextureFont::SkinTextureFont() {
	Clear();
}
void SkinTextureFont::Clear() {
	glyphs.clear();
	imgcnt = 0;
}
void SkinTextureFont::AddImageSrc(const std::string& imagepath) {
	AddImageSrc(imagepath, 0, 0, 0, 0);
}
void SkinTextureFont::AddImageSrc(const std::string& imagepath, uint16_t srcx, uint16_t srcy, uint16_t srcw, uint16_t srch) {
	this->imagepath[imgcnt] = imagepath;
	imagesrc[imgcnt++] = Glyph{ 0, srcx, srcy, srcw, srch };
}
void SkinTextureFont::AddGlyph(uint32_t unicode, uint8_t image, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	glyphs.insert(std::pair<uint32_t, Glyph>(unicode, { image, x, y, w, h }));
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
		ss << "image" << i << "=" << imagepath;
		if (imagesrc[i].w > 0)
			ss << "," << imagesrc[i].x << "," << imagesrc[i].y << "," 
			   << imagesrc[i].w << "," << imagesrc[i].h;
		ss << "\n";
	}
	ss << "imagecnt=" << imgcnt << "\n";

	ss << "[glyphs]\n";
	for (auto it = glyphs.begin(); it != glyphs.end(); ++it) {
		sprintf_s(_buffer, "%d=%d,%d,%d,%d,%d\n", it->first, it->second.image, 
			it->second.x, it->second.y, it->second.w, it->second.h);
		ss << _buffer;
	}

	out = ss.str();
}
SkinTextureFont::Glyph* SkinTextureFont::GetGlyph(uint32_t unicode) {
	if (glyphs.find(unicode) == glyphs.end())
		return 0;
	else
		return &glyphs[unicode];
}