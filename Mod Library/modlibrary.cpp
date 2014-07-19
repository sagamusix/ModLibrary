/*
 * modlibrary.cpp
 * --------------
 * Purpose: Implementation of the Mod Library main window.
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#include "modlibrary.h"
#include "modinfo.h"
#include "about.h"
#include "database.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QThread>
#include <QProgressDialog>
#include <QClipboard>
#include <QSettings>
#include <utility>
#include <libopenmpt/libopenmpt.hpp>


ModLibrary::ModLibrary(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	
	QSettings settings;
	settings.beginGroup("Window");
	resize(settings.value("size", size()).toSize());
	move(settings.value("pos", pos()).toPoint());
	if(settings.value("maximized", false).toBool())
	{
		setWindowState(windowState() | Qt::WindowMaximized);
	}
	settings.endGroup();
	lastDir = settings.value("lastdir", "").toString();

	try
	{
		ModDatabase::Instance().Open();
	} catch(ModDatabase::Exception &e)
	{
		QMessageBox(QMessageBox::Critical, "Mod Library", e.what()).exec();
		QTimer::singleShot(0, this, SLOT(close()));
		return;
	}

	// Menu
	connect(ui.actionAddFile, SIGNAL(triggered()), this, SLOT(OnAddFile()));
	connect(ui.actionAddFolder, SIGNAL(triggered()), this, SLOT(OnAddFolder()));
	connect(ui.actionExportPlaylist, SIGNAL(triggered()), this, SLOT(OnExportPlaylist()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(OnAbout()));

	// Search navigation
	connect(ui.doSearch, SIGNAL(clicked()), this, SLOT(OnSearch()));
	connect(ui.actionShow, SIGNAL(triggered()), this, SLOT(OnShowAll()));
	connect(ui.actionMaintain, SIGNAL(triggered()), this, SLOT(OnMaintain()));
	connect(ui.findWhat, SIGNAL(returnPressed()), this, SLOT(OnSearch()));
	connect(ui.melody, SIGNAL(returnPressed()), this, SLOT(OnSearch()));
	connect(ui.pasteMPT, SIGNAL(clicked()), this, SLOT(OnPasteMPT()));

	connect(ui.resultTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(OnCellClicked(int, int)));
	ui.resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui.resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
	ui.resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);

	checkBoxes.push_back(ui.findFilename);
	checkBoxes.push_back(ui.findTitle);
	checkBoxes.push_back(ui.findArtist);
	checkBoxes.push_back(ui.findSampleText);
	checkBoxes.push_back(ui.findInstrumentText);
	checkBoxes.push_back(ui.findComments);
	checkBoxes.push_back(ui.findPersonal);
	for(auto cb = checkBoxes.begin(); cb != checkBoxes.end(); cb++)
	{
		connect(*cb, SIGNAL(rightClicked(QCheckBoxEx *)), this, SLOT(OnSelectOne(QCheckBoxEx *)));
		connect(*cb, SIGNAL(middleClicked(QCheckBoxEx *)), this, SLOT(OnSelectAllButOne(QCheckBoxEx *)));
	}
}


ModLibrary::~ModLibrary()
{

}


void ModLibrary::closeEvent(QCloseEvent *event)
{
	QSettings settings;
	settings.beginGroup("Window");
	if(!isMaximized())
	{
		settings.setValue("size", size());
		settings.setValue("pos", pos());
	}
	settings.setValue("maximized", isMaximized());
	settings.endGroup();
	settings.setValue("lastdir", lastDir);

	event->accept();
}


void ModLibrary::OnAddFile()
{
	static QString modExtensions;
	if(modExtensions.isEmpty())
	{
		auto exts = openmpt::get_supported_extensions();
		for(auto ext = exts.cbegin(); ext != exts.cend(); ext++)
		{
			if(ext != exts.cbegin()) modExtensions += " ";
			modExtensions += QString("*.") + ext->c_str();
		}
	}

	QFileDialog dlg(this, tr("Select file(s) to add..."), lastDir, tr("Module files") + " (" + modExtensions + ");;" + tr("All files") + " (*.*)");
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::ExistingFiles);
	if(dlg.exec())
	{
		auto fileNames = dlg.selectedFiles();

		QProgressDialog progress(tr("Scanning files..."), tr("Cancel"), 0, 0, this);
		progress.setWindowModality(Qt::WindowModal);
		progress.setRange(0, fileNames.size());
		progress.setValue(0);
		progress.show();

		uint files = 0;
		for(auto file = fileNames.cbegin(); file != fileNames.cend() && !progress.wasCanceled(); file++)
		{
			if(file == fileNames.cbegin()) lastDir =  QFileInfo(*file).absoluteDir().absolutePath();
			ModDatabase::Instance().AddModule(*file);
			progress.setValue(++files);
			QCoreApplication::processEvents();
		}
	}
}


void ModLibrary::OnAddFolder()
{
	QFileDialog dlg(this, tr("Select folder to add..."), lastDir);
	dlg.setFileMode(QFileDialog::DirectoryOnly);
	if(dlg.exec())
	{
		const QString path = dlg.selectedFiles().first();
		lastDir = path;

		QDirIterator di(path, QDir::Files, QDirIterator::Subdirectories);

		QProgressDialog progress(tr("Scanning files..."), tr("Cancel"), 0, 0, this);
		progress.setWindowModality(Qt::WindowModal);
		progress.setRange(0, 0);
		progress.setValue(0);
		progress.show();

		uint addedFiles = 0, updatedFiles = 0;
		while(di.hasNext() && !progress.wasCanceled())
		{
			const QString fileName = di.next();
			progress.setLabelText(tr("Analyzing %1...\n%2 files added, %3 files updated.").arg(QDir::toNativeSeparators(fileName)).arg(addedFiles).arg(updatedFiles));
			QCoreApplication::processEvents();
			switch(ModDatabase::Instance().AddModule(fileName))
			{
			case ModDatabase::Added:
				addedFiles++;
				break;
			case ModDatabase::Updated:
				updatedFiles++;
				break;
			}
		}
	}
}


void ModLibrary::OnMaintain()
{
	QSqlQuery query(ModDatabase::Instance().GetDB());
	query.exec("SELECT COUNT(*) FROM `modlib_modules`");
	query.next();
	uint numFiles = query.value(0).toUInt();

	query.exec("SELECT `filename` FROM `modlib_modules`");

	QProgressDialog progress("Scanning files...", "Cancel", 0, 0, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setRange(0, numFiles);
	progress.setValue(0);
	progress.show();

	uint files = 0, updatedFiles = 0, removedFiles = 0;
	while(query.next() && !progress.wasCanceled())
	{
		const QString fileName = query.value(0).toString();
		progress.setLabelText(tr("Analyzing %1...\n%2 files updated, %3 files removed.").arg(QDir::toNativeSeparators(fileName)).arg(updatedFiles).arg(removedFiles));
		QCoreApplication::processEvents();
		switch(ModDatabase::Instance().UpdateModule(fileName))
		{
		case ModDatabase::Updated:
		case ModDatabase::Added:
			updatedFiles++;
			break;
		case ModDatabase::NoChange:
			break;
		case ModDatabase::IOError:
		case ModDatabase::NotAdded:
			removedFiles++;
			ModDatabase::Instance().RemoveModule(fileName);
			break;
		}
		progress.setValue(++files);
	}
}


void ModLibrary::DoSearch(bool showAll)
{
	QString what = ui.findWhat->text();
	what.replace('\\', "\\\\")
		.replace('%', "\\%")
		.replace('_', "\\_")
		.replace('*', "%")
		.replace('?', "_");
	what = "%" + what + "%";

	std::vector<QByteArray> melodyBytes;

	QString queryStr = "SELECT `filename`, `title`, `filesize`, `filedate` FROM `modlib_modules` ";
	if(!showAll)
	{
		queryStr += "WHERE (0 ";
		if(ui.findFilename->isChecked())		queryStr += "OR `filename` LIKE :str ESCAPE '\\' ";
		if(ui.findTitle->isChecked())			queryStr += "OR `title` LIKE :str ESCAPE '\\' ";
		if(ui.findArtist->isChecked())			queryStr += "OR `artist` LIKE :str ESCAPE '\\' ";
		if(ui.findSampleText->isChecked())		queryStr += "OR `sample_text` LIKE :str ESCAPE '\\' ";
		if(ui.findInstrumentText->isChecked())	queryStr += "OR `instrument_text` LIKE :str ESCAPE '\\' ";
		if(ui.findComments->isChecked())		queryStr += "OR `comments` LIKE :str ESCAPE '\\' ";
		if(ui.findPersonal->isChecked())		queryStr += "OR `personal_comments` LIKE :str ESCAPE '\\' ";
		queryStr += ") ";

		if(ui.limitSize->isChecked())
		{
			const auto factor = 1 << (10 * ui.limitSizeUnit->currentIndex());
			auto sizeMin = ui.limitMinSize->value() * factor, sizeMax = ui.limitMaxSize->value() * factor;
			if(sizeMin > sizeMax) std::swap(sizeMin, sizeMax);
			queryStr += "AND (`filesize` BETWEEN " + QString::number(sizeMin) + " AND " + QString::number(sizeMax) + ") ";
		}
		if(ui.limitFileDate->isChecked())
		{
			auto dateMin = QDateTime(ui.limitFileDateMin->date()).toTime_t();
			auto dateMax = QDateTime(ui.limitFileDateMax->date(), QTime(23, 59, 59)).toTime_t();
			if(dateMin > dateMax) std::swap(dateMin, dateMax);
			queryStr += "AND (`filedate` BETWEEN " + QString::number(dateMin) + " AND " + QString::number(dateMax) + ") ";
		}
		if(ui.limitYear->isChecked())
		{
			auto dateMin = QDateTime(ui.limitReleaseDateMin->date(), QTime(0, 0, 0)).toTime_t();
			auto dateMax = QDateTime(ui.limitReleaseDateMax->date(), QTime(23, 59, 59)).toTime_t();
			if(dateMin > dateMax) std::swap(dateMin, dateMax);
			queryStr += "AND (`editdate` BETWEEN " + QString::number(dateMin) + " AND " + QString::number(dateMax) + ") ";
		}
		if(ui.limitTime->isChecked())
		{
			auto timeMin = ui.limitTimeMin->value() * 1000, timeMax = ui.limitTimeMax->value() * 1000;
			if(timeMin > timeMax) std::swap(timeMin, timeMax);
			queryStr += "AND (`length` BETWEEN " + QString::number(timeMin) + " AND " + QString::number(timeMax) + ") ";
		}

		// Search for melody
		const auto melodies = ui.melody->text().split('|');
		int melodyCount = 0;
		for(auto melody = melodies.cbegin(); melody != melodies.cend(); melody++)
		{
			const auto notes = melody->simplified().split(' ');
			if(!notes.isEmpty())
			{
				melodyBytes.push_back(QByteArray());
				melodyBytes[melodyCount].reserve(notes.size());
				for(auto note = notes.cbegin(); note != notes.cend(); note++)
				{
					int8_t n = static_cast<int8_t>(note->toInt());
					melodyBytes[melodyCount].push_back(n);
				}
				queryStr += "AND INSTR(`note_data`, :note_data" + QString::number(melodyCount) + ") > 0 ";
				melodyCount++;
			}
		}
	}

	QSqlQuery query(ModDatabase::Instance().GetDB());
	query.prepare(queryStr);
	query.bindValue(":str", what);
	for(size_t i = 0; i < melodyBytes.size(); i++)
	{
		query.bindValue(":note_data" + QString::number(i), melodyBytes[i]);
	}
	query.exec();

	//ui.resultTable->setColumnCount(3);
	ui.resultTable->setRowCount(0);
	//ui.resultTable->setRowCount(query.size());	// Not supported by sqlite
	int row = 0;
	ui.resultTable->setUpdatesEnabled(false);
	ui.resultTable->setSortingEnabled(false);
	setCursor(Qt::BusyCursor);

	// TODO: This stuff is *slow*. Probably requires a custom data model.
	QString fileName, title, fileDate;
	int fileSize;
	while(query.next())
	{
		ui.resultTable->insertRow(row);

		fileName = query.value(0).toString();
		title = query.value(1).toString();
		fileSize = query.value(2).toInt();
		fileDate = QDateTime::fromTime_t(query.value("filedate").toInt()).toString(Qt::SystemLocaleShortDate);

		QTableWidgetItem *item = new QTableWidgetItem(title.isEmpty() ? QFileInfo(fileName).fileName() : title);
		item->setData(Qt::UserRole, fileName);
		item->setToolTip(QDir::toNativeSeparators(fileName));
		ui.resultTable->setItem(row, 0, item);

		QString sizeStr;
		if(fileSize < 1024)
			sizeStr = QString::number(fileSize) + " B";
		else if(fileSize < 1024 * 1024)
			sizeStr = QString::number(fileSize / 1024) + " KiB";
		else
			sizeStr = QString("%1.%2 MiB").arg(fileSize / (1024 * 1024)).arg((((fileSize / 1024) % 1024) * 100) / 1024, 2, 10, QChar('0'));
		item = new QTableWidgetItem(sizeStr);
		item->setData(Qt::TextAlignmentRole, (int)Qt::AlignRight | Qt::AlignVCenter);
		ui.resultTable->setItem(row, 1, item);

		ui.resultTable->setItem(row, 2, new QTableWidgetItem(fileDate));
		ui.resultTable->setRowHeight(row, 20);
		row++;
	}
	unsetCursor();
	ui.resultTable->setSortingEnabled(true);
	ui.resultTable->setUpdatesEnabled(true);
	ui.statusBar->showMessage(tr("%1 files found.").arg(row));

	if(row == 1 && !showAll)
	{
		// Show the only result
		OnCellClicked(0, 0);
	}
}


void ModLibrary::OnSelectOne(QCheckBoxEx *sender)
{
	sender->setChecked(true);
	for(auto cb = checkBoxes.begin(); cb != checkBoxes.end(); cb++)
	{
		if(sender != *cb)
		{
			(**cb).setChecked(false);
		}
	}
}


void ModLibrary::OnSelectAllButOne(QCheckBoxEx *sender)
{
	sender->setChecked(false);
	for(auto cb = checkBoxes.begin(); cb != checkBoxes.end(); cb++)
	{
		if(sender != *cb)
		{
			(**cb).setChecked(true);
		}
	}
}


void ModLibrary::OnCellClicked(int row, int /*col*/)
{
	QString fileName = ui.resultTable->item(row, 0)->data(Qt::UserRole).toString();
	ModInfo dlg(fileName, this);
	dlg.exec();
}


void ModLibrary::OnExportPlaylist()
{
	if(!ui.resultTable->rowCount())
	{
		OnShowAll();
	}

	const auto numRows = ui.resultTable->rowCount();
	if(!numRows)
	{
		QMessageBox mb(QMessageBox::Information, tr("Your library is empty."), tr("Mod Library"));
		mb.exec();
		return;
	}

	QFileDialog dlg(this, tr("Save Playlist..."), lastDir, tr("Playlist files (*.pls)"));
	dlg.setAcceptMode(QFileDialog::AcceptSave);
	if(dlg.exec())
	{
		auto files = dlg.selectedFiles();
		if(!files.isEmpty())
		{
			QFile file(files.first());
			if(file.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				setCursor(Qt::BusyCursor);
				QTextStream fout(&file);
				fout << "[playlist]" << endl;
				fout << "numberofentries=" << numRows << endl;
				for(auto row = 0; row < numRows; row++)
				{
					ui.resultTable->item(row, 0)->data(Qt::UserRole).toString();
					fout << "file" << (row + 1) << "=" << QDir::toNativeSeparators(ui.resultTable->item(row, 0)->data(Qt::UserRole).toString()) << endl;
					fout << "title" << (row + 1) << "=" << ui.resultTable->item(row, 0)->text() << endl;
				}
				unsetCursor();
			}
		}
	}
}


// Interpret pasted OpenMPT pattern format for melody search
void ModLibrary::OnPasteMPT()
{
	const QString notes[] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };
	const QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData();

	if(mimeData->hasText())
	{
		QString melody;
		QString data = mimeData->text();
		auto offset = data.indexOf("ModPlug Tracker ");
		if(offset == -1)
		{
			return;
		}
		data.remove(0, offset + 16);
		auto lines = data.split('\n');
		int channels = 0;
		int prevNote;
		do
		{
			prevNote = -1;
			for(auto line = lines.cbegin(); line != lines.cend(); line++)
			{
				int offset = -1;
				for(int i = 0; i <= channels; i++)
				{
					offset = line->indexOf("|", offset + 1);
					if(offset == -1)
					{
						break;
					}
				}
				if(offset == -1)
				{
					continue;
				}

				// This appears to be a valid channel
				if(prevNote == -1) prevNote = 0;

				int note = 0;
				char octave = line->at(offset + 3).toLatin1();
				if(octave >= '0' && octave <= '9')
				{
					const auto noteStr = line->midRef(offset + 1, 2);
					for(auto i = 0; i < 12; i++)
					{
						if(notes[i] == noteStr)
						{
							note = i + (octave - '0') * 12;
							break;
						}
					}
				}
				if(note)
				{
					int diff = note - prevNote;
					if(prevNote)
					{
						melody += QString::number(diff) + " ";
					}
					prevNote = note;
				}
			}
			channels++;
			if(prevNote)
			{
				melody += "|";
			}
		} while (prevNote != -1);


		ui.melody->setText(melody);
	}
}


void ModLibrary::OnAbout()
{
	AboutDialog dlg(this);
	dlg.exec();
}
