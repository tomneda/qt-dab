

#include	"xml-handler.h"
#include	<cstdio>
#include	<time.h>

struct kort_woord {
	uint8_t byte_1;
	uint8_t byte_2;
};

	xmlHandler::xmlHandler (FILE *f, int denominator, int frequency) {
uint8_t t	= 0;
	xmlFile		= f;
	this	-> denominator	= denominator;
	this	-> frequency	= frequency;
	for (int i = 0; i < 5000; i ++)
	   fwrite (&t, 1, 1, f);
	int16_t testWord	= 0xFF;

	struct kort_woord *p	= (struct kort_woord *)(&testWord);
	if (p -> byte_1  == 0xFF)
	   byteOrder	= "LSB";
	else
	   byteOrder	= "MSB";
	nrElements	= 0;
}

	xmlHandler::~xmlHandler	() {
}

void	xmlHandler::computeHeader	(QString &version, QString &model) {
QString s;
QString	topLine = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
	if (xmlFile == nullptr)
	   return;
	s = create_xmltree	(version, model);
	fseek (xmlFile, 0, SEEK_SET);
	fprintf (xmlFile, topLine. toLatin1 (). data ());
	char * cs = s. toLatin1 (). data ();
	int len = strlen (cs);
	fwrite (cs, 1, len, xmlFile);
}

#define	BLOCK_SIZE	8192
static int16_t buffer [BLOCK_SIZE];
static int bufferP	= 0;
void	xmlHandler::add		(std::complex<int16_t> * data, int count) {
	nrElements	+= 2 * count;
	for (int i = 0; i < count; i ++) {
	   buffer [bufferP ++] = real (data [i]);
	   buffer [bufferP ++] = imag (data [i]);
	   if (bufferP >= BLOCK_SIZE) {
	      fwrite (buffer, sizeof (int16_t), BLOCK_SIZE, xmlFile);
	      bufferP = 0;
	   }
	}
}

QString	xmlHandler::create_xmltree (QString  &version,
	                            QString  &Model) {
QDomDocument theTree;
QDomElement root	= theTree. createElement ("SDR");

	time_t rawtime;
	struct tm *timeinfo;
	time (&rawtime);
	timeinfo	= localtime (&rawtime);

	theTree. appendChild (root);
	QDomElement theRecorder = theTree. createElement ("Recorder");
	theRecorder. setAttribute ("Name", "Qt-DAB");
	theRecorder. setAttribute ("Version", version);
	root. appendChild (theRecorder);
	QDomElement theDevice = theTree. createElement ("Device");
	theDevice. setAttribute ("Name", "SDRplay");
	theDevice. setAttribute ("Model", Model);
	root. appendChild (theDevice);
	QDomElement theTime = theTree. createElement ("Time");
	theTime. setAttribute ("Unit", "UTC");
	char help [256];
	strcpy (help, asctime (timeinfo));
	help [strlen (help)] = 0;	// get rid of \n
	theTime. setAttribute ("Value", asctime (timeinfo));
	root. appendChild (theTime);
	QDomElement theSample = theTree. createElement ("Sample");
	QDomElement theRate   = theTree. createElement ("Samplerate");
	theRate. setAttribute ("Unit", "Hz");
	theRate. setAttribute ("Value", "2048000");
	theSample. appendChild (theRate);
	QDomElement theChannels = theTree. createElement ("Channels");
	theChannels. setAttribute ("Bits", "14");
	theChannels. setAttribute ("Container", "int16");
	theChannels. setAttribute ("Ordering", byteOrder);
	QDomElement I_Channel = theTree. createElement ("Channel");
	I_Channel. setAttribute ("Value", "I");
	theChannels. appendChild (I_Channel);
	QDomElement Q_Channel = theTree. createElement ("Channel");
	Q_Channel. setAttribute ("Value", "Q");
	theChannels. appendChild (Q_Channel);
	theSample. appendChild (theChannels);
	root. appendChild (theSample);

	QDomElement theDataBlocks	= theTree. createElement ("Datablocks");
	QDomElement theDataBlock	= theTree. createElement ("Datablock");
	theDataBlock. setAttribute ("Count", QString::number (nrElements));
	theDataBlock. setAttribute ("Number", "1");
	theDataBlock. setAttribute ("Unit",  "Channel");
	QDomElement theFrequency	= theTree. createElement ("Frequency");	
	theFrequency. setAttribute ("Value", 
	                                 QString::number (frequency / 1000));
	theFrequency. setAttribute ("Unit", "KHz");
	theDataBlock. appendChild (theFrequency);
	QDomElement theModulation	= theTree. createElement ("Modulation");
	theModulation. setAttribute ("Value", "DAB");
	theDataBlock. appendChild (theModulation);
	theDataBlocks. appendChild (theDataBlock);
	root. appendChild (theDataBlocks);

	return theTree. toString ();
}

