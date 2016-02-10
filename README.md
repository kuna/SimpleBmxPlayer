# SimpleBmxPlayer

### abstract
- just plays \*.bmx(bms, bme, bml, etc.) file.
- good to use when editing \*.bmx files.

### dependency
- ```SDL2```
- ```SDL2_image```
- ```SDL2_mixer```
- ```SDL2_ttf```
- ```SDL_FontCache```
- ```Zeranoe FFmpeg```
- ```bmsbel+```
- ```Lua```
- ```pthread (Win32)```
- ```tinyxml2```
- ```zlib```
- ```zziplib```

##### This Project doesn't include ```libSDL 2.0```, ```SDL2_ttf```, ```FFmpeg```
  - download ```Dev Lib / Runtime Binaries``` directly from [SDL](https://www.libsdl.org/download-2.0.php), and extract file to ```include/SDL, lib/x86, Release```.
  - download ffmpeg from [Zeranoe FFmpeg](http://ffmpeg.zeranoe.com/builds/), and extract to ```include/ffmpeg, lib/ffmpeg, Release```

### how to use
- drag BMS file to program.
- archive file available (ex: ./ex.zip/test.bms)

### keys
- default key config is -
  (1P) LS Z S X D C F V
  (2P) RS M K , L . ; /
  you can change it by changing preset files.
  (also supports controller)
- [Up/Down]      change speed.
- [Right/Left]   change sudden.\n"
- [F12]          float speed toggle\n"
- [Start+WB/BB]  change speed\n"
- [Start+SC]     (if float on) change float speed.
- [Start+SC]     (if sudden on) change sudden.
- [Start+SC]     (if sudden off) change lift.
- [Start+VEFX]   float speed toggle
- [Start double] toggle sudden.

- (WB: white button; generally call 1,3,5,7 buttons.)
- (BB: blue? black? btton; generally calls 2, 4, 6 buttons)
- (SC: scratch; LShift on keyboard)

### available options
-bgaoff: don't load image files (ignore image channel)
-replay: show replay file (if no replay file, it won't turn on)
-auto: autoplayed by DJ
-op__: set op for player (RANDOM ... etc; only support int value)

-g_: set gauge (0: GROOVE, 1: EASY, 2: HARD, 3: EXHARD, 4: HAZARD)
-n______: set user profile to play (default: NONAME)
-s_: start from n-th measure
-e_: end(cut) at n-th measure
-r_: repeat bms for n-times
-rate__: song rate (1.0 is normal, lower is faster; decimal)
-pace__: pacemaker percent (decimal)

default value can be set by changing settings.xml file.

### preview (youtube link)
- [@aaf3c68](https://www.youtube.com/watch?v=11DYI2wY4SU)

### known issue / etc
- This program will use new skin format, which is ID/event-based and xml-formatted. but I'll try to convert lr2skin to this skin format automatically.

### Additional standard suggestion
- Supporting UTF8 file format
  - *.bmx file formats are old, and most of them uses Shift_JIS. and that makes strings displayed wrong in Non_JP(?) language OS.
  - First, attempt to convert first 10 lines as UTF8.
  - If failes, then consider that file as Shift_JIS. (Fallback)
- Extended BMS Word *(NOT IMPLEMENTED)*
  - Uses 4 base36-character for a word.
  - *.bmx file will have almost x2 size than previous format.
  - That will make BMS composer to add more audio/image(video) files.
  - That will also make merging bms files easiler, like LR2\'s marathon mode.
  - To use this function, write ```#EXTENDWORD 1``` to BMS header.
- LongNote *(GAME FEATURE)*
  - Inherits O2Jam feature - that is, you have to key up at the end of the LongNote. If not, Combo break and Guage lose.
  - We may have 2 judges - LN start(press), LN end(up).
  - If start judge failes, then 2 miss will be generated.
  - If LN end judge failed, then 1 miss and start judge will be generated.
- Scratch LongNote *(GAME FEATURE)*
  - Have to press one more key after scratching longnote.
  - So, we may have 3 judges - LN start(press), LN end(up), LN end(press)
  - If start judge failes, then 2 miss will be generated.
  - If LN end(up or press) judge failes, then 1 miss and start judge will be generated.
- Hell LongNote *(GAME FEATURE)*
  - Combo is continually up during longnote pressing.
  - To use Hell LongNote, write ```#LNHELL 1``` to BMS header.
  - Middle notes between Longnotes will be judged PGREAT if you just stay key pressed during hellnote.
