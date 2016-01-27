
#include "game.h"
#include "skinrendertree.h"
#include "globalresources.h"	// for evaluation condition
#include "luamanager.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "tinyxml2.h"
using namespace tinyxml2;

// redefinition;
void ConstructSRCFromElement(ImageSRC &, XMLElement *);
void ConstructDSTFromElement(ImageDST &, XMLElement *);
namespace SkinRenderHelper {
	struct _Offset {
		int x, y;
	};
	std::vector<_Offset> _offset_stack;
	_Offset _offset_calculated = { 0, 0 };
	void RecalculateOffset() {
		_offset_calculated = { 0, 0 };
		for (auto it = _offset_stack.begin(); it != _offset_stack.end(); ++it) {
			_offset_calculated.x += it->x;
			_offset_calculated.y += it->y;
		}
	}
}

// ----- SkinRenderTree -------------------------------------

#pragma region SKINRENDERTREE
SkinRenderTree::SkinRenderTree(int w, int h) : SkinGroupObject(this) { 
	// set skin size
	SetSkinSize(w, h); 
}

void SkinRenderTree::SetSkinSize(int w, int h) { _scr_w = w, _scr_h = h; }

SkinRenderTree::~SkinRenderTree() {
	ReleaseAll();
	ReleaseAllResources();
}

void SkinRenderTree::ReleaseAll() {
	for (auto it = _objpool.begin(); it != _objpool.end(); ++it) {
		delete *it;
	}
	_objpool.clear();
	_idpool.clear();
}

void SkinRenderTree::ReleaseAllResources() {
	for (auto it = _imagekey.begin(); it != _imagekey.end(); ++it)
		IMAGEPOOL->Release(it->second);
	for (auto it = _fontkey.begin(); it != _fontkey.end(); ++it)
		FONTPOOL->Release(it->second);
	_imagekey.clear();
	_fontkey.clear();
}

void SkinRenderTree::RegisterImage(RString &id, RString &path) {
	Image *img = IMAGEPOOL->Load(path);
	if (img) {
		_imagekey.insert(std::pair<RString, Image*>(id, img));
	}
}

void SkinRenderTree::RegisterTTFFont(RString &id, RString &path, int size, int border, const char *texturepath) {
	Font *f = FONTPOOL->LoadTTFFont(id, path, size, FC_MakeColor(255, 255, 255, 255), 0, FC_MakeColor(0, 0, 0, 255), border, 0, texturepath);
	if (f) {
		_fontkey.insert(std::pair<RString, Font*>(id, f));
	}
}

void SkinRenderTree::RegisterTextureFont(RString &id, RString &path) {
	Font *f = FONTPOOL->LoadTextureFont(id, path);
	if (f) {
		_fontkey.insert(std::pair<RString, Font*>(id, f));
	}
}

void SkinRenderTree::RegisterTextureFontByData(RString &id, RString &textdata) {
	// TODO maybe I need to manipulate ID ...
	Font *f = FONTPOOL->LoadTextureFontFromTextData(id, textdata);
	if (f) {
		_fontkey.insert(std::pair<RString, Font*>(id, f));
	}
}

SDL_Texture* SkinRenderTree::GenerateTexture() {
	return SDL_CreateTexture(Game::RENDERER,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET, _scr_w, _scr_h);
}

int SkinRenderTree::GetWidth() { return _scr_w; }

int SkinRenderTree::GetHeight() { return _scr_h; }

SkinUnknownObject* SkinRenderTree::NewUnknownObject() {
	SkinUnknownObject* obj = new SkinUnknownObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinGroupObject* SkinRenderTree::NewGroupObject(bool clipping) {
	SkinGroupObject* obj = new SkinGroupObject(this, clipping);
	_objpool.push_back(obj);
	return obj;
}

SkinImageObject* SkinRenderTree::NewImageObject() {
	SkinImageObject* obj = new SkinImageObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinSliderObject* SkinRenderTree::NewSliderObject() {
	SkinSliderObject* obj = new SkinSliderObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinGrooveGaugeObject* SkinRenderTree::NewGrooveGaugeObject() {
	SkinGrooveGaugeObject* obj = new SkinGrooveGaugeObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinGraphObject* SkinRenderTree::NewGraphObject() {
	SkinGraphObject* obj = new SkinGraphObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinTextObject* SkinRenderTree::NewTextObject() {
	SkinTextObject* obj = new SkinTextObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinNumberObject* SkinRenderTree::NewNumberObject() {
	SkinNumberObject* obj = new SkinNumberObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinPlayObject* SkinRenderTree::NewPlayObject() {
	SkinPlayObject* obj = new SkinPlayObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinComboObject* SkinRenderTree::NewComboObject() {
	SkinComboObject* obj = new SkinComboObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinBgaObject* SkinRenderTree::NewBgaObject() {
	SkinBgaObject* obj = new SkinBgaObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinScriptObject* SkinRenderTree::NewScriptObject() {
	SkinScriptObject* obj = new SkinScriptObject(this);
	_objpool.push_back(obj);
	return obj;
}
#pragma endregion SKINRENDERTREE

// ------ SkinRenderHelper -----------------------------

#pragma region SKINRENDERHELPER

#define ISNAME(e, s) (strcmp(e->Name(), s) == 0)

/* private */
void ConstructSRCFromElement(ImageSRC &src, XMLElement *e) {
	src.x = e->IntAttribute("x");
	src.y = e->IntAttribute("y");
	src.w = e->IntAttribute("w");
	src.h = e->IntAttribute("h");
	src.timer = TIMERPOOL->Get("OnScene");
	const char* timer_name = e->Attribute("timer");
	if (timer_name)
		src.timer = TIMERPOOL->Get(timer_name);
	src.cycle = e->IntAttribute("cycle");
	src.divx = e->IntAttribute("divx");
	if (src.divx < 1) src.divx = 1;
	src.divy = e->IntAttribute("divy");
	if (src.divy < 1) src.divy = 1;
	src.loop = 1;
	if (e->Attribute("loop"))
		src.loop = e->IntAttribute("loop");
}

/* private */
void ConstructDSTFromElement(ImageDST &dst, XMLElement *e) {
	const char* timer = e->Attribute("timer");
	if (timer) {
		dst.timer = TIMERPOOL->Get(timer);
	}
	else {
		dst.timer = TIMERPOOL->Get("OnScene");		// very basic timer, if none setted.
	}
	dst.blend = e->IntAttribute("blend");
	dst.rotatecenter = e->IntAttribute("rotatecenter");
	dst.acctype = e->IntAttribute("acc");
	dst.loopstart = -1; e->QueryIntAttribute("loop", &dst.loopstart);
	XMLElement *ef = e->FirstChildElement("Frame");
	while (ef) {
		ImageDSTFrame f;
		f.time = ef->IntAttribute("time");
		f.x = ef->IntAttribute("x");
		f.y = ef->IntAttribute("y");
		f.w = ef->IntAttribute("w");
		f.h = ef->IntAttribute("h");
		f.a = 255;	ef->QueryIntAttribute("a", &f.a);
		f.r = 255;	ef->QueryIntAttribute("r", &f.r);
		f.g = 255;	ef->QueryIntAttribute("g", &f.g);
		f.b = 255;	ef->QueryIntAttribute("b", &f.b);
		f.angle = ef->IntAttribute("angle");
		dst.loopend = f.time;
		SkinRenderHelper::AddFrame(dst, f);
		ef = ef->NextSiblingElement("Frame");
	}
}

/* private */
/* TODO: change object creating by matching name to `Registered objects` */
void ConstructTreeFromElement(SkinRenderTree &rtree, SkinGroupObject *group, XMLElement *e) {
	while (e) {
		SkinRenderObject *obj = 0;

		if (ISNAME(e, "Init")) {
			/*
			 * Init object is only called once;
			 * it doesn't added to render tree if it's conditions is bad
			 * (TODO)
			 */
		}
		else if (ISNAME(e, "If")) {
			SkinGroupObject *g = rtree.NewGroupObject();
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "Ifnot")) {
			SkinGroupObject *g = rtree.NewGroupObject();
			g->InvertCondition(true);
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "Group")) {
			SkinGroupObject *g = rtree.NewGroupObject(true);
			g->SetSRC(e);
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "Image")) {
			SkinImageObject *img = rtree.NewImageObject();
			img->SetSRC(e);
			obj = img;
		}
		else if (ISNAME(e, "Combo")) {
			SkinComboObject *o = rtree.NewComboObject();
			SkinImageObject *judge = 0;
			SkinNumberObject *combo = 0;
			XMLElement *e_judge = e->FirstChildElement("Image");
			XMLElement *e_combo = e->FirstChildElement("Number");
			if (e_judge) {
				judge = rtree.NewImageObject();
				SkinRenderHelper::ConstructBasicRenderObject(judge, e_judge);
				judge->SetSRC(e_judge);
				o->SetJudgeObject(judge);
			}
			if (e_combo) {
				combo = rtree.NewNumberObject();
				SkinRenderHelper::ConstructBasicRenderObject(combo, e_combo);
				combo->SetValue(INTPOOL->Get(e_combo->Attribute("value")));
				combo->SetAlign(e_combo->IntAttribute("align"));
				combo->SetLength(e_combo->IntAttribute("length"));
				combo->Set24Mode(e_combo->IntAttribute("24mode"));
				combo->SetFont(e_combo->Attribute("resid"));
				o->SetComboObject(combo);
			}
			obj = o;
		}
		else if (ISNAME(e, "Slider")) {
			if (e->Attribute("value")) {
				SkinSliderObject *slider = rtree.NewSliderObject();
				slider->SetSRC(e);
				slider->SetValue(DOUBLEPOOL->Get(e->Attribute("value")));
				slider->SetRange(e->IntAttribute("range"));
				slider->SetDirection(e->IntAttribute("direction"));
				obj = slider;
			}
		}
		else if (ISNAME(e, "Graph")) {
			if (e->Attribute("value")) {
				SkinGraphObject *graph = rtree.NewGraphObject();
				graph->SetSRC(e);
				graph->SetValue(DOUBLEPOOL->Get(e->Attribute("value")));
				obj = graph;
			}
		}
		else if (ISNAME(e, "Text")) {
			if (e->Attribute("value")) {
				SkinTextObject *text = rtree.NewTextObject();
				text->SetValue(STRPOOL->Get(e->Attribute("value")));
				text->SetAlign(e->IntAttribute("align"));
				text->SetFont(e->Attribute("resid"));
				obj = text;
			}
		}
		else if (ISNAME(e, "Number")) {
			if (e->Attribute("value")) {
				SkinNumberObject *num = rtree.NewNumberObject();
				num->SetValue(INTPOOL->Get(e->Attribute("value")));
				num->SetAlign(e->IntAttribute("align"));
				num->SetLength(e->IntAttribute("length"));
				num->Set24Mode(e->IntAttribute("24mode"));
				num->SetFont(e->Attribute("resid"));
				obj = num;
			}
		}
		else if (ISNAME(e, "Lua")) {
			SkinScriptObject *script = rtree.NewScriptObject();
			if (e->Attribute("OnRender"))
				script->SetRunCondition(true);
			if (e->Attribute("src"))
				script->LoadFile(e->Attribute("src"));
			else if (e->GetText())
				script->SetScript(e->GetText());
			obj = script;
		}
		/*
		 * Special object parsing start
		 */
		else if (ISNAME(e, "Play")) {
			SkinPlayObject *p = rtree.NewPlayObject();
			p->x = e->IntAttribute("x");
			p->y = e->IntAttribute("y");
			p->w = e->IntAttribute("w");
			p->h = e->IntAttribute("h");
			XMLElement *lane = e->FirstChildElement("Note");
			while (lane) {
				p->ConstructLane(lane);
				lane = lane->NextSiblingElement("Note");
			}
			p->SetJudgelineObject(e->FirstChildElement("JUDGELINE"));
			p->SetLineObject(e->FirstChildElement("LINE"));
			// TODO: fetch lane effect, or other lane relative images as child
			ConstructTreeFromElement(rtree, p, e->FirstChildElement());

			obj = p;
		}
		else if (ISNAME(e, "Bga")) {
			SkinBgaObject *bga = rtree.NewBgaObject();
			obj = bga;
		}
		else if (ISNAME(e, "GrooveGauge")) {
			SkinGrooveGaugeObject *g = rtree.NewGrooveGaugeObject();
			g->SetSRC(e);
			g->SetObject(e);
			obj = g;
		}
		else {
			// parsed as unknown object
			// only parses SRC/DST condition
			obj = rtree.NewUnknownObject();
		}

		if (obj) {
			group->AddChild(obj);
			// parse common attribute: DST, condition
			SkinRenderHelper::ConstructBasicRenderObject(obj, e);
		}

		// search for next element
		e = e->NextSiblingElement();
	}
}

bool SkinRenderHelper::ConstructTreeFromSkin(SkinRenderTree &rtree, Skin &s) {
	XMLElement *e = s.skinlayout.FirstChildElement("Skin")->FirstChildElement();
	ConstructTreeFromElement(rtree, &rtree, e);

	return true;
}

bool SkinRenderHelper::LoadResourceFromSkin(SkinRenderTree &rtree, Skin &s) {
	XMLElement *res = s.skinlayout.FirstChildElement("Resource");
	if (res) {
		XMLElement *img = res->FirstChildElement("Image");
		while (img) {
			RString path = img->Attribute("path");
			RString id = img->Attribute("name");
			rtree.RegisterImage(id, path);
			img = img->NextSiblingElement("Image");
		}
		XMLElement *font = res->FirstChildElement("Font");
		while (font) {
			RString path = font->Attribute("path");
			RString id = font->Attribute("name");
			rtree.RegisterTTFFont(id, path, font->IntAttribute("size"),
				font->IntAttribute("border"), font->Attribute("texturepath"));
			font = font->NextSiblingElement("Font");
		}
		XMLElement *tfont = res->FirstChildElement("TextureFont");
		while (tfont) {
			if (!tfont->Attribute("path"))
			{
				if (!tfont->GetText()) continue;
				RString data = tfont->GetText();
				RString id = tfont->Attribute("name");
				rtree.RegisterTextureFontByData(id, data);
			}
			else {
				RString path = tfont->Attribute("path");
				RString id = tfont->Attribute("name");
				rtree.RegisterTextureFont(id, path);
			}
			tfont = tfont->NextSiblingElement("TextureFont");
		}
	}
	else {
		return false;
	}

	return true;
}

void SkinRenderHelper::ConstructBasicRenderObject(SkinRenderObject *obj, XMLElement *e) {
	const char* condition = e->Attribute("condition");
	if (condition) obj->SetCondition(condition);
	ImageSRC _s;
	ConstructSRCFromElement(_s, e);
	XMLElement *dst = e->FirstChildElement("DST");
	while (dst) {
		ImageDST _d;
		ConstructDSTFromElement(_d, dst);
		const char* _cond = e->Attribute("condition");
		obj->AddDST(_d, _cond ? _cond : "");
		dst = e->NextSiblingElement("DST");
	}
}

void SkinRenderHelper::AddFrame(ImageDST &d, ImageDSTFrame &f) {
	d.frame.push_back(f);
}

SDL_Rect SkinRenderHelper::ToRect(ImageSRC &d) {
	// if SRC x, y < 0 then Set width/height with image's total width/height.
	int dx = d.w / d.divx;
	int dy = d.h / d.divy;
	Uint32 time = 0;
	if (d.timer) time = d.timer->GetTick();
	int frame = d.cycle ? time * d.divx * d.divy / d.cycle : 0;
	if (!d.loop && frame >= d.divx * d.divy)
		frame = d.divx * d.divy - 1;
	int x = frame % d.divx;
	int y = (frame / d.divx) % d.divy;
	return { d.x + x*dx, d.y + y*dy, dx, dy };
}

SDL_Rect SkinRenderHelper::ToRect(ImageDSTFrame &d) {
	return { d.x, d.y, d.w, d.h };
}

bool SkinRenderHelper::CalculateFrame(ImageDST &dst, ImageDSTFrame &frame) {
	if (dst.timer == 0 || dst.frame.size() == 0) {
		// this shouldnt happened
		return false;
	}

	// timer should alive
	if (!dst.timer->IsStarted())
		return false;
	uint32_t time = dst.timer->GetTick();

	// get current frame
	if (dst.loopstart != dst.loopend && dst.loopstart >= 0 && time > dst.loopstart) {
		time = (time - dst.loopstart) % (dst.loopend - dst.loopstart) + dst.loopstart;
	}
	int nframe = 0;
	for (; nframe < dst.frame.size(); nframe++) {
		if (dst.frame[nframe].time > time)
			break;
	}
	nframe--;
	// if smaller then first DST time, then hide
	// (TODO) work with current speed, if time is 0 (acc is NONE)
	if (nframe < 0)
		return false;

	// has next frame?
	// if not, return current frame 
	// if it does, Tween it.
	if (nframe >= dst.frame.size() - 1) {
		frame = dst.frame[nframe];
	}
	else {
		// very-very rare case, but we should prevent `div by 0`
		if (dst.frame[nframe + 1].time == dst.frame[nframe].time) {
			frame = dst.frame[nframe];
		}
		else {
			double t = (double)(time - dst.frame[nframe].time) / (dst.frame[nframe+1].time - dst.frame[nframe].time);
			frame = Tween(dst.frame[nframe], dst.frame[nframe + 1], t, dst.acctype);
		}
	}

	// if alpha == 0, then don't draw.
	if (frame.a == 0) return false;

	return true;
}

ImageDSTFrame SkinRenderHelper::Tween(ImageDSTFrame &a, ImageDSTFrame &b, double t, int acctype) {
	/* 0.5: kinda of lightweight-round-function */
	switch (acctype) {
	case ACCTYPE::LINEAR:
#define TWEEN(a, b) ((a)*(1-t) + (b)*t + 0.5)
		return{
			TWEEN(a.time, b.time),
			TWEEN(a.x, b.x),
			TWEEN(a.y, b.y),
			TWEEN(a.w, b.w),
			TWEEN(a.h, b.h),
			TWEEN(a.a, b.a),
			TWEEN(a.r, b.r),
			TWEEN(a.g, b.g),
			TWEEN(a.b, b.b),
			TWEEN(a.angle, b.angle),
		};
#undef TWEEN
		break;
	case ACCTYPE::DECEL:
#define T (sqrt(1-(t-1)*(t-1)))
#define TWEEN(a, b) ((a)*(1-T) + (b)*T + 0.5)
		return{
			TWEEN(a.time, b.time),
			TWEEN(a.x, b.x),
			TWEEN(a.y, b.y),
			TWEEN(a.w, b.w),
			TWEEN(a.h, b.h),
			TWEEN(a.a, b.a),
			TWEEN(a.r, b.r),
			TWEEN(a.g, b.g),
			TWEEN(a.b, b.b),
			TWEEN(a.angle, b.angle),
		};
#undef TWEEN
#undef T
		break;
	case ACCTYPE::ACCEL:
#define T (-sqrt(1-t*t)+1)
#define TWEEN(a, b) ((a)*(1-T) + (b)*T + 0.5)
		return{
			TWEEN(a.time, b.time),
			TWEEN(a.x, b.x),
			TWEEN(a.y, b.y),
			TWEEN(a.w, b.w),
			TWEEN(a.h, b.h),
			TWEEN(a.a, b.a),
			TWEEN(a.r, b.r),
			TWEEN(a.g, b.g),
			TWEEN(a.b, b.b),
			TWEEN(a.angle, b.angle),
		};
#undef TWEEN
#undef T
		break;
	case ACCTYPE::NONE:
#define TWEEN(a, b) (a)
		return{
			TWEEN(a.time, b.time),
			TWEEN(a.x, b.x),
			TWEEN(a.y, b.y),
			TWEEN(a.w, b.w),
			TWEEN(a.h, b.h),
			TWEEN(a.a, b.a),
			TWEEN(a.r, b.r),
			TWEEN(a.g, b.g),
			TWEEN(a.b, b.b),
			TWEEN(a.angle, b.angle),
		};
#undef TWEEN
		break;
	}
}

void SkinRenderHelper::PushRenderOffset(int x, int y) {
	_offset_stack.push_back({ x, y });
	RecalculateOffset();
}

void SkinRenderHelper::PopRenderOffset() {
	_offset_stack.pop_back();
	RecalculateOffset();
}

void SkinRenderHelper::Render(Image *img, ImageSRC *src, ImageDSTFrame *frame, int blend, int rotationcenter) {
	// if alpha == 0 then don't draw
	if (frame->a == 0) return;

	SDL_SetTextureAlphaMod(img->GetPtr(), frame->a);
	SDL_SetTextureColorMod(img->GetPtr(), frame->r, frame->g, frame->b);

	switch (blend) {
	case 0:
		/*
		 * LR2 is strange; NO blending does BLENDING, actually.
		 *
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_NONE);
		break;
		*/
	case 1:
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_BLEND);
		break;
	case 2:
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_ADD);
		break;
	case 4:
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_MOD);
		break;
	default:
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_BLEND);
	}

	if (src->w <= 0) src->w = img->GetWidth();
	if (src->h <= 0) src->h = img->GetHeight();
	SDL_Rect src_rect = ToRect(*src);
	SDL_Rect dst_rect = ToRect(*frame);

	SDL_Point rot_center = { 0, 0 };
	switch (rotationcenter) {
	case ROTATIONCENTER::TOPLEFT:
		rot_center = { 0, 0 };
		break;
	case ROTATIONCENTER::TOPCENTER:
		rot_center = { dst_rect.w / 2, 0 };
		break;
	case ROTATIONCENTER::TOPRIGHT:
		rot_center = { dst_rect.w, 0 };
		break;
	case ROTATIONCENTER::CENTERLEFT:
		rot_center = { 0, dst_rect.h / 2 };
		break;
	case 0:
	case ROTATIONCENTER::CENTER:
		rot_center = { dst_rect.w / 2, dst_rect.h / 2 };
		break;
	case ROTATIONCENTER::CENTERRIGHT:
		rot_center = { dst_rect.w, dst_rect.h / 2 };
		break;
	case ROTATIONCENTER::BOTTOMLEFT:
		rot_center = { 0, dst_rect.h };
		break;
	case ROTATIONCENTER::BOTTOMCENTER:
		rot_center = { dst_rect.w / 2, dst_rect.h };
		break;
	case ROTATIONCENTER::BOTTOMRIGHT:
		rot_center = { dst_rect.w, dst_rect.h };
		break;
	}

	// in LR2, negative size doesn't mean flipping.
	if (dst_rect.h < 0) {
		dst_rect.y += dst_rect.h;
		dst_rect.h *= -1;
	}
	if (dst_rect.w < 0) {
		dst_rect.x += dst_rect.h;
		dst_rect.w *= -1;
	}

	// Offset
	dst_rect.x += _offset_calculated.x;
	dst_rect.y += _offset_calculated.y;

	SDL_RendererFlip flip = SDL_RendererFlip::SDL_FLIP_NONE;
	SDL_RenderCopyEx(Game::RENDERER, img->GetPtr(), &src_rect, &dst_rect,
		frame->angle, &rot_center, flip);
}

#pragma endregion SKINRENDERHELPER