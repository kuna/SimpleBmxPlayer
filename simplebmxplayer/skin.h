// for game
// minimalized function - this CANNOT read all lr2skin.

#pragma once

#include "image.h"
#include "timer.h"
#include "skinelement.h"
#include "SDL\SDL_FontCache.h"
#include <vector>
#include <map>

/*
 * Skin infos that we're using
 */
struct SkinFont {
	std::string filepath;		// ttf filepath
	std::string texture;		// foreground texture path
	int fontsize;
	int thickness;
	int style;
	int border;
};

class Skin {
public:
	// very basic metadata
	char filepath[256];
	char skinname[256];		// default name: IO::getFilename(filepath)
	char author[256];
	int type;

	// skin headers
	// (store some metadatas ...)
	std::map<std::string, std::vector<std::string>> headers;

	// skin resource list
	std::vector<std::string> resource_imgs;
	std::vector<SkinFont> resource_fonts;

	// skin options
	// supports 2 kind of option - string, file
	// both of them saved as 
	std::map<std::string, std::vector<std::string>> option_string;
	std::map<std::string, std::string> option_file;
public:
	// contains skin ID related information
	SkinElementGroup skinelement_all;							// contains all skin elements (used for release)
	std::map<std::string, SkinElementGroup> skinelement_id;		// 
	std::map<std::string, SkinElementGroup> skinelement_class;	// 

public:
	Skin();
	~Skin();
	void Release();
	bool Parse(const char *filepath);
	/*
	 * This method will copy SkinElement object itself.
	 */
	void AddElement(SkinElement *e);

	/*
	 * Get SkinElement by Element Id
	 * nothing found, then return 0.
	 */
	SkinElement* GetElementById(char *id);
	/*
	* Get SkinElements by Element ClassName
	* if (e == 0), only check for existence
	*/
	bool GetElementsByClassName(char *classname, SkinElementGroup *v);
	/*
	 * Returns all elements that has no classname/id or else specific attribute
	 * These objects should all be all drawn during all skin rendering.
	 */
	void GetPlainElements(SkinElementGroup *v);
};

/*
* SkinOption
* stores previously setted option value / provides value from option
*/
class SkinOption {
public:
	struct FileOption {
		std::string path;
		int type;			// 0: image, 1: font
	};

private:
	// the real stored value in option file
	std::map<std::string, int> option_string_idx;
	std::map<std::string, FileOption> option_file_idx;

public:
	// used when modify/add option
	void ModifyOption(const std::string& key, int val);
	void ModifyFileOption(const std::string& key, const std::string& path);
	void SetOptionType(const std::string& key, int type);

	//
	int GetOption(const std::string& key);
	std::string& GetFileOption(const std::string& key);
	// this method surely provides existing file so don't worry.
	bool IsOptionKeyExists(const std::string& key);
	bool IsFileOptionKeyExists(const std::string& key);

	void DefaultSkinOption(Skin &s);	// TODO
	void LoadSkinOption(Skin &s);
	void SaveSkinOption();

	void Clear();
};

/*
* SkinResource
* stores current skin's resource
*/
class SkinResource {
private:
	// skin resources
	// (currently only supports Image type)
	Image imgs[50];
	FC_Font* fonts[50];
public:
	~SkinResource();
	void Release();
	void LoadResource(Skin &s);
	Image* GetImageResource(int idx);
	FC_Font* GetFontResource(int idx);
};


// -------------------------------------------------------------

/*
* Depreciated; only for backward compatibility
* these classes converts *.lr2skin to *.xml (modern skin format)
*/
class _LR2SkinElement {
public:
	/*
	 * Specific attributes
	 * these attributes has great hint for discriminating some specific objects.
	 */

	// type
	// 0: image, 1: text, 2: button, 3. number
	// 4: slider, 
	// 10: judgeline, 11: note(include LN/AUTO ...), 12: judgeline
	// 
	std::string objname;
	// timer number
	int timer;
	int timer_src;
	// current option
	std::vector<int> options;
	// resource_id
	// we can get resource name from this.
	int resource_id;
	int value_id;
	int muki;			// only for bargraph; 1=vertical, 0=horizon

	/*
	 * General attributes (SRC)
	 */
	int divx, divy;
	int cycle;
	int rotatecenter;
	
	/*
	 * General attributes (DST)
	 * these options has data about drawing
	 */
	ImageSRC src;
	std::vector<ImageDST> dst;
	int blend;
	int usefilter;
	int center;		// this would be depreciated (always center)
	int looptime;
public:
	/*
	* returns is object have SRC & DST
	* used when we need to determine whether we add new object or not
	*/
	bool IsValid();

	void AddSrc(char **args);
	void AddDst(char **args);
	void CopyOptions(std::vector<int>& opts);

	ImageSRC* GetLastSrc();
	ImageDST* GetLastDst();

	// ONLY positive number can make good result.
	bool CheckOption(int option);
	void Clear();
};

class _LR2SkinParser {
private:
	Skin *s;
	SkinOption *option;
private:
	/*
	 * ONLY for LR2 SKIN parsing (*depreciated*)
	 */
	bool current_skin_option[1024];			// current skin option (work for IF~SETSWITCH~ENDIF clause)
	_LR2SkinElement element[100];			// temporary used - LR2Skin's each element
	int parser_level;						// current level
	int parser_level_condition_num[100];	// condition count for each level
	std::vector<int> parser_condition;		// currently containing condition
	void ParseLR2SkinArgs(char **args);		// parser for each line
	/* MUST delete object yourself! */
	SkinElement* ConvertToElement(_LR2SkinElement *e);	// converts LR2SkinElement to modern one
	int current_line;									// contains current line information
	std::map<int, std::string> option_id_name;			// option ID -> option name (option)
	std::map<std::string, std::string> option_fn_name;	// option fn -> option name (file)

	SkinElement *prv_obj;								// used to detect the object is same as previous one (ConvertToElement)
private:
	void Parse(const char *filepath);
	bool CheckOption(int option);
public:
	/*
	 * you can provide information to skin like this:
	 * 11: Battle mode?
	 * 13: Course mode?
	 * 292: Expert mode?
	 * 293: Grade mode?
	 */
	void SetSkinOption(int idx);

	/*
	 * caution: SkinOption's value will be overwritten after this method called
	 * So, if you want to load preserved SkinOption, Load them after Skin is loaded.
	 */
	bool ParseLR2Skin(const char *filepath, Skin *s, SkinOption *soption);

	void Clear();
};