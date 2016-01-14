#include "skinrendertree.h"

SkinRenderObject::SkinRenderObject():
objtype(NONE), srccnt(0), dstcnt(0), tag(0), clickable(false), focusable(false) { }

void SkinRenderObject::Clear() {
	srccnt = 0;
	dstcnt = 0;
}

void SkinRenderObject::AddSRC(ImageSRC &src, const RString &condition) {
	this->src[srccnt] = src;
	this->src_condition[srccnt] = condition;
	srccnt++;
}

void SkinRenderObject::AddDST(ImageDST &dst, const RString &condition) {
	this->dst[dstcnt] = dst;
	this->dst_condition[dstcnt] = condition;
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