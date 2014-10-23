/*
  This file is a part of KMC software distributed under GNU GPL 3 licence.
  The homepage of the KMC project is http://sun.aei.polsl.pl/kmc
  
  Authors: Sebastian Deorowicz, Agnieszka Debudaj-Grabysz, Marek Kokot
  
  Version: 2.0
  Date   : 2014-07-04
*/

#ifndef _DEFS_H
#define _DEFS_H

#define KMC_VER		"2.0"
#define KMC_DATE	"2014-07-04"

#define _CRT_SECURE_NO_WARNINGS

#define MIN(x,y)	((x) < (y) ? (x) : (y))
#define MAX(x,y)	((x) > (y) ? (x) : (y))
#define NORM(x, lower, upper)	((x) < (lower) ? (lower) : (x) > (upper) ? (upper) : (x))

#define uchar	unsigned char

#include <time.h>

//#define DEBUG_MODE
//#define DEVELOP_MODE

#define USE_META_PROG

#define KMER_X		3

#define STATS_FASTQ_SIZE (1 << 28)

#define EXPAND_BUFFER_RECS (1 << 16)


#define MAX_BINS 512


#ifndef MAX_K
#define MAX_K		256
#endif

#define MIN_K		10

#define MIN_MEM		4

// Range of number of FASTQ/FASTA reading threads
#define MIN_SF		1
#define MAX_SF		32

// Range of number of signature length
#define MIN_SL		5
#define MAX_SL		8

// Range of number of splitting threads
#define MIN_SP		1
#define MAX_SP		64

// Range of number of sorting threads
#define MIN_SO		1
#define MAX_SO		64

// Range of number of threads per single sorting thread
#define MIN_SR		1
#define MAX_SR		16


typedef float	count_t;

#define KMER_WORDS		((MAX_K + 31) / 32)

#ifdef _DEBUG
#define A_memcpy	memcpy
#define A_memset	memset
#endif


// Choose between:
//#define BOOST_THREAD			// Boost threads
//#define THREADS_NATIVE		// C++11 threads

#ifdef WIN32
#define my_fopen	fopen
#define my_fseek	_fseeki64
#define my_ftell	_ftelli64
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

#define THREADS_BOOST
//#define THREADS_NATIVE
#else
#define my_fopen	fopen
#define my_fseek	fseek
#define my_ftell	ftell
#define _TCHAR	char
#define _tmain	main

typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define THREADS_BOOST
//#define THREADS_NATIVE
#endif


const int32 MAX_STR_LEN = 32768;
#define ALIGNMENT 0x100

#define BYTE_LOG(x) (((x) < (1 << 8)) ? 1 : ((x) < (1 << 16)) ? 2 : ((x) < (1 << 24)) ? 3 : 4)

#endif

// ***** EOF
