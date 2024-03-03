
// Library to query CodeQuery database files
//
// This library is MIT-licensed, so that it may be used to create plugins
// for editors, IDEs and other software without license restrictions
//
// This license applies only to this file:
//
// Copyright (c) 2011 ruben2020 https://github.com/ruben2020/
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <algorithm>
#include <sqlite3.h>
#include "small_lib.h"
#include "sqlquery.h"


#define SQL_SYM "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\") INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_FUNC_MACRO "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\") AND (symtbl.symType=\"$\" OR symtbl.symType=\"#\") INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_CLASS_STRUCT "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\") AND (symtbl.symType=\"c\" OR symtbl.symType=\"s\") INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_CALLINGFUNC "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT callerID FROM calltbl WHERE calledID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\")) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_CALLEDFUNC "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT calledID FROM calltbl WHERE callerID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\")) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_CALLS_OF_FUNC "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\") AND (symtbl.symType=\"`\") INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_MEMBERS "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT memberID FROM membertbl WHERE groupID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\")) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_OWNERCLASS "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT groupID FROM membertbl WHERE memberID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\")) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_PARENTCLASS "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT parentID FROM inherittbl WHERE childID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\")) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_CHILDCLASS "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT childID FROM inherittbl WHERE parentID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\")) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_INCLUDE "SELECT filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName LIKE ? ESCAPE \";\" AND symType=\"~\") INNER JOIN filestbl ON linestbl.fileID=filestbl.fileID;"
#define SQL_FILEPATH "SELECT DISTINCT filePath, fileID FROM filestbl WHERE filePath LIKE ? ESCAPE \";\";"
#define SQL_FILESLIST "SELECT DISTINCT filePath, fileID FROM filestbl WHERE filePath LIKE ? ESCAPE \";\" ORDER BY filePath ASC;"
#define SQL_AUTOCOMPLETE "SELECT DISTINCT symName FROM symtbl WHERE symName LIKE ? ORDER BY symName LIMIT 20;"
#define SQL_FUNCSINFILE "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE (symtbl.symType=\"$\" OR symtbl.symType=\"#\")) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"

#define SQL_FUNCSINONEFILE "SELECT symtbl.symName,filestbl.filePath,linestbl.linenum FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE (symtbl.symType=\"$\")) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"

#define SQL_FUNCSINONEFILE2 "SELECT symtbl.symName,filestbl.filePath,linestbl.linenum FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE (symtbl.symType=\"$\")) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND linestbl.fileID=?);"

#define SQL_EM_SYM "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName=?) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_FUNC_MACRO "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName=?) AND (symtbl.symType=\"$\" OR symtbl.symType=\"#\") INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_CLASS_STRUCT "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName=?) AND (symtbl.symType=\"c\" OR symtbl.symType=\"s\") INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_CALLINGFUNC "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT callerID FROM calltbl WHERE calledID IN (SELECT symID FROM symtbl WHERE symName=?)) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_CALLEDFUNC "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT calledID FROM calltbl WHERE callerID IN (SELECT symID FROM symtbl WHERE symName=?)) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_CALLS_OF_FUNC "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName=?) AND (symtbl.symType=\"`\") INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_MEMBERS "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT memberID FROM membertbl WHERE groupID IN (SELECT symID FROM symtbl WHERE symName=?)) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_OWNERCLASS "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT groupID FROM membertbl WHERE memberID IN (SELECT symID FROM symtbl WHERE symName=?)) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_PARENTCLASS "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT parentID FROM inherittbl WHERE childID IN (SELECT symID FROM symtbl WHERE symName=?)) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_CHILDCLASS "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT childID FROM inherittbl WHERE parentID IN (SELECT symID FROM symtbl WHERE symName=?)) INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID AND filestbl.filePath LIKE ? ESCAPE \";\");"
#define SQL_EM_INCLUDE "SELECT filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.lineID=linestbl.lineID AND symtbl.symID IN (SELECT symID FROM symtbl WHERE symName=? AND symType=\"~\") INNER JOIN filestbl ON linestbl.fileID=filestbl.fileID;"
#define SQL_EM_FILEPATH "SELECT DISTINCT filePath, fileid FROM filestbl WHERE filePath=?;"
#define SQL_EM_FILESLIST "SELECT DISTINCT filePath, fileid FROM filestbl WHERE filePath=? ORDER BY filePath ASC;"
#define SQL_DECLARATION "SELECT symtbl.symName,symtbl.symType,filestbl.filePath,linestbl.linenum,linestbl.linetext,linestbl.fileID FROM symtbl INNER JOIN linestbl ON symtbl.symID IN (SELECT symID FROM symtbl WHERE symName=?) AND (symtbl.symType=\"$\" OR symtbl.symType=\"#\" OR symtbl.symType=\"c\" OR symtbl.symType=\"s\") AND symtbl.lineID=linestbl.lineID INNER JOIN filestbl ON (linestbl.fileID=filestbl.fileID) LIMIT 1;"

struct nameasc
{
    bool operator()( const sqlqueryresult& lx, const sqlqueryresult& rx ) const {
    	return lx.symname2.compare(rx.symname2) < 0;
    }
};

struct numasc
{
    bool operator()( const sqlqueryresult& lx, const sqlqueryresult& rx ) const {
    	return lx.intLinenum < rx.intLinenum;
    }
};


tempstmt::tempstmt()
:m_stmt(NULL)
{
}

tempstmt::~tempstmt()
{
	finalize();
}

void tempstmt::finalize(void)
{
	sqlite3_finalize(m_stmt);
	m_stmt = NULL;
	qry.clear();
}

sqlite3_stmt* tempstmt::get(void)
{
	return m_stmt;
}

sqlqueryresultlist::sqlqueryresultlist()
:result_type(sqlresultERROR)
,sqlerrmsg("empty")
{
}

sqlqueryresultlist::sqlqueryresultlist(const sqlqueryresultlist& copy)
{
	result_type = copy.result_type;
	sqlerrmsg = copy.sqlerrmsg;
	resultlist = copy.resultlist;
}

sqlqueryresultlist& sqlqueryresultlist::operator= (const sqlqueryresultlist& copy)
{
	if (&copy != this)
	{
		result_type = copy.result_type;
		sqlerrmsg = copy.sqlerrmsg;
		resultlist.clear();
		resultlist = copy.resultlist;
	}
	return *this;
}

void sqlqueryresultlist::sort_by_name(void)
{
	std::sort(resultlist.begin(), resultlist.end(), nameasc());
}

void sqlqueryresultlist::sort_by_linenum(void)
{
	std::sort(resultlist.begin(), resultlist.end(), numasc());
}

sqlquery::sqlquery()
:m_db(NULL)
{
}

sqlquery::~sqlquery()
{
	close_dbfile();
}


sqlquery::en_filereadstatus sqlquery::open_dbfile(tStr dbfn)
{
	if (dbfn.STRISEMPTY()) return sqlfileOPENERROR;

	smartFILE fp;
	// Does the file exist?
	if (check_fileExists(dbfn.C_STR()) == false) {return sqlfileOPENERROR;}
	// Try to open the file for reading
	fp = fopen(dbfn.C_STR(), "r");
	if (fp == NULL) {return sqlfileOPENERROR;}
	fp.close_file();

	int rc = sqlite3_open_v2(dbfn.C_STR(),
						&m_db, SQLITE_OPEN_READONLY, NULL);
	if ((rc != SQLITE_OK)||(m_db == NULL)) 
	{
		close_dbfile();
		return sqlfileOPENERROR;
	}
	tempstmt stmt;

	sqlite3_exec(m_db, /*"PRAGMA synchronous = OFF;"
		"PRAGMA journal_mode = OFF;"
		"PRAGMA locking_mode = EXCLUSIVE;"
		"PRAGMA automatic_index = FALSE;"*/
		"PRAGMA cache_size = 20000;", NULL, 0, NULL);

	tStr majorver = read_configtbl("DB_MAJOR_VER", stmt.get());
	tStr minorver = read_configtbl("DB_MINOR_VER", stmt.get());
	if ((majorver.STRISEMPTY())||(minorver.STRISEMPTY()))
		{return sqlfileNOTCORRECTDB;}
	if (majorver.compare(tStr("0")) != 0) return sqlfileINCORRECTVER;
	if (minorver.compare(tStr("1")) != 0) return sqlfileINCORRECTVER;
	m_basepath = read_configtbl("DB_BASE_PATH", stmt.get());
	if (m_basepath.STRISEMPTY()) {return sqlfileNOTCORRECTDB;}
	rc = sqlite3_prepare_v2(m_db, SQL_AUTOCOMPLETE, strlen(SQL_AUTOCOMPLETE),
							&(m_autocompstmt.m_stmt), NULL);
	rc = sqlite3_prepare_v2(m_db, SQL_FUNCSINONEFILE, strlen(SQL_FUNCSINONEFILE),
							&(m_funclistfilenamestmt.m_stmt), NULL);
	rc = sqlite3_prepare_v2(m_db, SQL_FUNCSINONEFILE2, strlen(SQL_FUNCSINONEFILE2),
							&(m_funclistfileidstmt.m_stmt), NULL);
	rc = sqlite3_prepare_v2(m_db, SQL_DECLARATION, strlen(SQL_DECLARATION),
							&(m_declarationstmt.m_stmt), NULL);
	if (rc != SQLITE_OK) {return sqlfileNOTCORRECTDB;}
	return sqlfileOK;
}

void sqlquery::close_dbfile(void)
{
	m_declarationstmt.finalize();
	m_autocompstmt.finalize();
	m_funclistfilenamestmt.finalize();
	m_funclistfileidstmt.finalize();
	m_searchstmt.finalize();
	sqlite3_close(m_db);
	m_db = NULL;
	m_basepath.clear();
}

tStr sqlquery::read_configtbl(const char *key, sqlite3_stmt *stmt)
{
	tStr result = "";
	if ((key == NULL)||(strlen(key) == 0)||(m_db == NULL)) return result;
	int rc;
	if (stmt == NULL)
	{
		rc = sqlite3_prepare_v2(m_db, "SELECT configVal FROM configtbl WHERE configKey=?;",
							strlen("SELECT configVal FROM configtbl WHERE configKey=?;"),
							&stmt, NULL);
		if (rc != SQLITE_OK) {return result;}
	}
	else sqlite3_reset(stmt);
	rc = sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_STATIC);
	if (rc != SQLITE_OK) {return result;}
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW) {return result;}
	result = (const char*) sqlite3_column_text(stmt, 0);
	return result;
}

sqlqueryresultlist sqlquery::search_funclist_filename(const char* searchstr)
{
	//printf("search_funclist_filename %s\n", searchstr);
	sqlqueryresultlist result;
	if (searchstr == NULL) return result;
	result.result_type = sqlqueryresultlist::sqlresultERROR;
	tStr srchterm("%");
	srchterm.append(searchstr);
	if ((searchstr == NULL)||(strlen(searchstr) < 1)||(m_db == NULL)) return result;
	sqlite3_reset(m_funclistfilenamestmt.get());
	int rc = sqlite3_bind_text(m_funclistfilenamestmt.get(), 1, srchterm.C_STR(), srchterm.size(), SQLITE_STATIC);
	if (rc != SQLITE_OK) {printf("Err: %s\n", sqlite3_errmsg(m_db)); return result;}
	result = search_func_in_one_file(m_funclistfilenamestmt.get());
	return result;
}

sqlqueryresultlist sqlquery::search_funclist_fileid(int& fileid)
{
	//printf("search_funclist_fileid %d\n", fileid);
	sqlqueryresultlist result;
	result.result_type = sqlqueryresultlist::sqlresultERROR;
	if ((fileid < 0)||(m_db == NULL)) return result;
	sqlite3_reset(m_funclistfileidstmt.get());
	int rc = sqlite3_bind_int(m_funclistfileidstmt.get(), 1, fileid);
	if (rc != SQLITE_OK) {printf("Err: %s\n", sqlite3_errmsg(m_db)); return result;}
	result = search_func_in_one_file(m_funclistfileidstmt.get());
	return result;
}

tVecStr sqlquery::search_autocomplete(const char* searchstr)
{
	tVecStr result;
	//int ctr = 0;
	if ((searchstr == NULL)||(strlen(searchstr) < 1)||(m_db == NULL)) return result;
	tStr srchterm = process_searchterm_autocomplete(searchstr);
	sqlite3_reset(m_autocompstmt.get());
	int rc = sqlite3_bind_text(m_autocompstmt.get(), 1, srchterm.C_STR(), srchterm.size(), SQLITE_STATIC);
	if (rc != SQLITE_OK) {printf("Err: %s\n", sqlite3_errmsg(m_db)); return result;}
	do
	{
		rc = sqlite3_step(m_autocompstmt.get());
		if (rc == SQLITE_ROW)
		{
			result.push_back(tStr((const char*) sqlite3_column_text(m_autocompstmt.get(), 0)));
			//if (ctr++ > 300) rc = SQLITE_DONE;
		}
	} while (rc == SQLITE_ROW);
	if (rc != SQLITE_DONE)
	{
		printf("Err: %s\n", sqlite3_errmsg(m_db));
	}
	return result;
}

sqlqueryresultlist sqlquery::search(
						tStr searchstr,
						sqlquery::en_queryType querytype,
						bool exactmatch,
						tStr filterstr)
{
	sqlqueryresultlist result;
	int rc;
	bool twoTerms = true;
	result.result_type = sqlqueryresultlist::sqlresultERROR;
	if ((m_db == NULL)||(searchstr.STRISEMPTY())||(m_basepath.STRISEMPTY())) return result;
	tStr sqlqry, srchterm, filterterm;
	sqlqueryresultlist::en_resultType resultType = sqlqueryresultlist::sqlresultFULL;
	if (exactmatch && (querytype == sqlresultFUNCSINFILE)) {searchstr.insert(0, "%");}
	srchterm = process_searchterm(searchstr.C_STR(), exactmatch);
	if (filterstr.STRISEMPTY()) {filterterm = "%";}
	else {filterterm = process_searchterm(filterstr.C_STR(), false);}
	switch (querytype)
	{
		case sqlquerySYMBOL:
			sqlqry = exactmatch ? SQL_EM_SYM : SQL_SYM;
			break;
		case sqlresultFUNC_MACRO:
			sqlqry = exactmatch ? SQL_EM_FUNC_MACRO : SQL_FUNC_MACRO;
			break;
		case sqlresultCLASS_STRUCT:
			sqlqry = exactmatch ? SQL_EM_CLASS_STRUCT : SQL_CLASS_STRUCT;
			break;
		case sqlresultINCLUDE:
			sqlqry = exactmatch ? SQL_EM_INCLUDE : SQL_INCLUDE;
			resultType = sqlqueryresultlist::sqlresultFILE_LINE;
			twoTerms = false;
			break;
		case sqlresultFILEPATH:
			sqlqry = exactmatch ? SQL_EM_FILEPATH : SQL_FILEPATH;
			resultType = sqlqueryresultlist::sqlresultFILE_ONLY;
			twoTerms = false;
			break;
		case sqlresultFILESLIST:
			sqlqry = exactmatch ? SQL_EM_FILESLIST : SQL_FILESLIST;
			resultType = sqlqueryresultlist::sqlresultFILE_ONLY;
			twoTerms = false;
			break;
		case sqlresultCALLINGFUNC:
			sqlqry = exactmatch ? SQL_EM_CALLINGFUNC : SQL_CALLINGFUNC;
			break;
		case sqlresultCALLEDFUNC:
			sqlqry = exactmatch ? SQL_EM_CALLEDFUNC : SQL_CALLEDFUNC;
			break;
		case sqlresultCALLSOFFUNC:
			sqlqry = exactmatch ? SQL_EM_CALLS_OF_FUNC : SQL_CALLS_OF_FUNC;
			break;
		case sqlresultMEMBERS:
			sqlqry = exactmatch ? SQL_EM_MEMBERS : SQL_MEMBERS;
			break;
		case sqlresultOWNERCLASS:
			sqlqry = exactmatch ? SQL_EM_OWNERCLASS : SQL_OWNERCLASS;
			break;
		case sqlresultPARENTCLASS:
			sqlqry = exactmatch ? SQL_EM_PARENTCLASS : SQL_PARENTCLASS;
			break;
		case sqlresultCHILDCLASS:
			sqlqry = exactmatch ? SQL_EM_CHILDCLASS : SQL_CHILDCLASS;
			break;
		case sqlresultFUNCSINFILE:
			sqlqry = SQL_FUNCSINFILE;
			twoTerms = false;
			break;
		case sqlresultAUTOCOMPLETE:
			resultType = sqlqueryresultlist::sqlresultSYM_ONLY;
			twoTerms = false;
			break;
		default:
			result.sqlerrmsg = "Unknown search type";
			return result;
	}

	if (m_searchstmt.qry.compare(sqlqry) != 0)
	{
		sqlite3_finalize(m_searchstmt.get());
		rc = sqlite3_prepare_v2(m_db, sqlqry.C_STR(),
					sqlqry.size(),
					&(m_searchstmt.m_stmt), NULL);
		m_searchstmt.qry = (rc == SQLITE_OK) ? sqlqry : "";
	}
	else
	{
		rc = sqlite3_reset(m_searchstmt.get());
	}
	if (rc != SQLITE_OK) {result.sqlerrmsg = sqlite3_errmsg(m_db); return result;}
	rc = sqlite3_bind_text(m_searchstmt.get(), 1, srchterm.C_STR(), srchterm.size(), SQLITE_TRANSIENT);
	if (rc != SQLITE_OK) {result.sqlerrmsg = sqlite3_errmsg(m_db); return result;}
	if (twoTerms)
	{
		rc = sqlite3_bind_text(m_searchstmt.get(), 2, filterterm.C_STR(), filterterm.size(), SQLITE_TRANSIENT);
		if (rc != SQLITE_OK) {result.sqlerrmsg = sqlite3_errmsg(m_db); return result;}
	}
	if (resultType == sqlqueryresultlist::sqlresultFULL) result = search_full(m_searchstmt.get());
	else if (resultType == sqlqueryresultlist::sqlresultFILE_LINE) result = search_file_line(m_searchstmt.get());
	else if (resultType == sqlqueryresultlist::sqlresultFILE_ONLY)
	{
		if (querytype == sqlresultFILESLIST)
			result = search_filepath_only(m_searchstmt.get());
		else
			result = search_file_only(m_searchstmt.get());
	}
	return result;
}	

tStr sqlquery::process_searchterm(const char* searchterm, const bool& exactmatch)
{
	std::string srchterm, srchterm2;
	if (!exactmatch)
	{
		srchterm2 = add_escape_char(searchterm,        '%', ';').c_str();
		srchterm2 = add_escape_char( srchterm2.c_str(), '_', ';').c_str();
		srchterm = "%";
		srchterm += srchterm2;
		srchterm += "%";
		replacechar( srchterm.begin(), srchterm.end(), '*', '%');
		replacechar( srchterm.begin(), srchterm.end(), '?', '_');
	}
	else srchterm = searchterm;
#ifdef QT_VERSION
	return  QString::fromStdString(srchterm);
#else
	return srchterm;
#endif
}

tStr sqlquery::process_searchterm_autocomplete(const char* searchterm)
{
	std::string srchterm(searchterm);
	srchterm += "%";
	replacechar( srchterm.begin(), srchterm.end(), '*', '%');
	replacechar( srchterm.begin(), srchterm.end(), '?', '_');
#ifdef QT_VERSION
	return  QString::fromStdString(srchterm);
#else
	return srchterm;
#endif
}

sqlqueryresultlist sqlquery::search_declaration(const char* searchstr)
{
	int rc;
	sqlqueryresultlist result;
	tStr fp;
	sqlite3_stmt* stmt = m_declarationstmt.get();
	if ((searchstr == NULL)||(strlen(searchstr) < 1)||(m_db == NULL)) return result;
	result.result_type = sqlqueryresultlist::sqlresultERROR;
	sqlqueryresult item;
	sqlite3_reset(stmt);
	rc = sqlite3_bind_text(stmt, 1, searchstr, strlen(searchstr), SQLITE_STATIC);
	if (rc != SQLITE_OK) {printf("Err: %s\n", sqlite3_errmsg(m_db)); return result;}
	do
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			item.symname  = (const char*) sqlite3_column_text(stmt, 0);
			item.symtype  = (const char*) sqlite3_column_text(stmt, 1);
			fp            = (const char*) sqlite3_column_text(stmt, 2);
			item.linenum  = (const char*) sqlite3_column_text(stmt, 3);
			item.linetext = (const char*) sqlite3_column_text(stmt, 4);
			item.filename = extract_filename(fp.C_STR());
			if (isAbsolutePath(fp) == false)
			{
				item.filepath = m_basepath;
				item.filepath += DIRSEP;
				item.filepath += fp;
			}
			else item.filepath = fp;
			result.resultlist.push_back(item);
		}
	} while (rc == SQLITE_ROW);
	if (rc != SQLITE_DONE)
	{
		result.result_type = sqlqueryresultlist::sqlresultERROR;
		result.sqlerrmsg = sqlite3_errmsg(m_db);
	}
	else
	{
		result.result_type = sqlqueryresultlist::sqlresultFULL;
	}
	return result;
}


sqlqueryresultlist sqlquery::search_full(sqlite3_stmt* stmt)
{
	int rc;
	sqlqueryresultlist result;
	tStr fp;
	result.result_type = sqlqueryresultlist::sqlresultERROR;
	sqlqueryresult item;
	do
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			item.symname  = (const char*) sqlite3_column_text(stmt, 0);
			item.symtype  = (const char*) sqlite3_column_text(stmt, 1);
			fp            = (const char*) sqlite3_column_text(stmt, 2);
			item.linenum  = (const char*) sqlite3_column_text(stmt, 3);
			item.linetext = (const char*) sqlite3_column_text(stmt, 4);
			item.fileid   =               sqlite3_column_int (stmt, 5);
			item.filename = extract_filename(fp.C_STR());
			if (isAbsolutePath(fp) == false)
			{
				item.filepath = m_basepath;
				item.filepath += DIRSEP;
				item.filepath += fp;
			}
			else item.filepath = fp;
			result.resultlist.push_back(item);
		}
	} while (rc == SQLITE_ROW);
	if (rc != SQLITE_DONE)
	{
		result.result_type = sqlqueryresultlist::sqlresultERROR;
		result.sqlerrmsg = sqlite3_errmsg(m_db);
	}
	else
	{
		result.result_type = sqlqueryresultlist::sqlresultFULL;
	}
	return result;
}

sqlqueryresultlist sqlquery::search_func_in_one_file(sqlite3_stmt* stmt)
{
	int rc;
	sqlqueryresultlist result;
	tStr fp;
	result.result_type = sqlqueryresultlist::sqlresultERROR;
	sqlqueryresult item;
	do
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			item.symname  = (const char*) sqlite3_column_text(stmt, 0);
			fp            = (const char*) sqlite3_column_text(stmt, 1);
			item.linenum  = (const char*) sqlite3_column_text(stmt, 2);
			item.filename = extract_filename(fp.C_STR());
			item.intLinenum = atoi(item.linenum.C_STR());
			STRTOLOWER(item.symname2, item.symname);
			if (isAbsolutePath(fp) == false)
			{
				item.filepath = m_basepath;
				item.filepath += DIRSEP;
				item.filepath += fp;
			}
			else item.filepath = fp;
			result.resultlist.push_back(item);
		}
	} while (rc == SQLITE_ROW);
	if (rc != SQLITE_DONE)
	{
		result.result_type = sqlqueryresultlist::sqlresultERROR;
		result.sqlerrmsg = sqlite3_errmsg(m_db);
	}
	else
	{
		result.result_type = sqlqueryresultlist::sqlresultFUNC_IN_ONE_FILE;
	}
	return result;
}

sqlqueryresultlist sqlquery::search_file_line(sqlite3_stmt* stmt)
{
	int rc;
	sqlqueryresultlist result;
	tStr fp;
	result.result_type = sqlqueryresultlist::sqlresultERROR;
	sqlqueryresult item;
	do
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			fp            = (const char*) sqlite3_column_text(stmt, 0);
			item.linenum  = (const char*) sqlite3_column_text(stmt, 1);
			item.linetext = (const char*) sqlite3_column_text(stmt, 2);
			item.fileid   =               sqlite3_column_int (stmt, 3);
			item.filename = extract_filename(fp.C_STR());
			if (isAbsolutePath(fp) == false)
			{
				item.filepath = m_basepath;
				item.filepath += DIRSEP;
				item.filepath += fp;
			}
			else item.filepath = fp;
			result.resultlist.push_back(item);
		}
	} while (rc == SQLITE_ROW);
	if (rc != SQLITE_DONE)
	{
		result.result_type = sqlqueryresultlist::sqlresultERROR;
		result.sqlerrmsg = sqlite3_errmsg(m_db);
	}
	else
	{
		result.result_type = sqlqueryresultlist::sqlresultFILE_LINE;
	}
	return result;
}

sqlqueryresultlist sqlquery::search_file_only(sqlite3_stmt* stmt)
{
	int rc;
	sqlqueryresultlist result;
	tStr fp;
	result.result_type = sqlqueryresultlist::sqlresultERROR;
	sqlqueryresult item;
	do
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			fp            = (const char*) sqlite3_column_text(stmt, 0);
			item.linenum  = "1";
			item.filename = extract_filename(fp.C_STR());
			item.fileid   = sqlite3_column_int (stmt, 1);
			if (isAbsolutePath(fp) == false)
			{
				item.filepath = m_basepath;
				item.filepath += DIRSEP;
				item.filepath += fp;
			}
			else item.filepath = fp;
			result.resultlist.push_back(item);
		}
	} while (rc == SQLITE_ROW);
	if (rc != SQLITE_DONE)
	{
		result.result_type = sqlqueryresultlist::sqlresultERROR;
		result.sqlerrmsg = sqlite3_errmsg(m_db);
	}
	else
	{
		result.result_type = sqlqueryresultlist::sqlresultFILE_ONLY;
	}
	return result;
}

sqlqueryresultlist sqlquery::search_filepath_only(sqlite3_stmt* stmt)
{
	int rc;
	sqlqueryresultlist result;
	tStr fp;
	result.result_type = sqlqueryresultlist::sqlresultERROR;
	sqlqueryresult item;
	do
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			fp            = (const char*) sqlite3_column_text(stmt, 0);
			item.linenum  = "1";
			item.filename = fp;
			item.fileid   = sqlite3_column_int (stmt, 1);
			if (isAbsolutePath(fp) == false)
			{
				item.filepath = m_basepath;
				item.filepath += DIRSEP;
				item.filepath += fp;
			}
			else item.filepath = fp;
			result.resultlist.push_back(item);
		}
	} while (rc == SQLITE_ROW);
	if (rc != SQLITE_DONE)
	{
		result.result_type = sqlqueryresultlist::sqlresultERROR;
		result.sqlerrmsg = sqlite3_errmsg(m_db);
	}
	else
	{
		result.result_type = sqlqueryresultlist::sqlresultFILE_ONLY;
	}
	return result;
}

// Take to and from node numbers as input, return xmltext and dottext by reference
// Also changes color to light blue if highlighted
void sqlquery::search_funcgraph_drawnode(unsigned int nodenum, const tStr& searchstr, tStr& xmltext, tStr& dottext, bool highlight) {
	xmltext += string_format(tStr("<node fill=\"#%s\" id=\"%d\" label=\"%s\"/>\n"), (highlight ? "e2ffff" : "ffffff"), nodenum, searchstr.C_STR());
	dottext += string_format(tStr("node%d [label=\"%s\" style=filled fillcolor=\"#e2ffff\" shape=\"box\" ];\n"),
		nodenum, searchstr.C_STR(), (highlight ? "e2ffff" : "ffffff"));
}

// Take two nodes and a relationship as input as input, return xmltext and dottext by reference
// Empirically determie ordering of noednum_a and nodenum_b with given querytype
void sqlquery::search_funcgraph_drawedge(unsigned int nodenum_a, unsigned int nodenum_b, sqlquery::en_queryType querytype, tStr& xmltext, tStr& dottext) {
	if(querytype == sqlresultCALLINGFUNC) {
		xmltext += string_format(tStr("<edge target=\"%d\" source=\"%d\"/>\n"), nodenum_b, nodenum_a);
		dottext += string_format(tStr("node%d -> node%d;\n"), nodenum_b, nodenum_a);
	} else if(querytype == sqlresultCALLEDFUNC) {
		xmltext += string_format(tStr("<edge target=\"%d\" source=\"%d\"/>\n"), nodenum_a, nodenum_b);
		dottext += string_format(tStr("node%d -> node%d;\n"), nodenum_a, nodenum_b);
	}
}

// Inputs are the search string, the query type (i.e. sqlresultCALLEDFUNC or sqlresultCALLINGFUNC), whether or not the match should be exact,
// the depth (i.e. how many levels have been traversed), the levels (the maximum number of levels up and down), the nodenum, the predecessor nodenum,
// a map of the visitted notes
// Outputs are the xmltext and dottext strings
// An error string is returned via pointer in errstr
bool sqlquery::search_funcgraph_recursive(const tStr& searchstr, sqlquery::en_queryType querytype, bool exactmatch, int depth, int levels,
	unsigned int& nodenum, unsigned int predecessor_nodenum, std::unordered_map<tStr, unsigned int>& visited_nodes, tStr& xmltext, tStr& dottext, tStr* errstr) {

	// If have hit the level limit, terminate
	if(depth == levels) {
		return predecessor_nodenum;
	}

	sqlqueryresultlist result = search(searchstr, querytype, exactmatch);

	if (result.result_type == sqlqueryresultlist::sqlresultERROR)
	{
		if (errstr) *errstr = result.sqlerrmsg;
		return false;
	}

	unique_symnames(result);

	for (int i = 0; i < result.resultlist.size(); i++)
	{
		const tStr& symname = result.resultlist[i].symname;
		if(visited_nodes.find(symname) != visited_nodes.end()) {
			// If have already visited node, just draw edge and exit.
			// This avoids infinite recursive looping for any cyclic graph
			unsigned int temp_nodenum = visited_nodes[symname];
			search_funcgraph_drawedge(temp_nodenum, predecessor_nodenum, querytype, xmltext, dottext);
		} else {
			// Claim the current nodenum, and increment it
			unsigned int current_nodenum = nodenum++;

			// If have not yet visited node, record the visit
			visited_nodes[symname] = current_nodenum;

			// Then, draw node and edge
			search_funcgraph_drawnode(current_nodenum, symname, xmltext, dottext);
			search_funcgraph_drawedge(current_nodenum, predecessor_nodenum, querytype, xmltext, dottext);

			// Then, visit all children recursively, and handle error
			if(!search_funcgraph_recursive(symname, querytype, exactmatch, depth + 1, levels, nodenum, current_nodenum, visited_nodes, xmltext, dottext, errstr)) {
				return false;
			}
		}
	}

	return true;
}

bool sqlquery::search_funcgraph(tStr searchstr, bool exactmatch, tVecStr& xmlout, tVecStr& dotout, int levels, tStr* errstr)
{
	tStr xmltext = "<graph>\n";
	tStr dottext = "digraph graphname {\n";

	unsigned int nodenum = 1;

	search_funcgraph_drawnode(nodenum, searchstr, xmltext, dottext, true);
	nodenum++;

	std::unordered_map<tStr, unsigned int> visited_nodes;
	visited_nodes[searchstr] = 1;
	bool success = search_funcgraph_recursive(searchstr, sqlresultCALLINGFUNC, exactmatch, 0, levels, nodenum, 1, visited_nodes, xmltext, dottext, errstr);
	success &= search_funcgraph_recursive(searchstr, sqlresultCALLEDFUNC, exactmatch, 0, levels, nodenum, 1, visited_nodes, xmltext, dottext, errstr);

	if (!success) {
		return false;
	}

	xmltext += "</graph>\n";
	dottext += "}\n";
	xmlout.push_back(xmltext);
	dotout.push_back(dottext);
	return true;
}

bool sqlquery::search_classinheritgraph(tStr searchstr, bool exactmatch, tVecStr& xmlout, tVecStr& dotout, tStr* errstr)
{

	sqlqueryresultlist result_children, result_parent1, result_cousins1, result_parent2;
	tStr xmltext = "<graph>";
	tStr dottext = "digraph graphname {\n";
	int nodenum = 1;
	int parent1 = 0;

	xmltext += string_format(tStr("<node fill=\"#e2ffff\" id=\"%d\" label=\"%s\"/>"), nodenum, searchstr.C_STR());
	dottext += string_format(tStr("node%d [label=\"%s\" style=filled fillcolor=\"#e2ffff\" shape=\"box\" ];\n"), nodenum, searchstr.C_STR());
	nodenum++;

	result_children = search(searchstr, sqlresultCHILDCLASS, exactmatch);
	if (result_children.result_type == sqlqueryresultlist::sqlresultERROR)
	{
		if (errstr) *errstr = result_children.sqlerrmsg;
		return false;
	}
	result_parent1 = search(searchstr, sqlresultPARENTCLASS, exactmatch);
	if (result_parent1.result_type == sqlqueryresultlist::sqlresultERROR)
	{
		if (errstr) *errstr = result_parent1.sqlerrmsg;
		return false;
	}
	if (result_parent1.resultlist.size() > 0)
	{
		result_parent2 = search(result_parent1.resultlist[0].symname, sqlresultPARENTCLASS, exactmatch);
		if (result_parent2.result_type == sqlqueryresultlist::sqlresultERROR)
		{
			if (errstr) *errstr = result_parent2.sqlerrmsg;
			return false;
		}
		result_cousins1 = search(result_parent1.resultlist[0].symname, sqlresultCHILDCLASS, exactmatch);
		if (result_cousins1.result_type == sqlqueryresultlist::sqlresultERROR)
		{
			if (errstr) *errstr = result_cousins1.sqlerrmsg;
			return false;
		}
	}
	unique_symnames(result_children);
	unique_symnames(result_parent1);
	unique_symnames(result_parent2);
	unique_symnames(result_cousins1);
	remove_symname(result_cousins1, searchstr); // I am not my own cousin
	for (unsigned int i=0; i < result_children.resultlist.size(); i++)
	{
		xmltext += string_format(tStr("<node fill=\"#ffffff\" id=\"%d\" label=\"%s\"/>"),
			nodenum, result_children.resultlist[i].symname.C_STR());
		xmltext += string_format(tStr("<edge target=\"1\" source=\"%d\"/>"), nodenum);
		dottext += string_format(tStr("node%d [label=\"%s\" style=filled fillcolor=\"#ffffff\" shape=\"box\" ];\n"),
			nodenum, result_children.resultlist[i].symname.C_STR());
		dottext += string_format(tStr("node%d -> node1 [arrowhead=\"empty\"];\n"), nodenum);
		nodenum++;
	}
	for (unsigned int i=0; i < result_parent1.resultlist.size(); i++)
	{
		xmltext += string_format(tStr("<node fill=\"#ffffff\" id=\"%d\" label=\"%s\"/>"),
			nodenum, result_parent1.resultlist[i].symname.C_STR());
		xmltext += string_format(tStr("<edge target=\"%d\" source=\"1\"/>"), nodenum);
		dottext += string_format(tStr("node%d [label=\"%s\" style=filled fillcolor=\"#ffffff\" shape=\"box\" ];\n"),
			nodenum, result_parent1.resultlist[i].symname.C_STR());
		dottext += string_format(tStr("node1 -> node%d [arrowhead=\"empty\"];\n"), nodenum);
		if (i == 0) parent1 = nodenum;
		nodenum++;
	}
	for (unsigned int i=0; i < result_parent2.resultlist.size(); i++)
	{
		xmltext += string_format(tStr("<node fill=\"#ffffff\" id=\"%d\" label=\"%s\"/>"),
			nodenum, result_parent2.resultlist[i].symname.C_STR());
		xmltext += string_format(tStr("<edge target=\"%d\" source=\"%d\"/>"), nodenum, parent1);
		dottext += string_format(tStr("node%d [label=\"%s\" style=filled fillcolor=\"#ffffff\" shape=\"box\" ];\n"),
			nodenum, result_parent2.resultlist[i].symname.C_STR());
		dottext += string_format(tStr("node%d -> node%d [arrowhead=\"empty\"];\n"), parent1, nodenum);
		nodenum++;
	}
	for (unsigned int i=0; i < result_cousins1.resultlist.size(); i++)
	{
		xmltext += string_format(tStr("<node fill=\"#ffffff\" id=\"%d\" label=\"%s\"/>"),
			nodenum, result_cousins1.resultlist[i].symname.C_STR());
		xmltext += string_format(tStr("<edge target=\"%d\" source=\"%d\"/>"), parent1, nodenum);
		dottext += string_format(tStr("node%d [label=\"%s\" style=filled fillcolor=\"#ffffff\" shape=\"box\" ];\n"),
			nodenum, result_cousins1.resultlist[i].symname.C_STR());
		dottext += string_format(tStr("node%d -> node%d [arrowhead=\"empty\"];\n"), nodenum, parent1);
		nodenum++;
	}
	xmltext += "</graph>";
	dottext += "}\n";
	xmlout.push_back(xmltext);
	dotout.push_back(dottext);
	return true;
}

// make the list of symnames unique, no elements repeated
void sqlquery::unique_symnames(sqlqueryresultlist& res)
{
	tSetStr setstr;
	sqlqueryresultlist out;
	sqlqueryresult item;
	for(unsigned int i=0; i < res.resultlist.size(); i++)
	{
		setstr.insert(res.resultlist[i].symname);
	}
	for (auto it = setstr.begin(); it != setstr.end(); it++)
	{
		item.symname = *it;
		out.resultlist.push_back(item);
	}
	res = out;
}

// remove a symname from the list
void sqlquery::remove_symname(sqlqueryresultlist& res, tStr name)
{
	for (auto it = res.resultlist.begin(); it != res.resultlist.end(); it++)
	{
		if (it->symname.compare(name) == 0)
		{
			res.resultlist.erase(it);
			break;
		}
	}
}


