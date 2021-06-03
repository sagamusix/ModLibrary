/*
 * tablemodel.h
 * ------------
 * Purpose: Data model for the main result table
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once
#include <QAbstractTableModel>
#include <QSqlQuery>
#include <cstdint>
#include <algorithm>
#include <chromaprint/src/chromaprint.h>
#include <QCollator>
#include <QDateTime>
#include <QFileInfo>
#include <QSize>


static const uint8_t BitsSetTable256[256] =
{
#	define B2(n) n,     n+1,     n+1,     n+2
#	define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#	define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
	B6(0), B6(1), B6(1), B6(2)
};

class TableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	struct Entry
	{
		QString fileName, title, dateStr, sizeStr;
		uint fileDate;
		int fileSize;
		int match;	// Fingerprint match quality and cache flag at the same time (-1 = not cached yet)

		Entry() : match(-1) { }
	};

	// Database columns
	enum DBColumns { FILENAME_COLUMN = 0, TITLE_COLUMN = 1, FILESIZE_COLUMN = 2, FILEDATE_COLUMN = 3, FINGERPRINT_COLUMN = 4, };
	enum TableColumns { TITLE_TABLE = 0, FILESIZE_TABLE = 1, FILEDATE_TABLE = 2, FINGERPRINT_TABLE = 3, };

	mutable QSqlQuery query;
	std::vector<Entry> modules;
	std::vector<Entry *> modulesSorted;	// Module order according to current sorting scheme

	uint32_t *rawFingerprint;
	int rawFingerprintSize;
	int numRows;
#ifdef _MSC_VER
	bool hasPopCnt;
#endif

	TableModel(QSqlQuery &query, uint32_t *fp, int fpsize) : query(query), numRows(0), rawFingerprint(fp), rawFingerprintSize(fpsize)
	{
#ifdef _MSC_VER
		int CPUInfo[4];
		__cpuid(CPUInfo, 1);
		hasPopCnt = (CPUInfo[2] & (1 << 23)) != 0;
#endif

		query.exec();
		// SQLite doesn't have query.size()...
		while(query.next())
		{
			numRows++;
		}
		modules.resize(numRows);
		modulesSorted.resize(numRows);
		for(int i = 0; i < numRows; i++)
		{
			modulesSorted[i] = &modules[i];
		}
	}

	~TableModel()
	{
		chromaprint_dealloc(rawFingerprint);
	}

	int rowCount(const QModelIndex & = QModelIndex()) const { return numRows; }
	int columnCount(const QModelIndex & = QModelIndex()) const { return rawFingerprintSize ? 4 : 3; }

	bool CacheEntry(Entry &entry) const
	{
		entry.match = 0;
		if(!query.seek(&entry - modules.data()))
		{
			return false;
		}

		entry.fileName = query.value(FILENAME_COLUMN).toString();
		entry.title = query.value(TITLE_COLUMN).toString();
		if(entry.title.isEmpty()) entry.title = QFileInfo(entry.fileName).fileName();
		entry.fileSize = query.value(FILESIZE_COLUMN).toInt();
		entry.fileDate = query.value(FILEDATE_COLUMN).toInt();
		entry.dateStr = QLocale::system().toString(QDateTime::fromSecsSinceEpoch(entry.fileDate), QLocale::ShortFormat);

		if(entry.fileSize < 1024)
			entry.sizeStr = QString::number(entry.fileSize) + " B";
		else if(entry.fileSize < 1024 * 1024)
			entry.sizeStr = QString::number(entry.fileSize / 1024) + " KiB";
		else
			entry.sizeStr = QString("%1.%2 MiB").arg(entry.fileSize / (1024 * 1024)).arg((((entry.fileSize / 1024) % 1024) * 100) / 1024, 2, 10, QChar('0'));

		if(rawFingerprintSize)
		{
			auto modFingerprint = query.value(FINGERPRINT_COLUMN).toByteArray();
			uint32_t *modRawFingerprint = nullptr;
			int modRawFingerprintSize = 0;
			chromaprint_decode_fingerprint(modFingerprint.data(), modFingerprint.size(), &modRawFingerprint, &modRawFingerprintSize, nullptr, 0);
			const int compareLength = std::min(rawFingerprintSize, modRawFingerprintSize);
			const int maxMatches = 32 * std::max(rawFingerprintSize, modRawFingerprintSize);
			int bestDifference = INT_MAX;

			for(int offset = 0; offset < 32 && bestDifference > 0; offset++)
			{
				const int thisLength = compareLength - offset;
				int differences = 32 * std::abs(rawFingerprintSize - modRawFingerprintSize);
#ifdef _MSC_VER
				if(hasPopCnt)
				{
					for(int i = 0; i < thisLength; i++)
					{
						differences += _mm_popcnt_u32(rawFingerprint[offset + i] ^ modRawFingerprint[i]);
					}
				} else
#elif defined(__GNUC__)
				for(int i = 0; i < thisLength; i++)
				{
					differences += __builtin_popcount(rawFingerprint[offset + i] ^ modRawFingerprint[i]);
				}
				if(0)
#endif
				{
					for(int i = 0; i < thisLength; i++)
					{
						union { uint32_t u32; uint8_t u8[4]; } v;
						v.u32 = rawFingerprint[offset + i] ^ modRawFingerprint[i];
						differences += BitsSetTable256[v.u8[0]]
						+ BitsSetTable256[v.u8[1]]
						+ BitsSetTable256[v.u8[2]]
						+ BitsSetTable256[v.u8[3]];
					}
				}
				bestDifference = std::min(differences, bestDifference);
			}

			entry.match = (100 * (maxMatches - bestDifference)) / maxMatches;
			chromaprint_dealloc(modRawFingerprint);
		}
		return true;
	}

	QVariant data(const QModelIndex &index, int role) const
	{
		if(size_t(index.row()) >= modules.size())
		{
			return QVariant();
		}
		Entry &entry = *modulesSorted[index.row()];

		if(entry.match == -1)
		{
			// Entry isn't cached yet, generate info
			if(!CacheEntry(entry))
			{
				return "n/a";
			}
		}

		if(role == Qt::DisplayRole)
		{
			switch(index.column())
			{
			case TITLE_TABLE:
				return entry.title;
			case FILESIZE_TABLE:
				return entry.sizeStr;
			case FILEDATE_TABLE:
				return entry.dateStr;
			case FINGERPRINT_TABLE:
				return entry.match;
			}
		} else if(role == Qt::ToolTipRole || role == Qt::UserRole)
		{
			return entry.fileName;
		}
		return QVariant();
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role) const
	{
		if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
		{
			switch(section)
			{
			case TITLE_TABLE:
				return tr("Title");
			case FILESIZE_TABLE:
				return tr("File Size");
			case FILEDATE_TABLE:
				return tr("Last Modified");
			case FINGERPRINT_TABLE:
				return tr("Match %");
			}
		}
		return QVariant();
	}

	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder)
	{
		// Complete table needs to be cached for sorting, obviously.
		for(auto &m : modules)
		{
			if(m.match == -1)
			{
				CacheEntry(m);
			}
		}

		QCollator collator;
		collator.setNumericMode(true);
		collator.setCaseSensitivity(Qt::CaseInsensitive);

		switch(column)
		{
		case TITLE_TABLE:
			std::sort(modulesSorted.begin(), modulesSorted.end(), [&collator](const Entry *a, const Entry *b) { return collator.compare(a->title, b->title) < 0; });
			break;
		case FILESIZE_TABLE:
			std::sort(modulesSorted.begin(), modulesSorted.end(), [](const Entry *a, const Entry *b) { return a->fileSize < b->fileSize; });
			break;
		case FILEDATE_TABLE:
			std::sort(modulesSorted.begin(), modulesSorted.end(), [](const Entry *a, const Entry *b) { return a->fileDate < b->fileDate; });
			break;
		case FINGERPRINT_TABLE:
			std::sort(modulesSorted.begin(), modulesSorted.end(), [](const Entry *a, const Entry *b) { return a->match < b->match; });
			break;
		}
		if(order == Qt::DescendingOrder)
		{
			std::reverse(modulesSorted.begin(), modulesSorted.end());
		}

		emit dataChanged(QAbstractItemModel::createIndex(0, 0), QAbstractItemModel::createIndex(rowCount() - 1, columnCount() - 1));
	}
};

