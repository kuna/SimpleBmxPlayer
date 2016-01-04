#include "tinyxml2.h"
#include "skinutil.h"
#include <vector>
using namespace tinyxml2;
using namespace SkinUtil;

/*
 * This class is renderer-dependent, so take care of that
 */

/*
 * general SRC/DST
 */
struct RenderSRC {
	int x, y, w, h, divx, divy, cycle;
};
struct RenderDST {
	int x, y, w, h, time;
	unsigned char a, r, g, b;
	int angle;
};

/*
 * general object (usually image)
 */
class SkinRenderTree;
class SkinRenderObject {
protected:
	// have dependency
	/*
	Image *img;
	Timer timer;
	*/

	// draw information
	RenderSRC src;
	RenderSRC src_onmouse;
	std::vector<RenderDST> dst;
	int blend;

	// parent/child
	SkinRenderTree* baseparent;
	std::vector<SkinRenderObject*> childs;

	// original XMLElement
	XMLElement *elem;

	// state of element
	ClassAttribute classes;
	bool isVisible;
	bool isOnMouse;
	int state;

	// event of element
public:
	SkinRenderObject(SkinRenderTree *base, XMLElement *e);
	void SetSRC(const RenderSRC& src);
	void AddDST(const RenderDST& dst);
	int CountChilds();
	void LinkEndChild(SkinRenderObject* child);

	/* common command */
	void TriggerTimer();
	bool IsCollsion(int x, int y);
	SkinRenderObject* FindCollsion(int x, int y);

	/* safe-conversion */
	void ToButton();
	void ToBga();
	void ToText();
	void ToSlider();
	void ToNumber();
};

/*
 * base object
 */
class SkinRenderTree : public SkinRenderObject {
private:
	std::vector<SkinRenderObject*> allobjs;
public:
	SkinRenderTree();
	~SkinRenderTree();
	void Release();
	SkinRenderObject* CreateNewObject(XMLElement *e);
};