/*
 *    Copyright (C) 2013, 2014, 2015, 2016, 2017
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
#include	"coordinates.h"
#include	<QDoubleValidator>
#include	<QFormLayout>
#include	<QFormLayout>
#include	<QVBoxLayout>
#include	<QSettings>

coordinates::coordinates(QSettings *dabSettings)
{
  this->mpDabSettings = dabSettings;
  mpLatitudeText      = new QLabel(this);
  mpLatitudeText->setText("latitude (decimal)");
  QDoubleValidator *la = new QDoubleValidator(-90.0, 9.0, 5);

  mpLatitude = new QLineEdit(this);
  mpLatitude->setValidator(la);
  mpLongitudeText = new QLabel(this);
  mpLongitudeText->setText("longitude (decimal)");
  QDoubleValidator *lo = new QDoubleValidator(-180.0, 180.0, 5);

  mpLongitude = new QLineEdit(this);
  mpLongitude->setValidator(lo);
  QFormLayout *layout = new QFormLayout;

  layout->addWidget(mpLatitudeText);
  layout->addWidget(mpLatitude);
  layout->addWidget(mpLongitudeText);
  layout->addWidget(mpLongitude);
  setWindowTitle("select coordinates");
  mpAcceptButton = new QPushButton("accept");
  QVBoxLayout *total = new QVBoxLayout;

  total->addItem(layout);
  total->addWidget(mpAcceptButton);
  setLayout(total);

  connect(mpLatitude,     SIGNAL(returnPressed()), this, SLOT(set_latitude()));
  connect(mpLongitude,    SIGNAL(returnPressed()), this, SLOT(set_longitude()));
  connect(mpAcceptButton, SIGNAL(clicked()),       this, SLOT(handle_acceptButton()));

  show();
  mLatitudeValue  = false;
  mLongitudeValue = false;
}

coordinates::~coordinates()
{
  hide();
  delete  mpAcceptButton;
  delete  mpLatitudeText;
  delete  mpLongitudeText;
  delete  mpLatitude;
  delete  mpLongitude;
}

void coordinates::set_latitude()
{
  mLatitudeValue = true;
}

void coordinates::set_longitude()
{
  mLongitudeValue = true;
}

void coordinates::handle_acceptButton()
{
  if (!mpLatitude || !mpLongitude)
    return;

  mpDabSettings->setValue("latitude", mpLatitude->text());
  mpDabSettings->setValue("longitude", mpLongitude->text());
  QDialog::done(0);
}

