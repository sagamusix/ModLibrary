/*
 * about.h
 * -------
 * Purpose: Implementation of the Mod Library about dialog.
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <QtWidgets/QDialog>
#include "ui_about.h"

class AboutDialog : public QDialog
{
	Q_OBJECT

public:
	AboutDialog(QWidget *parent = 0);

private:
	Ui_AboutDialog ui;
};
