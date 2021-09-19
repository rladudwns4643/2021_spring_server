#pragma once
#include "pch.h"
constexpr auto NAME_LEN = 11;

class DataBase
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;

	SQLINTEGER nID, nLEVEL;
	SQLLEN m_cbName = 0, cbCustID = 0, cbLEVEL = 0;
	SQLLEN m_xPos, m_yPos;
	SQLLEN m_exp, m_level, m_hp;
public:
	SQLWCHAR szName[NAME_LEN];
	void Init();
	void ShowCharacterList();
	DataBase();
	~DataBase();
	void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
	void show_error();
	void FindID(const std::string& s, short& x, short& y, short& hp, int& level, int& exp);
	void UpdateDB(const std::string& id, short x, short y, short lv, short hp, int exp);
};