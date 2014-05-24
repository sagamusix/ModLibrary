/*
 * modinfo.h
 * ---------
 * Purpose: Implementation of the Mod Library information window.
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <QtWidgets/QDialog>
#include "ui_modinfo.h"
#include "database.h"

class AudioThread;

class DefaultPrograms
{
	QString name, path, parameters;
};

class ModInfo : public QDialog
{
	Q_OBJECT

protected:
	QString fileName;
	AudioThread *audio;

public:
	ModInfo(const QString &fileName, QWidget *parent = 0);
	~ModInfo();

protected slots:
	void OnOpenFileMenu();
	void OnOpenExplorer();
	void OnPlay();
	void OnVolumeChanged(int);

private:
	Ui_ModInfo ui;
};

