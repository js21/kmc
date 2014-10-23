/*
  This file is a part of KMC software distributed under GNU GPL 3 licence.
  The homepage of the KMC project is http://sun.aei.polsl.pl/kmc
  
  Authors: Sebastian Deorowicz, Agnieszka Debudaj-Grabysz, Marek Kokot
  
  Version: 2.0
  Date   : 2014-07-04
*/
#ifndef _RADIX_H
#define _RADIX_H

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <omp.h>
#include <algorithm>
#include "libs/asmlib.h"
#include "defs.h"
#include "queues.h"
#include <boost/static_assert.hpp>

#ifdef WIN32
typedef unsigned __int8 uint8_t;
#else
#include <stdint.h>
#endif

#define MAX_NUM_THREADS 32
#define BUFFER_WIDTH 32
#define ALIGNMENT 0x100
#define WIN_ALIGNMENT 64

#define shift_BUFFER_WIDTH 5
#define BUFFER_WIDTH_MINUS_1 31
#define BUFFER_WIDTH_MUL_sizeof_UINT 256

void RadixSort_uint8(uint32 *&data_ptr, uint32 *&tmp_ptr, uint64 size, unsigned rec_size, unsigned data_offset, unsigned data_size, const unsigned n_phases, const unsigned n_threads);
void RadixSort_buffer(CMemoryPool *pmm_radix_buf, uint64 *&data, uint64 *&tmp, uint64 size, const unsigned n_phases, const unsigned n_threads);

#endif

// ***** EOF
