#include "DataBase.h"
void DataBase::Init()
{
	setlocale(LC_ALL, "korean");

	// Allocate environment handle  
	//SQL_HANDLE_ENV �� ��� SQL_NULL_HANDLE�Դϴ�.
	//HandleType �� SQL_HANDLE_DBC �� ����̴� ȯ�� �ڵ� �̾�� �� �� 
	//SQL_HANDLE_STMT �Ǵ� SQL_HANDLE_DESC �� ��� ���� �ڵ� �̾�� �մϴ�
	// sqlalloc�� ���� ��������ߵȤ���. �̴ֿ��� null������ ������ Ŭ�������ִ� ������� �־��ֱ�. 
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2016180012", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
				{
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
				}
			}
		}
	}


}
void DataBase::ShowCharacterList()
{
}
DataBase::DataBase()
{
}
DataBase::~DataBase()
{
	//SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	//SQLFreeHandle(SQL_HANDLE_ENV, henv);
	SQLDisconnect(hdbc);

}
/************************************************************************
/* HandleDiagnosticRecord : display error/warning information
/*
/* Parameters:
/* hHandle ODBC handle
/* hType Type of handle (SQL_HANDLE_STMT, SQL_HANDLE_ENV, SQL_HANDLE_DBC)
/* RetCode Return code of failing command
/************************************************************************/
void DataBase::HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE)
	{
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}
void DataBase::UpdateDB(const std::string& id, short x, short y, short lv, short hp, int exp)
{
	//statement ó���� �� �� ���� ������ ���� statement ������ ���� �׼����� ���� �մϴ�.
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	SQLLEN posX = x, posY = y;
	//cout << "��� �����ϴ� �� x : " << posX << " y: " << y << endl;

	wchar_t text[100];
	TCHAR* wchar_id = (TCHAR*)id.c_str();
	// ���� 5 TODO
	// 1. ���⼭ �����͸� �Է¹޾� ������Ʈ�� ��Ų��. 
	wsprintf(text, L"EXEC SetPos %S, %d, %d, %d, %d, %d", wchar_id, x, y, lv, hp, exp);

	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)text, SQL_NTS);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		if (retcode == SQL_ERROR)
		{
			HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
		}
	}
	else
	{
		HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
	}
	// Process data  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		SQLCancel(hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}
}


void DataBase::show_error()
{
	printf("error\n");
}

void DataBase::FindID(const std::string& s, short& x, short& y, short& hp, int& level, int& exp)
{
	if (retcode == SQL_NO_DATA_FOUND)
		std::cout << "data non exist" << std::endl;
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	SQLLEN cbPosX = 0, cbPosY = 0, cbHP = 0, cbLevel = 0, cbEXP = 0;
	wchar_t text[100];
	TCHAR* id = (TCHAR*)s.c_str();

	wsprintf(text, L"EXEC FindID %S", id);

	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)text, SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

		retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR, szName, NAME_LEN, &m_cbName);
		retcode = SQLBindCol(hstmt, 2, SQL_C_SHORT, &x, 4, &cbPosX);
		retcode = SQLBindCol(hstmt, 3, SQL_C_SHORT, &y, 4, &cbPosY);
		retcode = SQLBindCol(hstmt, 4, SQL_C_SHORT, &level, 4, &cbLevel);
		retcode = SQLBindCol(hstmt, 5, SQL_C_SHORT, &hp, 4, &cbHP);
		retcode = SQLBindCol(hstmt, 6, SQL_C_LONG, &exp, 4, &cbEXP);

		// Fetch and print each row of data. On an error, display a message and exit.  
		for (int i = 0; ; i++) {
			retcode = SQLFetch(hstmt);
			if (retcode == SQL_ERROR)
				std::cout << "error\n";
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				//wprintf(L"Position: %d %d\n", &x, &y);
			}
			else
				break;
		}
	}
	else
	{
		//cout << "idã���� ����: " << endl;
		HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
	}
	// Process data  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		SQLCancel(hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}
}
