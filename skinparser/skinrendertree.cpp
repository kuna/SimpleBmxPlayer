#include "skinrendertree.h"

SkinRenderObject::SkinRenderObject(SkinRenderTree *base, XMLElement *e) {
	if (e) {

	}
	baseparent = base;
	isVisible = true;
}

int SkinRenderObject::CountChilds() {
	int r = childs.size();
	for (auto it = childs.begin(); it != childs.end(); ++it) {
		r += (*it)->CountChilds();
	}
	return r;
}

void SkinRenderObject::LinkEndChild(SkinRenderObject *child) {
	childs.push_back(child);
}

// ------------------ SkinRenderTree ----------------------

SkinRenderTree::SkinRenderTree(): SkinRenderObject(this, 0) {

}

SkinRenderTree::~SkinRenderTree() {
	Release();
}

void SkinRenderTree::Release() {
	// recursively count tree
	_ASSERT(CountChilds() == allobjs.size());
	for (auto it = allobjs.begin(); it != allobjs.end(); ++it)
		delete *it;
	allobjs.clear();
}

SkinRenderObject* SkinRenderTree::CreateNewObject(XMLElement *e) {
	if (strcmp(e->Name(), "Image") == 0) {
		SkinRenderObject* obj = new SkinRenderObject(this, e);
		allobjs.push_back(obj);
		return obj;
	}
	else {
		printf("unknown object: %s\n", e->Name());
		return 0;
	}
}