#include "skinrendertree.h"
#include "globalresources.h"	// for evaluation condition
#include "luamanager.h"
#include "util.h"
#include "tinyxml2.h"
using namespace tinyxml2;

// ------ SkinRenderCondition ----------------------

RenderCondition::RenderCondition() : condcnt(0) {
	memset(not, 0, sizeof(not));
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
		if (!cond[condcnt] || (not[condcnt] && cond[condcnt]->IsStarted()) || (!not[condcnt] && !cond[condcnt]->IsStarted()))
			return false;
	}
	return true;
}

// ------ Skin General rendering Objects -----------------

SkinRenderObject::SkinRenderObject(int type):
srccnt(0), dstcnt(0), tag(0), clickable(false), focusable(false), objtype(type) { }

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

void SkinRenderObject::SetCondition(const RString &str) { condition = str; }

SkinUnknownObject::SkinUnknownObject() : SkinRenderObject(UNKNOWN) {}

SkinGroupObject::SkinGroupObject() : SkinRenderObject(GROUP) {}

void SkinGroupObject::AddChild(SkinRenderObject *obj) {
	_childs.push_back(obj);
}

std::vector<SkinRenderObject*>::iterator
SkinGroupObject::begin() { return _childs.begin(); }

std::vector<SkinRenderObject*>::iterator
SkinGroupObject::end() { return _childs.end(); }

SkinImageObject::SkinImageObject() : SkinRenderObject(IMAGE) {}

SkinNumberObject::SkinNumberObject() : SkinRenderObject(NUMBER) {}

SkinTextObject::SkinTextObject() : SkinRenderObject(TEXT) {}

SkinGraphObject::SkinGraphObject() : SkinRenderObject(GRAPH) {}

SkinSliderObject::SkinSliderObject() : SkinRenderObject(SLIDER) {}

// ----- SkinRenderTree -------------------------------------

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
	_imagekey.insert(std::pair<RString, Image*>(id, img));
}

void SkinRenderTree::ReleaseAllResources() {
	for (auto it = _imagekey.begin(); it != _imagekey.end(); ++it)
		IMAGEPOOL->Release(it->second);
	_imagekey.clear();
}

SkinUnknownObject* SkinRenderTree::NewUnknownObject() {
	SkinUnknownObject* obj = new SkinUnknownObject();
	_objpool.push_back(obj);
	return obj;
}

SkinGroupObject* SkinRenderTree::NewGroupObject() {
	SkinGroupObject* obj = new SkinGroupObject();
	_objpool.push_back(obj);
	return obj;
}

SkinImageObject* SkinRenderTree::NewImageObject() {
	SkinImageObject* obj = new SkinImageObject();
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
	src.divy = e->IntAttribute("divy");
	const char *resid = e->Attribute("resid");
	if (resid) src.resid = resid;
}

/* private */
void ConstructDSTFromElement(ImageDST &dst, XMLElement *e) {
	dst.blend = e->IntAttribute("blend");
	dst.rotatecenter = e->IntAttribute("rotatecenter");
	XMLElement *ef = e->FirstChildElement("Frame");
	while (ef) {
		ImageDSTFrame f;
		f.time = ef->IntAttribute("time");
		f.x = ef->IntAttribute("x");
		f.y = ef->IntAttribute("y");
		f.w = ef->IntAttribute("w");
		f.h = ef->IntAttribute("h");
		f.a = ef->IntAttribute("a");
		f.r = ef->IntAttribute("r");
		f.g = ef->IntAttribute("g");
		f.b = ef->IntAttribute("b");
		f.angle = ef->IntAttribute("angle");
		SkinRenderHelper::AddFrame(dst, f);
		ef = ef->NextSiblingElement("Frame");
	}
}

/* private */
void ConstructTreeFromElement(SkinRenderTree &rtree, SkinGroupObject *group, XMLElement *e) {
	while (e) {
		SkinRenderObject *obj;

		if (ISNAME(e, "If") || ISNAME(e, "Group")) {
			SkinGroupObject *g = rtree.NewGroupObject();
			rtree.AddChild(g);
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "Image")) {
			obj = rtree.NewImageObject();
		}
		/*else if (ISNAME(e, "Number")) {

		}*/
		else {
			// parsed as unknown object
			// only parses SRC/DST condition
			obj = rtree.NewUnknownObject();
			rtree.AddChild(obj);
		}

		// parse common attribute: SRC, DST, condition
		const char* condition = e->Attribute("condition");
		if (condition) obj->SetCondition(condition);
		XMLElement *src = e->FirstChildElement("SRC");
		while (src) {
			ImageSRC _s;
			ConstructSRCFromElement(_s, src);
			const char* _cond = e->Attribute("condition");
			obj->AddSRC(_s, _cond?_cond:"");
			src = e->NextSiblingElement("SRC");
		}
		XMLElement *dst = e->FirstChildElement("DST");
		while (dst) {
			ImageDST _d;
			ConstructDSTFromElement(_d, dst);
			const char* _cond = e->Attribute("condition");
			obj->AddDST(_d, _cond?_cond:"");
			dst = e->NextSiblingElement("DST");
		}
		
		// some elements need processing after SRC/DST is read

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

void SkinRenderHelper::AddFrame(ImageDST &d, ImageDSTFrame &f) {
	d.frame.push_back(f);
}

SDL_Rect& SkinRenderHelper::ToRect(ImageSRC &d, int time) {
	SDL_Rect r;
	return r;
}

SDL_Rect& SkinRenderHelper::ToRect(ImageDST &d, int time) {
	SDL_Rect r;
	return r;
}
