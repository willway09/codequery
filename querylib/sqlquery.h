
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


#ifndef SQLQUERY_H_CQ
#define SQLQUERY_H_CQ

#include "small_lib.h"
#include <unordered_map>

// forward declaration
struct sqlite3;
struct sqlite3_stmt;

class tempstmt
{
public:
sqlite3_stmt *m_stmt;
tStr qry;
tempstmt();
~tempstmt();
void finalize(void);
sqlite3_stmt* get(void);
};

class sqlqueryresult
{
public:
	tStr symname;
	tStr symname2;
	tStr symtype;
	tStr linenum;
	unsigned int intLinenum;
	tStr filename;
	tStr filepath;
	int fileid;
	tStr linetext;
	sqlqueryresult() : linenum((const char*)"1"), fileid(-99), intLinenum(0){}
};

class sqlqueryresultlist
{
public:
enum en_resultType
	{
		sqlresultFULL = 0,
		sqlresultFILE_LINE,
		sqlresultFILE_ONLY,
		sqlresultSYM_ONLY,
		sqlresultFUNC_IN_ONE_FILE,
		sqlresultERROR
	};
	en_resultType result_type;
	std::vector<sqlqueryresult> resultlist;
	sqlqueryresultlist();
	sqlqueryresultlist(const sqlqueryresultlist& copy);
	sqlqueryresultlist& operator= (const sqlqueryresultlist& copy);
	void sort_by_name(void);
	void sort_by_linenum(void);
	tStr sqlerrmsg;
};


class sqlquery
{
public:
enum en_queryType
	{
		sqlquerySYMBOL = 0,
		sqlresultFUNC_MACRO,
		sqlresultCLASS_STRUCT,
		sqlresultINCLUDE,
		sqlresultFILESLIST,
		sqlresultCALLINGFUNC,
		sqlresultCALLEDFUNC,
		sqlresultCALLSOFFUNC,
		sqlresultMEMBERS,
		sqlresultOWNERCLASS,
		sqlresultCHILDCLASS,
		sqlresultPARENTCLASS,
		sqlresultFUNCSINFILE,
		sqlresultGREP,
		sqlresultAUTOCOMPLETE,
		sqlresultFILEPATH,
		sqlresultDEFAULT = 100
	};
enum en_filereadstatus
	{
		sqlfileOK = 0,
		sqlfileOPENERROR,
		sqlfileNOTCORRECTDB,
		sqlfileINCORRECTVER,
		sqlfileUNKNOWNERROR
	};	
	sqlquery();
	~sqlquery();
	bool isDBOpen(void) {return (m_db != NULL);}
	en_filereadstatus open_dbfile(tStr dbfn);
	void close_dbfile(void);
	sqlqueryresultlist search(tStr searchstr,
				en_queryType querytype = sqlquerySYMBOL,
				bool exactmatch=false,
				tStr filterstr = "");
	sqlqueryresultlist search_funclist_filename(const char* searchstr);
	sqlqueryresultlist search_funclist_fileid(int& fileid);
	tVecStr search_autocomplete(const char* searchstr);
	sqlqueryresultlist search_declaration(const char* searchstr);
	bool search_funcgraph(tStr searchstr, bool exactmatch, tVecStr& xmlout, tVecStr& dotout, int levels = 1, tStr* errstr = nullptr);
	bool search_classinheritgraph(tStr searchstr, bool exactmatch, tVecStr& xmlout, tVecStr& dotout, tStr* errstr = nullptr);

private:
	sqlite3 *m_db;
	tStr m_basepath;
	tempstmt m_declarationstmt;
	tempstmt m_autocompstmt;
	tempstmt m_funclistfilenamestmt;
	tempstmt m_funclistfileidstmt;
	tempstmt m_searchstmt;
	sqlqueryresultlist search_full(sqlite3_stmt* stmt);
	sqlqueryresultlist search_file_line(sqlite3_stmt* stmt);
	sqlqueryresultlist search_file_only(sqlite3_stmt* stmt);
	sqlqueryresultlist search_filepath_only(sqlite3_stmt* stmt);
	sqlqueryresultlist search_func_in_one_file(sqlite3_stmt* stmt);
	tStr read_configtbl(const char *key, sqlite3_stmt *stmt);
	tStr process_searchterm(const char* searchterm, const bool& exactmatch);
	tStr process_searchterm_autocomplete(const char* searchstr);
	void unique_symnames(sqlqueryresultlist& res);
	void remove_symname(sqlqueryresultlist& res, tStr name);
	bool search_funcgraph_recursive(const tStr& searchstr, sqlquery::en_queryType querytype, bool exactmatch, int depth, int levels,
		unsigned int& nodenum, unsigned int predecessor_nodenum, std::unordered_map<tStr, unsigned int>& visited_nodes, tStr& xmltext, tStr& dottext, tStr* errstr);
	void search_funcgraph_drawnode(unsigned int nodenum, const tStr& searchstr, tStr& xmltext, tStr& dottext, bool highlight = false);
	void search_funcgraph_drawedge(unsigned int nodenum_a, unsigned int nodenum_b, sqlquery::en_queryType querytype, tStr& xmltext, tStr& dottext);
};

#endif

