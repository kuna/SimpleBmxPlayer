#include "skinrendertree.h"
#include "globalresources.h"	// for evaluation condition
#include "luamanager.h"
#include "util.h"
#include "tinyxml2.h"
using namespace tinyxml2;

RenderCondition::RenderCondition() {
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

SkinRenderObject::SkinRenderObject():
objtype(NONE), srccnt(0), dstcnt(0), tag(0), clickable(false), focusable(false) { }

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

// ----- SkinRenderTree -------------------------------------

SkinRenderTree::~SkinRenderTree() {
	ReleaseAll();
}

void SkinRenderTree::ReleaseAll() {
	for (auto it = _objpool.begin(); it != _objpool.end(); ++it) {
		delete *it;
	}
	_objpool.clear();
}

SkinRenderObject* SkinRenderTree::NewNoneObject() {
	SkinRenderObject* obj = new SkinRenderObject();
	_objpool.push_back(obj);
	return obj;
}

#define ISNAME(e, s) (strcmp(e->Name(), s) == 0)

/* private */
void ConstructTreeFromElement(SkinRenderTree &rtree, SkinRenderObjectGroup *group, XMLElement *e) {
	while (e) {

		if (ISNAME(e, "If") || ISNAME(e, "Group")) {
			SkinRenderObjectGroup *g = rtree.NewGroupObject();
			rtree.AddChild(g);
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
		}
		else if (ISNAME(e, "Image")) {
			XMLElement *src = e->FirstChildElement("SRC");
			while (src) {
				src = e->FirstChildElement("SRC");
			}
		}
		else if (ISNAME(e, "Number")) {

		}
		else {
			// parsed as unknown object
			// only parses SRC/DST condition
		}

		// parse common attribute: SRC, DST, condition

		// search for next element
		e = e->NextSiblingElement();
	}
}

bool SkinRenderTreeHelper::ConstructTreeFromSkin(SkinRenderTree &rtree, Skin &s) {
	XMLElement *e = s.skinlayout.FirstChildElement("Skin");
	ConstructTreeFromElement(rtree, &rtree, e);

	return true;
}