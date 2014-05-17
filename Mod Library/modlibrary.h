/*
 * modlibrary.h
 * ------------
 * Purpose: Implementation of the Mod Library main window.
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include "ui_modlibrary.h"

class ModLibrary : public QMainWindow
{
	Q_OBJECT

protected:
	QString lastDir;
	std::vector<QCheckBoxEx *> checkBoxes;

public:
	ModLibrary(QWidget *parent = 0);
	~ModLibrary();

protected slots:
	void OnAddFile();
	void OnAddFolder();
	void OnMaintain();
	void OnSearch() { DoSearch(false); }
	void OnShowAll() { DoSearch(true); }
	void OnSelectOne(QCheckBoxEx *sender);
	void OnSelectAllButOne(QCheckBoxEx *sender);
	void OnCellClicked(int row, int col);
	void OnExportPlaylist();
	void OnPasteMPT();

protected:
	void DoSearch(bool showAll);

private:
	Ui_ModLibararyClass ui;
};

