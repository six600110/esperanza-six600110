/** 
 *  This file is a part of Esperanza, an XMMS2 Client.
 *
 *  Copyright (C) 2005-2006 XMMS2 Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */


#ifndef __VALUEBAR_H__
#define __VALUEBAR_H__

#include <QWidget>
#include <QPaintEvent>

class ValueBar : public QWidget
{
	Q_OBJECT
	public:
		ValueBar (QWidget *);
		void paintEvent (QPaintEvent *);
		void setValue (int i) {
			m_value = i;
			update ();
		};
	private:
		int m_value;
};

#endif
