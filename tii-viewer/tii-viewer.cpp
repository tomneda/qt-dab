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

#include  "tii-viewer.h"
#include  "iqdisplay.h"
#include  <QColor>
#include  <QPen>
#include  "color-selector.h"

tiiViewer::tiiViewer(RadioInterface * mr, QSettings * dabSettings, RingBuffer<TIQSmpFlt> * sbuffer)
  : mMyFrame(nullptr)
  , mSpectrumCurve("")
{
  QString colorString = "black";
  bool    brush;

  mpMyRadioInterface = mr;
  mpDabSettings      = dabSettings;
  mpTiiBuffer        = sbuffer;

  dabSettings->beginGroup("tiiViewer");
  colorString = dabSettings->value("displayColor", "black").toString();
  mDisplayColor = QColor(colorString);
  colorString  = dabSettings->value("gridColor", "black").toString();
  mGridColor   = QColor(colorString);
  colorString = dabSettings->value("curveColor", "white").toString();
  mCurveColor = QColor(colorString);

  brush = (dabSettings->value("brush", 0).toInt() == 1);
  mDisplaySize = dabSettings->value("displaySize", 512).toInt();
  dabSettings->endGroup();

  if ((mDisplaySize & (mDisplaySize - 1)) != 0)
  {
    mDisplaySize = 1024;
  }

  setupUi(&mMyFrame);

  mMyFrame.hide();
  mDisplayBuffer.resize(mDisplaySize);
  memset(mDisplayBuffer.data(), 0, mDisplaySize * sizeof(double));
  mSpectrumSize = 2 * mDisplaySize;
  mpSpectrum = (TIQSmpFlt *)fftwf_malloc(sizeof(fftwf_complex) * mSpectrumSize);
  mPlan = fftwf_plan_dft_1d(mSpectrumSize,
                            reinterpret_cast <fftwf_complex *>(mpSpectrum),
                            reinterpret_cast <fftwf_complex *>(mpSpectrum),
                            FFTW_FORWARD, FFTW_ESTIMATE);

  mpPlotgrid = tiiGrid;
  mpPlotgrid->setCanvasBackground(mDisplayColor);
  //grid = new QwtPlotGrid;

#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
  grid.setMajPen(QPen(gridColor, 0, Qt::DotLine));
#else
  mGrid.setMajorPen(QPen(mGridColor, 0, Qt::DotLine));
#endif

  mGrid.enableXMin(true);
  mGrid.enableYMin(true);

#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
  grid.setMinPen(QPen(gridColor, 0, Qt::DotLine));
#else
  mGrid.setMinorPen(QPen(mGridColor, 0, Qt::DotLine));
#endif

  mGrid.attach(mpPlotgrid);

  mpLm_picker = new QwtPlotPicker(mpPlotgrid->canvas());
  QwtPickerMachine *lpickerMachine = new QwtPickerClickPointMachine();

  mpLm_picker->setStateMachine(lpickerMachine);
  mpLm_picker->setMousePattern(QwtPlotPicker::MouseSelect1, Qt::RightButton);

  connect(mpLm_picker, SIGNAL(selected(const QPointF&)), this, SLOT(rightMouseClick(const QPointF&)));

  //spectrumCurve	= new QwtPlotCurve("");
  mSpectrumCurve.setPen(QPen(mCurveColor, 2.0));
  mSpectrumCurve.setOrientation(Qt::Horizontal);
  mSpectrumCurve.setBaseline(get_db(0));
  //ourBrush = new QBrush (curveColor);
  //ourBrush->setStyle(Qt::Dense3Pattern);

  if (brush)
  {
    QBrush ourBrush(mCurveColor);
    ourBrush.setStyle(Qt::Dense3Pattern);
    mSpectrumCurve.setBrush(ourBrush);
  }

  mSpectrumCurve.attach(mpPlotgrid);

  mpMarker = new QwtPlotMarker();
  mpMarker->setLineStyle(QwtPlotMarker::VLine);
  mpMarker->setLinePen(QPen(Qt::red));
  mpMarker->attach(mpPlotgrid);
  mpPlotgrid->enableAxis(QwtPlot::yLeft);

  // generate Blackman window
  mWindow.resize(mSpectrumSize);

  for (int16_t i = 0; i < mSpectrumSize; i++)
  {
    mWindow [i] = 0.42 - 0.50 * cos((2.0 * M_PI * i) / (mSpectrumSize - 1)) +
                         0.08 * cos((4.0 * M_PI * i) / (mSpectrumSize - 1));
  }

  setBitDepth(12);
}

tiiViewer::~tiiViewer()
{
  fftwf_destroy_plan(mPlan);
  fftwf_free(mpSpectrum);
  mMyFrame.hide();
  delete    mpMarker;
  //delete		ourBrush;
}

void tiiViewer::clear()
{
  transmitterDisplay->setText(" ");
}

static QString tiiNumber(int n)
{
  if (n >= 10)
  {
    return QString::number(n);
  }

  return QString("0") + QString::number(n);
}

void tiiViewer::showTransmitters(const QByteArray & transmitters)
{
  if (mMyFrame.isHidden())
  {
    return;
  }

  if (transmitters.size() == 0)
  {
    transmitterDisplay->setText(" ");
    return;
  }

  QString t = "Transmitter IDs ";

  for (int i = 0; i < transmitters.size(); i += 2)
  {
    uint8_t mainId = transmitters.at(i);
    uint8_t subId  = transmitters.at(i + 1);
    t = t + " (" + tiiNumber(mainId) + "+" + tiiNumber(subId) + ")";
  }

  transmitterDisplay->setText(t);
}

void tiiViewer::showSpectrum(int32_t amount)
{
  (void)amount;
  double  X_axis[mDisplaySize];
  double  Y_values[mDisplaySize];
  int16_t i, j;
  double  temp = (double)INPUT_RATE / 2 / mDisplaySize;
  int16_t averageCount = 3;

  if (mpTiiBuffer->GetRingBufferReadAvailable() < mSpectrumSize)
  {
    return;
  }

  mpTiiBuffer->getDataFromBuffer(mpSpectrum, mSpectrumSize);
  mpTiiBuffer->FlushRingBuffer();

  if (mMyFrame.isHidden())
  {
    return;
  }

  //	and window it
  //	first X axis labels
  for (i = 0; i < mDisplaySize; i++)
  {
    X_axis[i] = ((double)0 - (double)(INPUT_RATE / 2) + (double)((i) * (double)2 * temp)) / ((double)1000);
  }

  //	get the buffer data
  for (i = 0; i < mSpectrumSize; i++)
  {
    mpSpectrum [i] = cmul(mpSpectrum[i], mWindow[i]);
  }

  fftwf_execute(mPlan);

  const int16_t noSubElem = mSpectrumSize / mDisplaySize;

  //	and map the spectrumSize values onto displaySize elements
  for (i = 0; i < mDisplaySize / 2; i++)
  {
    double f = 0;

    for (j = 0; j < noSubElem; j++)
    {
      f += abs(mpSpectrum [noSubElem * i + j]);
    }

    Y_values [mDisplaySize / 2 + i] = f / noSubElem;
    f = 0;

    for (j = 0; j < noSubElem; j++)
    {
      f += abs(mpSpectrum [mSpectrumSize / 2 + noSubElem * i + j]);
    }

    Y_values [i] = f / noSubElem;
  }

  // average the image a little.
  for (i = 0; i < mDisplaySize; i++)
  {
    if (std::isnan(Y_values [i]) || std::isinf(Y_values [i]))
    {
      continue;
    }

    mDisplayBuffer[i] = (double)(averageCount - 1) / averageCount * mDisplayBuffer[i] + 1.0f / averageCount * Y_values [i];
  }

  memcpy(Y_values, mDisplayBuffer.data(), mDisplaySize * sizeof(double));

  ViewSpectrum(X_axis, Y_values, AmplificationSlider->value(), 0 / 1000);
}

void tiiViewer::ViewSpectrum(double *X_axis,
                             double *Y1_value,
                             double amp,
                             int32_t marker)
{
  float amp1 = amp / 100;

  amp = amp / 50.0 * (-get_db(0));
  mpPlotgrid->setAxisScale(QwtPlot::xBottom, (double)X_axis [0],X_axis [mDisplaySize - 1]);
  mpPlotgrid->enableAxis(QwtPlot::xBottom);
  mpPlotgrid->setAxisScale(QwtPlot::yLeft, get_db(0), get_db(0) + amp1 * 40);

  for (int16_t i = 0; i < mDisplaySize; i++)
  {
    Y1_value[i] = get_db(amp1 * Y1_value[i]);
  }

  mSpectrumCurve.setBaseline(get_db(0));
  Y1_value[0]               = get_db(0);
  Y1_value[mDisplaySize - 1] = get_db(0);

  mSpectrumCurve.setSamples(X_axis, Y1_value, mDisplaySize);
  mpMarker->setXValue(marker);
  mpPlotgrid->replot();
}

float tiiViewer::get_db(float x)
{
  return 20 * log10((x + 1) / mNormalizer);
}

void tiiViewer::setBitDepth(int16_t d)
{
  if (d < 0 || d > 32)
  {
    d = 24;
  }

  mNormalizer = 1.0f;

  while (--d > 0)
  {
    mNormalizer *= 2.0;
  }
}

void tiiViewer::show()
{
  mMyFrame.show();
}

void tiiViewer::hide()
{
  mMyFrame.hide();
}

bool tiiViewer::isHidden()
{
  return mMyFrame.isHidden();
}

void tiiViewer::rightMouseClick(const QPointF &point)
{
  (void)point;
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

  mpDabSettings->beginGroup("tiiViewer");
  mpDabSettings->setValue("displayColor", displayColor);
  mpDabSettings->setValue("gridColor", gridColor);
  mpDabSettings->setValue("curveColor", curveColor);
  mpDabSettings->endGroup();

  this->mDisplayColor = QColor(displayColor);
  this->mGridColor    = QColor(gridColor);
  this->mCurveColor   = QColor(curveColor);
  mSpectrumCurve.setPen(QPen(this->mCurveColor, 2.0));

#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
  grid.setMajPen(QPen(this->gridColor, 0, Qt::DotLine));
#else
  mGrid.setMajorPen(QPen(this->mGridColor, 0, Qt::DotLine));
#endif

  mGrid.enableXMin(true);
  mGrid.enableYMin(true);

#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
  grid.setMinPen(QPen(this->gridColor, 0, Qt::DotLine));
#else
  mGrid.setMinorPen(QPen(this->mGridColor, 0, Qt::DotLine));
#endif

  mpPlotgrid->setCanvasBackground(this->mDisplayColor);
}

static double Y_values[2000] = { 0 };

void tiiViewer::show_nullPeriod(const QVector<float> &v, double amp2)
{
  (void)amp2;
  double X_axis[mDisplaySize];
  //double amp = AmplificationSlider->value();


  for (int i = 0; i < mDisplaySize; i++)
  {
    X_axis [i] = i * v.size() / mDisplaySize;;
    double x = 0;

    for (int j = 0; j < v.size() / mDisplaySize; j++)
    {
      x = abs(v [i * v.size() / mDisplaySize + j]);
    }

    Y_values [i] = x;
  }

  float Max = 0;

  for (int i = 0; i < mDisplaySize; i++)
  {
    if (Y_values [i] > Max)
    {
      Max = Y_values [i];
    }
  }

  mpPlotgrid->setAxisScale(QwtPlot::xBottom, (double)X_axis [0], X_axis [mDisplaySize - 1]);
  mpPlotgrid->enableAxis(QwtPlot::xBottom);
  mpPlotgrid->setAxisScale(QwtPlot::yLeft, 0, Max * 2);
  //for (int i = 0; i < displaySize; i ++)
  //   Y_values [i] = get_db (amp * Y_values [i]);

  mSpectrumCurve.setBaseline(0);
  Y_values[0]                = 0;
  Y_values[mDisplaySize - 1] = 0;

  mSpectrumCurve.setSamples(X_axis, Y_values, mDisplaySize);
  mpPlotgrid->replot();
}

