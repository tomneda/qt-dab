
/**
 *  IW0HDV Extio
 *
 *  Copyright 2015 by Andrea Montefusco IW0HDV
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 *
 *	recoding, taking parts and extending for the airspyHandler interface
 *	for the Qt-DAB program
 *	jan van Katwijk
 *	Lazy Chair Computing
 */

#ifdef	__MINGW32__
#define	GETPROCADDRESS	GetProcAddress
#else
#define	GETPROCADDRESS	dlsym
#endif

#include	<QFileDialog>
#include	<QTime>
#include	<QDate>
#include	"airspy-handler.h"
#include	"airspyselect.h"
#include	"xml-filewriter.h"

static
const	int	EXTIO_NS	=  8192;
static
const	int	EXTIO_BASE_TYPE_SIZE = sizeof (float);

	airspyHandler::airspyHandler (QSettings *s,
	                              QString recorderVersion):
	                                 myFrame (nullptr),	
                                         _I_Buffer (4 * 1024 * 1024) {
int	result, i;
int	distance	= 1000000;
std::vector <uint32_t> sampleRates;
uint32_t samplerateCount;

	this	-> airspySettings	= s;
	this	-> recorderVersion	= recorderVersion;
	setupUi (&myFrame);
	myFrame. show		();

	airspySettings		-> beginGroup ("airspySettings");
	int16_t temp 		= airspySettings -> value ("linearity", 10).
	                                                             toInt();
	linearitySlider		-> setValue (temp);
	linearityDisplay	-> display  (temp);
	temp			= airspySettings -> value ("sensitivity", 10).
	                                                             toInt();
	sensitivitySlider	-> setValue (temp);
	sensitivityDisplay	-> display (temp);

	vgaGain			=
	           airspySettings -> value ("vga", 5).toInt();
	vgaSlider		-> setValue (vgaGain);
	vgaDisplay		-> display (vgaGain);
	mixerGain		=
	           airspySettings -> value ("mixer", 10). toInt();
	mixerSlider		-> setValue (mixerGain);
	mixerDisplay		-> display (mixerGain);
	mixer_agc		= false;
	lnaGain			=
	           airspySettings -> value ("lna", 5). toInt();
	lnaSlider		-> setValue (lnaGain);
	lnaDisplay		-> display  (lnaGain);
	mixer_agc		= false;
	lna_agc			= false;
	rf_bias			= false;
	coarseOffset		=
	           airspySettings -> value ("airspyOffset", 0). toInt();
	save_gainSettings	=
	          airspySettings -> value ("save_gainSettings", 1). toInt () != 0;
	airspySettings	-> endGroup();

	theFilter		= nullptr;
	device			= nullptr;
	serialNumber		= 0;
#ifdef	__MINGW32__
	const char *libraryString = "airspy.dll";
	Handle		= LoadLibrary ((wchar_t *)L"airspy.dll");
#else
	const char *libraryString = "libairspy.so";
	Handle		= dlopen ("libairspy.so", RTLD_LAZY);
	if (Handle == nullptr)
	   Handle	= dlopen ("libairspy.so.0", RTLD_LAZY);
#endif

	if (Handle == nullptr) {
	   fprintf (stderr, "failed to open %s\n", libraryString);
#ifndef	__MINGW32__
	   fprintf (stderr, "Error = %s\n", dlerror());
#endif
	   throw (20);
	}

	libraryLoaded	= true;

	if (!load_airspyFunctions()) {
	   fprintf (stderr, "problem in loading functions\n");
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	}
//
	strcpy (serial,"");
	result = this -> my_airspy_init ();
	if (result != AIRSPY_SUCCESS) {
	   printf ("my_airspy_init() failed: %s (%d)\n",
	             my_airspy_error_name((airspy_error)result), result);
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (21);
	}

	uint64_t deviceList [4];
	int	deviceIndex;
	int numofDevs = my_airspy_list_devices (deviceList, 4);
	fprintf (stderr, "we have %d devices\n", numofDevs);
	if (numofDevs == 0) {
	   fprintf (stderr, "No devices found\n");
	   throw (22);
	}
	if (numofDevs > 1) {
           airspySelect deviceSelector;
           for (deviceIndex = 0; deviceIndex < (int)numofDevs; deviceIndex ++) {
              deviceSelector.
                   addtoList (QString::number (deviceList [deviceIndex]));
           }
           deviceIndex = deviceSelector. QDialog::exec();
        }
	else
	   deviceIndex = 0;
	
	result = my_airspy_open (&device, deviceList [deviceIndex]);
	if (result != AIRSPY_SUCCESS) {
	   printf ("my_airpsy_open() failed: %s (%d)\n",
	             my_airspy_error_name ((airspy_error)result), result);
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (22);
	}

	(void) my_airspy_set_sample_type (device, AIRSPY_SAMPLE_INT16_IQ);
	(void) my_airspy_get_samplerates (device, &samplerateCount, 0);
	fprintf (stderr, "%d samplerates are supported\n", samplerateCount); 
	sampleRates. resize (samplerateCount);
	my_airspy_get_samplerates (device,
	                            sampleRates. data(), samplerateCount);

	selectedRate	= 0;
	for (i = 0; i < (int)samplerateCount; i ++) {
	   fprintf (stderr, "%d \n", sampleRates [i]);
	   if (abs ((int)sampleRates [i] - 2048000) < distance) {
	      distance	= abs ((int)sampleRates [i] - 2048000);
	      selectedRate = sampleRates [i];
	   }
	}

	if (selectedRate == 0) {
	   fprintf (stderr, "Sorry. cannot help you\n");
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (23);
	}
	else
	   fprintf (stderr, "selected samplerate = %d\n", selectedRate);
	airspySettings    -> beginGroup ("airspySettings");
        currentDepth    = airspySettings  -> value ("filterDepth", 5). toInt ();
        airspySettings    -> endGroup ();
        filterDepth     -> setValue (currentDepth);
	theFilter	= new LowPassFIR (currentDepth, 1560000 / 2, selectedRate);
	filtering	= false;
	result = my_airspy_set_samplerate (device, selectedRate);
	if (result != AIRSPY_SUCCESS) {
           printf("airspy_set_samplerate() failed: %s (%d)\n",
	             my_airspy_error_name ((enum airspy_error)result), result);
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   throw (24);
	}

//	airspySettings	-> beginGroup ("airspySettings");
//	airspySettings	-> endGroup();

//	The sizes of the mapTables follow from the input and output rate
//	(selectedRate / 1000) vs (2048000 / 1000)
//	so we end up with buffers with 1 msec content
	convBufferSize		= selectedRate / 1000;
	for (i = 0; i < 2048; i ++) {
	   float inVal	= float (selectedRate / 1000);
	   mapTable_int [i]	= int (floor (i * (inVal / 2048.0)));
	   mapTable_float [i]	= i * (inVal / 2048.0) - mapTable_int [i];
	}
	convIndex	= 0;
	convBuffer. resize (convBufferSize + 1);
//
	tabWidget	-> setCurrentIndex (0);
	set_vga_gain	(vgaSlider	-> value ());
	set_lna_gain	(lnaSlider	-> value ());
	set_mixer_gain	(mixerSlider	-> value ());
	connect (linearitySlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_linearity (int)));
	connect (sensitivitySlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_sensitivity (int)));
	connect (lnaSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_lna_gain (int)));
	connect (vgaSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_vga_gain (int)));
	connect (mixerSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_mixer_gain (int)));
	connect (lnaButton, SIGNAL (stateChanged (int)),
	         this, SLOT (set_lna_agc (int)));
	connect (mixerButton, SIGNAL (stateChanged (int)),
	         this, SLOT (set_mixer_agc (int)));
	connect (biasButton, SIGNAL (stateChanged (int)),
	         this, SLOT (set_rf_bias (int)));
	connect (tabWidget, SIGNAL (currentChanged (int)),
	         this, SLOT (show_tab (int)));
	connect (dumpButton, SIGNAL (clicked ()),
	         this, SLOT (set_xmlDump ()));
	connect (filterSelector, SIGNAL (stateChanged (int)),
	         this, SLOT (set_filter (int)));
//
//	and to restore settings, we need
	connect (this, SIGNAL (new_lnaGainValue (int)),
	         lnaSlider, SLOT (setValue (int)));
	connect (this, SIGNAL (new_vgaGainValue (int)),
	         vgaSlider, SLOT (setValue (int)));
	connect (this, SIGNAL (new_mixerValue (int)),
	         mixerSlider, SLOT (setValue (int)));
	connect (this, SIGNAL (new_linearityValue (int)),
	         linearitySlider, SLOT (setValue (int)));
	connect (this, SIGNAL (new_sensitivityValue (int)),
	         sensitivitySlider, SLOT (setValue (int)));
	connect (this, SIGNAL (new_tabSetting (int)),
	         tabWidget, SLOT (setCurrentIndex (int)));
	connect (this, SIGNAL (new_lnaDisplay (int)),
	         lnaDisplay, SLOT (display (int)));
	connect (this, SIGNAL (new_vgaDisplay (int)),
	         vgaDisplay, SLOT (display (int)));
	connect (this, SIGNAL (new_mixerDisplay (int)),
	         mixerDisplay, SLOT (display (int)));
	connect (this, SIGNAL (new_sensitivityDisplay (int)),
	         sensitivityDisplay, SLOT (display (int)));
	connect (this, SIGNAL (new_linearityDisplay (int)),
	         linearityDisplay, SLOT (display (int)));
	displaySerial	-> setText (getSerial());
	running. store (false);
	my_airspy_set_rf_bias (device, rf_bias ? 1 : 0);

	show_tab (0);			// will set currentTab
	dumping. store (false);
	xmlDumper	= nullptr;
}

	airspyHandler::~airspyHandler() {
	stopReader ();
	myFrame. hide ();
	filtering	= false;
	airspySettings	-> beginGroup ("airspySettings");
	airspySettings -> setValue ("linearity", linearitySlider -> value());
	airspySettings -> setValue ("sensitivity", sensitivitySlider -> value());
	airspySettings -> setValue ("vga", vgaGain);
	airspySettings -> setValue ("mixer", mixerGain);
	airspySettings -> setValue ("lna", lnaGain);
	airspySettings	-> setValue ("airspyOffset", coarseOffset);
	airspySettings	-> setValue ("filterDepth", filterDepth -> value ());
	airspySettings	-> endGroup();
	if (device != nullptr) {
	   int result = my_airspy_stop_rx (device);
	   if (result != AIRSPY_SUCCESS) {
	      printf ("my_airspy_stop_rx() failed: %s (%d)\n",
	             my_airspy_error_name((airspy_error)result), result);
	   }

	   result = my_airspy_close (device);
	   if (result != AIRSPY_SUCCESS) {
	      printf ("airspy_close() failed: %s (%d)\n",
	             my_airspy_error_name((airspy_error)result), result);
	   }
	}
	if (Handle == nullptr) {
	   return;	// nothing achieved earlier
	}
	if (theFilter != nullptr)
	   delete theFilter;
	my_airspy_exit();
#ifdef __MINGW32__
	FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
err:;
}

void	airspyHandler::setVFOFrequency (int32_t nf) {
int result = my_airspy_set_freq (device, nf);

	vfoFrequency	= nf;
	if (result != AIRSPY_SUCCESS) {
	   printf ("my_airspy_set_freq() failed: %s (%d)\n",
	            my_airspy_error_name((airspy_error)result), result);
	}
}

int32_t	airspyHandler::getVFOFrequency() {
	return lastFrequency;
}

int32_t	airspyHandler::defaultFrequency() {
	return Khz (220000);
}

void	airspyHandler::set_filter	(int c) {
	(void)c;
	filtering	= filterSelector -> isChecked ();
	fprintf (stderr, "filter set %s\n", filtering ? "on" : "off");
}

bool	airspyHandler::restartReader	(int32_t freq) {
int	result;
int32_t	bufSize	= EXTIO_NS * EXTIO_BASE_TYPE_SIZE * 2;

	if (running. load())
	   return true;

	vfoFrequency	= freq;
	if (save_gainSettings) 
	   update_gainSettings (freq / MHz (1));
//
//	sliders are now set,
	result = my_airspy_set_freq (device, freq);

	if (result != AIRSPY_SUCCESS) {
	   printf ("my_airspy_set_freq() failed: %s (%d)\n",
	            my_airspy_error_name((airspy_error)result), result);
	}
	_I_Buffer. FlushRingBuffer();
	result = my_airspy_set_sample_type (device, AIRSPY_SAMPLE_INT16_IQ);
//	result = my_airspy_set_sample_type (device, AIRSPY_SAMPLE_FLOAT32_IQ);
	if (result != AIRSPY_SUCCESS) {
	   printf ("my_airspy_set_sample_type() failed: %s (%d)\n",
	            my_airspy_error_name ((airspy_error)result), result);
	   return false;
	}

	result = my_airspy_start_rx (device,
	            (airspy_sample_block_cb_fn)callback, this);
	if (result != AIRSPY_SUCCESS) {
	   printf ("my_airspy_start_rx() failed: %s (%d)\n",
	         my_airspy_error_name((airspy_error)result), result);
	   return false;
	}
	fprintf (stderr, "currentTab %d\n", currentTab);
	my_airspy_set_lna_agc	(device, lna_agc ? 1 : 0);
	my_airspy_set_mixer_agc	(device, mixer_agc ? 1 : 0);
	my_airspy_set_rf_bias	(device, rf_bias ? 1 : 0);
	if (currentTab == 0)
	   set_sensitivity	(sensitivitySlider -> value());
	else 
	if (currentTab	== 1)
	   set_linearity	(linearitySlider -> value());
	else {
	   set_vga_gain		(vgaGain);
	   set_mixer_gain	(mixerGain);
	   set_lna_gain		(lnaGain);
	}
	
//
	running. store (true);
	return true;
}

void	airspyHandler::stopReader() {
int	result;

	if (!running. load())
	   return;

	close_xmlDump ();
	if (save_gainSettings)
	   record_gainSettings (vfoFrequency / MHz (1));
	result = my_airspy_stop_rx (device);

	if (result != AIRSPY_SUCCESS ) 
	   printf ("my_airspy_stop_rx() failed: %s (%d)\n",
	          my_airspy_error_name ((airspy_error)result), result);
	running. store (false);
	resetBuffer ();
}
//
//	Directly copied from the airspy extio dll from Andrea Montefusco
int airspyHandler::callback (airspy_transfer* transfer) {
airspyHandler *p;

	if (!transfer)
	   return 0;		// should not happen
	p = static_cast<airspyHandler *> (transfer -> ctx);

// we read  AIRSPY_SAMPLE_INT16_IQ:
	int32_t bytes_to_write = transfer -> sample_count * sizeof (int16_t) * 2; 
	uint8_t *pt_rx_buffer   = (uint8_t *)transfer->samples;
	p -> data_available (pt_rx_buffer, bytes_to_write);
	return 0;
}

//	called from AIRSPY data callback
//	2*2 = 4 bytes for sample, as per AirSpy USB data stream format
//	we do the rate conversion here, read in groups of 2 * xxx samples
//	and transform them into groups of 2 * 512 samples
int 	airspyHandler::data_available (void *buf, int buf_size) {	
int16_t	*sbuf	= (int16_t *)buf;
int nSamples	= buf_size / (sizeof (int16_t) * 2);
std::complex<float> temp [2048];
int32_t  i, j;

	if (dumping. load ())
	   xmlWriter -> add ((std::complex<int16_t> *)sbuf, nSamples);
	if (filtering) {
	   if (filterDepth -> value () != currentDepth) {
	      currentDepth = filterDepth -> value ();
	      theFilter -> resize (currentDepth);
	   }
	   for (i = 0; i < nSamples; i ++) {
	      convBuffer [convIndex ++] = theFilter -> Pass (
	                                     std::complex<float> (
	                                        sbuf [2 * i] / (float)2048,
	                                        sbuf [2 * i + 1] / (float)2048)
	                                     );
	      if (convIndex > convBufferSize) {
	         for (j = 0; j < 2048; j ++) {
	            int16_t  inpBase	= mapTable_int [j];
	            float    inpRatio	= mapTable_float [j];
	            temp [j]	= cmul (convBuffer [inpBase + 1], inpRatio) + 
	                          cmul (convBuffer [inpBase], 1 - inpRatio);
	         }

	         _I_Buffer. putDataIntoBuffer (temp, 2048);
//
//	shift the sample at the end to the beginning, it is needed
//	as the starting sample for the next time
	         convBuffer [0] = convBuffer [convBufferSize];
	         convIndex = 1;
	      }
	   }
	}
	else
	for (i = 0; i < nSamples; i ++) {
	   convBuffer [convIndex ++] = std::complex<float> (
	                                     sbuf [2 * i] / (float)2048,
	                                     sbuf [2 * i + 1] / (float)2048);
	   if (convIndex > convBufferSize) {
	      for (j = 0; j < 2048; j ++) {
	         int16_t  inpBase	= mapTable_int [j];
	         float    inpRatio	= mapTable_float [j];
	         temp [j]	= cmul (convBuffer [inpBase + 1], inpRatio) + 
	                          cmul (convBuffer [inpBase], 1 - inpRatio);
	      }

	      _I_Buffer. putDataIntoBuffer (temp, 2048);
//
//	shift the sample at the end to the beginning, it is needed
//	as the starting sample for the next time
	      convBuffer [0] = convBuffer [convBufferSize];
	      convIndex = 1;
	   }
	}
	return 0;
}
//
const char *airspyHandler::getSerial() {
airspy_read_partid_serialno_t read_partid_serialno;
int result = my_airspy_board_partid_serialno_read (device,
	                                          &read_partid_serialno);
	if (result != AIRSPY_SUCCESS) {
	   printf ("failed: %s (%d)\n",
	         my_airspy_error_name ((airspy_error)result), result);
	   return "UNKNOWN";
	} else {
	   snprintf (serial, sizeof(serial), "%08X%08X", 
	             read_partid_serialno. serial_no [2],
	             read_partid_serialno. serial_no [3]);
	}
	return serial;
}
//
//	not used here
int	airspyHandler::open() {
//int result = my_airspy_open (&device);
//
//	if (result != AIRSPY_SUCCESS) {
//	   printf ("airspy_open() failed: %s (%d)\n",
//	          my_airspy_error_name((airspy_error)result), result);
//	   return -1;
//	} else {
//	   return 0;
//	}
	return 0;
}

//
//	These functions are added for the SDR-J interface
void	airspyHandler::resetBuffer	() {
	_I_Buffer. FlushRingBuffer	();
}

int16_t	airspyHandler::bitDepth		() {
	return 13;
}

int32_t	airspyHandler::getSamples (std::complex<float> *v, int32_t size) {

	return _I_Buffer. getDataFromBuffer (v, size);
}

int32_t	airspyHandler::Samples		() {
	return _I_Buffer. GetRingBufferReadAvailable();
}
//
#define GAIN_COUNT (22)

uint8_t airspy_linearity_vga_gains[GAIN_COUNT] = { 13, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 9, 8, 7, 6, 5, 4 };
uint8_t airspy_linearity_mixer_gains[GAIN_COUNT] = { 12, 12, 11, 9, 8, 7, 6, 6, 5, 0, 0, 1, 0, 0, 2, 2, 1, 1, 1, 1, 0, 0 };
uint8_t airspy_linearity_lna_gains[GAIN_COUNT] = { 14, 14, 14, 13, 12, 10, 9, 9, 8, 9, 8, 6, 5, 3, 1, 0, 0, 0, 0, 0, 0, 0 };
uint8_t airspy_sensitivity_vga_gains[GAIN_COUNT] = { 13, 12, 11, 10, 9, 8, 7, 6, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
uint8_t airspy_sensitivity_mixer_gains[GAIN_COUNT] = { 12, 12, 12, 12, 11, 10, 10, 9, 9, 8, 7, 4, 4, 4, 3, 2, 2, 1, 0, 0, 0, 0 };
uint8_t airspy_sensitivity_lna_gains[GAIN_COUNT] = { 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 12, 12, 9, 9, 8, 7, 6, 5, 3, 2, 1, 0 };

void	airspyHandler::set_linearity (int value) {
int	result = my_airspy_set_linearity_gain (device, value);
int	temp;
	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_lna_gain() failed: %s (%d)\n",
	            my_airspy_error_name ((airspy_error)result), result);
	   return;
	}
	linearityDisplay	-> display (value);
	temp	= airspy_linearity_lna_gains [GAIN_COUNT - 1 - value];
	linearity_lnaDisplay	-> display (temp);
	temp	= airspy_linearity_mixer_gains [GAIN_COUNT - 1 - value];
	linearity_mixerDisplay	-> display (temp);
	temp	= airspy_linearity_vga_gains [GAIN_COUNT - 1 - value];
	linearity_vgaDisplay	-> display (temp);
}

void	airspyHandler::set_sensitivity (int value) {
int	result = my_airspy_set_sensitivity_gain (device, value);
int	temp;
	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_mixer_gain() failed: %s (%d)\n",
	            my_airspy_error_name ((airspy_error)result), result);
	   return;
	}
	sensitivityDisplay	-> display (value);
	temp	= airspy_sensitivity_lna_gains [GAIN_COUNT - 1 - value];
	sensitivity_lnaDisplay	-> display (temp);
	temp	= airspy_sensitivity_mixer_gains [GAIN_COUNT - 1 - value];
	sensitivity_mixerDisplay	-> display (temp);
	temp	= airspy_sensitivity_vga_gains [GAIN_COUNT - 1 - value];
	sensitivity_vgaDisplay	-> display (temp);
}

//

//	Original functions from the airspy extio dll
/* Parameter value shall be between 0 and 15 */
void	airspyHandler::set_lna_gain (int value) {
int result = my_airspy_set_lna_gain (device, lnaGain = value);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_lna_gain() failed: %s (%d)\n",
	            my_airspy_error_name ((airspy_error)result), result);
	}
	else
	   lnaDisplay	-> display (value);
}

/* Parameter value shall be between 0 and 15 */
void	airspyHandler::set_mixer_gain (int value) {
int result = my_airspy_set_mixer_gain (device, mixerGain = value);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_mixer_gain() failed: %s (%d)\n",
	            my_airspy_error_name ((airspy_error)result), result);
	}
	else
	   mixerDisplay	-> display (value);
}

/* Parameter value shall be between 0 and 15 */
void	airspyHandler::set_vga_gain (int value) {
int result = my_airspy_set_vga_gain (device, vgaGain = value);

	fprintf (stderr, "vgaGain %d, result %d\n",
	                             vgaGain, result);
	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_vga_gain() failed: %s (%d)\n",
	            my_airspy_error_name ((airspy_error)result), result);
	}
	else
	   vgaDisplay	-> display (value);
}
//
//	agc's
/* Parameter value:
	0=Disable LNA Automatic Gain Control
	1=Enable LNA Automatic Gain Control
*/
void	airspyHandler::set_lna_agc	(int dummy) {
	(void)dummy;
	lna_agc	= lnaButton	-> isChecked ();
	int result = my_airspy_set_lna_agc (device, lna_agc ? 1 : 0);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_lna_agc() failed: %s (%d)\n",
	            my_airspy_error_name ((airspy_error)result), result);
	}
}

/* Parameter value:
	0=Disable MIXER Automatic Gain Control
	1=Enable MIXER Automatic Gain Control
*/
void	airspyHandler::set_mixer_agc	(int dummy) {
	(void)dummy;
	mixer_agc	= mixerButton -> isChecked ();

int result = my_airspy_set_mixer_agc (device, mixer_agc ? 1 : 0);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_mixer_agc() failed: %s (%d)\n",
	            my_airspy_error_name ((airspy_error)result), result);
	}
}


/* Parameter value shall be 0=Disable BiasT or 1=Enable BiasT */
void	airspyHandler::set_rf_bias (int dummy) {
	(void)dummy;
	rf_bias	= biasButton -> isChecked ();
int result = my_airspy_set_rf_bias (device, rf_bias ? 1 : 0);

	if (result != AIRSPY_SUCCESS) {
	   printf("airspy_set_rf_bias() failed: %s (%d)\n",
	           my_airspy_error_name ((airspy_error)result), result);
	}
}

const char* airspyHandler::board_id_name() {
uint8_t bid;

	if (my_airspy_board_id_read (device, &bid) == AIRSPY_SUCCESS)
	   return my_airspy_board_id_name ((airspy_board_id)bid);
	else
	   return "UNKNOWN";
}
//
//
bool	airspyHandler::load_airspyFunctions() {
//
//	link the required procedures
	my_airspy_init	= (pfn_airspy_init)
	                       GETPROCADDRESS (Handle, "airspy_init");
	if (my_airspy_init == nullptr) {
	   fprintf (stderr, "Could not find airspy_init\n");
	   return false;
	}

	my_airspy_exit	= (pfn_airspy_exit)
	                       GETPROCADDRESS (Handle, "airspy_exit");
	if (my_airspy_exit == nullptr) {
	   fprintf (stderr, "Could not find airspy_exit\n");
	   return false;
	}

	my_airspy_list_devices	= (pfn_airspy_list_devices)
	                       GETPROCADDRESS (Handle, "airspy_list_devices");
	if (my_airspy_list_devices == nullptr) {
	   fprintf (stderr, "Could not find airspy_list_devices\n");
	   return false;
	}
	
	my_airspy_open	= (pfn_airspy_open)
	                       GETPROCADDRESS (Handle, "airspy_open");
	if (my_airspy_open == nullptr) {
	   fprintf (stderr, "Could not find airspy_open\n");
	   return false;
	}

	my_airspy_close	= (pfn_airspy_close)
	                       GETPROCADDRESS (Handle, "airspy_close");
	if (my_airspy_close == nullptr) {
	   fprintf (stderr, "Could not find airspy_close\n");
	   return false;
	}

	my_airspy_get_samplerates	= (pfn_airspy_get_samplerates)
	                       GETPROCADDRESS (Handle, "airspy_get_samplerates");
	if (my_airspy_get_samplerates == nullptr) {
	   fprintf (stderr, "Could not find airspy_get_samplerates\n");
	   return false;
	}

	my_airspy_set_samplerate	= (pfn_airspy_set_samplerate)
	                       GETPROCADDRESS (Handle, "airspy_set_samplerate");
	if (my_airspy_set_samplerate == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_samplerate\n");
	   return false;
	}

	my_airspy_start_rx	= (pfn_airspy_start_rx)
	                       GETPROCADDRESS (Handle, "airspy_start_rx");
	if (my_airspy_start_rx == nullptr) {
	   fprintf (stderr, "Could not find airspy_start_rx\n");
	   return false;
	}

	my_airspy_stop_rx	= (pfn_airspy_stop_rx)
	                       GETPROCADDRESS (Handle, "airspy_stop_rx");
	if (my_airspy_stop_rx == nullptr) {
	   fprintf (stderr, "Could not find airspy_stop_rx\n");
	   return false;
	}

	my_airspy_set_sample_type	= (pfn_airspy_set_sample_type)
	                       GETPROCADDRESS (Handle, "airspy_set_sample_type");
	if (my_airspy_set_sample_type == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_sample_type\n");
	   return false;
	}

	my_airspy_set_freq	= (pfn_airspy_set_freq)
	                       GETPROCADDRESS (Handle, "airspy_set_freq");
	if (my_airspy_set_freq == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_freq\n");
	   return false;
	}

	my_airspy_set_lna_gain	= (pfn_airspy_set_lna_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_lna_gain");
	if (my_airspy_set_lna_gain == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_lna_gain\n");
	   return false;
	}

	my_airspy_set_mixer_gain	= (pfn_airspy_set_mixer_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_mixer_gain");
	if (my_airspy_set_mixer_gain == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_mixer_gain\n");
	   return false;
	}

	my_airspy_set_vga_gain	= (pfn_airspy_set_vga_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_vga_gain");
	if (my_airspy_set_vga_gain == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_vga_gain\n");
	   return false;
	}
	
	my_airspy_set_linearity_gain = (pfn_airspy_set_linearity_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_linearity_gain");
	if (my_airspy_set_linearity_gain == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_linearity_gain\n");
	   fprintf (stderr, "You probably did install an old library\n");
	   return false;
	}

	my_airspy_set_sensitivity_gain = (pfn_airspy_set_sensitivity_gain)
	                       GETPROCADDRESS (Handle, "airspy_set_sensitivity_gain");
	if (my_airspy_set_sensitivity_gain == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_sensitivity_gain\n");
	   fprintf (stderr, "You probably did install an old library\n");
	   return false;
	}

	my_airspy_set_lna_agc	= (pfn_airspy_set_lna_agc)
	                       GETPROCADDRESS (Handle, "airspy_set_lna_agc");
	if (my_airspy_set_lna_agc == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_lna_agc\n");
	   return false;
	}

	my_airspy_set_mixer_agc	= (pfn_airspy_set_mixer_agc)
	                       GETPROCADDRESS (Handle, "airspy_set_mixer_agc");
	if (my_airspy_set_mixer_agc == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_mixer_agc\n");
	   return false;
	}

	my_airspy_set_rf_bias	= (pfn_airspy_set_rf_bias)
	                       GETPROCADDRESS (Handle, "airspy_set_rf_bias");
	if (my_airspy_set_rf_bias == nullptr) {
	   fprintf (stderr, "Could not find airspy_set_rf_bias\n");
	   return false;
	}

	my_airspy_error_name	= (pfn_airspy_error_name)
	                       GETPROCADDRESS (Handle, "airspy_error_name");
	if (my_airspy_error_name == nullptr) {
	   fprintf (stderr, "Could not find airspy_error_name\n");
	   return false;
	}

	my_airspy_board_id_read	= (pfn_airspy_board_id_read)
	                       GETPROCADDRESS (Handle, "airspy_board_id_read");
	if (my_airspy_board_id_read == nullptr) {
	   fprintf (stderr, "Could not find airspy_board_id_read\n");
	   return false;
	}

	my_airspy_board_id_name	= (pfn_airspy_board_id_name)
	                       GETPROCADDRESS (Handle, "airspy_board_id_name");
	if (my_airspy_board_id_name == nullptr) {
	   fprintf (stderr, "Could not find airspy_board_id_name\n");
	   return false;
	}

	my_airspy_board_partid_serialno_read	=
	                (pfn_airspy_board_partid_serialno_read)
	                       GETPROCADDRESS (Handle, "airspy_board_partid_serialno_read");
	if (my_airspy_board_partid_serialno_read == nullptr) {
	   fprintf (stderr, "Could not find airspy_board_partid_serialno_read\n");
	   return false;
	}

	return true;
}

void	airspyHandler::show_tab (int t) {
	if (t == 0)		// sensitivity
	   set_sensitivity	(sensitivitySlider -> value());
	else
	if (t == 1)		// linearity
	   set_linearity	(linearitySlider -> value());
	else {			// classic view
	   set_vga_gain		(vgaGain);
	   set_mixer_gain	(mixerGain);
	   set_lna_gain		(lnaGain);
	}
	currentTab	= t;
}

int	airspyHandler::getBufferSpace	() {
	return _I_Buffer. GetRingBufferWriteAvailable ();
}

QString	airspyHandler::deviceName	() {
	return "AIRspy";
}

void	airspyHandler::set_xmlDump () {
	if (xmlDumper == nullptr) {
	  if (setup_xmlDump ())
	      dumpButton	-> setText ("writing");
	}
	else {
	   close_xmlDump ();
	   dumpButton	-> setText ("Dump");
	}
}

static inline
bool	isValid (QChar c) {
	return c. isLetterOrNumber () || (c == '-');
}

bool	airspyHandler::setup_xmlDump () {
QTime	theTime;
QDate	theDate;
QString saveDir = airspySettings -> value ("saveDir_xmlDump",
                                           QDir::homePath ()). toString ();
        if ((saveDir != "") && (!saveDir. endsWith ("/")))
           saveDir += "/";
	QString channel		= airspySettings -> value ("channel", "xx").
	                                                     toString ();
        QString timeString      = theDate. currentDate (). toString () + "-" +
	                          theTime. currentTime (). toString ();
	for (int i = 0; i < timeString. length (); i ++)
	   if (!isValid (timeString. at (i)))
	      timeString. replace (i, 1, "-");
        QString suggestedFileName =
                saveDir + "AIRspy" + "-" + channel + "-" + timeString;
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
	                                      12,
	                                      "int16",
	                                      selectedRate,
	                                      getVFOFrequency (),
	                                      "AIRspy",
	                                      "I",
	                                      recorderVersion);
	dumping. store (true);

	QString dumper	= QDir::fromNativeSeparators (fileName);
	int x		= dumper. lastIndexOf ("/");
        saveDir		= dumper. remove (x, dumper. count () - x);
        airspySettings	-> setValue ("saveDir_xmlDump", saveDir);
	return true;
}

void	airspyHandler::close_xmlDump () {
	if (xmlDumper == nullptr)	// this can happen !!
	   return;
	dumping. store (false);
	usleep (1000);
	xmlWriter	-> computeHeader ();
	delete xmlWriter;
	fclose (xmlDumper);
	xmlDumper	= nullptr;
}

void	airspyHandler::show	() {
	myFrame. show ();
}

void	airspyHandler::hide	() {
	myFrame. hide	();
}

bool	airspyHandler::isHidden	() {
	return myFrame. isHidden ();
}

void	airspyHandler::record_gainSettings	(int key) {
int	linearity	= linearitySlider	-> value ();
int	sensitivity	= sensitivitySlider	-> value ();
int	lnaGainValue	= lnaSlider		-> value ();
int	mixerValue	= mixerSlider		-> value ();
int	vgaGainValue	= vgaSlider		-> value ();
int	currentTab	= tabWidget		-> currentIndex ();
QString theValue	= QString::number (linearity) + ":";

	theValue	+= QString::number (sensitivity) + ":";
	theValue	+= QString::number (lnaGainValue) + ":";
	theValue	+= QString::number (mixerValue) + ":";
	theValue	+= QString::number (vgaGainValue) + ":";
	theValue	+= QString::number (lna_agc) + ":";
	theValue	+= QString::number (mixer_agc) + ":";
	theValue	+= QString::number (rf_bias) + ":";
	theValue	+= QString::number (currentTab);

        airspySettings	-> beginGroup ("airspySettings");
        airspySettings	-> setValue (QString::number (key), theValue);
        airspySettings	-> endGroup ();
}

void	airspyHandler::update_gainSettings	(int key) {
int     linearity;
int     sensitivity;
int     lnaGainValue;
int     mixerGainValue;
int     vgaGainValue;
int	lna_agc_setting;
int	mixer_agc_setting;
int	rf_bias_setting;
int	tab_setting;
QString theValue;

        airspySettings	-> beginGroup ("airspySettings");
        theValue = airspySettings -> value (QString::number (key), ""). toString ();
        airspySettings	-> endGroup ();

	if (theValue == QString (""))
           return;              // or set some defaults here

        QStringList result      = theValue. split (":");
        if (result. size () != 9)        // should not happen
           return;

	lna_agc			= result. at (5). toInt ();
	mixer_agc		= result. at (6). toInt ();
	rf_bias			= result. at (7). toInt ();
	tab_setting		= result. at (8). toInt ();
	switch (tab_setting) {
	   case 0:	// sensitivity
              sensitivity		= result. at (1). toInt ();
	      sensitivitySlider -> blockSignals (true);
	      new_sensitivityValue	(sensitivity);
	      new_sensitivityDisplay	(sensitivity);
	      while (sensitivitySlider -> value () != sensitivity)
	         usleep (1000);
	      sensitivitySlider -> blockSignals (false);
	      break;
	   case 1:	// linearity
              linearity		= result. at (0). toInt ();
	      linearitySlider	-> blockSignals (true);
	      new_linearityValue	(linearity);
	      new_linearityDisplay	(linearity);
	      while (linearitySlider -> value () != linearity)
	         usleep (1000);
	      linearitySlider	-> blockSignals (false);
	      break;
	   default:
	      break;
	}
	lnaGainValue		= result. at (2). toInt ();
	mixerGainValue		= result. at (3). toInt ();
	vgaGainValue		= result. at (4). toInt ();
//
//	setting sliders should not lead to signals
	lnaSlider		-> blockSignals (true);
	new_lnaGainValue	(lnaGainValue);
	new_lnaDisplay		(lnaGainValue);
	while (lnaSlider -> value () != lnaGainValue)
	   usleep (1000);
	lnaSlider		-> blockSignals (false);


	mixerSlider	-> blockSignals (true);
	new_mixerValue	(mixerGainValue);
	new_mixerDisplay	(mixerGainValue);
	while (mixerSlider -> value () != mixerGainValue)
	   usleep (1000);
	mixerSlider	-> blockSignals (false);

	lnaButton	-> blockSignals (true);
	lnaButton	-> setChecked (lna_agc);
	usleep (1000);
	lnaButton	-> blockSignals (false);

	mixerButton	-> blockSignals (true);
	mixerButton	-> setChecked (mixer_agc);
	usleep	(1000);
	mixerButton	-> blockSignals (false);

	biasButton	-> blockSignals (true);
	biasButton	-> setChecked (rf_bias);
	usleep (1000);
	biasButton	-> blockSignals (false);

	vgaSlider	-> blockSignals (true);
	new_vgaGainValue	(vgaGainValue);
	new_vgaDisplay		(vgaGainValue);
	while (vgaSlider -> value () != vgaGainValue)
	   usleep (1000);
	vgaSlider	-> blockSignals (false);

	tabWidget	-> blockSignals (true);
	new_tabSetting (tab_setting);
	while (tabWidget -> currentIndex () != tab_setting)
	   usleep (1000);
	tabWidget	-> blockSignals (false);
}
