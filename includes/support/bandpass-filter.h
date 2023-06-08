
#ifndef	__BANDPASSFIR__
#define	__BANDPASSFIR__
/*
 *	The bandfilter is for the complex domain. 
 *	We create a lowpass filter, which stretches over the
 *	positive and negative half, then shift this filter
 *	to the right position to form a nice bandfilter.
 *	For the real domain, we use the Simple BandPass version.
 */

#include	"dab-constants.h"

class	BandPassFIR {
public:
		BandPassFIR	(int16_t filterSize,
	                          int32_t low, int32_t high,
	                          int32_t sampleRate);
		~BandPassFIR	();
cmplx	Pass	(cmplx);
float		Pass		(float);
private:

	int	filterSize;
	int	sampleRate;
	int	ip;
	cmplx *kernel;
	cmplx *buffer;
};
#endif
