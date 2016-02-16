#include <stdio.h>
//#include "skin.h"
//#include "skinoption.h"
#include "skinconverter.h"

//SkinOption soption;

#define LOADPLAY

int main(int argc, char **argv) {
#ifdef LOADPLAY
	SkinConverter::ConvertLR2SkinToLua("../skin/Wisp_HD_lua/play/HDPLAY_W.lr2skin");
#endif
#ifdef LOADSELECT
	SkinConverter::ConvertLR2SkinToXml("../skin/EndlessCirculation/SE-Select/SEselect.lr2skin");
#endif
	return 0;
}