#include "playerinfo.h"
#include "util.h"
#include "file.h"
#include "sqlite3.h"

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

	int GetIntValue(XMLElement *e, const char *childname) {
		XMLElement *i = e->FirstChildElement(childname);
		if (!i) return 0;
		else {
			if (i->GetText())
				return atoi(i->GetText());
			else return 0;
		}
	}
}

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
	"INSERT OR REPLACE INTO record VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
#define QUERY_TABLE_DELETE\
	"DELETE FROM record WHERE songhash=?"
#define QUERY_DB_BEGIN		"BEGIN;"
#define QUERY_DB_COMMIT		"COMMIT;"
#define QUERY_BIND_TEXT(p)	sqlite3_bind_text(stmt, 1, p, -1, SQLITE_STATIC);

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

namespace PlayerInfoHelper {
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
		bool QueryPlayRecord(PlayerSongRecord& record, const char *songhash) {
			RUNQUERY(QUERY_TABLE_SELECT);
			QUERY_BIND_TEXT(songhash);
			// only get one query
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				RString replay;
				int op1, op2, rseed;
				GETTEXT(0, record.hash);
				GETTEXT(1, record.scorehash);
				GETINT(2, record.playcount);
				GETINT(3, record.clearcount);
				GETINT(4, record.failcount);
				GETINT(5, record.status);
				GETINT(6, record.minbp);
				GETINT(7, record.maxcombo);
				GETINT(8, record.grade.grade[5]);
				GETINT(9, record.grade.grade[4]);
				GETINT(10, record.grade.grade[5]);
				GETINT(11, record.grade.grade[2]);
				GETINT(12, record.grade.grade[1]);
				GETTEXT(13, replay);					// replay
				record.replay.Parse(replay);
				r = true;
			}
			FINISHQUERY();
		}

		bool InsertPlayRecord(const PlayerSongRecord& record, const char* songhash) {
			RUNQUERY(QUERY_TABLE_SELECT);
			QUERY_BIND_TEXT(songhash);
			Begin();
			CHECKQUERY(SQLITE_DONE);
			if (r) r = Commit();
			FINISHQUERY();
		}

		bool DeletePlayRecord(const char* songhash) {
			RUNQUERY(QUERY_TABLE_DELETE);
			QUERY_BIND_TEXT(songhash);
			CHECKQUERY(SQLITE_DONE);
			FINISHQUERY();
		}
	}

	bool LoadPlayerRecord(PlayerSongRecord& record, const char* name, const char* songhash) {
		// create & convert db path to absolute
		RString absolute_db_path = ssprintf("../player/%s.db", name);
		FileHelper::ConvertPathToAbsolute(absolute_db_path);
		RString absolute_db_dir = FileHelper::GetParentDirectory(absolute_db_path);
		FileHelper::CreateFolder(absolute_db_dir);
		// start to query DB
		ASSERT(sql == 0);
		if (!OpenSQL(absolute_db_path))
			return false;
		if (!IsRecordTableExist())
			CreateRecordTable();
		bool recordfound = QueryPlayRecord(record, songhash);
		return recordfound && CloseSQL();
	}

	bool SavePlayerRecord(const PlayerSongRecord& record, const char* name) {

	}

	bool DeletePlayerRecord(const char* name, const char* songhash) {

	}

	namespace {
		void LoadBasicConfig(PlayerInfo& player, XMLNode *base) {
			player.name = base->FirstChildElement("Name")->GetText();
			player.spgrade = atoi(base->FirstChildElement("SPGrade")->GetText());
			player.spgrade = atoi(base->FirstChildElement("DPGrade")->GetText());
			player.playcount = atoi(base->FirstChildElement("Playcount")->GetText());
			player.failcount = atoi(base->FirstChildElement("Failcount")->GetText());
			player.clearcount = atoi(base->FirstChildElement("Clearcount")->GetText());
		}

		void SaveBasicConfig(PlayerInfo& player, XMLDocument *doc) {
			CreateElement(doc, "Name")->SetText(player.name);
			CreateElement(doc, "SPGrade")->SetText(player.name);
			CreateElement(doc, "DPGrade")->SetText(player.name);
			CreateElement(doc, "Playcount")->SetText(player.name);
			CreateElement(doc, "Failcount")->SetText(player.name);
			CreateElement(doc, "Clearcount")->SetText(player.name);
		}

		void LoadScoreInfo(const PlayerGrade &grade, XMLNode *base) {
			
		}

		void SaveScoreInfo(const PlayerGrade &grade, XMLNode *base) {

		}
	}

	void DefaultPlayerInfo(PlayerInfo& player) {
		// TODO
	}

	bool LoadPlayerInfo(PlayerInfo& player, const char* name) {
		// create & convert db path to absolute
		RString absolute_db_path = ssprintf("../player/%s.xml", name);
		FileHelper::ConvertPathToAbsolute(absolute_db_path);
		RString absolute_db_dir = FileHelper::GetParentDirectory(absolute_db_path);
		FileHelper::CreateFolder(absolute_db_dir);
		// start to parse XML file
		XMLDocument *doc = new XMLDocument();
		if (doc->Parse(absolute_db_path) != 0) {
			/* failed to load XML. you may have to initalize Player yourself. */
			delete doc;
			return false;
		}

		// parsing (basic)
		LoadBasicConfig(player, doc);

		// parsing (grade)
		LoadScoreInfo(player.grade, doc);

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
		RString absolute_db_path = ssprintf("../player/%s.xml", player.name);
		FileHelper::ConvertPathToAbsolute(absolute_db_path);
		RString absolute_db_dir = FileHelper::GetParentDirectory(absolute_db_path);
		FileHelper::CreateFolder(absolute_db_dir);
		// make new XML file
		XMLDocument *doc = new XMLDocument();
		if (!doc) return false;

		// serialize (basic)
		SaveBasicConfig(player, doc);

		// serialize (grade)
		SaveScoreInfo(player.grade, doc);

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
		return PlayerKeyIndex::NONE;
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
}

namespace PlayOptionHelper {
	void DefaultPlayConfig(PlayerPlayConfig &config) {
		config.speed = 100;		// 1x
		config.speedtype = SPEEDTYPE::NONE;
		config.lane = config.lift = 0;
		config.op_1p = config.op_2p = 0;
		config.guagetype = 0;

		config.pacemaker_type = PACEMAKERTYPE::PACEA;
		config.pacemaker_goal = 90;

		config.longnote = 0;
		config.morenote = 0;
		config.judge = 0;
		config.scratch = 0;
		config.freq = 1;
	}

	void LoadPlayConfig(PlayerPlayConfig &config, XMLNode *base) {

	}

	void SavePlayConfig(const PlayerPlayConfig &config, XMLNode *base) {

	}
}