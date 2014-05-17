/*
 * database.h
 * ----------
 * Purpose: Implementation of the Mod Library database functionality.
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <QtSql/QtSql>

struct Module
{
	QString hash;
	QString fileName;
	int fileSize;
	QDateTime fileDate;
	QDateTime editDate;
	QString format;
	QString title;
	int length;
	int numChannels;
	int numPatterns;
	int numOrders;
	int numSubSongs;
	int numSamples;
	int numInstruments;
	QString sampleText;
	QString instrumentText;
	QString comments;
	QString artist;
	QString personalComment;
};


class ModDatabase
{
protected:
	static ModDatabase instance;
	QSqlDatabase db;
	QSqlQuery insertQuery, updateQuery, updateCommentsQuery, selectQuery;

public:
	enum AddResult
	{
		NotAdded,
		IOError,
		Added,
		Updated,
		NoChange,
	};

	class Exception
	{
	protected:
		QString str;

	public:
		Exception(const QString &str, const QSqlError &error) : str(str + error.text()) { }
		const QString &what() const { return str; }
	};

	~ModDatabase();

	static ModDatabase &Instance() { return instance; }

	void Open();
	AddResult AddModule(const QString &path);
	AddResult UpdateModule(const QString &path);
	bool UpdateComments(const QString &path, const QString &comments);
	Module GetModule(const QString &path);
	static Module GetModule(QSqlQuery &query);
	QSqlDatabase &GetDB() { return db; }

protected:
	AddResult PrepareQuery(const QString &path, QSqlQuery &query);
};