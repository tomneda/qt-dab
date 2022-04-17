/*
 *    Copyright (C)  2014 .. 2017
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

#include "spectrum-viewer.h"
#include <QSettings>
#include "iqdisplay.h"
#include <QColor>
#include <QPen>
#include "color-selector.h"

spectrumViewer::spectrumViewer (RadioInterface * const mr, QSettings * const dabSettings,
                                RingBuffer<TIQSmpFlt> * const sbuffer,
                                RingBuffer<TIQSmpFlt> * const ibuffer)
  : mNormalizer(1.0f)
{
  mpMyRadioInterface = mr;
  mpDabSettings      = dabSettings;
  mpSpectrumBuffer   = sbuffer;
  mpIQBuffer         = ibuffer;

  dabSettings->beginGroup("spectrumViewer");
  QString colorString;

  colorString   = dabSettings->value("displayColor", "black").toString();
  mDisplayColor = QColor(colorString);
  colorString   = dabSettings->value("gridColor", "white").toString();
  mGridColor    = QColor(colorString);
  colorString   = dabSettings->value("curveColor", "white").toString();
  mCurveColor   = QColor(colorString);
  const bool brush = dabSettings->value("brush", 0).toInt() == 1;

  mDisplaySize = dabSettings->value("displaySize", 1024).toInt();
  dabSettings->endGroup();

  if ((mDisplaySize & (mDisplaySize - 1)) != 0)
  {
    mDisplaySize = 1024;
  }

  this->mpMyFrame = new QFrame(nullptr);
  setupUi(this->mpMyFrame);

  this->mpMyFrame->hide();
  mDisplayBuffer.resize(mDisplaySize);
  memset(mDisplayBuffer.data(), 0, mDisplaySize * sizeof(double));
  this->mSpectrumSize = 4 * mDisplaySize;

  mpSpectrum = (TIQSmpFlt *)fftwf_malloc(sizeof(fftwf_complex) * mSpectrumSize);

  mPlan = fftwf_plan_dft_1d(mSpectrumSize,
                            reinterpret_cast <fftwf_complex *>(mpSpectrum),
                            reinterpret_cast <fftwf_complex *>(mpSpectrum),
                            FFTW_FORWARD, FFTW_ESTIMATE);

  mpPlotgrid = dabScope;
  mpPlotgrid->setCanvasBackground(mDisplayColor);
  mpGrid = new QwtPlotGrid;

#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
  grid->setMajPen(QPen(gridColor, 0, Qt::DotLine));
#else
  mpGrid->setMajorPen(QPen(mGridColor, 0, Qt::DotLine));
#endif

  mpGrid->enableXMin(true);
  mpGrid->enableYMin(true);

#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
  grid->setMinPen(QPen(gridColor, 0, Qt::DotLine));
#else
  mpGrid->setMinorPen(QPen(mGridColor, 0, Qt::DotLine));
#endif

  mpGrid->attach(mpPlotgrid);

  mpLm_picker = new QwtPlotPicker(dabScope->canvas());
  QwtPickerMachine *lpickerMachine = new QwtPickerClickPointMachine();

  mpLm_picker->setStateMachine(lpickerMachine);
  mpLm_picker->setMousePattern(QwtPlotPicker::MouseSelect1, Qt::RightButton);

  connect(mpLm_picker, SIGNAL(selected(const QPointF&)), this, SLOT(rightMouseClick(const QPointF&)));

  mpSpectrumCurve = new QwtPlotCurve("");
  mpSpectrumCurve->setPen(QPen(mCurveColor, 2.0));
  mpSpectrumCurve->setOrientation(Qt::Horizontal);
  mpSpectrumCurve->setBaseline(get_db(0));

  mpOurBrush = new QBrush(mCurveColor);
  mpOurBrush->setStyle(Qt::Dense3Pattern);

  if (brush)
  {
    mpSpectrumCurve->setBrush(*mpOurBrush);
  }

  mpSpectrumCurve->attach(mpPlotgrid);

  mpMarker = new QwtPlotMarker();
  mpMarker->setLineStyle(QwtPlotMarker::VLine);
  mpMarker->setLinePen(QPen(Qt::red));
  mpMarker->attach(mpPlotgrid);
  mpPlotgrid->enableAxis(QwtPlot::yLeft);

  // generate Blackman window
  mWindow.resize(mSpectrumSize);

  for (int16_t i = 0; i < mSpectrumSize; i++)
  {
    mWindow [i] = 0.42 - 0.50 * cos((2.0 * M_PI * i) / (mSpectrumSize - 1))
                       + 0.08 * cos((4.0 * M_PI * i) / (mSpectrumSize - 1));
  }

  setBitDepth(12);

  mpMyIQDisplay = new IQDisplay(iqDisplay, 256);
}

spectrumViewer::~spectrumViewer()
{
  fftwf_destroy_plan(mPlan);
  fftwf_free(mpSpectrum);
  mpMyFrame->hide();

  delete mpMarker;
  delete mpOurBrush;
  delete mpSpectrumCurve;
  delete mpGrid;
  delete mpMyIQDisplay;
  delete mpMyFrame;
}

void spectrumViewer::showSpectrum(int32_t /*amount*/, int32_t vfoFrequency)
{
  double  X_axis[mDisplaySize];
  double  Y_values[mDisplaySize];
  double  temp = (double)INPUT_RATE / 2 / mDisplaySize;
  int16_t averageCount = 5;

  if (mpSpectrumBuffer->GetRingBufferReadAvailable() < mSpectrumSize)
  {
    return;
  }

  mpSpectrumBuffer->getDataFromBuffer(mpSpectrum, mSpectrumSize);
  mpSpectrumBuffer->FlushRingBuffer();

  if (mpMyFrame->isHidden())
  {
    mpSpectrumBuffer->FlushRingBuffer();
    return;
  }

  // first X axis labels
  for (int16_t i = 0; i < mDisplaySize; i++)
  {
    X_axis [i] = ((double)vfoFrequency - (double)(INPUT_RATE / 2) + (double)((i) * (double)2 * temp)) / ((double)1000);
  }

  // and window it
  // get the buffer data
  for (int16_t i = 0; i < mSpectrumSize; i++)
  {
    if (std::isnan(abs(mpSpectrum [i])) ||
        std::isinf(abs(mpSpectrum [i])))
    {
      mpSpectrum [i] = TIQSmpFlt (0, 0);
    }
    else
    {
      mpSpectrum [i] = cmul(mpSpectrum [i], mWindow [i]);
    }
  }

  fftwf_execute(mPlan);

  const int16_t noSubElem = mSpectrumSize / mDisplaySize;
  
  // and map the spectrumSize values onto displaySize elements
  for (int16_t i = 0; i < mDisplaySize / 2; i++)
  {
    double f = 0;
    for (int16_t j = 0; j < noSubElem; j++)
    {
      f += abs(mpSpectrum [noSubElem * i + j]);
    }

    Y_values [mDisplaySize / 2 + i] = f / noSubElem;

    f = 0;
    for (int16_t j = 0; j < noSubElem; j++)
    {
      f += abs(mpSpectrum [mSpectrumSize / 2 + noSubElem * i + j]);
    }

    Y_values [i] = f / noSubElem;
  }

  // average the image a little.
  for (int16_t i = 0; i < mDisplaySize; i++)
  {
    if (std::isnan(Y_values [i]) || std::isinf(Y_values [i]))
    {
      continue;
    }

    mDisplayBuffer [i] = (double)(averageCount - 1) / averageCount * mDisplayBuffer [i]
                         + 1.0f / averageCount * Y_values [i];
  }

  memcpy(Y_values, mDisplayBuffer.data(), mDisplaySize * sizeof(double));
  ViewSpectrum(X_axis, Y_values, scopeAmplification->value(), vfoFrequency / 1000);
}

void spectrumViewer::ViewSpectrum(double *X_axis,
                                  double *Y1_value,
                                  double amp,
                                  int32_t marker)
{
  float amp1 = amp / 100;

  amp = amp / 100.0 * (-get_db(0));
  mpPlotgrid->setAxisScale(QwtPlot::xBottom,
                           (double)X_axis [0],
                           X_axis [mDisplaySize - 1]);
  mpPlotgrid->enableAxis(QwtPlot::xBottom);
  mpPlotgrid->setAxisScale(QwtPlot::yLeft,
                           get_db(0), get_db(0) + amp);
//				         get_db (0), 0);
  for (int16_t i = 0; i < mDisplaySize; i++)
  {
    Y1_value [i] = get_db(amp1 * Y1_value [i]);
  }

  mpSpectrumCurve->setBaseline(get_db(0));
  Y1_value [0]                = get_db(0);
  Y1_value [mDisplaySize - 1] = get_db(0);

  mpSpectrumCurve->setSamples(X_axis, Y1_value, mDisplaySize);
  mpMarker->setXValue(marker);
  mpPlotgrid->replot();
}

float spectrumViewer::get_db(float x)
{
  return 20 * log10((x + 1) / mNormalizer);
}

void spectrumViewer::setBitDepth(int16_t d)
{
  if (d < 0 || d > 32)
  {
    d = 24;
  }

  mNormalizer = 1.0f;
  while (--d > 0)
  {
    mNormalizer *= 2.0f;
  }
}

void spectrumViewer::show()
{
  mpMyFrame->show();
}

void spectrumViewer::hide()
{
  mpMyFrame->hide();
}

bool spectrumViewer::isHidden()
{
  return mpMyFrame->isHidden();
}

void spectrumViewer::showIQ(int amount)
{
  TIQSmpFlt Values[amount];
  double              avg        = 0;
  const int           scopeWidth = scopeSlider->value();

  const int16_t       t = mpIQBuffer->getDataFromBuffer(Values, amount);

  if (mpMyFrame->isHidden())
  {
    return;
  }

  for (int16_t i = 0; i < t; i++)
  {
    float x = abs(Values [i]);
    if (!std::isnan(x) && !std::isinf(x))
    {
      avg += abs(Values [i]);
    }
  }

  avg /= t;
  mpMyIQDisplay->DisplayIQ(Values, scopeWidth / avg);
}

void spectrumViewer::showQuality(float q, float timeOffset, float sco, float freqOffset)
{
  if (mpMyFrame->isHidden())
  {
    return;
  }

  quality_display->display(q);
  timeOffsetDisplay->display(timeOffset);
  scoOffsetDisplay->display(sco);
  frequencyOffsetDisplay->display(freqOffset);
}

void spectrumViewer::show_clockErr(int e)
{
  if (!mpMyFrame->isHidden())
  {
    clockError->display(e);
  }
}

void spectrumViewer::rightMouseClick(const QPointF &point)
{
  colorSelector *selector;
  int           index;

  selector = new colorSelector("display color");
  index    = selector->QDialog::exec();
  QString displayColor = selector->getColor(index);

  delete selector;
  if (index == 0)
  {
    return;
  }

  selector = new colorSelector("grid color");
  index    = selector->QDialog::exec();
  QString gridColor = selector->getColor(index);

  delete selector;
  if (index == 0)
  {
    return;
  }

  selector = new colorSelector("curve color");
  index    = selector->QDialog::exec();
  QString curveColor = selector->getColor(index);

  delete selector;
  if (index == 0)
  {
    return;
  }

  mpDabSettings->beginGroup("spectrumViewer");
  mpDabSettings->setValue("displayColor", displayColor);
  mpDabSettings->setValue("gridColor", gridColor);
  mpDabSettings->setValue("curveColor", curveColor);
  mpDabSettings->endGroup();

  mDisplayColor = QColor(displayColor);
  mGridColor    = QColor(gridColor);
  mCurveColor   = QColor(curveColor);
  mpSpectrumCurve->setPen(QPen(this->mCurveColor, 2.0));
#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
  mpGrid->setMajPen(QPen(this->gridColor, 0, Qt::DotLine));
#else
  mpGrid->setMajorPen(QPen(this->mGridColor, 0, Qt::DotLine));
#endif
  mpGrid->enableXMin(true);
  mpGrid->enableYMin(true);
#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
  mpGrid->setMinPen(QPen(this->gridColor, 0, Qt::DotLine));
#else
  mpGrid->setMinorPen(QPen(this->mGridColor, 0, Qt::DotLine));
#endif
  mpPlotgrid->setCanvasBackground(this->mDisplayColor);
}

