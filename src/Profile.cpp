#include "Profile.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "DB.h"
#include "SDL\SDL.h"

/*
 * Profile may effect to SongPlayer option,
 * So MUST initialize that object first.
 */

using namespace tinyxml2;

Profile*		PROFILE[2];



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





Profile::Profile(int side) {
	// load theme metrics (TODO)sPlayerName;
	sPlayerName.SetFromPool(side, "Name");
	iPlayerLevel.SetFromPool(side, "Level");
	iPlayerSpGrade;
	iPlayerDpGrade;
	iPlayerScorePerfect.SetFromPool(side, "ScorePG");
	iPlayerScoreGreat.SetFromPool(side, "ScoreGR");
	iPlayerScoreGood.SetFromPool(side, "ScoreGD");
	iPlayerScoreBad.SetFromPool(side, "ScoreBD");
	iPlayerScorePoor.SetFromPool(side, "ScorePR");

	iPlayerGauge.SetFromPool(side, "Gauge");
	iPlayerRandomS1.SetFromPool(side, "RandomS1");
	iPlayerRandomS2.SetFromPool(side, "RandomS2");
	iPlayerScratch.SetFromPool(side, "SC");
	iPlayerFlip.SetFromPool(side, "Flip");
	iPlayerBattle.SetFromPool(side, "Battle");
	iPlayerSpeed.SetFromPool(side, "Speed");
	iPlayerSpeedtype.SetFromPool(side, "Speedtype");
	iPlayerRange.SetFromPool(side, "Range");
	iPlayerMode.SetFromPool(side, "Mode");

	iPlayerPacemaker.SetFromPool(side, "Pacemaker");
	iPlayerPacemakerGoal.SetFromPool(side, "");
	iPlayerJudgetiming;
	iPlayerJudgeoffset;
	isPlayerJudgePosition[0].SetFromPool("JudgeOff");
	isPlayerJudgePosition[1].SetFromPool("JudgeA");
	isPlayerJudgePosition[2].SetFromPool("JudgeB");
	isPlayerJudgePosition[3].SetFromPool("JudgeC");
	isPlayerGhostPosition[0].SetFromPool("GhostOff");
	isPlayerGhostPosition[1].SetFromPool("GhostA");
	isPlayerGhostPosition[2].SetFromPool("GhostB");
	isPlayerGhostPosition[3].SetFromPool("GhostC");

	iReplayExists;
	iRecordClearcount;
	iRecordFailcount;
	iRecordClear;
	iRecordScore[6];
	iRecordExScore;
	iRecordRate;
	iRecordRate_d;
	iRecordRank;
	iRecordCombo;
	iRecordMinBP;
	iRecordCBRK;
	iRecordMaxCombo;

	// not loaded
	playertype = PLAYERTYPE::NONE;
}

void Profile::UpdateInfoMetrics() {
	// TODO
	for (int i = 0; i < 4; i++) {
		if (i == config.ghost_type)
			isPlayerGhostPosition[i].Start();
		else
			isPlayerGhostPosition[i].Stop();
	}
	for (int i = 0; i < 4; i++) {
		if (i == config.judge_type)
			isPlayerJudgePosition[i].Start();
		else
			isPlayerJudgePosition[i].Stop();
	}
}

void Profile::UpdateConfigMetrics() {

}

void Profile::UpdateOptionMetrics() {

}

Profile::~Profile() {

}

void Profile::DefaultProfile(const RString& name) {
	this->name = name;

	// basic config
	spgrade = 0;
	dpgrade = 0;
	playcount = 0;
	failcount = 0;
	clearcount = 0;
	
	// score
	score = PlayScore();

	// play setting
	option = PlayOption();
	config = PlayConfig();

	playertype = PLAYERTYPE::HUMAN;
}

bool Profile::LoadProfile(const RString& name) {
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
	XMLElement *base = doc->FirstChildElement("Profile");
	if (!base) {
		LOG->Critical("Improper player profile xml, cannot load profile `%s`.", name.c_str());
		delete doc;
		return false;
	}

	this->name = name;

	// basic config
	XMLElement *conf = base->FirstChildElement("Info");
	spgrade = GetIntValue(conf, "SPGrade");
	dpgrade = GetIntValue(conf, "DPGrade");
	playcount = GetIntValue(conf, "Playcount");
	failcount = GetIntValue(conf, "Failcount");
	clearcount = GetIntValue(conf, "Clearcount");

	// score
	XMLElement *s = conf->FirstChildElement("Score");
	if (!s) return;
	score.score[5] = s->IntAttribute("pg");
	score.score[4] = s->IntAttribute("gr");
	score.score[3] = s->IntAttribute("gd");
	score.score[2] = s->IntAttribute("bd");
	score.score[1] = s->IntAttribute("pr");
	score.score[0] = s->IntAttribute("er");

	// play config
	XMLElement *playconfig = base->FirstChildElement("PlayConfig");
	config.ghost_type = GetIntValue(playconfig, "ghost_type");
	config.judge_type = GetIntValue(playconfig, "judge_type");
	config.pacemaker_type = GetIntValue(playconfig, "pacemaker_type");
	config.pacemaker_goal = GetIntValue(playconfig, "pacemaker_goal");

#if 0
	config.speed = GetIntValue(playconfig, "speed") / 100.0;
	config.speedtype = GetIntValue(playconfig, "speedtype");
	config.floatspeed = GetIntValue(playconfig, "floatspeed") / 1000.0;
	config.usefloatspeed = GetIntValue(playconfig, "usefloat");
	config.sudden = GetIntValue(playconfig, "sudden") / 1000.0;
	config.lift = GetIntValue(playconfig, "lift") / 1000.0;
	config.op_1p = GetIntValue(playconfig, "op_1p");
	config.op_2p = GetIntValue(playconfig, "op_2p");
	config.gaugetype = GetIntValue(playconfig, "gaugetype");

	config.longnote = GetIntValue(playconfig, "longnote");
	config.morenote = GetIntValue(playconfig, "morenote");
	config.judge = GetIntValue(playconfig, "judge");
	config.judgeoffset = GetIntValue(playconfig, "judgeoffset");
	config.judgecalibration = GetIntValue(playconfig, "judgecalibration");
	config.scratch = GetIntValue(playconfig, "scratch");
	config.freq = GetIntValue(playconfig, "freq") / 100.0;
#endif

	XMLElement *e_playoption = base->FirstChildElement("PlayOption");
	if (e_playoption && e_playoption->GetText())
		option.ParseOptionString(e_playoption->GetText());

	// end. clean-up
	delete doc;
	playertype = PLAYERTYPE::HUMAN;
	return true;
}

void Profile::SaveProfile(const RString& name) {
	if (!IsProfileLoaded()) return;

	// create & convert db path to absolute
	RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.xml", name.c_str()));
	FILEMANAGER->CreateDirectory(absolute_db_path);
	// make new XML file
	XMLDocument *doc = new XMLDocument();
	if (!doc) return;
	XMLElement *base = CreateElement(doc, "Profile");

	// basic config
	XMLElement *conf = CreateElement(base, "Info");
	//CreateElement(conf, "Name")->SetText(player.name);
	CreateElement(conf, "SPGrade")->SetText(spgrade);
	CreateElement(conf, "DPGrade")->SetText(dpgrade);
	CreateElement(conf, "Playcount")->SetText(playcount);
	CreateElement(conf, "Failcount")->SetText(failcount);
	CreateElement(conf, "Clearcount")->SetText(clearcount);

	// score
	XMLElement *s = CreateElement(conf, "Score");
	s->SetAttribute("pg", score.score[5]);
	s->SetAttribute("gr", score.score[4]);
	s->SetAttribute("gd", score.score[3]);
	s->SetAttribute("bd", score.score[2]);
	s->SetAttribute("pr", score.score[1]);
	s->SetAttribute("er", score.score[0]);
	
	CreateElement(conf, "hash")->SetText("TODO");

	// play config
	XMLElement *playconfig = doc->NewElement("PlayConfig");
	doc->LinkEndChild(playconfig);

	// 0.5 : kind of round() function
#if 0
	CreateElement(playconfig, "op_1p")->SetText(config.op_1p);
	CreateElement(playconfig, "op_2p")->SetText(config.op_2p);
	CreateElement(playconfig, "longnote")->SetText(config.longnote);
	CreateElement(playconfig, "morenote")->SetText(config.morenote);
	CreateElement(playconfig, "judge")->SetText(config.judge);
	CreateElement(playconfig, "scratch")->SetText(config.scratch);
	CreateElement(playconfig, "freq")->SetText(config.freq * 100);

	CreateElement(playconfig, "speed")->SetText((int)(config.speed * 100 + 0.5));
	CreateElement(playconfig, "speedtype")->SetText(config.speedtype);
	CreateElement(playconfig, "usefloat")->SetText(config.usefloatspeed);
	CreateElement(playconfig, "floatspeed")->SetText(config.floatspeed * 1000);
	CreateElement(playconfig, "usesudden")->SetText(config.uselift);
	CreateElement(playconfig, "sudden")->SetText(config.sudden * 1000);
	CreateElement(playconfig, "uselift")->SetText(config.uselift);
	CreateElement(playconfig, "lift")->SetText(config.lift * 1000);
	CreateElement(playconfig, "gaugetype")->SetText(config.gaugetype);
#endif

	CreateElement(playconfig, "ghost_type")->SetText(config.ghost_type);
	CreateElement(playconfig, "judge_type")->SetText(config.judge_type);
	CreateElement(playconfig, "pacemaker_type")->SetText(config.pacemaker_type);
	CreateElement(playconfig, "pacemaker_goal")->SetText(config.pacemaker_goal);

	CreateElement(playconfig, "judgeoffset")->SetText(config.judgeoffset);
	CreateElement(playconfig, "judgecalibration")->SetText(config.judgecalibration);

	XMLElement *e_playoption = CreateElement(doc, "PlayOption");
	e_playoption->SetText(option.GetOptionString());

	// songplayer config

	// save
	doc->SaveFile(absolute_db_path);

	// apply to metrics
	UpdateInfoMetrics();
	UpdateConfigMetrics();
	UpdateOptionMetrics();

	// end. clean-up
	delete doc;
}

void Profile::UnloadProfile() {
	playertype = PLAYERTYPE::NONE;
}

bool Profile::IsProfileLoaded() {
	return playertype != PLAYERTYPE::NONE;
}


// internal class for DB(sqlite) access
/*
* text - songhash
* text - scorehash
* int - playcount
* int - clearcount
* int - failcount
* int - clear
* int - minbp
* int - cbrk
* int - maxcombo
* int[6] - grade [pg, gr, gd, bd, pr, er]
*/
#define REC_TABLE_GENERATE(rec_table)\
rec_table.InsertColumn("songhash", DBCOLUMN::DB_COL_STRING);\
rec_table.InsertColumn("scorehash", DBCOLUMN::DB_COL_STRING);\
rec_table.InsertColumn("playcount", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("clearcount", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("failcount", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("clear", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("minbp", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("cbrk", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("maxcombo", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("pg", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("gr", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("gd", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("bd", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("pr", DBCOLUMN::DB_COL_INTEGER);\
rec_table.InsertColumn("er", DBCOLUMN::DB_COL_INTEGER);

bool Profile::LoadSongRecord(const RString& songhash, PlayRecord &rec) {
	// create & convert db path to absolute
	RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.db", name));
	FILEMANAGER->CreateDirectory(absolute_db_path);

	// start to query DB
	DBManager db;
	if (!db.OpenSQL(absolute_db_path)) {
		LOG->Warn("Failed to Open Song Record DB.");
		return false;
	}
	DBTable rec_table("record");
	REC_TABLE_GENERATE(rec_table);
	rec_table.SetColumn(0, songhash);
	if (!db.QueryRow(rec_table)) {
		// no warning, just cannot find song record.
		return false;
	}
	// copy data
	rec.playcount = rec_table.GetColumn<int>(2);
	rec.clearcount = rec_table.GetColumn<int>(3);
	rec.failcount = rec_table.GetColumn<int>(4);
	rec.clear = rec_table.GetColumn<int>(5);
	rec.minbp = rec_table.GetColumn<int>(6);
	rec.cbrk = rec_table.GetColumn<int>(7);
	rec.maxcombo = rec_table.GetColumn<int>(8);
	rec.score.score[5] = rec_table.GetColumn<int>(9);
	rec.score.score[4] = rec_table.GetColumn<int>(10);
	rec.score.score[3] = rec_table.GetColumn<int>(11);
	rec.score.score[2] = rec_table.GetColumn<int>(12);
	rec.score.score[1] = rec_table.GetColumn<int>(13);
	rec.score.score[0] = rec_table.GetColumn<int>(14);
	db.CloseSQL();

	// theme metrics
	// (TODO)
}

void Profile::SaveSongRecord(const RString& songhash, PlayRecord &rec) {
	RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.db", name));
	FILEMANAGER->CreateDirectory(absolute_db_path);
	// start to query DB
	DBManager db;
	if (!db.OpenSQL(absolute_db_path)) {
		LOG->Warn("Failed to Open Song Record DB.");
		return ;
	}
	DBTable rec_table("record");
	REC_TABLE_GENERATE(rec_table);
	rec_table.SetColumn(0, songhash);

	rec_table.SetColumn(2, rec.playcount);
	rec_table.SetColumn(3, rec.clearcount);
	rec_table.SetColumn(4, rec.failcount);
	rec_table.SetColumn(5, rec.clear);
	rec_table.SetColumn(6, rec.minbp);
	rec_table.SetColumn(7, rec.cbrk);
	rec_table.SetColumn(8, rec.maxcombo);
	rec_table.SetColumn(9, rec.score.score[5]);
	rec_table.SetColumn(10, rec.score.score[4]);
	rec_table.SetColumn(11, rec.score.score[3]);
	rec_table.SetColumn(12, rec.score.score[2]);
	rec_table.SetColumn(13, rec.score.score[1]);
	rec_table.SetColumn(14, rec.score.score[0]);

	if (!db.InsertRow(rec_table)) {
		LOG->Warn(db.GetError());
	}
	db.CloseSQL();

	// theme metrics ...?
}

void Profile::DeleteSongRecord(const RString& songhash) {
	RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.db", name));
	// start to query DB
	DBManager db;
	if (!db.OpenSQL(absolute_db_path)) {
		LOG->Warn("Failed to Open Song Record DB.");
		return;
	}
	DBTable rec_table("record");
	REC_TABLE_GENERATE(rec_table);
	rec_table.SetColumn(0, songhash);
	if (!db.DeleteRow(rec_table)) {
		LOG->Warn(db.GetError());
	}
	db.CloseSQL();
}

bool Profile::IsSongRecordExists(const RString& songhash) {
	RString absolute_db_path = FILEMANAGER->GetAbsolutePath(ssprintf("../player/%s.db", name));
	// start to query DB
	DBManager db;
	if (!db.OpenSQL(absolute_db_path)) {
		LOG->Warn("Failed to Open Song Record DB.");
		return false;
	}
	bool r = true;
	DBTable rec_table("record");
	REC_TABLE_GENERATE(rec_table);
	rec_table.SetColumn(0, songhash);
	if (!db.IsRowExists(rec_table)) {
		r = false;
	}
	db.CloseSQL();
	return r;
}





bool Profile::IsReplayDataExists(const RString& songhash) {
	RString path = ssprintf("../player/replay/%s/%s.rep", name, songhash);
	return FILEMANAGER->IsFile(path);
}

bool Profile::LoadReplayData(const RString& songhash, ReplayData &rep) {
	RString path = ssprintf("../player/replay/%s/%s.rep", name, songhash);
	RString repdata;
	if (GetFileContents(path, repdata)) {
		rep.Parse(repdata);
		return true;
	}
	else {
		return false;
	}
}

void Profile::SaveReplayData(const RString& songhash, ReplayData &rep) {
	RString path = ssprintf("../player/replay/%s/%s.rep", name, songhash);
	RString dir = get_filedir(path);
	RString repdata;
	rep.Serialize(repdata);
	FILEMANAGER->CreateDirectory(dir);
	File f;
	if (f.Open(path, "w")) {
		f.Write(repdata);
		f.Close();
	}
	else {
		LOG->Critical("Failed to write Replay data (%s)", path.c_str());
	}
}





PlayConfig::PlayConfig() {
	ghost_type = 0;
	judge_type = 0;
	pacemaker_type = PACEMAKERTYPE::PACEA;
	pacemaker_goal = 90;
	judgeoffset = 0;
	judgecalibration = 0;
}




PlayOption::PlayOption() {
	DefaultOption();
}

void PlayOption::DefaultOption() {
	randomS1 = randomS2 = 0;
	longnote = 0;			// off, legacy, 20%, 50%, 100%
	morenote = 0;			// off, -50%, -20%, 20%, 50%
	scratch = 0;			// off, assist, all_sc
	flip = 0;

	speed = 1;					// 1x
	speedtype = SPEEDTYPE::NONE;	//
	floatspeed = 1.0;			// 1 sec ...? well it'll automatically resetted.
	usefloatspeed = 0;			// false

	gaugetype = 0;				// negative: custom gaugetype
	judgetype = 0;

	showsudden = showlift = 0;
	sudden = lift = 0;

	longnote = 0;
	morenote = 0;
	scratch = 0;
}

bool PlayOption::ParseOptionString(const RString& option) {
	std::vector<RString> cmds;
	split(option, " ", cmds);
	for (auto it = cmds.begin(); it != cmds.end(); ++it) {
		int p = it->find_first_of(':');
		if (p == std::string::npos) return;
		RString cmd = RString(*it, p);
		RString val = RString((*it) + p, p+1);
		if (cmd == "RAN1" || cmd == "RANDOM") randomS1 = atoi(val);
		else if (cmd == "RAN2") randomS2 = atoi(val);
		else if (cmd == "FLIP") flip = atoi(val);
		else if (cmd == "LONGNOTE") longnote = atoi(val);
		else if (cmd == "MORENOTE") morenote = atoi(val);
		else if (cmd == "JUDGE") judgetype = atoi(val);
		else if (cmd == "SCRATCH") scratch = atoi(val);
	}
}

RString PlayOption::GetOptionString() {
	RString r;
	if (randomS1) r.append(ssprintf("RAN1:%d "), randomS1);
	if (randomS2) r.append(ssprintf("RAN2:%d "), randomS2);
	if (flip) r.append(ssprintf("FLIP:%d "), flip);
	if (longnote) r.append(ssprintf("LONGNOTE:%d "), longnote);
	if (morenote) r.append(ssprintf("MORENOTE:%d "), morenote);
	if (judgetype) r.append(ssprintf("JUDGE:%d "), judgetype);
	if (scratch) r.append(ssprintf("SCRATCH:%d "), scratch);
	return r;
}

#include "Setting.h"
bool PlayOption::IsAssisted() {
	// check SETTING (global setting)
	if (SETTING->trainingmode || SETTING->rate < 1.0 || SETTING->m_seed != -1) return true;

	// check player's own preference
	if (gaugetype == -1 || judgetype == -1)
		return true;

	return false;
}




#pragma region PLAYERSCORE
PlayScore::PlayScore() : PlayScore(0) {}
PlayScore::PlayScore(int notecnt) : totalnote(notecnt) {
	Clear();
}
void PlayScore::Clear() {
	memset(score, 0, sizeof(score));
	combo = 0;
	cbrk = 0;
	maxcombo = 0;
	fast = slow = 0;
}
int PlayScore::LastNoteFinished() const {
	return totalnote <= score[1] + score[2] + score[3] + score[4] + score[5];
}
int PlayScore::GetJudgedNote() const {
	return score[1] + score[2] + score[3] + score[4] + score[5];
}
int PlayScore::CalculateEXScore() const {
	return score[JUDGETYPE::JUDGE_PGREAT] * 2 + score[JUDGETYPE::JUDGE_GREAT];
}
double PlayScore::CurrentRate() const {
	return (double)CalculateEXScore() / GetJudgedNote() / 2;
}
int PlayScore::CalculateScore() const { return CalculateRate() * 200000; }
double PlayScore::CalculateRate() const {
	return (double)CalculateEXScore() / totalnote / 2;
}
int PlayScore::CalculateGrade() const {
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
void PlayScore::AddGrade(const int type) {
	score[type]++;
	if (type >= JUDGETYPE::JUDGE_GOOD) {
		combo++;
		if (maxcombo < combo) maxcombo = combo;
	}
	else if (type == JUDGETYPE::JUDGE_NPOOR) {
		// ignore
	} else {
		if (combo > 0) cbrk++;
		combo = 0;
	}
}
#pragma endregion





// ---- replay ----------------------------------------------------

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
	m_Cmd.clear();
	m_Songhash.clear();
}

void ReplayData::SetMetadata(const RString& cmd, const RString& songhash) {
	m_Cmd = cmd;
	m_Songhash = songhash;
}

#define MAX_REPLAY_BUFFER 1024000	// about 1000kb
void ReplayData::Serialize(RString &out) const {
	char *buf = (char*)malloc(MAX_REPLAY_BUFFER);
	// 
	//  0 ~ 32 byte: songhash
	// 40 ~    byte: play command
	// (dummy)
	// 116 ~ 120 byte: header size
	// 120 byte: header end, replay body data starts
	// (1 row per 4 * 3 = 12bytes)
	//
	memcpy(buf, m_Songhash.c_str(), m_Songhash.size());
	memcpy(buf + 40, m_Cmd.c_str(), m_Cmd.size());

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
	//
	m_Songhash = *buf;
	m_Cmd = *(buf+40);

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