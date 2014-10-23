/*
  This file is a part of KMC software distributed under GNU GPL 3 licence.
  The homepage of the KMC project is http://sun.aei.polsl.pl/kmc

  Authors: Sebastian Deorowicz and Agnieszka Debudaj-Grabysz

  Version: 2.0
  Date   : 2014-07-04
*/


#ifndef _KMER_DEFS_H
#define _KMER_DEFS_H

#define KMC_VER		"2.0"
#define KMC_DATE	"2014-07-04"

#define MIN(x,y)	((x) < (y) ? (x) : (y))

#ifndef WIN32
	#include <stdint.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <math.h>
	#include <string.h>

	#define _TCHAR	char
	#define _tmain	main

	#define my_fopen    fopen
	#define my_fseek    fseek
	#define my_ftell    ftell
#else
	#define my_fopen    fopen
	#define my_fseek    _fseeki64
	#define my_ftell    _ftelli64
#endif
	//typedef unsigned char uchar;

	typedef int int32;
	typedef unsigned int uint32;
	typedef long long int64;
	typedef unsigned long long uint64;
	typedef unsigned char uchar;
#endif

// ***** EOF
