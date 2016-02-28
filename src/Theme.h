#include "ActorRenderer.h"

/*
 * @description
 * manages to load & render skin data
 */
class Theme {
protected:
	// theme structure & option
	Skin skin;
	SkinOption skinoption;

	// rendering object
	std::vector<Actor*> objpool_;
	Actor* base_;
public:
	Actor* GetPtr();
	void Update();
	void Render();
	bool Load(const char* filepath);
	void Release();
};
