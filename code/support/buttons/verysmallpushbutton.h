/*
 *    Copyright (C) 2013, 2014, 2015, 2016, 2017, 2018, 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB (formerly SDR-J, JSDR).
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef  VERY_SMALL_BUTTON_H
#define  VERY_SMALL_BUTTON_H

#include  <QPushButton>
#include  <QSize>

// just redefining sizeHint
class verySmallPushButton : public QPushButton
{
Q_OBJECT
public:
  explicit verySmallPushButton(QWidget *);
  ~verySmallPushButton() override = default;
  QSize sizeHint() const override;
  void mousePressEvent(QMouseEvent * e) override;
signals:
  void rightClicked();
};

#endif

