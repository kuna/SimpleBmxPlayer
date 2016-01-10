#include <stdio.h>
#include "skin.h"
#include "skinoption.h"

Skin skin;
SkinOption soption;
_LR2SkinParser lr2skin;

#define LOADPLAY

int main(int argc, char **argv) {
#ifdef LOADPLAY
	lr2skin.ParseLR2Skin("../skin/play/HDPLAY_W_org.lr2skin", &skin);
	skin.GetDefaultOption(&soption);
	skin.Save("skin_conv_play.xml");
	soption.SaveSkinOption("skin_conv_play.option.xml");
#endif
#ifdef LOADSELECT
	lr2skin.ParseLR2Skin("../skin/EndlessCirculation/SE-Select/SEselect.lr2skin", &skin);
	skin.Save("skin_conv_select.xml");
#endif
	return 0;
}