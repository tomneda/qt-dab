/*
 *    Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of Qt-DAB
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
#include  "iqdisplay.h"
#include  "spectrogramdata.h"

/*
 *	iq circle plotter
 */
SpectrogramData * IQData = nullptr;
static std::array<std::complex<int>, 512> Points;

IQDisplay::IQDisplay(QwtPlot * plot, int16_t x)
  : QwtPlotSpectrogram()
{
  QwtLinearColorMap * colorMap = new QwtLinearColorMap(Qt::darkBlue, Qt::yellow);

  setRenderThreadCount(1);
  Radius = 100;
  plotgrid = plot;
  x_amount = 512;
  CycleCount = 0;
  this->setColorMap(colorMap);
  plotData.resize(2 * Radius * 2 * Radius);
  plot2.resize(2 * Radius * 2 * Radius);
  memset(plotData.data(), 0, 2 * 2 * Radius * Radius * sizeof(double));
  IQData = new SpectrogramData(plot2.data(), 0, 2 * Radius, 2 * Radius, 2 * Radius, 50.0);
  for (int i = 0; i < x_amount; i++)
  {
    Points[i] = std::complex<int>(0, 0);
  }
  this->setData(IQData);
  plot->enableAxis(QwtPlot::xBottom, false);
  plot->enableAxis(QwtPlot::yLeft, false);
  this->setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
  plotgrid->replot();
}

IQDisplay::~IQDisplay()
{
  this->detach();
  //	delete		IQData;
}

void IQDisplay::setPoint(int x, int y, int val)
{
  //plotData[(y + Radius - 1) * 2 * Radius + x + Radius - 1] = val;
  plotData.at((y + Radius - 1) * 2 * Radius + x + Radius - 1) = val;
}

template <class T> inline void symmetric_limit(T &ioVal, const T iLimit)
{
  if (ioVal > iLimit)
  {
    ioVal = iLimit;
  }
  else if (ioVal < -iLimit)
  {
    ioVal = -iLimit;
  }
}

void IQDisplay::DisplayIQ(const std::complex<float> * z, int amount, float scale, float ref)
{
  //	clean the screen
  for (int i = 0; i < amount; i++)
  {
    setPoint(real(Points[i]), imag(Points[i]), 0);
  }

  // draw cross
  for (int32_t i = -(Radius-1); i < Radius; i++)
  {
    setPoint(0, i, 10);
    setPoint(i, 0, 10);
  }

  // clear and draw unit circle
  draw_circle(lastCircleSize, 0);
  draw_circle(ref, 10);
  lastCircleSize = ref;

  for (int i = 0; i < amount; i++)
  {
    int x = (int)(scale * real(z[i]));
    int y = (int)(scale * imag(z[i]));

    symmetric_limit(x, Radius - 1);
    symmetric_limit(y, Radius - 1);

    Points[i] = std::complex<int>(x, y);
    setPoint(x, y, 100);
  }

  memcpy(plot2.data(), plotData.data(), 2 * 2 * Radius * Radius * sizeof(double));
  this->detach();
  this->setData(IQData);
  this->setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
  this->attach(plotgrid);
  plotgrid->replot();
}

void IQDisplay::draw_circle(float ref, int val)
{
  int32_t MAX_CIRCLE_POINTS = 45;
  float SCALE = ref / 100.0f;

  for (int32_t i = 0; i < MAX_CIRCLE_POINTS; ++i)
  {
    float phase = 0.5f * M_PI * i / MAX_CIRCLE_POINTS;

    int32_t h = (int32_t)(Radius * SCALE * cosf(phase));
    int32_t v = (int32_t)(Radius * SCALE * sinf(phase));

    symmetric_limit(h, Radius - 1);
    symmetric_limit(v, Radius - 1);

    setPoint(-h, -v, val);
    setPoint(-h, +v, val);
    setPoint(+h, -v, val);
    setPoint(+h, +v, val);
  }
}
