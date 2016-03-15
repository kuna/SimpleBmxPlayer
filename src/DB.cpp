#include "DB.h"
#include "Logger.h"
#include "global.h"



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
#define GETFLOAT(col, v)\
	CHECKTYPE(col, SQLITE_FLOAT), v = sqlite3_column_double(stmt, col)



DBTable::DBTable(const RString& tname) {
	primary_col = 0;
	tablename = tname;
}

void DBTable::InsertColumn(const RString& colname, int type) {
	DBColumn col;
	col.colname = colname;
	col.type = type;
	cols.push_back(col);
}

void DBTable::SetColumn(int colidx, const RString& v) {
	cols[colidx].val_str = v;
}

void DBTable::SetColumn(int colidx, int v) {
	char buf[128];
	itoa(v, buf, 10);
	SetColumn(colidx, buf);
}

void DBTable::SetColumn(int colidx, double v) {
	char buf[128];
	sprintf(buf, "%.3f", v);
	SetColumn(colidx, buf);
}

void DBTable::SetPrimaryColumn(int colidx) {
	primary_col = colidx;
}

RString DBTable::GetTableCreateSQL() const {
	RString v =
		"CREATE TABLE `" +  tablename + "` (";

	for (auto it = cols.begin(); it != cols.end(); ++it) {
		RString v2 = "`" + it->colname + "` ";
		if (it->type == DBCOLUMN::DB_COL_INTEGER)
			v2 += "INT";
		else if (it->type == DBCOLUMN::DB_COL_DOUBLE)
			v2 += "REAL";
		else if (it->type == DBCOLUMN::DB_COL_STRING)
			v2 += "TEXT";
		v += v2 + ",";
	}
	v += "PRIMARY KEY(" + cols[primary_col].colname + ")";
	/*
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
		");";
		*/

	v += ");";
	return v;
}

RString DBTable::GetInsertSQL() const {
	RString v = "INSERT OR REPLACE INTO record VALUES(";

	for (auto it = cols.begin(); it != cols.end(); ++it) {
		v += it->val_str + ", ";
	}

	v += ");";
	return v;
}

RString DBTable::GetQuerySQL() const {
	return "SELECT * FROM record WHERE " + cols[primary_col].colname + "=" + cols[primary_col].val_str;
}

RString DBTable::GetDeleteSQL() const {
	return "DELETE FROM record WHERE " + cols[primary_col].colname + "=" + cols[primary_col].val_str;
}

RString DBTable::GetTableExistsSQL() const {
	return "SELECT name FROM sqlite_master WHERE type='table' and name='" + tablename + "';";
}

RString DBTable::GetRowExistsSQL() const {
	return GetQuerySQL();
}

template<>
RString DBTable::GetColumn(int colidx) const {
	return cols[colidx].val_str;
}

template<>
int DBTable::GetColumn(int colidx) const {
	return atoi(cols[colidx].val_str);
}

template<>
double DBTable::GetColumn(int colidx) const {
	return atof(cols[colidx].val_str);
}





bool DBManager::OpenSQL(const RString& path) {
	CloseSQL();

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

bool DBManager::CloseSQL() {
	if (sql == 0) return true;
	bool r = (sqlite3_close(sql) == SQLITE_OK);
	sql = 0;
	return r;
}

bool DBManager::Commit() {
	RUNQUERY(QUERY_DB_COMMIT);
	CHECKQUERY(SQLITE_DONE);
	FINISHQUERY();
	return r;
}

RString DBManager::GetError() {
	return sqlite3_errmsg(sql);
}

bool DBManager::RunQuery(const RString& q, int type) {
	if (sql == 0) return false;

	RUNQUERY(q);
	CHECKQUERY(type);
	if (r) r = Commit();
	FINISHQUERY();
	return r;
}

bool DBManager::IsTableExists(const DBTable& table) {
	return RunQuery(table.GetTableExistsSQL(), SQLITE_ROW);
}

bool DBManager::CreateTable(const DBTable& table) {
	return RunQuery(table.GetTableCreateSQL());
}

bool DBManager::IsRowExists(const DBTable& table) {
	return RunQuery(table.GetRowExistsSQL(), SQLITE_ROW);
}

bool DBManager::InsertRow(const DBTable& table) {
	return RunQuery(table.GetDeleteSQL());
}

bool DBManager::QueryRow(DBTable& table) {
	RString q = table.GetQuerySQL();
	RUNQUERY(q);
	// only get one query
	if (r = (sqlite3_step(stmt) == SQLITE_ROW)) {
		int i = 0;
		for (auto it = table.ColBegin(); it != table.ColEnd(); ++it) {
			if (it->type == DBCOLUMN::DB_COL_STRING) {
				GETTEXT(i, it->val_str);
			}
			else if (it->type == DBCOLUMN::DB_COL_INTEGER) {
				int tmp;
				char tbuf[128];
				GETINT(i, tmp);
				itoa(tmp, tbuf, 10);
				it->val_str = tbuf;
			}
			else if (it->type == DBCOLUMN::DB_COL_DOUBLE) {
				double tmp;
				char tbuf[128];
				GETFLOAT(i, tmp);
				sprintf(tbuf, "%.3f", tmp);
				it->val_str = tbuf;
			}
			else {
				// well, just parse as text
				LOG->Warn("DBManager - Attr `%s` is unknown type", it->colname.c_str());
				GETTEXT(i, it->val_str);
			}
			i++;
		}
		r = true;
	}
	FINISHQUERY();
	return r;
}

bool DBManager::DeleteRow(DBTable& table) {
	return RunQuery(table.GetDeleteSQL());
}