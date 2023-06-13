#
/*
 *    Copyright (C) 2014 .. 2019
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
 *
 */
#ifndef	__XML_READER_H
#define	__XML_READER_H

#include "dab-constants.h"
#include	<QThread>
#include	<QMessageBox>
#include	<cstdio>
#include	"ringbuffer.h"
#include	<stdint.h>
#include	<vector>
#include	<atomic>

class	xml_fileReader;
class	xmlDescriptor;

class	xml_Reader:public QThread {
Q_OBJECT
public:
			xml_Reader (xml_fileReader	*mr,
	                            FILE		*f,
	                            xmlDescriptor	*fd,
	                            uint32_t		filePointer,
	                            RingBuffer<cmplx> *b);
			~xml_Reader	();
	void		stopReader	();
	bool		handle_continuousButton	();
private:
	std::atomic<bool>	continuous;
	FILE		*file;
	xmlDescriptor	*fd;
	uint32_t	filePointer;
	RingBuffer<cmplx> *sampleBuffer;
	xml_fileReader	*parent;
	int		nrElements;
	int		samplesToRead;
	std::atomic<bool> running;
	void		run ();
	int		compute_nrSamples 	(FILE *f, int blockNumber);
	int		readSamples		(FILE *f, 
	                                       void(xml_Reader::*)(FILE *,
	                                          cmplx *, int));
	void		readElements_IQ		(FILE *f,
	                                         cmplx *, int amount);
	void		readElements_QI		(FILE *f, 
	                                         cmplx *, int amount);
	void		readElements_I		(FILE *f, 
	                                         cmplx *, int amount);
	void		readElements_Q		(FILE *f, 
	                                         cmplx *, int amount);
//
//	for the conversion - if any
	int16_t         convBufferSize;
        int16_t         convIndex;
        std::vector <cmplx>   convBuffer;
        int16_t         mapTable_int   [2048];
        float           mapTable_float [2048];

signals:
	void		setProgress		(int, int);
};

#endif
