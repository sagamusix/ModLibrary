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
#include <QFileInfo>
#include <QDateTime>
#include <QSize>


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

	mutable QSqlQuery query;
	std::vector<Entry> modules;
	std::vector<Entry *> modulesSorted;

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
			entry.match = 0;
			if(!query.seek(index.row()))
			{
				return "n/a";
			}
			entry.fileName = query.value(0).toString();
			entry.title = query.value(1).toString();
			if(entry.title.isEmpty()) entry.title = QFileInfo(entry.fileName).fileName();
			entry.fileSize = query.value(2).toInt();
			entry.fileDate = query.value("filedate").toInt();
			entry.dateStr = QDateTime::fromTime_t(entry.fileDate).toString(Qt::SystemLocaleShortDate);

			if(entry.fileSize < 1024)
				entry.sizeStr = QString::number(entry.fileSize) + " B";
			else if(entry.fileSize < 1024 * 1024)
				entry.sizeStr = QString::number(entry.fileSize / 1024) + " KiB";
			else
				entry.sizeStr = QString("%1.%2 MiB").arg(entry.fileSize / (1024 * 1024)).arg((((entry.fileSize / 1024) % 1024) * 100) / 1024, 2, 10, QChar('0'));

			if(rawFingerprintSize)
			{
				auto modFingerprint = query.value(4).toByteArray();
				uint32_t *modRawFingerprint = nullptr;
				int modRawFingerprintSize = 0;
				chromaprint_decode_fingerprint(modFingerprint.data(), modFingerprint.size(), (void **)&modRawFingerprint, &modRawFingerprintSize, nullptr, 0);
				const auto compareLength = std::min(rawFingerprintSize, modRawFingerprintSize);
				int differences = 32 * std::abs(rawFingerprintSize - modRawFingerprintSize);
				const int maxMatches = 32 * std::max(rawFingerprintSize, modRawFingerprintSize);
#ifdef _MSC_VER
				if(hasPopCnt)
				{
					for(auto i = 0; i < compareLength; i++)
					{
						differences += _mm_popcnt_u32(rawFingerprint[i] ^ modRawFingerprint[i]);
					}
				} else
#elif defined(__GNUC__)
				for(auto i = 0; i < compareLength; i++)
				{
					differences += __builtin_popcount(rawFingerprint[i] ^ modRawFingerprint[i]);
				}
				if(0)
#endif
				{
					for(auto i = 0; i < compareLength; i++)
					{
						uint32_t v = rawFingerprint[i] ^ modRawFingerprint[i];
						for(; v; differences++)
						{
							v &= v - 1;
						}
					}
				}
				entry.match = (100 * (maxMatches - differences)) / maxMatches;
				chromaprint_dealloc(modRawFingerprint);
			}
		}

		if(role == Qt::DisplayRole)
		{
			switch(index.column())
			{
			case 0:
				return entry.title;
			case 1:
				return entry.sizeStr;
			case 2:
				return entry.dateStr;
			case 3:
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
			case 0:
				return tr("Title");
			case 1:
				return tr("File Size");
			case 2:
				return tr("Last Modified");
			case 3:
				return tr("Match %");
			}
		}
		return QVariant();
	}

	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder)
	{
		switch(column)
		{
		case 0:
			std::sort(modulesSorted.begin(), modulesSorted.end(), [](const Entry *a, const Entry *b) { return a->title == b->title; });
			break;
		case 1:
			std::sort(modulesSorted.begin(), modulesSorted.end(), [](const Entry *a, const Entry *b) { return a->fileSize < b->fileSize; });
			break;
		case 2:
			std::sort(modulesSorted.begin(), modulesSorted.end(), [](const Entry *a, const Entry *b) { return a->fileDate < b->fileDate; });
			break;
		case 3:
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

