#
/*
 *    Copyright (C) 2013 .. 2020
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
 *    along with Qt-TAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<stdio.h>
#include	<stdint.h>
#include	<stdlib.h>
#include	"epg-decoder.h"

#define	EPG_TAG			0X02
#define	SERVICE_TAG		0X03

//
	epgDecoder::epgDecoder	() {
}

	epgDecoder::~epgDecoder	() {
}
	  

static
uint8_t	bitTable [] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

int	epgDecoder::getBit (uint8_t *v, int bitnr) {
int bytenr	= bitnr / 8;

	bitnr 	= bitnr % 8;
	return (v [bytenr] & bitTable [bitnr]) != 0 ? 1 : 0;
}

uint32_t epgDecoder::getBits (uint8_t *v, int bitnr, int length) {
uint16_t res	= 0;
	for (int i = 0; i < length; i ++) {
	   res <<= 1;
	   res |= getBit (v, bitnr + i);
	}
	return res;
}

int	epgDecoder::process_epg	(uint8_t *v, int e_length,
	                         uint32_t SId, int subType) {
int	ind	= 0;
uint8_t	tag	= v [0];
int length	= v [1];
int	index	= 0;

	this	-> SId	= SId;
	this	-> subType	= subType;
	if ((v [0] != EPG_TAG) && (v [0] != SERVICE_TAG))
	   return e_length;

	if (length == 0xFE) {
	   length = (v [2] << 8) | v [3];
	   index	= 4;
	}
	else
	if (length == 0xFF) {
	   length = (v [2] << 16) | (v [3] << 8) | v [4];
	   index	= 5;
	}
	else
	   index	= 2;

	int endPoint	= index + length;
	if (tag == EPG_TAG) {
	   while (index < endPoint) {
	      switch (v [index]) {
	         case 0x04:		// process tokenTable
	            index = process_tokenTable (v, index);
	            break;

	         case 0x20:	// process ProgramGroups
	            index = process_programGroups (v, index);
	            break;

	         case 0x21:	// process_schedule (v, index);
	            index = process_schedule (v, index);
	            break;

	         case 0x05:	// obsolete
	            index = process_obsolete (v, index);
	            break;

	         case 0x06:	// default language
	            index = process_defaultLanguage (v, index);
	            break;

	         default:
	            return endPoint;
	      }
	   }
	   return endPoint;
	}
	
	if (tag == SERVICE_TAG)	{	// superfluous test
	   switch (v [index]) {
	      case 0x06:	// default language
	         index = process_defaultLanguage (v, index);
	         break;

	      case 0x26:	// ensemble
	         index = process_ensemble (v, index);
	         break;

	      case 0x28:	// process_service
	         index = process_service (v, index);
	         break;

	      case 0x80:	// version
	         index = process_483 (v, index);
	         break;

	      case 0x81:	// creation time
	         index = process_474 (v, index,  nullptr);
	         break;

	      case 0x82:	// originator
	         index =  process_440 (v, index);
	         break;

	      case 0x83:	// serviceProvider
	         index	= process_440 (v, index);
	   }
	}
	return endPoint;
}

int	epgDecoder::process_programGroups (uint8_t *v, int index) {
uint8_t	tag	= v [index];
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x23:	// programGroup
	         index	= process_programGroup (v, index);
	         break;

	      case 0x80:	// Version
	         index = process_483 (v, index);
	         break;

	      case 0x81:	// creation Time
	         index = process_474 (v, index, nullptr);
	         break;

	      case 0x82:	// Originator
	         index = process_440 (v, index);
	         break;

	      default:
	         fprintf (stderr, " in programGroups we missed %x\n", v [index]);
	         return endPoint;
	    }
	}
	return endPoint;
}

int	epgDecoder::process_programGroup (uint8_t *v, int index) {
int length	= v [index + 1];
progDesc p;
	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	p. clean ();
	while (index < endPoint) {
//	   fprintf (stderr, "In process_programmeGroup tag %x\n", v [index]);
	   switch (v [index]) {
	      case 0x10:	// shortName
	         index = process_shortName (v, index, &p);
	         break;

	      case 0x11:	// mediumName
	         index = process_mediumName (v, index, &p);
	         break;

	      case 0x12:	// longName
	         index = process_longName (v, index, &p);
	         break;

	      case 0x13:	// mediaDescription
	         index = process_mediaDescription (v, index, &p);
	         break;

	      case 0x14:	// genre
	         index =  process_genre (v, index, &p);
	         break;

	      case 0x16:	// keywords
	         index = process_keyWords (v, index);
	         break;

	      case 0x17:	// member of
	         index = process_memberOf (v, index);
	         break;

	      case 0x18:	// link
	         index	= process_link (v, index);
	         break;

	      case 0x20:	// programGroups
	         index	= process_programGroups (v, index);
	         break;

	      case 0x80:	// id
	         index	= process_471 (v, index);
	         break;

	      case 0x81:	// shortId
	         index	= process_472 (v, index);
	         break;

	      case 0x82:	// version
	         index	= process_483 (v, index);
	         break;

	      case 0x83:	// type
	         index	= process_46 (v, index);
	         break;

	      case 0x84:	// number of items
	         index	= process_484 (v, index);
	         break;

	      default:
	         fprintf (stderr, " in programGroup we missed %x\n", v [index]);
	         return endPoint;
	   }
	}
	record (&p);
	return endPoint;
}

int	epgDecoder::process_schedule (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
//	   fprintf (stderr, "in process_schedule with tag %x\n", v [index]);
	   switch (v [index]) {
	      case 0x1C:
	         index	= process_program (v, index);
	         break;

	      case 0x24:	// scope
	         index = process_scope (v, index);
	         break;

	      case 0x80:	// version
	         index	= process_483 (v, index);
	         break;

	      case 0x81:	// creation time
	         index	= process_474 (v, index, nullptr);
	         break;

	      case 0x82:	// originator
	         index	= process_440 (v, index);
	         break;

	      default:
	         fprintf (stderr, " in schedule missed %x\n", v [index]);
	         return endPoint;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_program (uint8_t *v, int index) {
int length	= v [index + 1];
progDesc theElement;

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	theElement. clean ();
	while (index < endPoint) {
//	   fprintf (stderr, "in process_programme with tag %x\n", v [index]);
	   switch (v [index]) {
	      case 0x10:	// shortName
	         index	= process_shortName (v, index, &theElement);
	         break;

	      case 0x11:	// mediumName
	         index	= process_mediumName (v, index, &theElement);
	         break;

	      case 0x12:	// longName
	         index	= process_longName (v, index, &theElement);
	         break;

	      case 0x13:	// mediaDescription
	         index	= process_mediaDescription (v, index, &theElement);
	         break;

	      case 0x14:	// genre
	         index	= process_genre (v, index, &theElement);
	         break;

	      case 0x16:	// keywords
	         index	= process_keyWords (v, index);
	         break;

	      case 0x17:	// memberOf
	         index	= process_memberOf (v, index);
	         break;

	      case 0x18:	// link
	         index	= process_link (v, index);
	         break;

	      case 0x19:	// location
	         index	= process_location (v, index, &theElement);
	         break;

	      case 0x2E:	// programmeEvent
	         index	= process_programmeEvent (v, index);
	         break;

	      case 0x36:	// inDemand
	         index	= process_onDemand (v, index);
	         break;

	      case 0x80:	// id
	         index	= process_471	(v, index);
	         break;

	      case 0x81:	// shortId
	         index	= process_472 (v, index);
	         break;

	      case 0x82:	// version
	         index	= process_483 (v, index);
	         break;

	      case 0x83:	// recommendation
	         index	= process_46 (v, index);
	         break;

	      case 0x84:	// broadcast
	         index	= process_46 (v, index);
	         break;

	      case 0x86:	// xml:lang
	         index	= process_481 (v, index);
	         break;

	      default:
	         fprintf (stderr, "In programme we missed %x\n", v [index]);
	         return endPoint;
	   }
	}
	record (&theElement);
	return endPoint;
}

int	epgDecoder::process_scope (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   int startTime;
	   int stopTime;
	   switch (v [index]) {
	      case 0x25:	// serviceScope
	         index	= process_serviceScope (v, index);
	         break;

	      case 0x80:	// start time
	         index	= process_474 (v, index,  nullptr);
	         break;

	      case 0x81:	// stop time
	         index	= process_474 (v, index, nullptr);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_serviceScope (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
//	   fprintf (stderr, "in serviceScope with tag %x\n", v [index]);
	   switch (v [index]) {
	      case 0x80:	// id
	         index	= process_476 (v, index);
	         break;

	      default:
	         fprintf (stderr, "in process_serviceScope %d\n", v [index]);
	   }
	}
	return endPoint;
}

int	epgDecoder::process_mediaDescription (uint8_t *v, int index,
	                                                       progDesc *p) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x1A:	// shortDescription
	         index	= process_shortDescription (v, index, p);
	         break;

	      case 0x1B:	// longDescription 
	         index	= process_longDescription (v, index, p);
	         break;

	      case 0x2B:	// multiMedia
	         index	= process_multiMedia (v, index);
	         break;

	      default:
	         return endPoint;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_ensemble (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
//	   fprintf (stderr, "in process_ensemble we have %x\n", v [index]);
	   switch (v [index]) {
	      case 0x10:	// shortName
	         index	= process_shortName (v, index, nullptr);
	         break;

	      case 0x11:	// mediumName
	         index	= process_mediumName	(v, index, nullptr);
	         break;

	      case 0x12:	// longName
	         index	= process_longName (v, index, nullptr);
	         break;

	      case 0x13:	// mediaDescription
	         index	= process_mediaDescription (v, index, nullptr);
	         break;

	      case 0x16:	// keywords
	         index	= process_keyWords (v, index);
	         break;

	      case 0x18:	// link
	         index	= process_link (v, index);
	         break;

	      case 0x28:	// service
	         index	= process_service (v, index);
	         break;

	      case 0x80:	// id
	         index	= process_4171 (v, index);
	         break;

	      default:
	         fprintf (stderr, "Process_ensemble: We cannot deal with %x\n", v [index]);
	         return endPoint;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_service (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x10:	// shortName
	         index	= process_shortName (v, index, nullptr);
	         break;

	      case 0x11:	// mediumName
	         index	= process_mediumName (v, index, nullptr);
	         break;

	      case 0x12:	// longname
	         index	= process_longName	(v, index, nullptr);
	         break;

	      case 0x13:	// media description 
	         index	= process_mediaDescription (v, index, nullptr);
	         break;

	      case 0x14:	// genre
	         index	= process_genre (v, index, nullptr);
	         break;

	      case 0x16:	// keyWords
	         index	= process_keyWords (v, index);
	         break;

	      case 0x29:	// bearer
	         index	= process_bearer (v, index);
	         break;

	      case 0x31:	// radiodns
	         index	= process_radiodns (v, index);
	         break;

	      case 0x32:	// geolocation
	         index	= process_geoLocation (v, index);
	         break;

	      case 0x80:	// version
	         index	= process_483 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_location (uint8_t *v, int index, progDesc *p) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	if (p == nullptr)
	   return endPoint;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x2C:	// time	
	         index	= process_time	(v, index, &p -> startTime);
	         break;

	      case 0x29:	// bearer
	         index	= process_bearer (v, index);
	         break;

	      case 0x2F:	// relativeTime
	         process_relativeTime (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_bearer (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x32:	// geolocation
	         index	= process_geoLocation (v, index);
	         break;

	      case 0x80:	// id
	         index	= process_476 (v, index);
	         break;

	      case 0x82:	// url
	         index	= process_440 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_geoLocation (uint8_t *v, int index ) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x33:	//  country
	         index	= process_country (v, index);
	         break;

	      case 0x34:	// Point
	         index	= process_point (v, index);
	         break;

	      case 0x35:	// polygon
	         index	= process_polygon (v, index);
	         break;

	      case 0x80:	// xml:id
	         index	= process_440 (v, index);
	         break;

	      case 0x81:	// ref
	         index	= process_440 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_programmeEvent (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x10:	// shortName
	         index	= process_shortName (v, index, nullptr);
	         break;

	      case 0x11:	// mediumName
	         index	= process_mediumName (v, index, nullptr);
	         break;

	      case 0x12:	// longName
	         index	= process_longName (v, index, nullptr);
	         break;

	      case 0x13:	// mediaDescription
	         index	= process_mediaDescription (v, index, nullptr);
	         break;

	      case 0x14:	// genre
	         index	= process_genre (v, index, nullptr);
	         break;

	      case 0x16:	// keywords
	         index	= process_keyWords (v, index);
	         break;

	      case 0x17:	// memberOf
	         index	= process_memberOf (v, index);
	         break;

	      case 0x18:	// link
	         index	= process_link (v, index);
	         break;

	      case 0x19:	// location
	         index	= process_location (v, index, nullptr);
	         break;

	      case 0x36:	// onDemand
	         index	= process_onDemand (v, index);
	         break;

	      case 0x80:	// id
	         index	= process_471 (v, index);
	         break;

	      case 0x81:	// shortId
	         index	= process_472 (v, index);
	         break;

	      case 0x82:	// version
	         index	= process_483 (v, index);
	         break;

	      case 0x83:	// recommendation
	         index	= process_46 (v, index);
	         break;

	      case 0x84:	// broadcast
	         index	= process_46 (v, index);
	         break;

	      case 0x86:	// xml:lang
	         index	= process_481 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_onDemand (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x29:	// bearer
	         index	= process_bearer (v, index);
	         break;

	      case 0x37:	// presentation time
	         index	= process_presentationTime (v, index);
	         break;

	      case 0x38:	// acquisitionTime
	         index	= process_acquisitionTime (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_genre (uint8_t *v, int index, progDesc *p) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	if (p == nullptr)
	   return endPoint;
	
	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// href
	         index	= process_412 (v, index);
	         break;

	      case 0x81:	// type
	         index	= process_46 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_keyWords (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// xml:lang
	         index	= process_481 (v, index);
	         break;

	      default:
//	         fprintf (stderr, "in process_keyWords with %d\n", v [index]);
	         return endPoint;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_link (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// uri
	         index	= process_440 (v, index);
	         break;

	      case 0x81:	// mimeValue
	         index	= process_473 (v, index);
	         break;

	      case 0x82:	// xml:lang
	         index	= process_481 (v, index);
	         break;

	      case 0x83:	// description
	         index	= process_440 (v, index);
	         break;

	      case 0x84:	// expiryTime
	         index	= process_474 (v, index, nullptr);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_shortName (uint8_t *v, int index, progDesc *p) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	if (p == nullptr)
	   return endPoint;

	if (v [index + 1] == 1) {
	   p -> shortName = stringTable [v [index + 2]]. toLatin1 (). data ();
	   return endPoint;
	}
	char t [255];
	for (int i = 0; i < length; i ++)
	   t [i] =  v [index + i + 2];
	t [length] = 0;
	p -> shortName = QString::fromUtf8 (t);
	return endPoint;
}

int	epgDecoder::process_mediumName (uint8_t *v, int index, progDesc *p) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	if (p == nullptr)
	   return endPoint;

	if (v [index + 1] == 1) {
	   p -> mediumName = stringTable [v [index + 2]]. toLatin1 (). data ();
	   return endPoint;
	}
	char t [255];
	for (int i = 0; i < length; i ++)
	   t [i] =  v [index + i + 2];
	t [length] = 0;
	p -> mediumName = QString::fromUtf8 (t);
	return endPoint;
}

int	epgDecoder::process_longName (uint8_t *v, int index, progDesc *p) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	if (p == nullptr)
	   return endPoint;

	if (v [index + 1] == 1) {
	   p -> longName = stringTable [v [index + 2]]. toLatin1 (). data ();
	   return endPoint;
	}
	char t [255];
	for (int i = 0; i < length; i ++)
	  t [i] = v [index + i + 2];
	t [length] = 0;
	p -> longName = QString::fromUtf8 (t);
	return endPoint;
}

int	epgDecoder::process_shortDescription (uint8_t *v, int index,
	                                                    progDesc *p) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	if (p == nullptr)
	   return endPoint;

	if (v [index + 1] == 1) {
	   p -> shortDescription =
	            stringTable [v [index + 2]]. toLatin1 (). data ();
	   return endPoint;
	}

	char t [511];
	for (int i = 0; i < length; i ++)
	   t [i] = v [index + i + 2];
	t [length] = 0;
	p  -> shortDescription = QString::fromUtf8 (t);
	return endPoint;
}

int	epgDecoder::process_longDescription (uint8_t *v,
	                                     int index, progDesc *p) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	if (p == nullptr)
	   return endPoint;

	if (v [index + 1] == 1) {
	   p -> longDescription =
	            stringTable [v [index + 2]]. toLatin1 (). data ();
	   return endPoint;
	}
	char t [511];
	for (int i = 0; i < length; i ++)
	   t [i] = v [index + i + 2];
	t [length] = 0;
	p -> longDescription = QString::fromUtf8 (t);
	return endPoint;
}

int	epgDecoder::process_multiMedia (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// mimevalue	
	         index	= process_473 (v, index);
	         break;

	      case 0x81:	// xml_lang
	         index	= process_481 (v, index);
	         break;

	      case 0x82:	// uri
	         index	= process_440 (v, index);
	         break;

	      case 0x83:	// type
	         index	= process_46 (v, index);
	         break;

	      case 0x84:	// width
	         index	= process_485 (v, index);
	         break;

	      case 0x85:
	         index	= process_485 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_radiodns (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// fqdn
	         index	= process_440 (v, index);
	         break;

	      case 0x81:	// serviceIdentifier
	         index	= process_440 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_time (uint8_t *v, int index, int *t) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// time
	         index	= process_474 (v, index, t);
	         break;

	      case 0x81:	// duration
	         index	= process_475 (v, index);
	         break;

	      case 0x82:	// actualTime
	         index	= process_474 (v, index, nullptr);
	         break;
	
	      case 0x83:	// actual duration
	         index	= process_475 (v, index);
	         break;
	   }
	}
	return endPoint;
}
	
int	epgDecoder::process_relativeTime (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// time
	         index	= process_474 (v, index, nullptr);
	         break;

	      case 0x81:	// duration
	         index	= process_475 (v, index);
	         break;

	      case 0x82:	// actualTime
	         index	= process_474 (v, index, nullptr);
	         break;
	
	      case 0x83:	// actual duration
	         index	= process_475 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_memberOf (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// id
	         index	= process_471 (v, index);
	         break;

	      case 0x81:	// shortId
	         index	= process_472 (v, index);
	         break;

	      case 0x82:	// Index
	         index	= process_482 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_presentationTime (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// start
	         index	= process_474 (v, index, nullptr);
	         break;

	      case 0x81:	// end
	         index	= process_474 (v, index, nullptr);
	         break;

	      case 0x82:	// duration
	         index	= process_475 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_acquisitionTime (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;

	while (index < endPoint) {
	   switch (v [index]) {
	      case 0x80:	// start
	         index	= process_474 (v, index, nullptr);
	         break;

	      case 0x81:	// end
	         index	= process_474 (v, index, nullptr);
	         break;

	      case 0x82:	// duration
	         index	= process_475 (v, index);
	         break;
	   }
	}
	return endPoint;
}

int	epgDecoder::process_country	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	return endPoint;
}

int	epgDecoder::process_point	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	return endPoint;
}

int	epgDecoder::process_polygon	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint	= index + length;
	return endPoint;
}

//
//////////////////////////////////////////////////////////////////////////
//	
//	Attribute handlers
//
/////////////////////////////////////////////////////////////////////////////
//
int	epgDecoder::process_412	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

int	epgDecoder::process_440	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

int	epgDecoder::process_46	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

int	epgDecoder::process_471	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

int	epgDecoder::process_472	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

int	epgDecoder::process_473	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}
//
//	ETSI TS 102 371: 4.7.4 time point
int	epgDecoder::process_474	(uint8_t *v, int index, int *t) {
int length	= v [index + 1];
uint8_t ltoFlag;
uint8_t utcFlag;
int     hours;
int     minutes;
int     ltoBase;


	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;
	int endPoint	= index +  length;

	if (t == nullptr)
	   return endPoint;
	ltoFlag	= getBit (v, 8 * index + 19);
	utcFlag	= getBit (v, 8 * index + 20);

	hours   = getBits (v, 8 * index + 21, 5);
	minutes = getBits (v, 8 * index + 26, 6);

	if (ltoFlag) {
	   uint16_t halfHours = getBits (v, 8 * index + ltoBase, 8);
	  if (halfHours & 0x20)
              halfHours = - halfHours & 0x1F;
           else
              halfHours = halfHours & 0x1F;
           minutes += halfHours * 30;
        }
	*t	= hours * 60 + minutes;
	return index + length;
}
//
//	ETSI TS 102 371: 4.7.5 Duration type
int	epgDecoder::process_475	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int duration	= (v [index] << 8) | v [index + 1];
//	fprintf (stderr, "duration %d (%d)\n", duration / 60, duration % 60); 
	return index + length;
}

int	epgDecoder::process_476	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
	uint8_t ensFlag = getBit (v, 8 * index + 1);
	uint8_t sidFlag = getBit (v, 8 * index + 3);

        if (ensFlag == 1)
           if (sidFlag == 0)
              fprintf (stderr, "SId = %X\n", getBits (v, 8 * index + 32, 16));
	else
        if (ensFlag == 0)
           if (sidFlag == 0)
              fprintf (stderr, "SId = %X\n", getBits (v, 8 * index + 8, 16));
	return index + length;
}

int	epgDecoder::process_481	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;
//	fprintf (stderr, "481 returns %d\n", index + length);
	return index + length;
}

int	epgDecoder::process_482	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

int	epgDecoder::process_483	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

int	epgDecoder::process_484	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int numbers = (v [index] << 8) | v [index + 1];
//	fprintf (stderr, "484 says %d elements\n", numbers);
	return index + length;
}

int	epgDecoder::process_485	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

int	epgDecoder::process_4171	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

int	epgDecoder::process_tokenTable	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	int endPoint = index + length;
//	token table element, section 4.9
	while (index < endPoint)
	   index = process_token (v, index);
	return endPoint;
}

int	epgDecoder::process_token (uint8_t *v, int index) {
uint8_t tag	= v [index];
int	length	= v [index + 1];
char	text [length + 1];

	index += 2;
	for (int i = 0; i < length; i ++)
	   text [i] = v [index + i];
	text [length] = 0;
	stringTable [tag] = QString::fromUtf8 (text);
	return index + length;
}

int	epgDecoder::process_defaultLanguage	(uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

//	fprintf (stderr, "default language with length %d\n", length);
	return index + length;
}

int	epgDecoder::process_obsolete (uint8_t *v, int index) {
int length	= v [index + 1];

	if (length == 0XFE) {
	   length = (v [index + 2] << 8) | v [index + 3];
	   index += 4;
	}
	else
	if (length == 0XFF) {
	   length = (v [index + 2] << 16) | (v [index + 3] << 8) | v [index + 4];
	   index += 5;
	}
	else
	   index += 2;

	return index + length;
}

void	epgDecoder::record (progDesc *theElement) {
	if (theElement -> startTime == -1)
	   return;
	
	if ((theElement -> mediumName == QString ("")) &&
	    (theElement -> longName   == QString ("")))
	   return;

	set_epgData (this -> SId, theElement -> startTime,
	             theElement -> longName != QString ("") ?
	                 theElement -> longName:
	                 theElement -> mediumName,
	             theElement -> shortDescription);
}
