/*
 * settings.h
 * ----------
 * Purpose: Implementation of the Mod Library settings dialog.
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <QtWidgets/QDialog>
#include "ui_settings.h"

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	SettingsDialog(QWidget *parent = 0);

private:
	Ui_Settings ui;
};
