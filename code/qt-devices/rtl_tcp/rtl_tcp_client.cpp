#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB program
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
 *
 *    A simple client for rtl_tcp
 */

#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QHostAddress>
#include	<QTcpSocket>
#include	<QFileDialog>
#include	<QDir>
#include	"rtl_tcp_client.h"

#include	"device-exceptions.h"

#define	DEFAULT_FREQUENCY	(Khz (220000))

	rtl_tcp_client::rtl_tcp_client	(QSettings *s):
	                                    myFrame (nullptr) {
	remoteSettings		= s;

	setupUi (&myFrame);
	myFrame. show		();
	myFrame. hide		();
	myFrame. show		();

    //	setting the defaults and constants
	theRate		= 2048000;
	remoteSettings	-> beginGroup ("rtl_tcp_client");
	theGain		= remoteSettings ->
	                          value ("rtl_tcp_client-gain", 20). toInt();
	thePpm		= remoteSettings ->
	                          value ("rtl_tcp_client-ppm", 0). toInt();
	vfoOffset	= remoteSettings ->
	                          value ("rtl_tcp_client-offset", 0). toInt();
	basePort = remoteSettings -> value ("rtl_tcp_port", 1234).toInt();
	remoteSettings	-> endGroup();
	tcp_gain	-> setValue (theGain);
	tcp_ppm		-> setValue (thePpm);
	vfoFrequency	= DEFAULT_FREQUENCY;
	_I_Buffer	= new RingBuffer<cmplx>(32 * 32768);
	connected	= false;
	hostLineEdit 	= new QLineEdit (nullptr);
	dumping		= false;
//
	connect (tcp_connect, SIGNAL (clicked (void)),
	         this, SLOT (wantConnect (void)));
	connect (tcp_disconnect, SIGNAL (clicked (void)),
	         this, SLOT (setDisconnect (void)));
	connect (tcp_gain, SIGNAL (valueChanged (int)),
	         this, SLOT (sendGain (int)));
	connect (tcp_ppm, SIGNAL (valueChanged (int)),
	         this, SLOT (set_fCorrection (int)));
	connect (khzOffset, SIGNAL (valueChanged (int)),
	         this, SLOT (set_Offset (int)));
	theState	-> setText ("waiting to start");
}

	rtl_tcp_client::~rtl_tcp_client() {
	remoteSettings ->  beginGroup ("rtl_tcp_client");
	if (connected) {		// close previous connection
	   stopReader();
//	   streamer. close();
	   remoteSettings -> setValue ("remote-server",
	                               toServer. peerAddress(). toString());
	   QByteArray datagram;
	}
	remoteSettings -> setValue ("rtl_tcp_client-gain",   theGain);
	remoteSettings -> setValue ("rtl_tcp_client-ppm",    thePpm);
	remoteSettings -> setValue ("rtl_tcp_client-offset", vfoOffset);
	remoteSettings -> endGroup();
	toServer. close();
	delete	_I_Buffer;
	delete	hostLineEdit;
}
//
void	rtl_tcp_client::wantConnect() {
QString ipAddress;
int16_t	i;
QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

	if (connected)
	   return;
	// use the first non-localhost IPv4 address
	for (i = 0; i < ipAddressesList.size(); ++i) {
	   if (ipAddressesList.at (i) != QHostAddress::LocalHost &&
	      ipAddressesList. at (i). toIPv4Address()) {
	      ipAddress = ipAddressesList. at(i). toString();
	      break;
	   }
	}
	// if we did not find one, use IPv4 localhost
	if (ipAddress. isEmpty())
	   ipAddress = QHostAddress (QHostAddress::LocalHost).toString();
	remoteSettings -> beginGroup ("rtl_tcp_client");
	ipAddress = remoteSettings ->
	                value ("remote-server", ipAddress). toString();
	remoteSettings -> endGroup();
	hostLineEdit -> setText (ipAddress);

	hostLineEdit	-> setInputMask ("000.000.000.000");
//	Setting default IP address
	hostLineEdit	-> show();
	theState	-> setText ("Enter IP address, \nthen press return");
	connect (hostLineEdit, SIGNAL (returnPressed (void)),
	         this, SLOT (setConnection (void)));
}

//	if/when a return is pressed in the line edit,
//	a signal appears and we are able to collect the
//	inserted text. The format is the IP-V4 format.
//	Using this text, we try to connect,
void	rtl_tcp_client::setConnection() {
QString s	= hostLineEdit -> text();
QHostAddress theAddress	= QHostAddress (s);

	serverAddress	= QHostAddress (s);
	disconnect (hostLineEdit, SIGNAL (returnPressed (void)),
	            this, SLOT (setConnection (void)));
	toServer. connectToHost (serverAddress, basePort);
	if (!toServer. waitForConnected (2000)) {
	   QMessageBox::warning (&myFrame, tr ("sdr"),
	                                   tr ("connection failed\n"));
	   return;
	}

	sendGain (theGain);
	sendRate (theRate);
	sendVFO	(DEFAULT_FREQUENCY - theRate / 4);
	toServer. waitForBytesWritten();
	theState -> setText ("Connected");
	connected	= true;
}

int32_t	rtl_tcp_client::getRate	() {
	return theRate;
}

int32_t	rtl_tcp_client::defaultFrequency() {
	return DEFAULT_FREQUENCY;	// choose any legal frequency here
}

void	rtl_tcp_client::setVFOFrequency	(int32_t newFrequency) {
	if (!connected)
	   return;
	vfoFrequency	= newFrequency;
//	here the command to set the frequency
	sendVFO (newFrequency);
}

int32_t	rtl_tcp_client::getVFOFrequency() {
	return vfoFrequency;
}

bool	rtl_tcp_client::restartReader	(int32_t freq) {
	if (!connected)
	   return true;
	vfoFrequency	= freq;
//	here the command to set the frequency
	sendVFO (freq);
	connect (&toServer, SIGNAL (readyRead (void)),
	         this, SLOT (readData (void)));
	return true;
}

void	rtl_tcp_client::stopReader() {
	if (!connected)
	   return;
	disconnect (&toServer, SIGNAL (readyRead (void)),
	            this, SLOT (readData (void)));
}
//
//
//	The brave old getSamples. For the dab stick, we get
//	size: still in I/Q pairs, but we have to convert the data from
//	uint8_t to DSPCOMPLEX *
int32_t	rtl_tcp_client::getSamples (cmplx *V, int32_t size) {
int32_t	amount =  0;
	amount = _I_Buffer	-> getDataFromBuffer (V, size);
	return amount;
}

int32_t	rtl_tcp_client::Samples() {
	return  _I_Buffer	-> GetRingBufferReadAvailable ();
}
//
int16_t	rtl_tcp_client::bitDepth() {
	return 8;
}

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

//	These functions are typical for network use
void	rtl_tcp_client::readData() {
uint8_t	buffer [8192];
cmplx localBuffer [4096];

	while (toServer. bytesAvailable() > 8192) {
	   toServer. read ((char *)buffer, 8192);
	   for (int i = 0; i < 4096; i ++)
	      localBuffer [i] = cmplx (
	                                    mapTable [buffer [2 * i]],
	                                    mapTable [buffer [2 * i + 1]]);
	   _I_Buffer -> putDataIntoBuffer (localBuffer, 4096);
	}
}
//
//
//	commands are packed in 5 bytes, one "command byte" 
//	and an integer parameter
struct command {
	unsigned char cmd;
	unsigned int param;
}__attribute__((packed));

#define	ONE_BYTE	8

void	rtl_tcp_client::sendCommand (uint8_t cmd, int32_t param) {
QByteArray datagram;

	datagram. resize (5);
	datagram [0] = cmd;		// command to set rate
	datagram [4] = param & 0xFF;  //lsb last
	datagram [3] = (param >> ONE_BYTE) & 0xFF;
	datagram [2] = (param >> (2 * ONE_BYTE)) & 0xFF;
	datagram [1] = (param >> (3 * ONE_BYTE)) & 0xFF;
	toServer. write (datagram. data(), datagram. size());
}

void rtl_tcp_client::sendVFO (int32_t frequency) {
	sendCommand (0x01, frequency);
}

void	rtl_tcp_client::sendRate (int32_t theRate) {
	sendCommand (0x02, theRate);
}

void	rtl_tcp_client::setGainMode (int32_t gainMode) {
	sendCommand (0x03, gainMode);
}

void	rtl_tcp_client::sendGain (int gain) {
	sendCommand (0x04, 10 * gain);
	theGain		= gain;
}

//	correction is in ppm
void	rtl_tcp_client::set_fCorrection	(int32_t ppm) {
	sendCommand (0x05, ppm);
	thePpm		= ppm;
}

void	rtl_tcp_client::setDisconnect() {
	if (connected) {		// close previous connection
	   stopReader();
	   remoteSettings -> beginGroup ("rtl_tcp_client");
	   remoteSettings -> setValue ("remote-server",
	                               toServer. peerAddress(). toString());
	   remoteSettings -> setValue ("rtl_tcp_client-gain", theGain);
	   remoteSettings -> setValue ("rtl_tcp_client-ppm", thePpm);
	   remoteSettings -> endGroup();
	   toServer. close();
	}
	connected	= false;
	theState	-> setText ("disconnected");
}

void	rtl_tcp_client::set_Offset	(int32_t o) {
	sendCommand (0x0a, Khz (o));
	vfoOffset	= o;
}

void	rtl_tcp_client::show		() {
	myFrame. show ();
}

void	rtl_tcp_client::hide		() {
//	myFrame. hide ();
}

bool	rtl_tcp_client::isHidden	() {
	return myFrame. isHidden ();
}

