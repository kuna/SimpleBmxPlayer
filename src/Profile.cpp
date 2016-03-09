#include "Profile.h"
#include "util.h"
#include "file.h"
#include "logger.h"
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
	"CREATE TABLE `record` ("\
	"`songhash` TEXT NOT NULL,"\
	"`scorehash` TEXT,"\
	"`playcount` INT,"\
	"`clearcount` INT,"\
	"`failcount` INT,"\
	"`status` INT,"\
	"`minbp` INT,"\
	"`maxcombo` INT,"\
	"`perfect` INT,"\
	"`great` INT,"\
	"`good` INT,"\
	"`bad` INT,"\
	"`poor` INT,"\
	"`poor` INT,"\
	"`fast` INT,"\
	"`slow` INT,"\
	"`op1` INT,"\
	"`op2` INT,"\
	"`rseed` INT,"\
	"`type` INT,"\
	"PRIMARY KEY(songhash)"\
	");"
#define QUERY_TABLE_EXIST\
	"SELECT name FROM sqlite_master WHERE type='table' and name='record';"
#define QUERY_TABLE_SELECT\
	"SELECT * FROM record WHERE songhash=?;"
#define QUERY_TABLE_INSERT\
	"INSERT OR REPLACE INTO record VALUES"\
	"(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
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
	r = true;\
	sqlite3_prepare_v2(sql, q, -1, &stmt, 0);
#define FINISHQUERY()\
	sqlite3_reset(stmt);\
	sqlite3_finalize(stmt);\
	return r;
#define CHECKQUERY(type)\
	if (sqlite3_step(stmt) != type) r = false;
#define CHECKTYPE(col, type)\
	ASSERT(sqlite3_column_type(stmt, col) == type)
#define GETTEXT(col, v)\
	CHECKTYPE(col, SQLITE_TEXT), v = (char*)sqlite3_column_text(stmt, col)
#define GETINT(col, v)\
	CHECKTYPE(col, SQLITE_INTEGER), v = sqlite3_column_int(stmt, col)


namespace PlayerRecordHelper {
	/** [private] sqlite3 for querying player record */
	sqlite3 *sql = 0;
	namespace {
		// private
		bool OpenSQL(const RString& path) {
			ASSERT(sql == 0);
			/*
			 * MUST convert \\ -> /
			 */
#ifdef ASF
			RString path_con = path;
			replace(path_con.begin(), path_con.end(), '/', '\\');
			int rc = sqlite3_open(path_con, &sql);
#else
			int rc = sqlite3_open(path, &sql);
#endif
			if (rc != SQLITE_OK) {
				LOG->Critical("SQLITE initization failed!");
				LOG->Critical(sqlite3_errmsg(sql));
				sqlite3_close(sql);
				sql = 0;
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
#ifdef _WIN32
			const char *p = QUERY_TABLE_CREATE;
#endif
			if (!Begin()) return false;
			RUNQUERY(QUERY_TABLE_CREATE);
			CHECKQUERY(SQLITE_DONE);
			if (r) r = Commit();
			else LOG->Warn(sqlite3_errmsg(sql));
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
				r = true;
			}
			FINISHQUERY();
		}

		bool InsertPlayRecord(const PlayerSongRecord& record) {
			// before insert, get previous play record if available
			// if exists, then set record with more higher score
			PlayerSongRecord record_;
			if (QueryPlayRecord(record_, record_.hash)) {
				if (record.score.CalculateEXScore() >= record_.score.CalculateEXScore())
					record_ = record;
			}
			else {
				record_ = record;
			}
#ifdef _WIN32
			const char *p = QUERY_TABLE_INSERT;
#endif
			RUNQUERY(QUERY_TABLE_INSERT);
			QUERY_BIND_TEXT(1, record.hash);
			QUERY_BIND_TEXT(2, "");					// TODO: generate scorehash
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
			QUERY_BIND_INT(13, record.score.score[1] + record.score.score[0]);
			QUERY_BIND_INT(14, record.score.fast);
			QUERY_BIND_INT(15, record.score.slow);
			QUERY_BIND_INT(16, record.op1);
			QUERY_BIND_INT(17, record.op2);
			QUERY_BIND_INT(18, record.rseed);
			QUERY_BIND_INT(19, record.type);
			CHECKQUERY(SQLITE_DONE);
			if (r) r = Commit();
			else LOG->Warn(sqlite3_errmsg(sql));
			FINISHQUERY();
		}

		bool DeletePlayRecord(const char* songhash) {
			RUNQUERY(QUERY_TABLE_DELETE);
			QUERY_BIND_TEXT(1, songhash);
			CHECKQUERY(SQLITE_DONE);
			if (!r) LOG->Warn(sqlite3_errmsg(sql));
			FINISHQUERY();
		}
	}

	bool LoadPlayerRecord(PlayerSongRecord& record, const char* name, const char* songhash) {
		// create & convert db path to absolute
		RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.db", name));
		FILEMANAGER->CreateDirectory(absolute_db_path);
		// start to query DB
		ASSERT(sql == 0);
		if (!OpenSQL(absolute_db_path))
			return false;
		if (!IsRecordTableExist())
			CreateRecordTable();
		bool recordfound = QueryPlayRecord(record, songhash);
		bool sql = CloseSQL();
		return recordfound && sql;
	}

	bool SavePlayerRecord(const PlayerSongRecord& record, const char* name) {
		RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.db", name));
		FILEMANAGER->CreateDirectory(absolute_db_path);
		// start to query DB
		ASSERT(sql == 0);
		if (!OpenSQL(absolute_db_path))
			return false;
		if (!IsRecordTableExist())
			CreateRecordTable();
		bool processrecord = InsertPlayRecord(record);
		bool sql = CloseSQL();
		return processrecord && sql;
	}

	bool DeletePlayerRecord(const char* name, const char* songhash) {
		RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.db", name));
		ASSERT(sql == 0);
		if (!OpenSQL(absolute_db_path))
			return false;
		bool processrecord = DeletePlayRecord(songhash);
		bool sql = CloseSQL();
		return processrecord && sql;
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

		PlayOptionHelper::DefaultPlayConfig(player.playconfig);
	}

	bool LoadPlayerInfo(PlayerInfo& player, const char* name) {
		// create & convert db path to absolute
		RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.xml", name));
		FILEMANAGER->CreateDirectory(absolute_db_path);
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

		// parsing (playconfig)
		PlayOptionHelper::LoadPlayConfig(player.playconfig, doc);

		// parsing end, clean-up.
		delete doc;
		return true;
	}

	bool SavePlayerInfo(PlayerInfo& player) {
		// create & convert db path to absolute
		RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.xml", player.name.c_str()));
		FILEMANAGER->CreateDirectory(absolute_db_path);
		// make new XML file
		XMLDocument *doc = new XMLDocument();
		if (!doc) return false;

		// serialize (basic)
		SaveBasicConfig(player, doc);

		// serialize (grade)
		SaveScoreInfo(player.score, doc);

		// serialize (playconfig)
		PlayOptionHelper::SavePlayConfig(player.playconfig, doc);

		// save
		doc->SaveFile(absolute_db_path);

		// end. clean-up
		delete doc;
		return true;
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

		config.ghost_type = 0;
		config.judge_type = 0;
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

		config.ghost_type = GetIntValue(playconfig, "ghost_type");
		config.judge_type = GetIntValue(playconfig, "judge_type");
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

		CreateElement(playconfig, "ghost_type")->SetText(config.ghost_type);
		CreateElement(playconfig, "judge_type")->SetText(config.judge_type);
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







bool ReplayEvent::IsJudge() {
	return (lane > 20);
}
int ReplayEvent::GetSide() {
	return lane == 0xAF ? 0 : 1;
}
int ReplayEvent::GetJudge() {
	return value % 10;
}
int ReplayEvent::GetFastSlow() {
	return (value & 0x11) / 0x10;
}
int ReplayEvent::GetSilent() {
	return value & 0x100;
}
void ReplayData::SetRound(int v) {
	round = v;
}
void ReplayData::AddPress(int time, int lane, int value) {
	objects[round].push_back({ time, lane, value });
}

void ReplayData::AddJudge(int time, int playside, int judge, int fastslow, int silent) {
	objects[round].push_back({
		time,
		0xA0 + playside,
		judge + 16 * fastslow + 256 * silent
	});
}

void ReplayData::Clear() {
	objects[round].clear();
}

#define MAX_REPLAY_BUFFER 1024000	// about 1000kb
void ReplayData::Serialize(RString &out) const {
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
	memcpy(buf + 40, &op_1p, sizeof(int));
	memcpy(buf + 44, &op_2p, sizeof(int));
	memcpy(buf + 48, &gauge, sizeof(int));
	memcpy(buf + 52, &rseed, sizeof(int));
	memcpy(buf + 56, &rate, sizeof(int));

	*(int*)(buf + 116) = 10;		// 10 of courses
	int ptr = 120;
	for (int i = 0; i < 10; i++) {
		int record_cnt = objects[i].size();
		memcpy(buf + ptr, (void*)&record_cnt, 4);	ptr += 4;
		for (int j = 0; j < record_cnt; j++) {
			memcpy(buf + ptr, &objects[i][j], 12);
			ptr += 12;
		}
	}

	// zip compress
	// (TODO)

	// serialize compressed data to base64
	char *b64;
	base64encode(buf, ptr, &b64);

	// delete original data
	out = b64;
	free(b64);
	free(buf);
}

void ReplayData::Parse(const RString& in) {
	// parse base64 data into binary
	char *repdata = (char*)malloc(in.size());
	int rep_len = base64decode(in, in.size(), repdata);

	// zip uncompress
	// (TODO)
	char *buf = repdata;

	// memcpy datas
	// TODO: load metadata (like op ...)
	int coursecnt;
	memcpy(&coursecnt, buf + 116, 4);
	int ptr = 120;
	for (int i = 0; i < coursecnt; i++) {
		int isize;
		memcpy(&isize, buf + ptr, 12);
		ptr += 4;
		for (int j = 0; j < isize; j++) {
			struct ReplayEvent _tmp;
			memcpy(&_tmp, buf + ptr, 12);
			ptr += 12;
			objects[i].push_back(_tmp);
		}
	}

	// cleanup
	free(repdata);
}

namespace PlayerReplayHelper {
	bool LoadReplay(
		ReplayData& rep,
		const char* playername,
		const char* songhash,
		const char* course) {
		RString path;
		if (course) {
			path = ssprintf("../player/replay/%s/%s/%s.rep", playername, course, songhash);
		}
		else {
			path = ssprintf("../player/replay/%s/%s.rep", playername, songhash);
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
		const ReplayData& rep,
		const char* playername,
		const char* songhash,
		const char* course) {

		RString path;
		if (course) {
			path = ssprintf("../player/replay/%s/%s/%s.rep", playername, course, songhash);
		}
		else {
			path = ssprintf("../player/replay/%s/%s.rep", playername, songhash);
		}
		RString dir = get_filedir(path);
		RString repdata;
		rep.Serialize(repdata);
		FILEMANAGER->CreateDirectory(dir);
		File f;
		if (f.Open(path, "w")) {
			f.Write(repdata);
			f.Close();
			return true;
		}
		else {
			return false;
		}
	}
}