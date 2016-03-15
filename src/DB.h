#pragma once
#include "sqlite3.h"
#include "global.h"
#include <vector>

/*
 * a very simple sqlite manager for game
 */

enum DBCOLUMN {
	DB_COL_STRING = 1,
	DB_COL_INTEGER = 2,
	DB_COL_DOUBLE = 3,
};

class DBTable {
	struct DBColumn {
		RString colname;
		int type;
		RString val_str;
		//int val_int;
		//double val_d;
	};
	RString tablename;
	std::vector<DBColumn> cols;
	int primary_col;
	
public:
	DBTable(const RString& tablename);
	void InsertColumn(const RString& colname, int type);
	void SetColumn(int colidx, const RString& v);
	void SetColumn(int colidx, int v);
	void SetColumn(int colidx, double v);
	void SetPrimaryColumn(int colidx);

	typedef std::vector<DBColumn>::iterator Iterator;
	Iterator ColBegin() { return cols.begin(); };
	Iterator ColEnd() { return cols.end(); };

	RString GetTableExistsSQL() const;
	RString GetTableCreateSQL() const;
	RString GetRowExistsSQL() const;
	RString GetInsertSQL() const;
	RString GetQuerySQL() const;
	RString GetDeleteSQL() const;
	template <typename T> T GetColumn(int colidx) const;
};

class DBManager {
protected:
	sqlite3 *sql = 0;
public:
	bool OpenSQL(const RString& path);
	bool CloseSQL();

	RString GetError();
	bool RunQuery(const RString& sql, int type = SQLITE_DONE);
	bool Commit();
	bool IsTableExists(const DBTable& table);
	bool CreateTable(const DBTable& table);
	bool IsRowExists(const DBTable& table);
	bool InsertRow(const DBTable& table);
	bool QueryRow(DBTable& table);			// search by primary col
	bool DeleteRow(DBTable& table);
};
