/*
 * qcheckboxex.h
 * -------------
 * Purpose: Implementation of a Qt checkbox that also supports right- and middle-clicking for user-specified actions.
 * Notes  : (currently none)
 * Authors: Johannes Schultz
 * The Mod Library source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <QCheckBox>
#include <QMouseEvent>

class QCheckBoxEx: public QCheckBox
{
	Q_OBJECT

public:
	explicit QCheckBoxEx(QWidget *parent=0) : QCheckBox(parent) { }
	explicit QCheckBoxEx(const QString &text, QWidget *parent=0) : QCheckBox(text, parent) { }

signals:
	void rightClicked(QCheckBoxEx *);
	void middleClicked(QCheckBoxEx *);

protected:
	virtual void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE
	{
		if(e->button() == Qt::RightButton)
		{
			emit rightClicked(this);
		} else if(e->button() == Qt::MiddleButton)
		{
			emit middleClicked(this);
		}
		QCheckBox::mouseReleaseEvent(e);
	}
};