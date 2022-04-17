/*
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB
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
#include  "element-selector.h"
#include  <QTime>
#include  <QVBoxLayout>
#include  <cstdio>

elementSelector::elementSelector (const QString &serviceName) :
  mTheService(serviceName),
  mDayBox(),
  mHourBox(),
  mMinuteBox(),
  mReadyBox("ready")
{
  QTime       currentTime = QTime::currentTime();
  QHBoxLayout *layOut     = new QHBoxLayout();
  QDate       currentDate = QDate::currentDate();
  QDate       workingDate = currentDate;

  mDayBox.addItem("today");
  workingDate = workingDate.addDays(1);
  mDayBox.addItem(QDate::shortDayName(workingDate.dayOfWeek()));
  workingDate = workingDate.addDays(1);
  mDayBox.addItem(QDate::shortDayName(workingDate.dayOfWeek()));
  workingDate = workingDate.addDays(1);
  mDayBox.addItem(QDate::shortDayName(workingDate.dayOfWeek()));
  workingDate = workingDate.addDays(1);
  mDayBox.addItem(QDate::shortDayName(workingDate.dayOfWeek()));
  workingDate = workingDate.addDays(1);
  mDayBox.addItem(QDate::shortDayName(workingDate.dayOfWeek()));
  workingDate = workingDate.addDays(1);
  mDayBox.addItem(QDate::shortDayName(workingDate.dayOfWeek()));
  mDayBox.setToolTip("days ahead");
  mHourBox.setToolTip("select the hour in the range 0 .. 23");
  mHourBox.setMaximum(23);
  mHourBox.setValue(currentTime.hour());
  mMinuteBox.setToolTip("select the minute");
  mMinuteBox.setMaximum(59);
  mMinuteBox.setValue(currentTime.minute());
  mReadyBox.setToolTip("click here when time is set");
  layOut->addWidget(&mTheService);
  layOut->addWidget(&mDayBox);
  layOut->addWidget(&mHourBox);
  layOut->addWidget(&mMinuteBox);
  layOut->addWidget(&mReadyBox);
  setWindowTitle(tr("time select"));
  setLayout(layOut);

  connect(&mReadyBox, &QCheckBox::stateChanged, this, &elementSelector::collectData);
}

void elementSelector::collectData()
{
  int val = mHourBox.value() * 60 + mMinuteBox.value();
  int x   = mDayBox.currentIndex();

  val |= x << 16;
  if (mReadyBox.isChecked())
  {
    QDialog::done(val);
  }
}

