
#include "game.h"
#include "ActorRenderer.h"
#include "globalresources.h"	// for evaluation condition
#include "luamanager.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "tinyxml2.h"
using namespace tinyxml2;

// ----- SkinRenderTree -------------------------------------

#pragma region SKINRENDERTREE
SkinRenderTree::SkinRenderTree(int w, int h) : SkinGroupObject(this) { 
	// set skin size
	SetSkinSize(w, h); 

	_offset_x = _offset_y = 0;
}

void SkinRenderTree::SetSkinSize(int w, int h) { 
	_scr_w = w, _scr_h = h; 
}

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
	Font *f = FONTPOOL->LoadTTFFont(id, path, size, 0xFFFFFFFF, 0, 0x000000FF, border, 0, texturepath);
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
	Font *f = FONTPOOL->LoadTextureFontFromTextData(id, textdata);
	if (f) {
		_fontkey.insert(std::pair<RString, Font*>(id, f));
	}
}

int SkinRenderTree::GetWidth() { return _scr_w; }

int SkinRenderTree::GetHeight() { return _scr_h; }

void SkinRenderTree::PushRenderOffset(int x, int y) {
	_offset_stack.push_back({ x, y });
	_offset_x += x;
	_offset_y += y;
}

void SkinRenderTree::PopRenderOffset() {
	_Offset _offset = _offset_stack.back();
	_offset_stack.pop_back();
	_offset_x -= _offset.x;
	_offset_y -= _offset.y;
}

void SkinRenderTree::Render() {
	for (auto it = begin(); it != end(); ++it) {
		(*it)->Render();
	}
}

void SkinRenderTree::Update() {
	drawable = true;
	for (auto it = begin(); it != end(); ++it) {
		(*it)->Update();
	}
}

void SkinRenderTree::SetObject(XMLElement *e) {
	if (!e) return;
	_keycount = atoi(e->FirstChildElement("key")->GetText());
	_scr_w = atoi(e->FirstChildElement("width")->GetText());
	_scr_h = atoi(e->FirstChildElement("height")->GetText());
	// to resize texture
	SetSkinSize(_scr_w, _scr_h);
}

#pragma endregion SKINRENDERTREE







// ------ SkinRenderHelper -----------------------------

#pragma region SKINRENDERHELPER

#define ISNAME(e, s) (strcmp(e->Name(), s) == 0)

namespace SkinRenderHelper {
	//
	// TODO: support extern actor
	//
	struct RegisteredObject {
		std::string name;
		bool(*register_object)();
	};
}

void SkinRenderHelper::ConstructSRCFromElement(ImageSRC &src, XMLElement *e) {
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

void SkinRenderHelper::ConstructDSTFromElement(ImageDST &dst, XMLElement *e) {
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
	XMLElement *ef = e->FirstChildElement("frame");
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
		ef = ef->NextSiblingElement("frame");
	}
}

/* private */
/* TODO: change object creating by matching name to `Registered objects` */
void ConstructTreeFromElement(SkinRenderTree &rtree, SkinGroupObject *group, XMLElement *e) {
	while (e) {
		SkinRenderObject *obj = 0;

		if (ISNAME(e, "init")) {
			/*
			 * Init object is only called once;
			 * it doesn't added to render tree if it's conditions is bad
			 * (TODO)
			 */
		}
		else if (ISNAME(e, "ifnot")) {
			SkinGroupObject *g = rtree.NewSkinGroupObject();
			g->InvertCondition(true);
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "if")) {
			SkinGroupObject *g = rtree.NewSkinGroupObject();
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "canvas")) {
			SkinGroupObject *g = rtree.NewSkinCanvasObject();
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
#define PARSEOBJ(name, o)\
	else if (ISNAME(e, name)) {\
	o* newobj = rtree.New##o();\
	obj = newobj; }
#define IGNOREOBJ(name)\
	else if (ISNAME(e, name)) {}
		PARSEOBJ("sprite", SkinImageObject)
		PARSEOBJ("slider", SkinSliderObject)
		PARSEOBJ("graph", SkinGraphObject)
		PARSEOBJ("text", SkinTextObject)
		PARSEOBJ("number", SkinNumberObject)
		PARSEOBJ("lua", SkinScriptObject)
		/*
		 * play scene object parsing
		 */
		PARSEOBJ("notefield", SkinNoteFieldObject)
		PARSEOBJ("groovegauge", SkinGrooveGaugeObject)
		PARSEOBJ("combo", SkinComboObject)
		PARSEOBJ("bga", SkinBgaObject)
		/*
		 * ignored objects
		 */
		IGNOREOBJ("include")
		IGNOREOBJ("image")
		IGNOREOBJ("font")
		IGNOREOBJ("texturefont")
		else {
			// parsed as unknown object
			// only parses SRC/DST condition
			obj = rtree.NewSkinUnknownObject();
		}

		if (obj) {
			obj->SetObject(e);
			group->AddChild(obj);
		}

		// search for next element
		e = e->NextSiblingElement();
	}
}

bool SkinRenderHelper::ConstructTreeFromSkin(SkinRenderTree &rtree, Skin &s) {
	XMLElement *e = s.skinlayout.FirstChildElement("skin")->FirstChildElement();
	ConstructTreeFromElement(rtree, &rtree, e);

	return true;
}

namespace {
	// private
	// only for LR2 compatibility
	std::map<std::string, std::string> filter_to_option;

	// some utils for recursing skin tree
	void DeepCopy(const XMLNode *from, XMLNode* to) {
		XMLDocument *doc = to->GetDocument();
		for (const XMLNode* node = from->FirstChild(); node; node = node->NextSibling()) {
			XMLNode* copy = node->ShallowClone(doc);
			to->InsertEndChild(copy);
			// check for child
			DeepCopy(node, copy);
		}
	}
#define ISNAME(name) (strcmp(e->Name(), name) == 0)
	void ParseSkinTree(SkinRenderTree &rtree, XMLElement *e) {
		for (e = e->FirstChildElement(); e; e = e->NextSiblingElement()) {
			if (ISNAME("if")) {
				RenderCondition cond;
				cond.Set(e->Attribute("condition"));
				if (cond.Evaluate())
					ParseSkinTree(rtree, e);
			}
			else if (ISNAME("ifnot")) {
				RenderCondition cond;
				cond.Set(e->Attribute("condition"));
				if (!cond.Evaluate())
					ParseSkinTree(rtree, e);
			}
			else if (ISNAME("include")) {
				// first get options from original lr2skin
				// (in case of LR2skin; this code is unnecessary if it's not LR2skin.)
				filter_to_option.clear();
				XMLDocument *doc = e->GetDocument();
				for (XMLElement *option = doc->FirstChildElement("skin")
					->FirstChildElement("option")
					->FirstChildElement("customfile"); option; option = option->NextSiblingElement("customfile")) {
					XMLComment *c = option->NextSibling()->ToComment();
					if (c) {
						filter_to_option.insert(std::pair<std::string, std::string>
							(c->Value(), option->Attribute("name")));
					}
				}
				// loadskin & deepcopy
				Skin *skin_temp_ = new Skin();
				SkinRenderHelper::LoadSkin(e->Attribute("path"), *skin_temp_);
				DeepCopy(skin_temp_->skinlayout.FirstChildElement("skin"), e->Parent());
				delete skin_temp_;
			}
			else if (ISNAME("image")) {
				RString path = e->Attribute("path");
				RString id = e->Attribute("name");
				rtree.RegisterImage(id, path);
			}
			else if (ISNAME("font")) {
				RString path = e->Attribute("path");
				RString id = e->Attribute("name");
				rtree.RegisterTTFFont(id, path, e->IntAttribute("size"),
					e->IntAttribute("border"), e->Attribute("texturepath"));
			}
			else if (ISNAME("texturefont")) {
				if (!e->Attribute("path"))
				{
					if (!e->GetText()) continue;
					RString data = e->GetText();
					RString id = e->Attribute("name");
					rtree.RegisterTextureFontByData(id, data);
				}
				else {
					RString path = e->Attribute("path");
					RString id = e->Attribute("name");
					rtree.RegisterTextureFont(id, path);
				}
			}
		}
	}
}

bool SkinRenderHelper::LoadSkin(const char* path, Skin& skin) {
	// - if skin is lr2skin, find *.main.xml converted skin
	//   otherwise convert & save one
	// - if skin is csv, find *.xml converted skin
	//   otherwise convert & save one
	RString abspath = path;
	FileHelper::ConvertPathToAbsolute(abspath);
	bool r = false;
	if (EndsWith(abspath, ".lr2skin")) {
		RString newpath = abspath;
		newpath = substitute_extension(newpath, ".main.xml");
		if (FileHelper::IsFile(newpath)) {
			abspath = newpath;
		}
		else {
			_LR2SkinParser *lr2skin = new _LR2SkinParser();
			r = lr2skin->ParseLR2Skin(abspath, &skin);
			delete lr2skin;
			skin.Save(newpath);
		}
	}
	else if (EndsWith(abspath, ".csv")) {
		RString newpath = abspath;
		newpath = substitute_extension(newpath, ".xml");
		if (FileHelper::IsFile(newpath)) {
			abspath = newpath;
		}
		else {
			// in case of CSV, there might be a parent lr2skin
			// which has information about translating #IMAGE path
			_LR2SkinParser *lr2skin = new _LR2SkinParser();
			for (auto it = filter_to_option.begin(); it != filter_to_option.end(); ++it)
				lr2skin->AddPathToOption(it->first, it->second);
			r = lr2skin->ParseLR2Skin(abspath, &skin);
			delete lr2skin;
			skin.Save(newpath);
		}
	}

	if (EndsWith(abspath, ".xml")) {
		r = skin.Load(abspath);
	}
	return r;
}

void SkinRenderHelper::LoadResourceFromSkin(SkinRenderTree &rtree, Skin &s) {
	//
	// recursively parses skin tree
	// - if `if`, then check condition & decide to go in.
	// - if `include`, then load into Skin tree.
	// - if `image / font / texturefont`, then load resource.
	//
	ParseSkinTree(rtree, s.skinlayout.FirstChildElement("skin"));
	filter_to_option.clear();
	//s.skinlayout.Print();
}

void SkinRenderHelper::ConstructBasicRenderObject(SkinRenderObject *obj, XMLElement *e) {
	const char* condition = e->Attribute("condition");
	if (condition) obj->SetCondition(condition);
	ImageSRC _s;
	SkinRenderHelper::ConstructSRCFromElement(_s, e);
	XMLElement *dst = e->FirstChildElement("DST");
	while (dst) {
		ImageDST _d;
		SkinRenderHelper::ConstructDSTFromElement(_d, dst);
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
	// this shouldnt happened
	if (dst.frame.size() == 0) {
		return false;
	}
	// if no timer, then use base timer
	if (dst.timer == 0)
		dst.timer = TIMERPOOL->Get("OnScene");

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

namespace {
#define SIZE 4
	float Vertex[SIZE * 3];
	float Texture[SIZE * 2] = {
		0, 0,
		1, 0,
		1, 1,
		0, 1
	};
	GLubyte Color[SIZE * 4];

	void UpdateRenderArgs(ImageDSTFrame *frame, int blend) {
		glEnable(GL_BLEND);
		switch (blend) {
		case 0:
			/*
			* LR2 is strange; NO blending does BLENDING, actually.
			*
			SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_NONE);
			break;
			*/
		case 1:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//SDL_SetTextureBlendMode(t, SDL_BlendMode::SDL_BLENDMODE_BLEND);
			break;
		case 2:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			//SDL_SetTextureBlendMode(t, SDL_BlendMode::SDL_BLENDMODE_ADD);
			break;
		case 3:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case 4:
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			//SDL_SetTextureBlendMode(t, SDL_BlendMode::SDL_BLENDMODE_MOD);
			break;
		default:
			//SDL_SetTextureBlendMode(t, SDL_BlendMode::SDL_BLENDMODE_BLEND);
			break;
		}
	}

	void UpdateTexRotate(SDL_Rect& dst_rect, int rotationcenter, float angle) {
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

		glPushMatrix();
		glTranslatef(-dst_rect.x, -dst_rect.y, 0);
		glRotatef(angle / 360.0f, 0, 0, 1);
		glTranslatef(dst_rect.x, dst_rect.y, 0);
	}

	void UpdateTexState(ImageSRC* src, ImageDSTFrame *frame, float width, float height, int rotationcenter) {
		SDL_Rect src_rect = SkinRenderHelper::ToRect(*src);
		SDL_Rect dst_rect = SkinRenderHelper::ToRect(*frame);
		// in LR2, negative size doesn't mean flipping.
		if (dst_rect.h < 0) {
			dst_rect.y += dst_rect.h;
			dst_rect.h *= -1;
		}
		if (dst_rect.w < 0) {
			dst_rect.x += dst_rect.h;
			dst_rect.w *= -1;
		}

		// rotateion
		UpdateTexRotate(dst_rect, rotationcenter, frame->angle);

		// set vertex/color/texture quad vertices.
		// TODO: move them to Display class
		Vertex[0 * 3 + 0] = 
			Vertex[2 * 3 + 0] =
			dst_rect.x;
		Vertex[1 * 3 + 0] = 
			Vertex[3 * 3 + 0] = 
			dst_rect.x + dst_rect.w;
		Vertex[0 * 3 + 1] = 
			Vertex[1 * 3 + 1] = 
			dst_rect.y;
		Vertex[2 * 3 + 1] = 
			Vertex[3 * 3 + 1] = 
			dst_rect.y + dst_rect.h;
		Vertex[0 * 3 + 2] =
			Vertex[1 * 3 + 2] =
			Vertex[2 * 3 + 2] =
			Vertex[3 * 3 + 2] =
			1.0f;

		Texture[0 * 2 + 0] =
			Texture[2 * 2 + 0] =
			src_rect.x / width;
		Texture[1 * 2 + 0] =
			Texture[3 * 2 + 0] =
			(src_rect.x + src_rect.w) / width;
		Texture[0 * 2 + 1] =
			Texture[1 * 2 + 1] =
			src_rect.y / height;
		Texture[2 * 2 + 1] =
			Texture[3 * 2 + 1] =
			(src_rect.y + src_rect.h) / height;

		for (int i = 0; i < SIZE; i++) {
			Color[i * 4 + 0] = frame->r;
			Color[i * 4 + 1] = frame->g;
			Color[i * 4 + 2] = frame->b;
			Color[i * 4 + 3] = frame->a;
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Vertex);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, Color);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, Texture);

		/*
		glBegin(GL_QUADS);
		glTexCoord2f(0.f, 0.f); glVertex2f(0.f, 0.f);
		glTexCoord2f(1.f, 0.f); glVertex2f(dst_rect.w, 0.f);
		glTexCoord2f(1.f, 1.f); glVertex2f(dst_rect.w, dst_rect.h);
		glTexCoord2f(0.f, 1.f); glVertex2f(0.f, dst_rect.h);
		glEnd();*/
	}
}
void SkinRenderHelper::Render(Image *img, ImageSRC *src, ImageDSTFrame *frame, int blend, int rotationcenter) {
	if (!img || !img->IsLoaded()) return;
	if (frame->a == 0) return;
	int width = img->GetWidth();
	int height = img->GetHeight();
	if (src->w <= 0) src->w = width;
	if (src->h <= 0) src->h = height;

	

	//SDL_SetTextureAlphaMod(t, frame->a);
	//SDL_SetTextureColorMod(t, frame->r, frame->g, frame->b);
	//SDL_RendererFlip flip = SDL_RendererFlip::SDL_FLIP_NONE;
	//SDL_RenderCopyEx(Game::RENDERER, t, &src_rect, &dst_rect,
	//	frame->angle, &rot_center, flip);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, img->GetTexID());
	UpdateRenderArgs(frame, blend);
	UpdateTexState(src, frame, width, height, rotationcenter);
	glBindTexture(GL_TEXTURE_2D, img->GetTexID());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// reset rotation
	glPopMatrix();
}
#pragma endregion SKINRENDERHELPER