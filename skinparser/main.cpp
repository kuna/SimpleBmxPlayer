#include <stdio.h>
//#include "skin.h"
//#include "skinoption.h"
#include "skinconverter.h"

//SkinOption soption;

#ifdef LUAMANAGER
// enable this line if you want to run test
//#define LUATEST
#endif

#define LOADPLAY

int main(int argc, char **argv) {
#ifdef LOADPLAY
	SkinConverter::ConvertLR2SkinToXml("../skin/Wisp_HD_lua/play/HDPLAY_W.lr2skin");
	//SkinConverter::ConvertLR2SkinToLua("../skin/Wisp_HD_lua/play/HDPLAY_W.lr2skin");
#ifdef LUATEST
	SkinTest::InitLua();
	if (!SkinTest::TestLuaSkin("../skin/Wisp_HD_lua/play/HDPLAY_W.lua"))
		printf("LuaTest: lua skin test failed.\n");
	SkinTest::CloseLua();
#endif
#endif
#ifdef LOADSELECTx
	SkinConverter::ConvertLR2SkinToXml("../skin/EndlessCirculation/SE-Select/SEselect.lr2skin");
#endif
	return 0;
}