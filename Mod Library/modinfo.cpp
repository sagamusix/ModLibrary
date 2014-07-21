/*
 * modinfo.cpp
 * -----------
 * Purpose: Implementation of the Mod Library information window.
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#include "modinfo.h"
#include "database.h"
#include "audioplayer.h"
#include <QMenu>
#include <QMessageBox>
#include <QClipboard>


ModInfo::ModInfo(const QString &fileName, QWidget *parent)
	: QDialog(parent), fileName(fileName), audio(nullptr)
{
	ui.setupUi(this);
	QString nativeName = QDir::toNativeSeparators(fileName);
	setWindowTitle(tr("Module Info: ") + QFileInfo(nativeName).fileName());
	ui.fileName->setText(nativeName);
	this->setWindowFlags(Qt::Dialog | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

	if(ModDatabase::Instance().UpdateModule(fileName) & ModDatabase::Error)
	{
		QMessageBox mb(QMessageBox::Question, tr("Mod Library"), tr("Error while loading information for file\n%1\nWould you like to remove it form the database?").arg(nativeName), QMessageBox::Yes | QMessageBox::No);
		mb.setDefaultButton(QMessageBox::Yes);
		if(mb.exec() == QMessageBox::Yes)
		{
			ModDatabase::Instance().RemoveModule(fileName);
			QTimer::singleShot(0, this, SLOT(close()));
			return;
		}
	}
	
	Module mod;
	ModDatabase::Instance().GetModule(fileName, mod);
	ui.songTitle->setText(mod.title);
	QString info;
	info +=
		tr("File Size: %1 Bytes\nDuration: %2:%3.%4\n%5 orders, %6 patterns, %7 samples, %8 instruments")
		.arg(mod.fileSize)
		.arg(mod.length / 60000)
		.arg((mod.length / 1000) % 60, 2, 10, QChar('0'))
		.arg((mod.length / 100) % 10)
		.arg(mod.numOrders)
		.arg(mod.numPatterns)
		.arg(mod.numSamples)
		.arg(mod.numInstruments);
	ui.varInfo->setPlainText(info);
	ui.editArtist->setText(mod.artist);

	setUpdatesEnabled(false);
	auto names = mod.sampleText.split('\n');
	for(int i = 0; i < mod.numSamples; i++)
	{
		const QString name = QString("%1").arg(i + 1, 2, 10, QChar('0')) + ": " + names[i];
		ui.sampleNames->addItem(name);
	}

	names = mod.instrumentText.split('\n');
	for(int i = 0; i < mod.numInstruments; i++)
	{
		const QString name = QString("%1").arg(i + 1, 2, 10, QChar('0')) + ": " + names[i];
		ui.instrumentNames->addItem(name);
	}
	setUpdatesEnabled(true);

	ui.comments->setPlainText(mod.comments);
	ui.personalComments->setPlainText(mod.personalComment);

	connect(ui.close, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui.openFile, SIGNAL(clicked()), this, SLOT(OnOpenFileMenu()));
	connect(ui.play, SIGNAL(clicked()), this, SLOT(OnPlay()));
	connect(ui.volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(OnVolumeChanged(int)));
	connect(ui.copyFingerprint, SIGNAL(clicked()), this, SLOT(OnCopyFingerprint()));
}


ModInfo::~ModInfo()
{
	if(audio != nullptr)
	{
		audio->kill = true;
	}
	ModDatabase::Instance().UpdateCustom(fileName, ui.editArtist->text(), ui.personalComments->toPlainText());
}


void ModInfo::OnOpenFileMenu()
{
	QMenu menu(this);
#ifdef WIN32
	menu.addAction(tr("&Open in Explorer"), this, SLOT(OnOpenExplorer()));
#endif
	//menu.addAction("&Add Applications...", this, SLOT(OnOpenExplorer()));
	menu.exec(this->mapToGlobal(ui.openFile->pos() + QPoint(0, ui.openFile->height())));
}


void ModInfo::OnOpenExplorer()
{
	QStringList args;
	args << "/select," << QDir::toNativeSeparators(fileName);
	QProcess::startDetached("explorer", args);
}


void ModInfo::OnPlay()
{
	if(audio == nullptr)
	{
		QFile file(fileName);
		if(!file.open(QIODevice::ReadOnly))
		{
			return;
		}

		QThread *thread = new QThread;
		audio = new AudioThread(file, ui.volumeSlider->value());
		audio->moveToThread(thread);
		connect(thread, SIGNAL(started()), audio, SLOT(process()));
		connect(audio, SIGNAL(finished()), thread, SLOT(quit()));
		connect(audio, SIGNAL(finished()), audio, SLOT(deleteLater()));
		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
		thread->start();
		ui.play->setText("&Stop");
	} else
	{
		audio->kill = true;
		audio = nullptr;
		ui.play->setText("&Play");
	}
}


void ModInfo::OnVolumeChanged(int volume)
{
	if(audio != nullptr)
	{
		audio->setVolume(volume);
	}
}


void ModInfo::OnCopyFingerprint()
{
	QApplication::clipboard()->setText(ModDatabase::Instance().GetPrintableFingerprint(fileName));
}
