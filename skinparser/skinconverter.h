/*
 * @description
 * things with converting skin, mainly:
 * csv(lr2skin) -> xml
 * xml -> lua
 * - @lazykuna
 *
 * converting LR2 file is written at skinlr2.cpp
 */

#include "skin.h"
#include "tinyxml2.h"
#include <map>
#include <vector>

// -------------------------------------------------------------
#define MAX_LINE_CHARACTER_	1024
#define MAX_ARGS_COUNT_		100

class _LR2SkinParser {
private:
	typedef char line_[MAX_LINE_CHARACTER_];
	typedef const char* args_[MAX_ARGS_COUNT_];
	typedef const char** args_read_;
	struct line_v_ { line_ line__; };
	struct args_v_ { args_ args__; };

private:
	Skin *s;
	SkinMetric *sm;
	tinyxml2::XMLElement *cur_e;				// current base element
	std::vector<line_v_> lines_;				// contains line data
	std::vector<args_v_> line_args_;			// contains arguments per line
	int line_total;								// the line we totally read
	char filepath[1024];						// current file name

	/*
	* parsing status
	*/
	int image_cnt;												// only for setting image name
	int font_cnt;												// only for setting font name
	std::map<std::string, std::string> filter_to_optionname;	// convert filter to optionfile name
	std::map<int, int> texturefont_id;							// check texturefont existence

	int currentline;
	tinyxml2::XMLElement *condition_element[100];
	int condition_level;

	/*
	* local translator
	*/
	int GenerateTexturefontString(tinyxml2::XMLElement *srcelement);	// for #XXX_NUMBER object
	void ConvertToTextureFont(tinyxml2::XMLElement *numele);

	/*
	* Inner skin loaders
	*/
	// load and stores skin in splited line data
	int LoadSkin(const char *filepath);
	void SplitCSVLine(char *p, const char **args);
	// parse all loaded skin
	void ParseSkinCSVLine();
	void ParseSkinMetricLine();
private:
	bool ProcessDepreciated(const args_read_& args);
	bool ProcessCondition(const args_read_& args);
	bool IsMetadata(const char* cmd);
	bool ProcessMetadata(const args_read_& args);
	bool ProcessResource(const args_read_& args);
	void ProcessNumber(tinyxml2::XMLElement* obj, int sop1, int sop2, int sop3);
	int ProcessLane(const args_read_& args, tinyxml2::XMLElement *src, int resid);			// process commands about lane
	int ProcessCombo(const args_read_& args, tinyxml2::XMLElement *obj);		// process commands about combo
	int ProcessSelectBar(const args_read_& args, tinyxml2::XMLElement *obj);	// process commands about select bar
	int ProcessSelectBar_DST(const args_read_& args);					// process commands about select bar
	// pacemaker: use default XML
public:
	// for extern use
	void AddPathToOption(const std::string& path, const std::string& option) {
		filter_to_optionname.insert(std::pair<std::string, std::string>(path, option));
	};

	// this includes skin layout
	// use this method after LR2Skin is parsed.
	bool ParseCSV(const char* filepath, Skin *s);
	// this includes metadata
	// metadata should be saved as xml file.
	bool ParseLR2Skin(const char *filepath, SkinMetric *s);
	void Clear();

	_LR2SkinParser() { Clear(); }
};

// enable this line if you have lua library
#define LUA 1
#ifdef LUA
extern "C" {
#include "Lua/lua.h"
#include "Lua/lualib.h"
#include "Lua/lauxlib.h"
}
#include "LuaBridge.h"


typedef lua_State Lua;
namespace LuaHelper {
	using namespace std;

	template<class T>
	void Push(lua_State *L, const T &Object);

	template<class T>
	bool FromStack(lua_State *L, T &Object, int iOffset);

	template<class T>
	bool Pop(lua_State *L, T &val)
	{
		bool bRet = FromStack(L, val, -1);
		lua_pop(L, 1);
		return bRet;
	}

	int GetLuaStack(lua_State *L);
	bool RunExpression(Lua *L, const string &sExpression, const string &sName);
	bool RunScriptFile(Lua *L, const string &sFile, int ReturnValues = 0);
	bool RunScriptOnStack(Lua *L, string &Error, int Args, int ReturnValues, bool ReportError);
	bool RunScript(Lua *L, const string &Script, const string &Name, string &Error, int Args, int ReturnValues, bool ReportError);
}
#endif

namespace SkinConverter {
	bool ConvertLR2SkinToXml(const char* lr2skinpath);
	bool ConvertLR2SkinToLua(const char* lr2skinpath);
}

namespace SkinTest {
#ifdef LUA
	void InitLua();
	void CloseLua();
	bool TestLuaSkin(const char* luaskinpath);
#endif
}