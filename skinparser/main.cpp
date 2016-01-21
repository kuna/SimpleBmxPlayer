#include <stdio.h>
#include "skin.h"
#include "skinoption.h"

Skin skin;
SkinOption soption;
_LR2SkinParser lr2skin;

#define LOADPLAY

int main(int argc, char **argv) {
#ifdef LOADPLAY
	lr2skin.ParseLR2Skin("../skin/Wisp_HD/play/HDPLAY_W.lr2skin", &skin);
	skin.Save("skin_conv_play.xml");
	skin.GetDefaultOption(&soption);
	soption.SaveSkinOption("skin_conv_play.option.xml");
#endif
#ifdef LOADSELECT
	lr2skin.ParseLR2Skin("../skin/EndlessCirculation/SE-Select/SEselect.lr2skin", &skin);
	skin.Save("skin_conv_select.xml");
	skin.GetDefaultOption(&soption);
	soption.SaveSkinOption("skin_conv_select.option.xml");
#endif
	return 0;
}