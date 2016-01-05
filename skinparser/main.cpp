#include <stdio.h>
#include "skin.h"

Skin skin;
SkinOption soption;
_LR2SkinParser lr2skin;

int main(int argc, char **argv) {
	lr2skin.ParseLR2Skin("../skin/play/HDPLAY_W.lr2skin", &skin);
	skin.Save("skin_conv_play.xml");
	//lr2skin.ParseLR2Skin("../skin/EndlessCirculation/SE-Select/se_select.csv", &skin);
	//skin.Save("skin_conv_select.xml");
	return 0;
}