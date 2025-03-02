#
/*
 *    Copyright (C) 2013 .. 2017
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
 *    along with Qt-SDR; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * 	This particular driver is a very simple wrapper around the
 * 	librtlsdr.  In order to keep things simple, we dynamically
 * 	load the dll (or .so). The librtlsdr is osmocom software and all rights
 * 	are greatly acknowledged
 */

#include	<QThread>
#include	<QFileDialog>
#include	<QTime>
#include	<QDate>
#include	<QDir>
#include	"rtlsdr-handler.h"
#include	"rtl-dongleselect.h"
#include	"rtl-sdr.h"
#include	"xml-filewriter.h"
#include	"device-exceptions.h"
//#define	__X_VERSION_ 1

#define	READLEN_DEFAULT	(4 * 8192)
//
//	For the callback, we do need some environment which
//	is passed through the ctx parameter
//
//	This is the user-side call back function
//	ctx is the calling task

static 
float mapTable [] = {
 -128 / 128.0 , -127 / 128.0 , -126 / 128.0 , -125 / 128.0 , -124 / 128.0 , -123 / 128.0 , -122 / 128.0 , -121 / 128.0 , -120 / 128.0 , -119 / 128.0 , -118 / 128.0 , -117 / 128.0 , -116 / 128.0 , -115 / 128.0 , -114 / 128.0 , -113 / 128.0 
, -112 / 128.0 , -111 / 128.0 , -110 / 128.0 , -109 / 128.0 , -108 / 128.0 , -107 / 128.0 , -106 / 128.0 , -105 / 128.0 , -104 / 128.0 , -103 / 128.0 , -102 / 128.0 , -101 / 128.0 , -100 / 128.0 , -99 / 128.0 , -98 / 128.0 , -97 / 128.0 
, -96 / 128.0 , -95 / 128.0 , -94 / 128.0 , -93 / 128.0 , -92 / 128.0 , -91 / 128.0 , -90 / 128.0 , -89 / 128.0 , -88 / 128.0 , -87 / 128.0 , -86 / 128.0 , -85 / 128.0 , -84 / 128.0 , -83 / 128.0 , -82 / 128.0 , -81 / 128.0 
, -80 / 128.0 , -79 / 128.0 , -78 / 128.0 , -77 / 128.0 , -76 / 128.0 , -75 / 128.0 , -74 / 128.0 , -73 / 128.0 , -72 / 128.0 , -71 / 128.0 , -70 / 128.0 , -69 / 128.0 , -68 / 128.0 , -67 / 128.0 , -66 / 128.0 , -65 / 128.0 
, -64 / 128.0 , -63 / 128.0 , -62 / 128.0 , -61 / 128.0 , -60 / 128.0 , -59 / 128.0 , -58 / 128.0 , -57 / 128.0 , -56 / 128.0 , -55 / 128.0 , -54 / 128.0 , -53 / 128.0 , -52 / 128.0 , -51 / 128.0 , -50 / 128.0 , -49 / 128.0 
, -48 / 128.0 , -47 / 128.0 , -46 / 128.0 , -45 / 128.0 , -44 / 128.0 , -43 / 128.0 , -42 / 128.0 , -41 / 128.0 , -40 / 128.0 , -39 / 128.0 , -38 / 128.0 , -37 / 128.0 , -36 / 128.0 , -35 / 128.0 , -34 / 128.0 , -33 / 128.0 
, -32 / 128.0 , -31 / 128.0 , -30 / 128.0 , -29 / 128.0 , -28 / 128.0 , -27 / 128.0 , -26 / 128.0 , -25 / 128.0 , -24 / 128.0 , -23 / 128.0 , -22 / 128.0 , -21 / 128.0 , -20 / 128.0 , -19 / 128.0 , -18 / 128.0 , -17 / 128.0 
, -16 / 128.0 , -15 / 128.0 , -14 / 128.0 , -13 / 128.0 , -12 / 128.0 , -11 / 128.0 , -10 / 128.0 , -9 / 128.0 , -8 / 128.0 , -7 / 128.0 , -6 / 128.0 , -5 / 128.0 , -4 / 128.0 , -3 / 128.0 , -2 / 128.0 , -1 / 128.0 
, 0 / 128.0 , 1 / 128.0 , 2 / 128.0 , 3 / 128.0 , 4 / 128.0 , 5 / 128.0 , 6 / 128.0 , 7 / 128.0 , 8 / 128.0 , 9 / 128.0 , 10 / 128.0 , 11 / 128.0 , 12 / 128.0 , 13 / 128.0 , 14 / 128.0 , 15 / 128.0 
, 16 / 128.0 , 17 / 128.0 , 18 / 128.0 , 19 / 128.0 , 20 / 128.0 , 21 / 128.0 , 22 / 128.0 , 23 / 128.0 , 24 / 128.0 , 25 / 128.0 , 26 / 128.0 , 27 / 128.0 , 28 / 128.0 , 29 / 128.0 , 30 / 128.0 , 31 / 128.0 
, 32 / 128.0 , 33 / 128.0 , 34 / 128.0 , 35 / 128.0 , 36 / 128.0 , 37 / 128.0 , 38 / 128.0 , 39 / 128.0 , 40 / 128.0 , 41 / 128.0 , 42 / 128.0 , 43 / 128.0 , 44 / 128.0 , 45 / 128.0 , 46 / 128.0 , 47 / 128.0 
, 48 / 128.0 , 49 / 128.0 , 50 / 128.0 , 51 / 128.0 , 52 / 128.0 , 53 / 128.0 , 54 / 128.0 , 55 / 128.0 , 56 / 128.0 , 57 / 128.0 , 58 / 128.0 , 59 / 128.0 , 60 / 128.0 , 61 / 128.0 , 62 / 128.0 , 63 / 128.0 
, 64 / 128.0 , 65 / 128.0 , 66 / 128.0 , 67 / 128.0 , 68 / 128.0 , 69 / 128.0 , 70 / 128.0 , 71 / 128.0 , 72 / 128.0 , 73 / 128.0 , 74 / 128.0 , 75 / 128.0 , 76 / 128.0 , 77 / 128.0 , 78 / 128.0 , 79 / 128.0 
, 80 / 128.0 , 81 / 128.0 , 82 / 128.0 , 83 / 128.0 , 84 / 128.0 , 85 / 128.0 , 86 / 128.0 , 87 / 128.0 , 88 / 128.0 , 89 / 128.0 , 90 / 128.0 , 91 / 128.0 , 92 / 128.0 , 93 / 128.0 , 94 / 128.0 , 95 / 128.0 
, 96 / 128.0 , 97 / 128.0 , 98 / 128.0 , 99 / 128.0 , 100 / 128.0 , 101 / 128.0 , 102 / 128.0 , 103 / 128.0 , 104 / 128.0 , 105 / 128.0 , 106 / 128.0 , 107 / 128.0 , 108 / 128.0 , 109 / 128.0 , 110 / 128.0 , 111 / 128.0 
, 112 / 128.0 , 113 / 128.0 , 114 / 128.0 , 115 / 128.0 , 116 / 128.0 , 117 / 128.0 , 118 / 128.0 , 119 / 128.0 , 120 / 128.0 , 121 / 128.0 , 122 / 128.0 , 123 / 128.0 , 124 / 128.0 , 125 / 128.0 , 126 / 128.0 , 127 / 128.0 };

static
void	RTLSDRCallBack (uint8_t *buf, uint32_t len, void *ctx) {
rtlsdrHandler	*theStick	= (rtlsdrHandler *)ctx;

	if ((theStick == nullptr) || (len != READLEN_DEFAULT)) {
	   fprintf (stderr, "%d \n", len);
	   return;
	}

	if (theStick -> isActive. load ()) {
	   if (theStick -> _I_Buffer. GetRingBufferWriteAvailable () < len / 2)
	      fprintf (stderr, "xx? ");
	   (void)theStick -> _I_Buffer.
	             putDataIntoBuffer ((std::complex<uint8_t> *)buf, len / 2);
	}
}
//
//	for handling the events in libusb, we need a controlthread
//	whose sole purpose is to process the rtlsdr_read_async function
//	from the lib.
class	dll_driver : public QThread {
private:
	rtlsdrHandler	*theStick;
public:

	dll_driver (rtlsdrHandler *d) {
	theStick	= d;
	start();
}

	~dll_driver() {
}

private:
void	run () {
	rtlsdr_read_async (theStick -> theDevice,
	                   (rtlsdr_read_async_cb_t)&RTLSDRCallBack,
	                   (void *)theStick,
	                   0,
	                   READLEN_DEFAULT);
	fprintf (stderr, "dll_task terminates\n");
	}
};
//
//	Our wrapper is a simple classs
	rtlsdrHandler::rtlsdrHandler (QSettings *s,
	                              QString &recorderVersion):
	                                 _I_Buffer (8 * 1024 * 1024),
	                                 myFrame (nullptr),
	                                 theFilter (5, 1560000 / 2, 2048000) {
int16_t	deviceCount;
int32_t	r;
int16_t	deviceIndex;
int16_t	i;
QString	temp;
int	k;
char	manufac [256], product [256], serial [256];

	rtlsdrSettings			= s;
	this	-> recorderVersion	= recorderVersion;
	rtlsdrSettings	-> beginGroup ("rtlsdrSettings");
	int x   = rtlsdrSettings -> value ("position-x", 100). toInt ();
        int y   = rtlsdrSettings -> value ("position-y", 100). toInt ();
        rtlsdrSettings -> endGroup ();
        setupUi (&myFrame);
        myFrame. move (QPoint (x, y));
	myFrame. show();
	filtering			= false;

	rtlsdrSettings	-> beginGroup ("rtlsdrSettings");
	currentDepth	= rtlsdrSettings -> value ("filterDepth", 5). toInt ();
	rtlsdrSettings	-> endGroup ();
	filterDepth	-> setValue (currentDepth);
	theFilter. resize (currentDepth);

	inputRate		= 2048000;
	workerHandle		= nullptr;
	isActive. store (false);

//	Ok, from here we have the library functions accessible
	deviceCount 		= rtlsdr_get_device_count ();
	if (deviceCount == 0) {
	   throw (rtlsdr_exception ("No device found\n"));
	}

	deviceIndex = 0;	// default
	if (deviceCount > 1) {
	   rtl_dongleSelect dongleSelector;
	   for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex ++) {
	      dongleSelector.
	           addtoDongleList (rtlsdr_get_device_name (deviceIndex));
	   }
	   deviceIndex = dongleSelector. QDialog::exec();
	}
//
//	OK, now open the hardware
	r		=  rtlsdr_open (&theDevice, deviceIndex);
	if (r < 0) {
	   throw (rtlsdr_exception ("Opening rtlsdr device failed"));
	}

	deviceModel	= rtlsdr_get_device_name (deviceIndex);
	deviceVersion	-> setText (deviceModel);
	r		= rtlsdr_set_sample_rate (theDevice, inputRate);
	if (r < 0) {
	   throw (rtlsdr_exception ("Setting samplerate failed\n"));
	}

	gainsCount = rtlsdr_get_tuner_gains (theDevice, nullptr);
	fprintf (stderr, "Supported gain values (%d): ", gainsCount);
	{  int gains [gainsCount];
	   gainsCount	= rtlsdr_get_tuner_gains (theDevice, gains);
	   for (i = gainsCount; i > 0; i--) {
	      fprintf (stderr, "%.1f ", gains [i - 1] / 10.0);
	      gainControl -> addItem (QString::number (gains [i - 1]));
	   }
	   fprintf (stderr, "\n");
	}

	rtlsdr_set_tuner_bandwidth (theDevice, KHz (1536));
	rtlsdr_set_tuner_gain_mode (theDevice, 1);
//
//	See what the saved values are and restore the GUI settings
	rtlsdrSettings	-> beginGroup ("rtlsdrSettings");
	temp	= rtlsdrSettings -> value ("externalGain", "10"). toString();
	k	= gainControl -> findText (temp);
	gainControl	-> setCurrentIndex (k != -1 ? k : gainsCount / 2);

	temp		= rtlsdrSettings -> value ("autogain",
	                                      "autogain_on"). toString();
	agcControl	-> setChecked (temp == "autogain_on");
	
	ppm_correction	->
	     setValue (rtlsdrSettings -> value ("ppm_correction", 0). toInt());
	save_gainSettings	=
	     rtlsdrSettings -> value ("save_gainSettings", 1). toInt () != 0;
	rtlsdrSettings	-> endGroup();

	rtlsdr_get_usb_strings (theDevice, manufac, product, serial);
	fprintf (stderr, "%s %s %s\n",
	            manufac, product, serial);

//	all sliders/values are set to previous values, now do the settings
//	based on these slider values
	if (agcControl -> isChecked ())
	   rtlsdr_set_agc_mode (theDevice, 1);
	else
	   rtlsdr_set_agc_mode (theDevice, 0);
	rtlsdr_set_tuner_gain	(theDevice, 
	                         gainControl -> currentText (). toInt ());
	set_ppmCorrection	(ppm_correction -> value());

//	and attach the buttons/sliders to the actions
	connect (gainControl, SIGNAL (activated (const QString &)),
	         this, SLOT (set_ExternalGain (const QString &)));
	connect (agcControl, SIGNAL (stateChanged (int)),
	         this, SLOT (set_autogain (int)));
	connect (ppm_correction, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ppmCorrection  (int)));
	connect (xml_dumpButton, SIGNAL (clicked ()),
	         this, SLOT (set_xmlDump ()));
	connect (iq_dumpButton, SIGNAL (clicked ()),
	         this, SLOT (set_iqDump ()));
	connect (biasControl, SIGNAL (stateChanged (int)),
	         this, SLOT (set_biasControl (int)));
	connect (filterSelector, SIGNAL (stateChanged (int)),
	         this, SLOT (set_filter (int)));
	xmlDumper       = nullptr;
//
//	and for saving/restoring the gain setting:
	connect (this, SIGNAL (new_gainIndex (int)),
	         gainControl, SLOT (setCurrentIndex (int)));
	connect (this, SIGNAL (new_agcSetting (bool)),
	         agcControl, SLOT (setChecked (bool)));
	iqDumper	= nullptr;
	iq_dumping. store (false);
	xml_dumping. store (false);
}

	rtlsdrHandler::~rtlsdrHandler() {
	stopReader	();
	if (workerHandle != nullptr) {
	   rtlsdr_cancel_async (theDevice);
	   while (!workerHandle -> isFinished()) 
	      usleep (200);
	   _I_Buffer. FlushRingBuffer();
	   delete	workerHandle;
	   workerHandle	= nullptr;
//	   rtlsdr_clode (theDevice);
	}
	rtlsdrSettings	-> beginGroup ("rtlsdrSettings");
        rtlsdrSettings	-> setValue ("position-x", myFrame. pos (). x ());
        rtlsdrSettings	-> setValue ("position-y", myFrame. pos (). y ());

	rtlsdrSettings	-> setValue ("externalGain",
	                              gainControl -> currentText());
	rtlsdrSettings	-> setValue ("autogain",
	                              agcControl -> isChecked () ? "1" : "0");
	rtlsdrSettings	-> setValue ("ppm_correction",
	                              ppm_correction -> value());
	rtlsdrSettings	-> setValue ("filterDepth", filterDepth -> value ());
	rtlsdrSettings	-> sync ();
	rtlsdrSettings	-> endGroup();
	usleep (1000);
	myFrame. hide ();
}

void	rtlsdrHandler::setVFOFrequency	(int32_t f) {
	(void)(rtlsdr_set_center_freq (theDevice, f));
}

int32_t	rtlsdrHandler::getVFOFrequency () {
	return (int32_t)(rtlsdr_get_center_freq (theDevice));
}
//
void	rtlsdrHandler::set_filter	(int c) {
	(void)c;
	filtering       = filterSelector -> isChecked ();
}

bool	rtlsdrHandler::restartReader	(int32_t freq) {
	_I_Buffer. FlushRingBuffer();

	(void)(rtlsdr_set_center_freq (theDevice, freq));
	if (save_gainSettings)
	   update_gainSettings (freq / MHz (1));

	set_autogain (agcControl -> isChecked ());
	set_ExternalGain (gainControl -> currentText ());
	if (workerHandle == nullptr) {
	   (void)rtlsdr_reset_buffer (theDevice);
	   workerHandle	= new dll_driver (this);
	}
	isActive. store (true);
	return true;
}

void	rtlsdrHandler::stopReader () {
	isActive. store (false);
	_I_Buffer. FlushRingBuffer();
	close_xmlDump ();
	if (save_gainSettings)
	   record_gainSettings	((int32_t)(rtlsdr_get_center_freq (theDevice)) / MHz (1));
}
//
//	when selecting  the gain from a table, use the table value
void	rtlsdrHandler::set_ExternalGain	(const QString &gain) {
	rtlsdr_set_tuner_gain (theDevice, gain. toInt());
}
//
void	rtlsdrHandler::set_autogain	(int dummy) {
	(void)dummy;
	rtlsdr_set_agc_mode (theDevice, agcControl -> isChecked () ? 1 : 0);
	rtlsdr_set_tuner_gain (theDevice, 
	                       gainControl -> currentText (). toInt ());
}
//
void	rtlsdrHandler::set_biasControl	(int dummy) {
	(void)dummy;
	rtlsdr_set_bias_tee (theDevice, biasControl -> isChecked () ? 1 : 0);
}
//	correction is in Hz
void	rtlsdrHandler::set_ppmCorrection	(int32_t ppm) {
	rtlsdr_set_freq_correction (theDevice, ppm);
}

int32_t	rtlsdrHandler::getSamples (cmplx *V, int32_t size) {
std::complex<uint8_t> temp [size];
int	amount;
static uint8_t dumpBuffer [4096];
static int iqTeller	= 0;

	if (!isActive. load ())
	   return 0;

	amount = _I_Buffer. getDataFromBuffer (temp, size);
	if (filtering) {
	   if (filterDepth -> value () != currentDepth) {
	      currentDepth = filterDepth -> value ();
	      theFilter. resize (currentDepth);
	   }
	   for (int i = 0; i < amount; i ++) 
	      V [i] = theFilter. Pass (
	               cmplx (mapTable [real (temp [i]) & 0xFF],
	                                    mapTable [imag (temp [i]) & 0xFF]));
	}
	else
	   for (int i = 0; i < amount; i ++) 
	      V [i] = cmplx (mapTable [real (temp [i]) & 0xFF],
	                                   mapTable [imag (temp [i]) & 0xFF]);
	if (xml_dumping. load ())
	   xmlWriter -> add (temp, amount);
	else
	if (iq_dumping. load ()) {
	   for (int i = 0; i < size; i ++) {
	      dumpBuffer [iqTeller]	= real (temp [i]);
	      dumpBuffer [iqTeller + 1]	= imag (temp [i]);
	      iqTeller += 2;
	      if (iqTeller >= 4096) {
	         fwrite (dumpBuffer, 1, 4096, iqDumper);
	         iqTeller = 0;
	      }
	   }
	}
	return amount;
}

int32_t	rtlsdrHandler::Samples () {
	if (!isActive. load ())
	   return 0;
	return _I_Buffer. GetRingBufferReadAvailable ();
}
//

void	rtlsdrHandler::resetBuffer() {
	_I_Buffer. FlushRingBuffer();
}

int16_t	rtlsdrHandler::maxGain() {
	return gainsCount;
}

int16_t	rtlsdrHandler::bitDepth() {
	return 8;
}

QString	rtlsdrHandler::deviceName	() {
	return deviceModel;
}

void	rtlsdrHandler::show		() {
	myFrame. show ();
}

void	rtlsdrHandler::hide		() {
	myFrame. hide ();
}

bool	rtlsdrHandler::isHidden		() {
	return myFrame. isHidden ();
}

void	rtlsdrHandler::set_iqDump	() {
	if (iqDumper == nullptr) {
	   if (setup_iqDump ()) {
	      iq_dumpButton	-> setText ("writing raw file");
	      xml_dumpButton	-> hide ();
	   }
	}
	else {
	   close_iqDump ();
	   iq_dumpButton	-> setText ("Dump to raw");
	   xml_dumpButton	-> show ();
	}
}

bool	rtlsdrHandler::setup_iqDump () {
	QString fileName = QFileDialog::getSaveFileName (nullptr,
	                                         tr ("Save file ..."),
	                                         QDir::homePath(),
	                                         tr ("raw (*.raw)"));
	fileName        = QDir::toNativeSeparators (fileName);
	iqDumper	= fopen (fileName. toUtf8 (). data (), "w");
	if (iqDumper == nullptr)
	   return false;
	
	iq_dumping. store (true);
	return true;
}

void	rtlsdrHandler::close_iqDump () {
	if (iqDumper == nullptr)	// this can happen !!
	   return;
	iq_dumping. store (false);
	fclose (iqDumper);
	iqDumper	= nullptr;
}
	   
void	rtlsdrHandler::set_xmlDump () {
	if (!xml_dumping. load ()) {
	  if (setup_xmlDump ())
	      xml_dumpButton	-> setText ("writing xml file");
	  iq_dumpButton	-> hide	();
	}
	else {
	   close_xmlDump ();
	   xml_dumpButton	-> setText ("Dump to xml");
	   iq_dumpButton	-> show	();
	}
}

static inline
bool	isValid (QChar c) {
	return c. isLetterOrNumber () || (c == '-');
}

bool	rtlsdrHandler::setup_xmlDump () {
QTime	theTime;
QDate	theDate;
QString saveDir = rtlsdrSettings -> value ("saveDir_xmlDump",
	                                   QDir::homePath ()). toString ();

	if (xml_dumping. load ())
	   return false;

	if ((saveDir != "") && (!saveDir. endsWith ("/")))
	   saveDir += "/";

	QString channel		= rtlsdrSettings -> value ("channel", "xx").
	                                                      toString ();
	QString timeString      = theDate. currentDate (). toString () + "-" +
	                          theTime. currentTime(). toString ();
	for (int i = 0; i < timeString. length (); i ++)
	   if (!isValid (timeString. at (i)))
	      timeString. replace (i, 1, '-');
	QString suggestedFileName =
	            saveDir + deviceModel + "-" + channel + "-" + timeString;
	QString fileName =
	           QFileDialog::getSaveFileName (nullptr,
	                                         tr ("Save file ..."),
	                                         suggestedFileName + ".uff",
	                                         tr ("Xml (*.uff)"));
	fileName        = QDir::toNativeSeparators (fileName);
	xmlDumper	= fopen (fileName. toUtf8(). data(), "w");
	if (xmlDumper == nullptr)
	   return false;
	
	xmlWriter	= new xml_fileWriter (xmlDumper,
	                                      8,
	                                      "uint8",
	                                      2048000,
	                                      getVFOFrequency (),
	                                      "rtlsdr",
	                                      deviceModel,
	                                      recorderVersion);
	xml_dumping. store (true);

	QString	dumper	= QDir::fromNativeSeparators (fileName);
	int x		= dumper. lastIndexOf ("/");
	saveDir		= dumper. remove (x, dumper. count () - x);
	rtlsdrSettings	-> setValue ("saveDir_xmlDump", saveDir);

	return true;
}

void	rtlsdrHandler::close_xmlDump () {
	if (!xml_dumping. load ())
	   return;
	if (xmlDumper == nullptr)	// cannot happen
	   return;
	xml_dumping. store (false);
	usleep (1000);
	xmlWriter	-> computeHeader ();
	delete xmlWriter;
	fclose (xmlDumper);
	xmlDumper	= nullptr;
}

////////////////////////////////////////////////////////////////////////
//
//      the frequency (the MHz component) is used as key
//
void    rtlsdrHandler::record_gainSettings (int freq) {
QString	gain	= gainControl	-> currentText ();
int	agc	= agcControl	-> isChecked () ? 1 : 0;
QString theValue        = gain + ":" + QString::number (agc);

	rtlsdrSettings         -> beginGroup ("rtlsdrSettings");
	rtlsdrSettings         -> setValue (QString::number (freq), theValue);
	rtlsdrSettings         -> endGroup ();
}

void	rtlsdrHandler::update_gainSettings (int freq) {
int	agc;
QString	theValue	= "";

	rtlsdrSettings	-> beginGroup ("rtlsdrSettings");
	theValue	= rtlsdrSettings -> value (QString::number (freq), ""). toString ();
	rtlsdrSettings	-> endGroup ();

	if (theValue == QString (""))
	   return;		// or set some defaults here

	QStringList result	= theValue. split (":");
	if (result. size () != 2) 	// should not happen
	   return;

	QString temp = result. at (0);
	agc	= result. at (1). toInt ();

	int k	= gainControl -> findText (temp);
	if (k != -1) {
	   gainControl	-> blockSignals (true);
	   new_gainIndex (k);
	   while (gainControl -> currentText () != temp)
	      usleep (1000);
	   gainControl	-> blockSignals (false);
	}

	agcControl	-> blockSignals (true);
	new_agcSetting (agc == 1);
	while (agcControl -> isChecked () != (agc == 1))
	   usleep (1000);
	set_autogain (agcControl -> isChecked ());
	agcControl	-> blockSignals (false);
}

