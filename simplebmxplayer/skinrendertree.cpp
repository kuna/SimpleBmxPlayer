
#include "game.h"
#include "skinrendertree.h"
#include "globalresources.h"	// for evaluation condition
#include "luamanager.h"
#include "util.h"
#include "logger.h"
#include "tinyxml2.h"
using namespace tinyxml2;

// redefinition;
void ConstructSRCFromElement(ImageSRC &, XMLElement *);
void ConstructDSTFromElement(ImageDST &, XMLElement *);

// ------ SkinRenderCondition ----------------------

RenderCondition::RenderCondition() : condcnt(0) {
	memset(not, 0, sizeof(not));
	memset(cond, 0, sizeof(cond));
}

void RenderCondition::Set(const RString &condition) {
	std::vector<RString> conditions;
	split(condition, ",", conditions);
	for (auto it = conditions.begin(); it != conditions.end(); ++it) {
		Trim(*it);
		key[condcnt] = *it;
		if (key[condcnt][0] == '!') {
			not[condcnt] = true;
			key[condcnt] = key[condcnt].substr(1);
		}
		else {
			not[condcnt] = false;
		}
		cond[condcnt] = TIMERPOOL->Get(*it);
		condcnt++;
	}
}

void RenderCondition::SetLuacondition(const RString &condition) {
	luacondition = condition;
}

bool RenderCondition::Evaluate() {
	if (luacondition.size()) {
		// getting Lua object doesn't cost much; but evaluating does.
		// aware not to use lua conditional script.
		Lua* l = LUA->Get();
		RString e;
		LuaHelpers::RunScript(l, luacondition, "RenderCondition", e, 0, 1);
		bool r = (bool)lua_toboolean(l, -1);
		lua_pop(l, 1);
		LUA->Release(l);
		return r;
	}
	for (int i = 0; i < condcnt; i++) {
		if (cond[i] == 0 || (not[i] && cond[i]->IsStarted()) || (!not[i] && !cond[i]->IsStarted()))
			return false;
	}
	return true;
}

// ------ Skin General rendering Objects -----------------

SkinRenderObject::SkinRenderObject(SkinRenderTree* owner, int type):
rtree(owner), srccnt(0), dstcnt(0), tag(0), clickable(false), focusable(false), objtype(type) { }

void SkinRenderObject::Clear() {
	srccnt = 0;
	dstcnt = 0;
}

void SkinRenderObject::AddSRC(ImageSRC &src, const RString &condition, bool lua) {
	this->src[srccnt] = src;
	if (!lua)
		this->src_condition[srccnt].Set(condition);
	else
		this->src_condition[srccnt].SetLuacondition(condition);
	srccnt++;
}

void SkinRenderObject::AddDST(ImageDST &dst, const RString &condition, bool lua) {
	this->dst[dstcnt] = dst;
	if (!lua)
		this->dst_condition[dstcnt].Set(condition);
	else
		this->dst_condition[dstcnt].SetLuacondition(condition);
	dstcnt++;
}

bool SkinRenderObject::EvaluateCondition() {
	return condition.Evaluate();
}

bool SkinRenderObject::IsGroup() {
	switch (objtype) {
	case OBJTYPE::GROUP:
		return true;
	}
	return false;
}

bool SkinRenderObject::IsGeneral() {
	switch (objtype) {
	case OBJTYPE::IMAGE:
	case OBJTYPE::NUMBER:
	case OBJTYPE::GRAPH:
	case OBJTYPE::SLIDER:
	case OBJTYPE::TEXT:
	case OBJTYPE::BUTTON:
		return true;
	}
	return false;
}

SkinUnknownObject* SkinRenderObject::ToUnknown() {
	if (objtype == OBJTYPE::UNKNOWN)
		return (SkinUnknownObject*)this;
	else
		return 0;
}

SkinGroupObject* SkinRenderObject::ToGroup() {
	if (objtype == OBJTYPE::GROUP)
		return (SkinGroupObject*)this;
	else
		return 0;
}

SkinImageObject* SkinRenderObject::ToImage() {
	if (objtype == OBJTYPE::IMAGE)
		return (SkinImageObject*)this;
	else
		return 0;
}

// do nothing
void SkinRenderObject::Render() {  }

bool SkinRenderObject::Click(int x, int y) {
	if (clickable) {

	}
	else {
		return false;
	}
}

bool SkinRenderObject::Hover(int x, int y) {
	if (focusable) {

	}
	else {
		return false;
	}
}

void SkinRenderObject::SetCondition(const RString &str) { condition.Set(str); }

SkinUnknownObject::SkinUnknownObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::UNKNOWN) {}

SkinGroupObject::SkinGroupObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::GROUP) {}

void SkinGroupObject::AddChild(SkinRenderObject *obj) {
	_childs.push_back(obj);
}

std::vector<SkinRenderObject*>::iterator
SkinGroupObject::begin() { return _childs.begin(); }

std::vector<SkinRenderObject*>::iterator
SkinGroupObject::end() { return _childs.end(); }

SkinImageObject::SkinImageObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::IMAGE) {
	memset(img, 0, sizeof(img));
}

void SkinImageObject::Render() {
	/* check for this object's basic render condition */
	if (!condition.Evaluate()) return;

	Image* _img = 0;
	ImageSRC *_src = 0;
	for (int i = 0; i < srccnt; i++) {
		if (src_condition[i].Evaluate()) {
			/* if image isn't cached, then find it and cache */
			if (img[i] == 0) {
				img[i] = rtree->_imagekey[src[i].resid];
				if (img[i] == 0) img[i] = (Image*)-1;
			}
			if ((int)img[i] == -1) break;		// this image is already failed to search
			/* now we gotcha proper SRC & img */
			_img = img[i];
			_src = &src[i];
			break;
		}
	}
	/* if we hand't find proper SRC/IMG, don't render */
	if (_img == 0 || _src == 0) return;

	/* find proper DST */
	ImageDST* _dst = 0;
	for (int j = 0; j < dstcnt; j++) {
		if (dst_condition[j].Evaluate()) {
			_dst = &dst[j];
			break;
		}
	}
	/* if timer or dst is invalid, don't render */
	if (!_dst) return;
	if (!_dst->timer || !_dst->timer->IsStarted()) return;

	/* now all prepared? then start to draw */
	ImageDSTFrame _frame;
	if (SkinRenderHelper::CalculateFrame(*_dst, _frame))
		SkinRenderHelper::Render(_img, _src, &_frame, _dst->blend, _dst->rotatecenter);
}

SkinNumberObject::SkinNumberObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::NUMBER) {}

SkinTextObject::SkinTextObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::TEXT) {}

SkinGraphObject::SkinGraphObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::GRAPH) {}

SkinSliderObject::SkinSliderObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::SLIDER) {}

SkinPlayObject::SkinPlayObject(SkinRenderTree* owner) : 
SkinRenderObject(owner, OBJTYPE::PLAYLANE), imgobj_judgeline(0), imgobj_line(0) {}

void SkinPlayObject::ConstructLane(XMLElement *lane) {
	if (!lane)
		return;

	int idx = lane->IntAttribute("index");
	if (idx < 0 || idx >= 20) {
		LOG->Warn("Invalid Lane found in <Play> object (%d), ignore.", idx);
		return;
	}
	XMLElement *src_element;
	src_element = lane->FirstChildElement("SRC_NOTE");
	if (src_element) {
		ConstructSRCFromElement(Note[idx].normal, src_element);
		Note[idx].img = rtree->_imagekey[Note[idx].normal.resid];
	}
	src_element = lane->FirstChildElement("SRC_LN_START");
	if (src_element) ConstructSRCFromElement(Note[idx].ln_start, src_element);
	src_element = lane->FirstChildElement("SRC_LN_BODY");
	if (src_element) ConstructSRCFromElement(Note[idx].ln_body, src_element);
	src_element = lane->FirstChildElement("SRC_LN_END");
	if (src_element) ConstructSRCFromElement(Note[idx].ln_end, src_element);
	src_element = lane->FirstChildElement("SRC_MINE");
	if (src_element) ConstructSRCFromElement(Note[idx].mine, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_NOTE");
	if (src_element) {
		ConstructSRCFromElement(AutoNote[idx].normal, src_element);
		AutoNote[idx].img = rtree->_imagekey[Note[idx].normal.resid];
	}
	src_element = lane->FirstChildElement("SRC_AUTO_LN_START");
	if (src_element) ConstructSRCFromElement(AutoNote[idx].ln_start, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_BODY");
	if (src_element) ConstructSRCFromElement(AutoNote[idx].ln_body, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_END");
	if (src_element) ConstructSRCFromElement(AutoNote[idx].ln_end, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_MINE");
	if (src_element) ConstructSRCFromElement(AutoNote[idx].mine, src_element);
	XMLElement *dst = lane->FirstChildElement("DST");
	if (dst) {
		ConstructDSTFromElement(Note[idx].dst, dst);
		//ConstructDSTFromElement(AutoNote[idx].dst, dst);
	}
}

void SkinPlayObject::SetJudgelineObject(XMLElement *judgelineobj) {
	if (!imgobj_judgeline) 
		imgobj_judgeline = rtree->NewImageObject();
	SkinRenderHelper::ConstructBasicRenderObject(imgobj_judgeline, judgelineobj);
}

void SkinPlayObject::SetLineObject(XMLElement *lineobj) {
	if (!imgobj_line)
		imgobj_line = rtree->NewImageObject();
	SkinRenderHelper::ConstructBasicRenderObject(imgobj_line, lineobj);
}

Uint32 SkinPlayObject::GetLaneHeight() { return h; }

bool SkinPlayObject::IsLaneExists(int laneindex) { return Note[laneindex].normal.w > 0; }

void SkinPlayObject::RenderLane(int laneindex, double pos, bool mine) { 
	// TODO: in case of Auto?
	if (IsLaneExists(laneindex)) {
		ImageDSTFrame frame;
		if (!SkinRenderHelper::CalculateFrame(Note[laneindex].dst, frame))
			return;
		frame.y += h * pos;
		SkinRenderHelper::Render(Note[laneindex].img, &Note[laneindex].normal, &frame);
	}
}

// ----- SkinRenderTree -------------------------------------

SkinRenderTree::SkinRenderTree() : SkinGroupObject(this) {}

SkinRenderTree::~SkinRenderTree() {
	ReleaseAll();
	ReleaseAllResources();
}

void SkinRenderTree::ReleaseAll() {
	for (auto it = _objpool.begin(); it != _objpool.end(); ++it) {
		delete *it;
	}
	_objpool.clear();
}

void SkinRenderTree::RegisterImage(RString &id, RString &path) {
	Image *img = IMAGEPOOL->Load(path);
	if (img) {
		_imagekey.insert(std::pair<RString, Image*>(id, img));
	}
}

void SkinRenderTree::ReleaseAllResources() {
	for (auto it = _imagekey.begin(); it != _imagekey.end(); ++it)
		IMAGEPOOL->Release(it->second);
	_imagekey.clear();
}

SkinUnknownObject* SkinRenderTree::NewUnknownObject() {
	SkinUnknownObject* obj = new SkinUnknownObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinGroupObject* SkinRenderTree::NewGroupObject() {
	SkinGroupObject* obj = new SkinGroupObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinImageObject* SkinRenderTree::NewImageObject() {
	SkinImageObject* obj = new SkinImageObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinPlayObject* SkinRenderTree::NewPlayObject() {
	SkinPlayObject* obj = new SkinPlayObject(this);
	_objpool.push_back(obj);
	return obj;
}

// ------ SkinRenderHelper -----------------------------

#define ISNAME(e, s) (strcmp(e->Name(), s) == 0)

/* private */
void ConstructSRCFromElement(ImageSRC &src, XMLElement *e) {
	src.x = e->IntAttribute("x");
	src.y = e->IntAttribute("y");
	src.w = e->IntAttribute("w");
	src.h = e->IntAttribute("h");
	src.cycle = e->IntAttribute("cycle");
	src.divx = e->IntAttribute("divx");
	if (src.divx < 1) src.divx = 1;
	src.divy = e->IntAttribute("divy");
	if (src.divy < 1) src.divy = 1;
	const char *resid = e->Attribute("resid");
	if (resid) src.resid = resid;
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
	dst.loopstart = 0;
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
		if (ef->Attribute("loopstart")) {
			dst.loopstart = f.time;
		}
		dst.loopend = f.time;
		SkinRenderHelper::AddFrame(dst, f);
		ef = ef->NextSiblingElement("Frame");
	}
	
	// CASE: no loop (div by 0 error)
	if (dst.loopstart == dst.loopend)
		dst.loopstart = 0;
}

/* private */
void ConstructTreeFromElement(SkinRenderTree &rtree, SkinGroupObject *group, XMLElement *e) {
	while (e) {
		SkinRenderObject *obj;

		if (ISNAME(e, "If") || ISNAME(e, "Group")) {
			SkinGroupObject *g = rtree.NewGroupObject();
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "Image")) {
			obj = rtree.NewImageObject();
		}
		/*else if (ISNAME(e, "Number")) {

		}*/
		/*
		 * Special object parsing start
		 */
		else if (ISNAME(e, "Play")) {
			SkinPlayObject *p = rtree.NewPlayObject();
			p->x = e->IntAttribute("x");
			p->y = e->IntAttribute("y");
			p->w = e->IntAttribute("w");
			p->h = e->IntAttribute("h");
			XMLElement *lane = e->FirstChildElement("Lane");
			while (lane) {
				p->ConstructLane(lane);
				lane = lane->NextSiblingElement("Lane");
			}
			p->SetJudgelineObject(e->FirstChildElement("JUDGELINE"));
			p->SetLineObject(e->FirstChildElement("LINE"));
			obj = p;
		}
		else {
			// parsed as unknown object
			// only parses SRC/DST condition
			obj = rtree.NewUnknownObject();
		}
		group->AddChild(obj);

		// parse common attribute: SRC, DST, condition
		SkinRenderHelper::ConstructBasicRenderObject(obj, e);

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
	}
	else {
		return false;
	}

	return true;
}

void SkinRenderHelper::ConstructBasicRenderObject(SkinRenderObject *obj, XMLElement *e) {
	const char* condition = e->Attribute("condition");
	if (condition) obj->SetCondition(condition);
	XMLElement *src = e->FirstChildElement("SRC");
	while (src) {
		ImageSRC _s;
		ConstructSRCFromElement(_s, src);
		const char* _cond = e->Attribute("condition");
		obj->AddSRC(_s, _cond ? _cond : "");
		src = e->NextSiblingElement("SRC");
	}
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

SDL_Rect SkinRenderHelper::ToRect(ImageSRC &d, int time) {
	// (TODO) if SRC x, y < 0 then we have to get it manually 
	int dx = d.w / d.divx;
	int dy = d.h / d.divy;
	int frame = d.cycle ? time / d.cycle : 0;
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
	if (dst.loopstart > 0 && time > dst.loopstart) {
		time = (time - dst.loopstart) % (dst.loopend - dst.loopstart) + dst.loopstart;
	}
	int nframe = 0;
	for (; nframe < dst.frame.size(); nframe++) {
		if (dst.frame[nframe].time >= time)
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

	return true;
}

ImageDSTFrame SkinRenderHelper::Tween(ImageDSTFrame &a, ImageDSTFrame &b, double t, int acctype) {
	switch (acctype) {
	case ACCTYPE::LINEAR:
#define TWEEN(a, b) ((a)*(1-t) + (b)*t)
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
#define TWEEN(a, b) ((a)*(1-T) + (b)*T)
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
#define TWEEN(a, b) ((a)*(1-T) + (b)*T)
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

	static Timer* basetimer(TIMERPOOL->Get("OnScene"));
	SDL_Rect src_rect = ToRect(*src, basetimer->GetTick());
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

	SDL_RenderCopyEx(Game::RENDERER, img->GetPtr(), &src_rect, &dst_rect,
		frame->angle, &rot_center, SDL_RendererFlip::SDL_FLIP_NONE);
}