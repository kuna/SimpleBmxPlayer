#include "playerinfo.h"
#include "util.h"
#include "file.h"
#include "sqlite3.h"
#include "SDL\SDL.h"

using namespace tinyxml2;

// common private function (util)
namespace {
	XMLNode *DeepClone(XMLNode *src, XMLDocument *destDoc)
	{
		XMLNode *current = src->ShallowClone(destDoc);
		for (XMLNode *child = src->FirstChild(); child; child = child->NextSibling())
		{
			current->InsertEndChild(DeepClone(child, destDoc));
		}
		return current;
	}

	XMLElement* CreateElement(XMLNode *base, const char* name) {
		XMLElement *e = base->GetDocument()->NewElement(name);
		base->LinkEndChild(e);
		return e;
	}

	int GetIntValue(XMLNode *base, const char *childname) {
		XMLElement *i = base->FirstChildElement(childname);
		if (!i) return 0;
		else {
			if (i->GetText())
				return atoi(i->GetText());
			else return 0;
		}
	}

	double GetDoubleValue(XMLNode *base, const char *childname) {
		XMLElement *i = base->FirstChildElement(childname);
		if (!i) return 0;
		else {
			if (i->GetText())
				return atof(i->GetText());
			else return 0;
		}
	}
}

#pragma region PLAYERSCORE
PlayerScore::PlayerScore() : PlayerScore(0) {}
PlayerScore::PlayerScore(int notecnt) : totalnote(notecnt) {
	Clear();
}
void PlayerScore::Clear() {
	memset(score, 0, sizeof(score));
	combo = 0;
	maxcombo = 0;
	fast = slow = 0;
}
int PlayerScore::LastNoteFinished() const {
	return totalnote <= score[1] + score[2] + score[3] + score[4] + score[5];
}
int PlayerScore::GetJudgedNote() const {
	return score[1] + score[2] + score[3] + score[4] + score[5];
}
int PlayerScore::CalculateEXScore() const {
	return score[JUDGETYPE::JUDGE_PGREAT] * 2 + score[JUDGETYPE::JUDGE_GREAT];
}
double PlayerScore::CurrentRate() const {
	return (double)CalculateEXScore() / GetJudgedNote() / 2;
}
int PlayerScore::CalculateScore() const { return CalculateRate() * 200000; }
double PlayerScore::CalculateRate() const {
	return (double)CalculateEXScore() / totalnote / 2;
}
int PlayerScore::CalculateGrade() const {
	double rate = CalculateRate();
	if (rate >= 8.0 / 9)
		return GRADETYPE::GRADE_AAA;
	else if (rate >= 7.0 / 9)
		return GRADETYPE::GRADE_AA;
	else if (rate >= 6.0 / 9)
		return GRADETYPE::GRADE_A;
	else if (rate >= 5.0 / 9)
		return GRADETYPE::GRADE_B;
	else if (rate >= 4.0 / 9)
		return GRADETYPE::GRADE_C;
	else if (rate >= 3.0 / 9)
		return GRADETYPE::GRADE_D;
	else if (rate >= 2.0 / 9)
		return GRADETYPE::GRADE_E;
	else
		return GRADETYPE::GRADE_F;
}
void PlayerScore::AddGrade(const int type) {
	score[type]++;
	if (type >= JUDGETYPE::JUDGE_GOOD) {
		combo++;
		if (maxcombo < combo) maxcombo = combo;
	}
	else {
		combo = 0;
	}
}
#pragma endregion

/*
* text - songhash
* text - scorehash
* int - playcount
* int - clearcount
* int - failcount
* int - status
* int - minbp
* int - maxcombo
* int[5] - grade [pg, gr, gd, bd, pr]
* text - replay
*/
#define QUERY_TABLE_CREATE\
	"CREATE TABLE record("\
	"songhash TEXT PRIMARY KEY NOT NULL,"\
	"scorehash TEXT,"\
	"playcount INT,"\
	"clearcount INT,"\
	"failcount INT,"\
	"status INT,"\
	"minbp INT,"\
	"maxcombo INT,"\
	"perfect INT,"\
	"great INT,"\
	"good INT,"\
	"bad INT,"\
	"poor INT,"\
	"replay TEXT,"\
	"op1 INT,"\
	"op2 INT,"\
	"rseed INT,"\
	");"
#define QUERY_TABLE_EXIST\
	"SELECT name FROM sqlite_master WHERE type='table' and name='record';"
#define QUERY_TABLE_SELECT\
	"SELECT * FROM record WHERE songhash=?;"
#define QUERY_TABLE_INSERT\
	"INSERT OR REPLACE INTO record VALUES"\
	"(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
#define QUERY_TABLE_DELETE\
	"DELETE FROM record WHERE songhash=?"
#define QUERY_DB_BEGIN		"BEGIN;"
#define QUERY_DB_COMMIT		"COMMIT;"
#define QUERY_BIND_TEXT(idx, p)	sqlite3_bind_text(stmt, idx, p, -1, SQLITE_STATIC);
#define QUERY_BIND_INT(idx, p)	sqlite3_bind_int(stmt, idx, p);

#define RUNQUERY(q)\
	bool r;\
	sqlite3_stmt *stmt;\
	ASSERT(sql != 0);\
	r = false;\
	sqlite3_prepare16_v2(sql, q, -1, &stmt, 0);
#define FINISHQUERY()\
	sqlite3_reset(stmt);\
	sqlite3_finalize(stmt);\
	return r;
#define CHECKQUERY(type)\
	if (sqlite3_step(stmt) == type) r = true;
#define CHECKTYPE(col, type)\
	ASSERT(sqlite3_column_type(stmt, col) == type)
#define GETTEXT(col, v)\
	CHECKTYPE(col, SQLITE_TEXT), v = (char*)sqlite3_column_text16(stmt, col)
#define GETINT(col, v)\
	CHECKTYPE(col, SQLITE_INTEGER), v = sqlite3_column_int(stmt, col)

void PlayerReplayRecord::AddPress(int time, int lane, int value) {
	objects.push_back({ time, lane, value });
}

void PlayerReplayRecord::AddJudge(int time, int playside, int judge, int fastslow, int silent) {
	objects.push_back({ 
		time, 
		0xA0 + playside, 
		judge + 16 * fastslow + 256 * silent 
	});
}

void PlayerReplayRecord::Clear() {
	objects.clear();
}

#define MAX_REPLAY_BUFFER 1024000	// about 1000kb
void PlayerReplayRecord::Serialize(RString &out) const {
	char *buf = (char*)malloc(MAX_REPLAY_BUFFER);
	// 
	//  0 ~ 32 byte: songhash
	// 40 ~ 44 byte: op1 code
	// 44 ~ 48 byte: op2 code
	// 48 ~ 52 byte: gauge 
	// 52 ~ 56 byte: rseed
	// 56 ~ 72 byte: rate (double)
	// (dummy)
	// 116 ~ 120 byte: header size
	// 120 byte: header end, replay body data starts
	// (1 row per 4 * 3 = 12bytes)
	//
	strcpy(buf, songhash);
	memcpy(buf+40, &op_1p, sizeof(int));
	memcpy(buf+44, &op_2p, sizeof(int));
	memcpy(buf+48, &gauge, sizeof(int));
	memcpy(buf+52, &rseed, sizeof(int));
	memcpy(buf+56, &rate, sizeof(int));

	memcpy(buf + 116, (void*)objects.size(), 4);
	for (int i = 0; i < objects.size(); i++) {
		memcpy(buf + 120 + i * 12, &objects[i], 12);
	}

	// zip compress
	// (TODO)

	// serialize compressed data to base64
	char *b64;
	base64encode(buf, 120 + objects.size() * 12, &b64);

	// delete original data
	out = b64;
	free(b64);
	free(buf);
}

void PlayerReplayRecord::Parse(const RString& in) {
	// parse base64 data into binary
	char *repdata = (char*)malloc(in.size());
	int rep_len = base64decode(in, in.size(), repdata);

	// zip uncompress
	// (TODO)
	char *buf = repdata;

	// memcpy datas
	int isize;
	memcpy(&isize, buf + 116, 4);
	for (int i = 0; i < isize; i++) {
		struct ReplayObject _tmp;
		memcpy(&_tmp, buf + 120 + i * 12, 12);
		objects.push_back(_tmp);
	}

	// cleanup
	free(repdata);
}





/*
 * Helpers
 */

namespace PlayerReplayHelper {
	bool LoadReplay(
		PlayerReplayRecord& rep, 
		const char* playername, 
		const char* songhash, 
		const char* course) {
		RString path;
		if (course) {
			path = ssprintf("../replay/%s/%s/%s.rep", playername, course, songhash);
		}
		else {
			path = ssprintf("../replay/%s/%s.rep", playername, songhash);
		}
		RString repdata;
		if (GetFileContents(path, repdata)) {
			rep.Parse(repdata);
			return true;
		}
		else {
			return false;
		}
	}

	bool SaveReplay(
		const PlayerReplayRecord& rep, 
		const char* playername, 
		const char* songhash, 
		const char* course) {

		RString path;
		if (course) {
			path = ssprintf("../replay/%s/%s/%s.rep", playername, course, songhash);
		}
		else {
			path = ssprintf("../replay/%s/%s.rep", playername, songhash);
		}
		RString repdata;
		rep.Serialize(repdata);
		FileHelper::ConvertPathToSystem(path);
		FileHelper::CreateFolder(path);
		File f;
		if (f.Open(path, "w")) {
			f.Write(repdata);
			f.Close();
			return true;
		} else {
			return false;
		}
	}
}

namespace PlayerRecordHelper {
	/** [private] sqlite3 for querying player record */
	namespace {
		// private; used for load/save PlayerRecord
		sqlite3 *sql = 0;

		// private
		bool OpenSQL(const char* path) {
			ASSERT(sql == 0);
			int rc = sqlite3_open16(path, &sql);
			if (rc != SQLITE_OK) {
				sqlite3_close(sql);
				return false;
			}
			return true;
		}

		// private
		bool CloseSQL() {
			ASSERT(sql != 0);
			bool r = (sqlite3_close(sql) == SQLITE_OK);
			sql = 0;
			return r;
		}

		// private
		bool Begin() {
			RUNQUERY(QUERY_DB_BEGIN);
			CHECKQUERY(SQLITE_DONE);
			FINISHQUERY();
		}

		// private
		bool Commit() {
			RUNQUERY(QUERY_DB_COMMIT);
			CHECKQUERY(SQLITE_DONE);
			FINISHQUERY();
		}

		// private
		bool IsRecordTableExist() {
			RUNQUERY(QUERY_TABLE_EXIST);
			CHECKQUERY(SQLITE_ROW);
			FINISHQUERY();
		}

		// private
		bool CreateRecordTable() {
			if (!Begin()) return false;
			RUNQUERY(QUERY_TABLE_CREATE);
			CHECKQUERY(SQLITE_ROW);
			if (r) r = Commit();
			FINISHQUERY();
		}

		/*
		 * text - songhash
		 * text - scorehash (only for in/out data)
		 * int - playcount
		 * int - clearcount
		 * int - failcount
		 * int - status
		 * int - minbp
		 * int - maxcombo
		 * int[5] - grade [pg, gr, gd, bd, pr]
		 */
		bool QueryPlayRecord(PlayerSongRecord& record, const char *songhash) {
			RUNQUERY(QUERY_TABLE_SELECT);
			QUERY_BIND_TEXT(1, songhash);
			// only get one query
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				RString replay;
				int op1, op2, rseed;
				GETTEXT(0, record.hash);
				//GETTEXT(1, record.scorehash);		// TODO: load scorehash and check validation
				GETINT(2, record.playcount);
				GETINT(3, record.clearcount);
				GETINT(4, record.failcount);
				GETINT(5, record.status);
				GETINT(6, record.minbp);
				GETINT(7, record.maxcombo);
				GETINT(8, record.score.score[5]);
				GETINT(9, record.score.score[4]);
				GETINT(10, record.score.score[5]);
				GETINT(11, record.score.score[2]);
				GETINT(12, record.score.score[1]);
				GETINT(13, record.score.score[0]);
				// COMMENT: ��POOR?
				record.replay.Parse(replay);
				r = true;
			}
			FINISHQUERY();
		}

		bool InsertPlayRecord(const PlayerSongRecord& record, const char* songhash) {
			// before insert, get previous play record if available
			// if exists, then set record with more higher score
			PlayerSongRecord record_;
			if (QueryPlayRecord(record_, songhash)) {
				if (record.score.CalculateEXScore() >= record_.score.CalculateEXScore())
					record_ = record;
			}
			else {
				record_ = record;
			}
			RUNQUERY(QUERY_TABLE_INSERT);
			QUERY_BIND_TEXT(1, songhash);
			QUERY_BIND_TEXT(2, "");		// TODO: generate scorehash
			QUERY_BIND_INT(3, record.playcount);
			QUERY_BIND_INT(4, record.clearcount);
			QUERY_BIND_INT(5, record.failcount);
			QUERY_BIND_INT(6, record.status);
			QUERY_BIND_INT(7, record.minbp);
			QUERY_BIND_INT(8, record.maxcombo);
			QUERY_BIND_INT(9, record.score.score[5]);
			QUERY_BIND_INT(10, record.score.score[4]);
			QUERY_BIND_INT(11, record.score.score[3]);
			QUERY_BIND_INT(12, record.score.score[2]);
			QUERY_BIND_INT(13, record.score.score[1]);
			QUERY_BIND_INT(14, record.score.score[0]);
			CHECKQUERY(SQLITE_DONE);
			if (r) r = Commit();
			FINISHQUERY();
		}

		bool DeletePlayRecord(const char* songhash) {
			RUNQUERY(QUERY_TABLE_DELETE);
			QUERY_BIND_TEXT(1, songhash);
			CHECKQUERY(SQLITE_DONE);
			FINISHQUERY();
		}
	}

	bool LoadPlayerRecord(PlayerSongRecord& record, const char* name, const char* songhash) {
		// create & convert db path to absolute
		RString absolute_db_path = ssprintf("../player/%s.db", name);
		FileHelper::ConvertPathToSystem(absolute_db_path);
		//RString absolute_db_dir = FileHelper::GetParentDirectory(absolute_db_path);
		//FileHelper::CreateFolder(absolute_db_dir);
		// start to query DB
		ASSERT(sql == 0);
		if (!OpenSQL(absolute_db_path))
			return false;
		if (!IsRecordTableExist())
			CreateRecordTable();
		bool recordfound = QueryPlayRecord(record, songhash);
		return recordfound && CloseSQL();
	}

	bool SavePlayerRecord(const PlayerSongRecord& record, const char* name, const char *songhash) {
		RString absolute_db_path = ssprintf("../player/%s.db", name);
		FileHelper::ConvertPathToSystem(absolute_db_path);
		RString absolute_db_dir = FileHelper::GetParentDirectory(absolute_db_path);
		FileHelper::CreateFolder(absolute_db_dir);
		// start to query DB
		ASSERT(sql == 0);
		if (!OpenSQL(absolute_db_path))
			return false;
		if (!IsRecordTableExist())
			CreateRecordTable();
		bool processrecord = InsertPlayRecord(record, songhash);
		return processrecord && CloseSQL();
	}

	bool DeletePlayerRecord(const char* name, const char* songhash) {
		RString absolute_db_path = ssprintf("../player/%s.db", name);
		FileHelper::ConvertPathToSystem(absolute_db_path);
		//RString absolute_db_dir = FileHelper::GetParentDirectory(absolute_db_path);
		//FileHelper::CreateFolder(absolute_db_dir);
		ASSERT(sql == 0);
		if (!OpenSQL(absolute_db_path))
			return false;
		bool processrecord = DeletePlayRecord(songhash);
		return processrecord && CloseSQL();
	}
}

namespace PlayerInfoHelper {
	namespace {
		void LoadBasicConfig(PlayerInfo& player, XMLNode *base) {
			XMLElement *conf = base->FirstChildElement("BasicConfig");
			player.name = conf->FirstChildElement("Name")->GetText();
			player.spgrade = GetIntValue(conf, "SPGrade");
			player.dpgrade = GetIntValue(conf, "DPGrade");
			player.playcount = GetIntValue(conf, "Playcount");
			player.failcount = GetIntValue(conf, "Failcount");
			player.clearcount = GetIntValue(conf, "Clearcount");
		}

		void SaveBasicConfig(const PlayerInfo& player, XMLNode *base) {
			XMLElement *conf = CreateElement(base, "BasicConfig");
			CreateElement(conf, "Name")->SetText(player.name);
			CreateElement(conf, "SPGrade")->SetText(player.spgrade);
			CreateElement(conf, "DPGrade")->SetText(player.dpgrade);
			CreateElement(conf, "Playcount")->SetText(player.playcount);
			CreateElement(conf, "Failcount")->SetText(player.failcount);
			CreateElement(conf, "Clearcount")->SetText(player.clearcount);
		}

		void LoadScoreInfo(PlayerScore &score, XMLNode *base) {
			XMLElement *s = base->FirstChildElement("Score");
			if (!s) return;
			score.score[5] = s->IntAttribute("perfect");
			score.score[4] = s->IntAttribute("great");
			score.score[3] = s->IntAttribute("good");
			score.score[2] = s->IntAttribute("bad");
			score.score[1] = s->IntAttribute("poor");
		}

		void SaveScoreInfo(const PlayerScore &score, XMLNode *base) {
			XMLElement *s = CreateElement(base, "Score");
			s->SetAttribute("perfect", score.score[5]);
			s->SetAttribute("great", score.score[4]);
			s->SetAttribute("good", score.score[3]);
			s->SetAttribute("bad", score.score[2]);
			s->SetAttribute("poor", score.score[1]);
		}

		// defaults
		// (don't touch name)
		void DefaultBasicConfig(PlayerInfo& player) {
			player.spgrade = 0;
			player.dpgrade = 0;
			player.playcount = 0;
			player.failcount = 0;
			player.failcount = 0;
		}

		void DefaultScoreInfo(PlayerScore &score) {
			score = PlayerScore();
		}
	}

	void DefaultPlayerInfo(PlayerInfo& player) {
		DefaultBasicConfig(player);

		DefaultScoreInfo(player.score);

		PlayerKeyHelper::DefaultKeyConfig(player.keyconfig);

		PlayOptionHelper::DefaultPlayConfig(player.playconfig);
	}

	bool LoadPlayerInfo(PlayerInfo& player, const char* name) {
		// create & convert db path to absolute
		RString absolute_db_path = ssprintf("../player/%s.xml", name);
		FileHelper::ConvertPathToSystem(absolute_db_path);
		RString absolute_db_dir = FileHelper::GetParentDirectory(absolute_db_path);
		FileHelper::CreateFolder(absolute_db_dir);
		// start to parse XML file
		XMLDocument *doc = new XMLDocument();
		if (doc->LoadFile(absolute_db_path) != 0) {
			/* failed to load XML. you may have to initalize Player yourself. */
			delete doc;
			return false;
		}

		player.name = name;

		// parsing (basic)
		LoadBasicConfig(player, doc);

		// parsing (grade)
		LoadScoreInfo(player.score, doc);

		// parsing (keyconfig)
		PlayerKeyHelper::LoadKeyConfig(player.keyconfig, doc);

		// parsing (playconfig)
		PlayOptionHelper::LoadPlayConfig(player.playconfig, doc);

		// parsing end, clean-up.
		delete doc;
		return true;
	}

	bool SavePlayerInfo(PlayerInfo& player) {
		// create & convert db path to absolute
		RString absolute_db_path = ssprintf("../player/%s.xml", player.name.c_str());
		FileHelper::ConvertPathToSystem(absolute_db_path);
		RString absolute_db_dir = FileHelper::GetParentDirectory(absolute_db_path);
		FileHelper::CreateFolder(absolute_db_dir);
		// make new XML file
		XMLDocument *doc = new XMLDocument();
		if (!doc) return false;

		// serialize (basic)
		SaveBasicConfig(player, doc);

		// serialize (grade)
		SaveScoreInfo(player.score, doc);

		// serialize (keyconfig)
		PlayerKeyHelper::SaveKeyConfig(player.keyconfig, doc);

		// serialize (playconfig)
		PlayOptionHelper::SavePlayConfig(player.playconfig, doc);

		// save
		doc->SaveFile(absolute_db_path);

		// end. clean-up
		delete doc;
		return true;
	}
}

namespace PlayerKeyHelper {
	RString GetKeyCodeName(int keycode) {
		// TODO
		return "TODO";
	}

	int GetKeyCodeFunction(const PlayerKeyConfig &config, int keycode) {
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < _MAX_KEYCONFIG_MATCH; j++) {
				if (config.keycode[i][j] == keycode)
					return i;
			}
		}
		return -1;
	}

	void LoadKeyConfig(PlayerKeyConfig &config, XMLNode *base) {
		// clear'em first
		memset(config.keycode, 0, sizeof(config.keycode));
		// load
		XMLElement *keyconfig = base->FirstChildElement("KeyConfig");
		if (!base) return;
		for (int i = 0; i < 40; i++) {
			XMLElement *e = keyconfig->FirstChildElement(ssprintf("Key%d", i));
			if (!e) continue;
			for (int j = 0; j < _MAX_KEYCONFIG_MATCH; j++) {
				config.keycode[i][j] = GetIntValue(e, ssprintf("Index%d", j));
			}
		}
	}

	void SaveKeyConfig(const PlayerKeyConfig &config, XMLNode *base) {
		XMLDocument *doc = base->GetDocument();
		XMLElement *keyconfig = doc->NewElement("KeyConfig");
		base->LinkEndChild(keyconfig);
		for (int i = 0; i < 40; i++) {
			XMLElement *e = doc->NewElement(ssprintf("Key%d", i));
			if (!e) continue;
			for (int j = 0; j < _MAX_KEYCONFIG_MATCH; j++) {
				XMLElement *e2 = doc->NewElement(ssprintf("Index%d", j));
				e2->SetText(config.keycode[i][j]);
				e->LinkEndChild(e2);
			}
			keyconfig->LinkEndChild(e);
		}
	}

	void DefaultKeyConfig(PlayerKeyConfig &config) {
		config.keycode[PlayerKeyIndex::P1_BUTTON1][0] = SDL_SCANCODE_Z;
		config.keycode[PlayerKeyIndex::P1_BUTTON2][0] = SDL_SCANCODE_S;
		config.keycode[PlayerKeyIndex::P1_BUTTON3][0] = SDL_SCANCODE_X;
		config.keycode[PlayerKeyIndex::P1_BUTTON4][0] = SDL_SCANCODE_D;
		config.keycode[PlayerKeyIndex::P1_BUTTON5][0] = SDL_SCANCODE_C;
		config.keycode[PlayerKeyIndex::P1_BUTTON6][0] = SDL_SCANCODE_F;
		config.keycode[PlayerKeyIndex::P1_BUTTON7][0] = SDL_SCANCODE_V;
		config.keycode[PlayerKeyIndex::P1_BUTTON1][1] = 1001;
		config.keycode[PlayerKeyIndex::P1_BUTTON2][1] = 1002;
		config.keycode[PlayerKeyIndex::P1_BUTTON3][1] = 1003;
		config.keycode[PlayerKeyIndex::P1_BUTTON4][1] = 1004;
		config.keycode[PlayerKeyIndex::P1_BUTTON5][1] = 1005;
		config.keycode[PlayerKeyIndex::P1_BUTTON6][1] = 1006;
		config.keycode[PlayerKeyIndex::P1_BUTTON7][1] = 1007;
		config.keycode[PlayerKeyIndex::P1_BUTTONSCUP][0] = SDL_SCANCODE_LSHIFT;
		config.keycode[PlayerKeyIndex::P1_BUTTONSCDOWN][0] = SDL_SCANCODE_LCTRL;
		config.keycode[PlayerKeyIndex::P1_BUTTONSCUP][1] = 1100;					// up
		config.keycode[PlayerKeyIndex::P1_BUTTONSCDOWN][1] = 1101;					// down
		config.keycode[PlayerKeyIndex::P1_BUTTONSTART][0] = SDL_SCANCODE_1;

		config.keycode[PlayerKeyIndex::P2_BUTTON1][0] = SDL_SCANCODE_M;
		config.keycode[PlayerKeyIndex::P2_BUTTON2][0] = SDL_SCANCODE_K;
		config.keycode[PlayerKeyIndex::P2_BUTTON3][0] = SDL_SCANCODE_COMMA;
		config.keycode[PlayerKeyIndex::P2_BUTTON4][0] = SDL_SCANCODE_L;
		config.keycode[PlayerKeyIndex::P2_BUTTON5][0] = SDL_SCANCODE_PERIOD;
		config.keycode[PlayerKeyIndex::P2_BUTTON6][0] = SDL_SCANCODE_SEMICOLON;
		config.keycode[PlayerKeyIndex::P2_BUTTON7][0] = SDL_SCANCODE_SLASH;
		config.keycode[PlayerKeyIndex::P2_BUTTONSCUP][0] = SDL_SCANCODE_RSHIFT;
		config.keycode[PlayerKeyIndex::P2_BUTTONSCDOWN][0] = SDL_SCANCODE_RCTRL;
		config.keycode[PlayerKeyIndex::P2_BUTTONSTART][0] = SDL_SCANCODE_2;
	}
}

namespace PlayOptionHelper {
	void DefaultPlayConfig(PlayerPlayConfig &config) {
		config.speed = 1;					// 1x
		config.speedtype = SPEEDTYPE::NONE;	//
		config.floatspeed = 1.0;			// 1 sec ...? well it'll automatically resetted.
		config.sudden = config.lift = 0;
		config.op_1p = config.op_2p = 0;
		config.gaugetype = 0;

		config.pacemaker_type = PACEMAKERTYPE::PACEA;
		config.pacemaker_goal = 90;

		config.longnote = 0;
		config.morenote = 0;
		config.judge = 0;
		config.scratch = 0;
		config.freq = 1;
	}

	void LoadPlayConfig(PlayerPlayConfig &config, XMLNode *base) {
		XMLElement *playconfig = base->FirstChildElement("PlayConfig");

		config.speed = GetIntValue(playconfig, "speed") / 100.0;
		config.speedtype = GetIntValue(playconfig, "speedtype");
		config.floatspeed = GetIntValue(playconfig, "floatspeed") / 1000.0;
		config.usefloatspeed = GetIntValue(playconfig, "usefloat");
		config.sudden = GetIntValue(playconfig, "sudden") / 1000.0;
		config.lift = GetIntValue(playconfig, "lift") / 1000.0;
		config.op_1p = GetIntValue(playconfig, "op_1p");
		config.op_2p = GetIntValue(playconfig, "op_2p");
		config.gaugetype = GetIntValue(playconfig, "gaugetype");

		config.pacemaker_type = GetIntValue(playconfig, "pacemaker_type");
		config.pacemaker_goal = GetIntValue(playconfig, "pacemaker_goal");

		config.longnote = GetIntValue(playconfig, "longnote");
		config.morenote = GetIntValue(playconfig, "morenote");
		config.judge = GetIntValue(playconfig, "judge");
		config.judgeoffset = GetIntValue(playconfig, "judgeoffset");
		config.judgecalibration = GetIntValue(playconfig, "judgecalibration");
		config.scratch = GetIntValue(playconfig, "scratch");
		config.freq = GetIntValue(playconfig, "freq") / 100.0;
	}

	void SavePlayConfig(const PlayerPlayConfig &config, XMLNode *base) {
		XMLDocument *doc = base->GetDocument();
		XMLElement *playconfig = doc->NewElement("PlayConfig");
		doc->LinkEndChild(playconfig);

		// 0.5 : kind of round() function
		CreateElement(playconfig, "speed")->SetText((int)(config.speed * 100 + 0.5));
		CreateElement(playconfig, "speedtype")->SetText(config.speedtype);
		CreateElement(playconfig, "floatspeed")->SetText(config.floatspeed * 1000);
		CreateElement(playconfig, "usefloat")->SetText(config.usefloatspeed);
		CreateElement(playconfig, "sudden")->SetText(config.sudden * 1000);
		CreateElement(playconfig, "lift")->SetText(config.lift * 1000);
		CreateElement(playconfig, "op_1p")->SetText(config.op_1p);
		CreateElement(playconfig, "op_2p")->SetText(config.op_2p);
		CreateElement(playconfig, "gaugetype")->SetText(config.gaugetype);

		CreateElement(playconfig, "pacemaker_type")->SetText(config.pacemaker_type);
		CreateElement(playconfig, "pacemaker_goal")->SetText(config.pacemaker_goal);

		CreateElement(playconfig, "longnote")->SetText(config.longnote);
		CreateElement(playconfig, "morenote")->SetText(config.morenote);
		CreateElement(playconfig, "judge")->SetText(config.judge);
		CreateElement(playconfig, "judgeoffset")->SetText(config.judgeoffset);
		CreateElement(playconfig, "judgecalibration")->SetText(config.judgecalibration);
		CreateElement(playconfig, "scratch")->SetText(config.scratch);
		CreateElement(playconfig, "freq")->SetText(config.freq * 100);
	}
}

// global
// accessible from everywhere
PlayerInfo				PLAYERINFO[2];