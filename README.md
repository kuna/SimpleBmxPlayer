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
- ```iconv```

##### This Project doesn't include ```libSDL 2.0```, ```FFmpeg```
  - download ```Dev Lib / Runtime Binaries``` directly from [SDL](https://www.libsdl.org/download-2.0.php), and extract file to ```include/SDL, lib/x86, Release```.
  - download ffmpeg from [Zeranoe FFmpeg](http://ffmpeg.zeranoe.com/builds/), and extract to ```include/ffmpeg, lib/ffmpeg, Release```


### how to use
- drag BMS file to program.

### available options
- (TODO)

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
  - 4 Combo per a bar(measure).
