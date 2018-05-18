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
	ModLibrary(QWidget *parent = nullptr);
	~ModLibrary();

protected slots:
	void OnAddFile();
	void OnAddFolder();
	void OnMaintain();
	void OnSearch() { DoSearch(false); }
	void OnShowAll() { DoSearch(true); }
	void OnSelectOne(QCheckBoxEx *sender);
	void OnSelectAllButOne(QCheckBoxEx *sender);
	void OnCellClicked(const QModelIndex &index);
	void OnFindDupes();
	void OnExportPlaylist();
	void OnPasteMPT();
	void OnSettings();
	void OnAbout();

protected:
	void DoSearch(bool showAll);
	void closeEvent(QCloseEvent *event);

private:
	Ui_ModLibararyClass ui;
};

