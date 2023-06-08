#
/*
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of Qt-DAB
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<QThread>
#include	<QSettings>
#include	<QTime>
#include	<QDate>
#include	<QLabel>
#include	<QFileDialog>
#include	"sdrplay-handler-v3.h"
#include	"sdrplay-commands.h"
#include	"xml-filewriter.h"

static
int     RSP1_Table []	= {0, 24, 19, 43};

static
int     RSP1A_Table []	= {0, 6, 12, 18, 20, 26, 32, 38, 57, 62};

static
int     RSP2_Table []	= {0, 10, 15, 21, 24, 34, 39, 45, 64};

static
int	RSPduo_Table []	= {0, 6, 12, 18, 20, 26, 32, 38, 57, 62};

static
int     RSPDx_Table []  = {0, 3, 6, 9, 12, 15, 24,27, 30, 33, 36, 39, 42, 45,
                           48, 51, 54, 57, 60, 53, 66, 69, 72, 75, 78, 81, 84};

static
int	get_lnaGRdB (int hwVersion, int lnaState) {
	switch (hwVersion) {
	   case 1:
	   default:
	      return RSP1_Table [lnaState];

	   case 2:
	      return RSP2_Table [lnaState];

	   case 255: 
	      return RSP1A_Table [lnaState];

	   case 3:
	      return RSPduo_Table [lnaState];

	   case 4:
	      return RSPDx_Table [lnaState];
	}
}

	sdrplayHandler_v3::sdrplayHandler_v3  (QSettings *s,
	                                       QString &recorderVersion):
	                                          _I_Buffer (4 * 1024 * 1024),
	                                          myFrame (nullptr) {
	sdrplaySettings			= s;
	this	-> recorderVersion	= recorderVersion;
	setupUi (&myFrame);
	myFrame. show	();
	antennaSelector		-> hide	();
	tunerSelector		-> hide	();
	nrBits			= 12;	// default
	denominator		= 2048;	// default

	xmlDumper		= nullptr;
	dumping. store	(false);
//	See if there are settings from previous incarnations
//	and config stuff

	sdrplaySettings		-> beginGroup ("sdrplaySettings_v3");
	GRdBSelector 		-> setValue (
	            sdrplaySettings -> value ("sdrplay-ifgrdb", 20). toInt());

	lnaGainSetting		-> setValue (
	            sdrplaySettings -> value ("sdrplay-lnastate", 4). toInt());

	lnaState		=
	            sdrplaySettings -> value ("sdrplay-lnastate", 4). toInt();

	ppmControl		-> setValue (
	            sdrplaySettings -> value ("sdrplay-ppm", 0). toInt());

	agcMode		=
	       sdrplaySettings -> value ("sdrplay-agcMode", 0). toInt() != 0;
	sdrplaySettings	-> endGroup	();

	if (agcMode) {
	   agcControl -> setChecked (true);
	   GRdBSelector         -> hide ();
	   gainsliderLabel      -> hide ();
	}

	
//	and be prepared for future changes in the settings
	connect (GRdBSelector, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ifgainReduction (int)));
	connect (lnaGainSetting, SIGNAL (valueChanged (int)),
	         this, SLOT (set_lnagainReduction (int)));
	connect (agcControl, SIGNAL (stateChanged (int)),
	         this, SLOT (set_agcControl (int)));
	connect (ppmControl, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ppmControl (int)));
	connect (antennaSelector, SIGNAL (activated (const QString &)),
	         this, SLOT (set_antennaSelect (const QString &)));
	connect (dumpButton, SIGNAL (clicked ()),
                 this, SLOT (set_xmlDump ()));
	vfoFrequency	= MHz (220);
	theGain		= -1;
	debugControl	-> hide ();
	failFlag. store (false);
	successFlag. store (false);
	errorCode	= 0;
	start ();
	while (!failFlag. load () && !successFlag. load () && isRunning ())
	   usleep (1000);
	if (failFlag. load ()) {
	   while (isRunning ())
	      usleep (1000);
	   throw (errorCode);
	}
	
	fprintf (stderr, "setup sdrplay v3 seems successfull\n");
}

	sdrplayHandler_v3::~sdrplayHandler_v3 () {
	threadRuns. store (false);
	while (isRunning ())
	   usleep (1000);
//	thread should be stopped by now
	myFrame. hide ();
	sdrplaySettings	-> beginGroup ("sdrplaySettings_v3");
	sdrplaySettings -> setValue ("sdrplay-ppm",
	                                           ppmControl -> value ());
	sdrplaySettings -> setValue ("sdrplay-ifgrdb",
	                                           GRdBSelector -> value ());
	sdrplaySettings -> setValue ("sdrplay-lnastate",
	                                           lnaGainSetting -> value ());
	sdrplaySettings	-> setValue ("sdrplay-agcMode",
	                                  agcControl -> isChecked() ? 1 : 0);
	sdrplaySettings	-> endGroup ();
	sdrplaySettings	-> sync();
}

/////////////////////////////////////////////////////////////////////////
//	Implementing the interface
/////////////////////////////////////////////////////////////////////////

int32_t	sdrplayHandler_v3::defaultFrequency	() {
	return Mhz (220);
}

int32_t	sdrplayHandler_v3::getVFOFrequency() {
	return vfoFrequency;
}

bool	sdrplayHandler_v3::restartReader	(int32_t newFreq) {
restartRequest r (newFreq);

        if (receiverRuns. load ())
           return true;
        vfoFrequency    = newFreq;
	return messageHandler (&r);
}

void	sdrplayHandler_v3::stopReader	() {
stopRequest r;
	close_xmlDump ();
        if (!receiverRuns. load ())
           return;
        messageHandler (&r);
}
//
int32_t	sdrplayHandler_v3::getSamples (cmplx *V, int32_t size) {
std::complex<int16_t> temp [size];
int	i;

	int amount      = _I_Buffer. getDataFromBuffer (temp, size);
        for (i = 0; i < amount; i ++)
           V [i] = cmplx (real (temp [i]) / (float) denominator,
                                        imag (temp [i]) / (float) denominator);
        if (dumping. load ())
           xmlWriter -> add (temp, amount);
        return amount;
}

int32_t	sdrplayHandler_v3::Samples	() {
	return _I_Buffer. GetRingBufferReadAvailable();
}

void	sdrplayHandler_v3::resetBuffer	() {
	_I_Buffer. FlushRingBuffer();
}

int16_t	sdrplayHandler_v3::bitDepth	() {
	return nrBits;
}

QString	sdrplayHandler_v3::deviceName	() {
	return deviceModel;
}

///////////////////////////////////////////////////////////////////////////
//	Handling the GUI
//////////////////////////////////////////////////////////////////////
//
//	Since the daemon is not threadproof, we have to package the
//	actual interface into its own thread.
//	Communication with that thread is synchronous!
//
void	sdrplayHandler_v3::set_lnabounds(int low, int high) {
	lnaGainSetting	-> setRange (low, high);
	lnaGRdBDisplay	->
	         display (get_lnaGRdB (hwVersion, lnaGainSetting -> value ()));
}

void	sdrplayHandler_v3::set_deviceName (const QString& s) {
	deviceLabel	-> setText (s);
}

void	sdrplayHandler_v3::set_serial	(const QString& s) {
	serialNumber	-> setText (s);
}

void	sdrplayHandler_v3::set_apiVersion (float version) {
QString v = QString::number (version, 'r', 2);
	api_version	-> display (v);
}

void	sdrplayHandler_v3::set_ifgainReduction	(int GRdB) {
GRdBRequest r (GRdB);

	if (!receiverRuns. load ())
           return;
        messageHandler (&r);
}

void	sdrplayHandler_v3::set_lnagainReduction (int lnaState) {
lnaRequest r (lnaState);

	if (!receiverRuns. load ())
           return;
        messageHandler (&r);
	lnaGRdBDisplay	-> display (get_lnaGRdB (hwVersion, lnaState));
}

void	sdrplayHandler_v3::set_agcControl (int dummy) {
bool    agcMode = agcControl -> isChecked ();
agcRequest r (agcMode, 30);
	(void)dummy;
        messageHandler (&r);
	if (agcMode) {
           GRdBSelector         -> hide ();
           gainsliderLabel      -> hide ();
	}
	else {
	   GRdBRequest r2 (GRdBSelector -> value ());
	   GRdBSelector		-> show ();
	   gainsliderLabel	-> show ();
	   messageHandler  (&r2);
	}
}

void	sdrplayHandler_v3::set_ppmControl (int ppm) {
ppmRequest r (ppm);
        messageHandler (&r);
}

void	sdrplayHandler_v3::set_biasT (int v) {
biasT_Request r (biasT_selector -> isChecked () ? 1 : 0);

	messageHandler (&r);
	sdrplaySettings -> setValue ("biasT_selector",
	                              biasT_selector -> isChecked () ? 1 : 0);
}

void	sdrplayHandler_v3::set_antennaSelect	(const QString &s) {
	messageHandler (new antennaRequest (s == "Antenna A" ? 'A' : 'B'));
}

void	sdrplayHandler_v3::set_xmlDump () {
	if (xmlDumper == nullptr) {
	  if (setup_xmlDump ())
	      dumpButton	-> setText ("writing");
	}
	else {
	   close_xmlDump ();
	   dumpButton	-> setText ("Dump");
	}
}
//
////////////////////////////////////////////////////////////////////////
//	showing data
////////////////////////////////////////////////////////////////////////
void	sdrplayHandler_v3::set_antennaSelect (bool b) {
	if (b)
	   antennaSelector		-> show	();
	else
	   antennaSelector		-> hide	();
}

void	sdrplayHandler_v3::show_tunerSelector	(bool b) {
	if (b)
	   tunerSelector	-> show	();
	else
	   tunerSelector	-> hide	();
}

static inline
bool	isValid (QChar c) {
	return c. isLetterOrNumber () || (c == '-');
}

bool	sdrplayHandler_v3::setup_xmlDump () {
QTime theTime;
QDate theDate;
QString saveDir = sdrplaySettings -> value ("saveDir_xmlDump",
                                           QDir::homePath ()). toString ();
        if ((saveDir != "") && (!saveDir. endsWith ("/")))
           saveDir += "/";

	QString channel		= sdrplaySettings -> value ("channel", "xx").
	                                                      toString ();
	QString timeString      = theDate. currentDate (). toString () + "-" +
	                          theTime. currentTime (). toString ();
        for (int i = 0; i < timeString. length (); i ++)
           if (!isValid (timeString. at (i)))
              timeString. replace (i, 1, "-");

	QString suggestedFileName =
                    saveDir + deviceModel + "-" + channel + "-" +timeString;
	QString fileName =
	           QFileDialog::getSaveFileName (nullptr,
	                                         tr ("Save file ..."),
	                                         suggestedFileName +".uff",
	                                         tr ("Xml (*.uff)"));
        fileName        = QDir::toNativeSeparators (fileName);
        xmlDumper	= fopen (fileName. toUtf8(). data(), "w");
	if (xmlDumper == nullptr)
	   return false;
	
	xmlWriter	= new xml_fileWriter (xmlDumper,
	                                      nrBits,
	                                      "int16",
	                                      2048000,
	                                      vfoFrequency,
	                                      "SDRplay",
	                                      "????",
	                                      recorderVersion);
	dumping. store (true);

	QString dumper	= QDir::fromNativeSeparators (fileName);
	int x		= dumper. lastIndexOf ("/");
	saveDir		= dumper. remove (x, dumper. count () - x);
        sdrplaySettings -> setValue ("saveDir_xmlDump", saveDir);
	return true;
}

void	sdrplayHandler_v3::close_xmlDump () {
	if (xmlDumper == nullptr)	// this can happen !!
	   return;
	dumping. store (false);
	usleep (1000);
	xmlWriter	-> computeHeader ();
	delete xmlWriter;
	fclose (xmlDumper);
	xmlDumper	= nullptr;
}
//
///////////////////////////////////////////////////////////////////////
//	the real controller starts here
///////////////////////////////////////////////////////////////////////

bool    sdrplayHandler_v3::messageHandler (generalCommand *r) {
        server_queue. push (r);
	serverjobs. release (1);
	while (!r -> waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return false;
	return true;
}

static
void    StreamACallback (short *xi, short *xq,
                         sdrplay_api_StreamCbParamsT *params,
                         unsigned int numSamples,
	                 unsigned int reset,
                         void *cbContext) {
sdrplayHandler_v3 *p	= static_cast<sdrplayHandler_v3 *> (cbContext);
std::complex<int16_t> localBuf [numSamples];

	(void)params;
	if (reset)
	   return;
	if (!p -> receiverRuns. load ())
	   return;

	for (int i = 0; i <  (int)numSamples; i ++) {
	   std::complex<int16_t> symb = std::complex<int16_t> (xi [i], xq [i]);
	   localBuf [i] = symb;
	}
	p -> _I_Buffer. putDataIntoBuffer (localBuf, numSamples);
}

static
void	StreamBCallback (short *xi, short *xq,
                         sdrplay_api_StreamCbParamsT *params,
                         unsigned int numSamples, unsigned int reset,
                         void *cbContext) {
	(void)xi; (void)xq; (void)params; (void)cbContext;
        if (reset)
           printf ("sdrplay_api_StreamBCallback: numSamples=%d\n", numSamples);
}

static
void	EventCallback (sdrplay_api_EventT eventId,
                       sdrplay_api_TunerSelectT tuner,
                       sdrplay_api_EventParamsT *params,
                       void *cbContext) {
sdrplayHandler_v3 *p	= static_cast<sdrplayHandler_v3 *> (cbContext);
	(void)tuner;
	p -> theGain	= params -> gainParams. currGain;
	switch (eventId) {
	   case sdrplay_api_GainChange:
	      break;

	   case sdrplay_api_PowerOverloadChange:
	      p -> update_PowerOverload (params);
	      break;

	   default:
	      fprintf (stderr, "event %d\n", eventId);
	      break;
	}
}

void	sdrplayHandler_v3::
	         update_PowerOverload (sdrplay_api_EventParamsT *params) {
	sdrplay_api_Update (chosenDevice -> dev,
	                    chosenDevice -> tuner,
	                    sdrplay_api_Update_Ctrl_OverloadMsgAck,
	                    sdrplay_api_Update_Ext1_None);
	if (params -> powerOverloadParams.powerOverloadChangeType ==
	                                    sdrplay_api_Overload_Detected) {
//	   fprintf (stderr, "Qt-DAB sdrplay_api_Overload_Detected");
	}
	else {
//	   fprintf (stderr, "Qt-DAB sdrplay_api_Overload Corrected");
	}
}

void	sdrplayHandler_v3::run		() {
sdrplay_api_ErrT        err;
sdrplay_api_DeviceT     devs [6];
uint32_t                ndev;

        threadRuns. store (false);
	receiverRuns. store (false);

	chosenDevice		= nullptr;
	deviceParams		= nullptr;

	connect (this, SIGNAL (set_lnabounds_signal (int, int)),
	         this, SLOT (set_lnabounds (int, int)));
	connect (this, SIGNAL (set_deviceName_signal (const QString &)),
	         this, SLOT (set_deviceName (const QString &)));
	connect (this, SIGNAL (set_serial_signal (const QString &)),
	         this, SLOT (set_serial (const QString &)));
	connect (this, SIGNAL (set_apiVersion_signal (float)),
	         this, SLOT (set_apiVersion (float)));
	connect (this, SIGNAL (set_antennaSelect_signal (bool)),
	         this, SLOT (set_antennaSelect (bool)));
	connect (biasT_selector, SIGNAL (stateChanged (int)),
	         this, SLOT (set_biasT (int)));

	denominator		= 2048;		// default
	nrBits			= 12;		// default

	Handle			= fetchLibrary ();
	if (Handle == nullptr) {
	   failFlag. store (true);
	   errorCode	= 1;
	   return;
	}

//	load the functions
	bool success	= loadFunctions ();
	if (!success) {
	   failFlag. store (true);
	   releaseLibrary ();
	   errorCode	= 2;
	   return;
        }
	fprintf (stderr, "functions loaded\n");

//	try to open the API
	err	= sdrplay_api_Open ();
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_Open failed %s\n",
	                          sdrplay_api_GetErrorString (err));
	   failFlag. store (true);
	   releaseLibrary ();
	   errorCode	= 3;
	   return;
	}

	fprintf (stderr, "api opened\n");

//	Check API versions match
        err = sdrplay_api_ApiVersion (&apiVersion);
        if (err  != sdrplay_api_Success) {
           fprintf (stderr, "sdrplay_api_ApiVersion failed %s\n",
                                     sdrplay_api_GetErrorString (err));
	   errorCode	= 4;
	   goto closeAPI;
        }

	if (apiVersion < 3.05) {
//	if (apiVersion < (SDRPLAY_API_VERSION - 0.01)) {
           fprintf (stderr, "API versions don't match (local=%.2f dll=%.2f)\n",
                                              SDRPLAY_API_VERSION, apiVersion);
	   errorCode	= 5;
	   goto closeAPI;
	}
	
	fprintf (stderr, "api version %f detected\n", apiVersion);
//
//	lock API while device selection is performed
	sdrplay_api_LockDeviceApi ();
	{  int s	= sizeof (devs) / sizeof (sdrplay_api_DeviceT);
	   err	= sdrplay_api_GetDevices (devs, &ndev, s);
	   if (err != sdrplay_api_Success) {
	      fprintf (stderr, "sdrplay_api_GetDevices failed %s\n",
	                      sdrplay_api_GetErrorString (err));
	      errorCode		= 6;
	      goto unlockDevice_closeAPI;
	   }
	}

	if (ndev == 0) {
	   fprintf (stderr, "no valid device found\n");
	   errorCode	= 7;
	   goto unlockDevice_closeAPI;
	}

	fprintf (stderr, "%d devices detected\n", ndev);
	chosenDevice	= &devs [0];
	err	= sdrplay_api_SelectDevice (chosenDevice);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_SelectDevice failed %s\n",
	                         sdrplay_api_GetErrorString (err));
	   errorCode	= 8;
	   goto unlockDevice_closeAPI;
	}
//
//	we have a device, unlock
	sdrplay_api_UnlockDeviceApi ();

	err	= sdrplay_api_DebugEnable (chosenDevice -> dev, 
	                                         (sdrplay_api_DbgLvl_t)1);
//	retrieve device parameters, so they can be changed if needed
	err	= sdrplay_api_GetDeviceParams (chosenDevice -> dev,
	                                                     &deviceParams);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_GetDeviceParams failed %s\n",
	                         sdrplay_api_GetErrorString (err));
	   errorCode	= 9;
	   goto closeAPI;
	}

	if (deviceParams == nullptr) {
	   fprintf (stderr, "sdrplay_api_GetDeviceParams return null as par\n");
	   errorCode = 10;
	   goto closeAPI;
	}
//
	vfoFrequency	= Khz (220000);		// default
//	set the parameter values
	chParams	= deviceParams -> rxChannelA;
	deviceParams	-> devParams -> fsFreq. fsHz	= 2048000.0;
	chParams	-> tunerParams. bwType = sdrplay_api_BW_1_536;
	chParams	-> tunerParams. ifType = sdrplay_api_IF_Zero;
//
//	these will change:
	chParams	-> tunerParams. rfFreq. rfHz    = 220000000.0;
	chParams	-> tunerParams. gain.gRdB	= 
	                                    GRdBSelector -> value ();

	chParams	-> tunerParams. gain.LNAstate	= lnaState;
	chParams	-> ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
	if (agcMode) {
	   chParams    -> ctrlParams. agc. setPoint_dBfs = -30;
	   chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_5HZ;
	}
	else
	   chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_DISABLE;
//
//	assign callback functions
	cbFns. StreamACbFn	= StreamACallback;
	cbFns. StreamBCbFn	= StreamBCallback;
	cbFns. EventCbFn	= EventCallback;

	err	= sdrplay_api_Init (chosenDevice -> dev, &cbFns, this);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_Init failed %s\n",
                                       sdrplay_api_GetErrorString (err));
	   errorCode	= 11;
	   goto unlockDevice_closeAPI;
	}
//
	serial		= devs [0]. SerNo;
	hwVersion	= devs [0]. hwVer;
	sdrplay_api_ReasonForUpdateT notcher;
	switch (hwVersion) {
	   case 1:		// old RSP
	      lna_upperBound	= 3;
	      deviceModel	= "RSP-I";
	      denominator	= 2048;
	      nrBits		= 12;
	      has_antennaSelect	= false;
	      notcher		= sdrplay_api_Update_None;
	      break;
	   case 2:		// RSP II
	      lna_upperBound	= 8;
	      deviceModel 	= "RSP-II";
	      denominator	= 2048;
	      nrBits		= 14;
	      has_antennaSelect	= true;
	      notcher		= sdrplay_api_Update_Rsp2_RfNotchControl;
	      deviceParams    -> rxChannelA -> rsp2TunerParams.  rfNotchEnable = 1;
	      break;
	   case 3:		// RSP-DUO
	      lna_upperBound	= 9;
	      deviceModel	= "RSP-DUO";
	      denominator	= 2048;
	      nrBits		= 12;
	      has_antennaSelect	= false;
	      notcher		= sdrplay_api_Update_RspDuo_RfNotchControl;
	      deviceParams    -> rxChannelA -> rspDuoTunerParams.  rfNotchEnable = 1;
	      break;
	   case 4:		// RSPDx
	      lna_upperBound	= 26;
	      deviceModel	= "RSPDx";
	      denominator	= 2048;
	      nrBits		= 14;
	      has_antennaSelect	= true;
	      break;

	   default:
	   case 255:		// RSP-1A
	      lna_upperBound	= 9;
	      deviceModel	= "RSP-1A";
	      denominator	= 8192;
	      nrBits		= 14;
	      has_antennaSelect	= false;
	      notcher		= sdrplay_api_Update_Rsp1a_RfNotchControl;
	      deviceParams    -> devParams -> rsp1aParams. rfNotchEnable = 1;
	      break;
	}

	err = sdrplay_api_Update (chosenDevice -> dev,
                                  chosenDevice -> tuner,
	                          notcher,
                                  sdrplay_api_Update_Ext1_None);

	if (err != sdrplay_api_Success) 
	   fprintf (stderr, "setting the notch failed\n");
	set_lnabounds_signal		(0, lna_upperBound);
	set_deviceName_signal		(deviceModel);
	set_serial_signal		(serial);
	set_apiVersion_signal		(apiVersion);
	set_antennaSelect_signal 	(has_antennaSelect);
	threadRuns. store (true);	// it seems we can do some work
//
//	The state of the successFlag is polled 
	successFlag. store (true);

//
//	before we really (re)start we set - if needed - the biasT,
//	which is too complex to have  it done inline
	if (sdrplaySettings -> value ("biasT_selector", 0). toInt () != 0) {
	   biasT_selector -> setChecked (true);
	   set_biasValue (1);
	}

	while (threadRuns. load ()) {
	   while (!serverjobs. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      goto normal_exit;

//	here we could assert that the server_queue is not empty
//	Note that we emulate synchronous calling, so
//	we signal the caller when we are done
	   switch (server_queue. front () -> cmd) {
	      case RESTART_REQUEST: {
	         restartRequest *p = (restartRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. rfFreq. rfHz =
	                                            (float)(p -> frequency);
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Frf,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "restart: error %s\n",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
                 }
	         receiverRuns. store (true);
	         p -> waiter. release (1);
	         break;
	      }
	       
	      case STOP_REQUEST: {
	         stopRequest *p =
	                  (stopRequest *)(server_queue. front ());
	         server_queue. pop ();
	         receiverRuns. store (false);
	         p -> waiter. release (1);
	         break;
	      }
	       
	      case AGC_REQUEST: {
	         agcRequest *p = 
	                    (agcRequest *)(server_queue. front ());
	         server_queue. pop ();
	         if (p -> agcMode) {
	            chParams    -> ctrlParams. agc. setPoint_dBfs =
	                                              - p -> setPoint;
                    chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_5HZ;
	         }
	         else
	            chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_DISABLE;

	         p -> result = true;
	         err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Ctrl_Agc,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "agc: error %s\n",
	                                   sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case GRDB_REQUEST: {
	         GRdBRequest *p =  (GRdBRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. gain. gRdB = p -> GRdBValue;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Gr,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "grdb: error %s\n",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case PPM_REQUEST: {
	         ppmRequest *p = (ppmRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result	= false;
	         deviceParams    -> devParams -> ppm = p -> ppmValue;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Dev_Ppm,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "lna: error %s\n",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case BIAS_T_REQUEST: {
	         biasT_Request *p = (biasT_Request *)(server_queue. front ());
	         set_biasValue (p -> biasT_value);
	         server_queue. pop ();
	         p -> result = true;
	         p -> waiter. release (1);
	         break;
	      }
	         
	      case LNA_REQUEST: {
	         lnaRequest *p = (lnaRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. gain. LNAstate =
	                                          p -> lnaState;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Gr,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "grdb: error %s\n",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case ANTENNASELECT_REQUEST: {
	         antennaRequest *p = (antennaRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         deviceParams -> rxChannelA -> rsp2TunerParams. antennaSel =
                                    p -> antenna == 'A' ?
                                             sdrplay_api_Rsp2_ANTENNA_A:
                                             sdrplay_api_Rsp2_ANTENNA_B;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                              chosenDevice -> tuner,
                                              sdrplay_api_Update_Rsp2_AntennaControl,
                                              sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success)
	            p -> result = false;

	         p -> waiter. release (1);
	         break;
	      }
	
	      case GAINVALUE_REQUEST: {
	         gainvalueRequest *p = 
	                        (gainvalueRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         p -> gainValue = theGain;
	         p -> waiter. release (1);
	         break;
	      }

	      default:		// cannot happen
	         fprintf (stderr, "Helemaal fout\n");
	         break;
	   }
	}


normal_exit:
	err = sdrplay_api_Uninit	(chosenDevice -> dev);
	if (err != sdrplay_api_Success) 
	   fprintf (stderr, "sdrplay_api_Uninit failed %s\n",
	                          sdrplay_api_GetErrorString (err));

	err = sdrplay_api_ReleaseDevice	(chosenDevice);
	if (err != sdrplay_api_Success) 
	   fprintf (stderr, "sdrplay_api_ReleaseDevice failed %s\n",
	                          sdrplay_api_GetErrorString (err));

//	sdrplay_api_UnlockDeviceApi	(); ??
        sdrplay_api_Close               ();
	if (err != sdrplay_api_Success) 
	   fprintf (stderr, "sdrplay_api_Close failed %s\n",
	                          sdrplay_api_GetErrorString (err));

	releaseLibrary			();
	fprintf (stderr, "library released, ready to stop thread\n");
	msleep (200);
	return;

unlockDevice_closeAPI:
	sdrplay_api_UnlockDeviceApi	();
closeAPI:	
	failFlag. store (true);
	sdrplay_api_ReleaseDevice       (chosenDevice);
        sdrplay_api_Close               ();
	releaseLibrary	();
	fprintf (stderr, "De taak is gestopt\n");
}

/////////////////////////////////////////////////////////////////////////////
//	handling the library
/////////////////////////////////////////////////////////////////////////////

HINSTANCE	sdrplayHandler_v3::fetchLibrary () {
HINSTANCE	Handle	= nullptr;
#ifdef	__MINGW32__
HKEY APIkey;
wchar_t APIkeyValue [256];
ULONG APIkeyValue_length = 255;

	wchar_t *libname = (wchar_t *)L"sdrplay_api.dll";
	Handle	= LoadLibrary (libname);
	if (Handle == nullptr) {
	   if (RegOpenKey (HKEY_LOCAL_MACHINE,
	                   TEXT("Software\\MiricsSDR\\API"),
	                   &APIkey) != ERROR_SUCCESS) {
              fprintf (stderr,
	           "failed to locate API registry entry, error = %d\n",
	           (int)GetLastError());
	      return nullptr;
	   }

	   RegQueryValueEx (APIkey,
	                    (wchar_t *)L"Install_Dir",
	                    nullptr,
	                    nullptr,
	                    (LPBYTE)&APIkeyValue,
	                    (LPDWORD)&APIkeyValue_length);
//	Ok, make explicit it is in the 32/64 bits section
	   wchar_t *x =
#ifndef __BITS64__
	        wcscat (APIkeyValue, (wchar_t *)L"\\x86\\sdrplay_api.dll");
#else
	        wcscat (APIkeyValue, (wchar_t *)L"\\x64\\sdrplay_api.dll");
#endif
	   RegCloseKey(APIkey);

	   Handle	= LoadLibrary (x);
	   if (Handle == nullptr) {
	      const wchar_t *y =
	              L"C:\\Program Files\\SDRplay\\API\\x86\\sdrplay_api.dll";
	      Handle	= LoadLibrary (y);
	   }
	   if (Handle == nullptr) {
	      fprintf (stderr, "Failed to open sdrplay_api.dll\n");
	      return nullptr;
	   }
	}
#else
	Handle		= dlopen ("libusb-1.0.so", RTLD_NOW | RTLD_GLOBAL);
	Handle		= dlopen ("libsdrplay_api.so", RTLD_NOW);
	if (Handle == nullptr) {
	   fprintf (stderr, "error report %s\n", dlerror());
	   return nullptr;
	}
#endif
	return Handle;
}

void	sdrplayHandler_v3::releaseLibrary () {
#ifdef __MINGW32__
        FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
}

bool	sdrplayHandler_v3::loadFunctions () {
	sdrplay_api_Open	= (sdrplay_api_Open_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Open");
	if ((void *)sdrplay_api_Open == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Open\n");
	   return false;
	}

	sdrplay_api_Close	= (sdrplay_api_Close_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Close");
	if (sdrplay_api_Close == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Close\n");
	   return false;
	}

	sdrplay_api_ApiVersion	= (sdrplay_api_ApiVersion_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_ApiVersion");
	if (sdrplay_api_ApiVersion == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_ApiVersion\n");
	   return false;
	}

	sdrplay_api_LockDeviceApi	= (sdrplay_api_LockDeviceApi_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_LockDeviceApi");
	if (sdrplay_api_LockDeviceApi == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_LockdeviceApi\n");
	   return false;
	}

	sdrplay_api_UnlockDeviceApi	= (sdrplay_api_UnlockDeviceApi_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_UnlockDeviceApi");
	if (sdrplay_api_UnlockDeviceApi == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_UnlockdeviceApi\n");
	   return false;
	}

	sdrplay_api_GetDevices		= (sdrplay_api_GetDevices_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetDevices");
	if (sdrplay_api_GetDevices == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_GetDevices\n");
	   return false;
	}

	sdrplay_api_SelectDevice	= (sdrplay_api_SelectDevice_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_SelectDevice");
	if (sdrplay_api_SelectDevice == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_SelectDevice\n");
	   return false;
	}

	sdrplay_api_ReleaseDevice	= (sdrplay_api_ReleaseDevice_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_ReleaseDevice");
	if (sdrplay_api_ReleaseDevice == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_ReleaseDevice\n");
	   return false;
	}

	sdrplay_api_GetErrorString	= (sdrplay_api_GetErrorString_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetErrorString");
	if (sdrplay_api_GetErrorString == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_GetErrorString\n");
	   return false;
	}

	sdrplay_api_GetLastError	= (sdrplay_api_GetLastError_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetLastError");
	if (sdrplay_api_GetLastError == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_GetLastError\n");
	   return false;
	}

	sdrplay_api_DebugEnable		= (sdrplay_api_DebugEnable_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_DebugEnable");
	if (sdrplay_api_DebugEnable == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_DebugEnable\n");
	   return false;
	}

	sdrplay_api_GetDeviceParams	= (sdrplay_api_GetDeviceParams_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetDeviceParams");
	if (sdrplay_api_GetDeviceParams == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_GetDeviceParams\n");
	   return false;
	}

	sdrplay_api_Init		= (sdrplay_api_Init_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Init");
	if (sdrplay_api_Init == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Init\n");
	   return false;
	}

	sdrplay_api_Uninit		= (sdrplay_api_Uninit_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Uninit");
	if (sdrplay_api_Uninit == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Uninit\n");
	   return false;
	}

	sdrplay_api_Update		= (sdrplay_api_Update_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Update");
	if (sdrplay_api_Update == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Update\n");
	   return false;
	}

	return true;
}

void	sdrplayHandler_v3::show		() {
	myFrame. show ();
}

void	sdrplayHandler_v3::hide		() {
	myFrame. hide ();
}

bool	sdrplayHandler_v3::isHidden	() {
	return myFrame. isHidden ();
}

void	sdrplayHandler_v3::set_biasValue (int biasT_value) {
int err	= sdrplay_api_GetDeviceParams (chosenDevice -> dev, &deviceParams);
sdrplay_api_RxChannelParamsT *channelParams =
	                                 deviceParams -> rxChannelA;
sdrplay_api_ReasonForUpdateT updateValue;

	switch (hwVersion) {
	   case SDRPLAY_RSP1_ID:
	      return;;			// no BiasT support

	   case SDRPLAY_RSP1A_ID: {
	      sdrplay_api_Rsp1aTunerParamsT *rsp1aTunerParams;
	      rsp1aTunerParams	= &(channelParams -> rsp1aTunerParams);
	      rsp1aTunerParams -> biasTEnable = biasT_value;
	      updateValue	= sdrplay_api_Update_Rsp1a_BiasTControl;
	      (void) sdrplay_api_Update (chosenDevice -> dev,
                                         chosenDevice -> tuner,
	                                 updateValue,
                                         sdrplay_api_Update_Ext1_None);
	   }
	   return;

	   case SDRPLAY_RSP2_ID: {
	      sdrplay_api_Rsp2TunerParamsT *rsp2TunerParams;
	      rsp2TunerParams	= &(channelParams -> rsp2TunerParams);
	      rsp2TunerParams -> biasTEnable = biasT_value;
	      updateValue	= sdrplay_api_Update_Rsp2_BiasTControl;
	      (void) sdrplay_api_Update (chosenDevice -> dev,
                                         chosenDevice -> tuner,
	                                 updateValue,
                                         sdrplay_api_Update_Ext1_None);
	   }
	   return;

	   case SDRPLAY_RSPduo_ID: {
	      sdrplay_api_RspDuoTunerParamsT *rspDuoTunerParams;
	      rspDuoTunerParams = &(channelParams -> rspDuoTunerParams);
	      rspDuoTunerParams -> biasTEnable = biasT_value;
	      updateValue	= sdrplay_api_Update_RspDuo_BiasTControl;
	      (void) sdrplay_api_Update (chosenDevice -> dev,
                                         chosenDevice -> tuner,
	                                 updateValue,
                                         sdrplay_api_Update_Ext1_None);
	   }
	   return;
//
//	Unfortunately, Dx is different from the others
	   case SDRPLAY_RSPdx_ID: {
	      sdrplay_api_DevParamsT *xxx;
	      sdrplay_api_RspDxParamsT *rspDxParams;
	      xxx = deviceParams -> devParams;
	      rspDxParams	= &(xxx -> rspDxParams);
	      rspDxParams	-> biasTEnable = biasT_value;
	      (void) sdrplay_api_Update (chosenDevice -> dev,
	                                 chosenDevice -> tuner,
	                                 sdrplay_api_Update_None,
	               	                 sdrplay_api_Update_RspDx_BiasTControl);
	   }
	   return;

	   default:
	      return;
	}
}

