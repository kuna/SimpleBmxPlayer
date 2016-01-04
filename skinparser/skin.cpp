#include "skin.h"
#include "skinutil.h"
using namespace SkinUtil;

// ---------------------------------------------------------

/*
 * Creates render tree for rendering
 * This also cuts and reduces class conditions
 */
void Skin::CreateRenderTree() {

}

bool Skin::Parse(const char *filepath) {
	if (!skinlayout.Parse(filepath)) {
		return false;
	}
	// make render tree
	CreateRenderTree();
	return true;
}

bool Skin::Save(const char *filepath) {
	if (skinlayout.SaveFile(filepath))
		return true;
	return false;
}

void Skin::Release() {
	skinlayout.Clear();
}

Skin::Skin() {}
Skin::~Skin() { Release(); }

// --------------------- Skin End --------------------------
