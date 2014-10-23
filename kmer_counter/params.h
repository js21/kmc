/*
  This file is a part of KMC software distributed under GNU GPL 3 licence.
  The homepage of the KMC project is http://sun.aei.polsl.pl/kmc
  
  Authors: Sebastian Deorowicz, Agnieszka Debudaj-Grabysz, Marek Kokot
  
  Version: 2.0
  Date   : 2014-07-04
*/

#ifndef _PARAMS_H
#define _PARAMS_H

#include "defs.h"
#include "queues.h"
#include "s_mapper.h"
#include <vector>
#include <string>

typedef enum {fasta, fastq, multiline_fasta} input_type;

using namespace std;

// Structure for passing KMC parameters
struct CKMCParams {
	
	// Input parameters
	int p_m;							// max. total RAM usage
	int p_k;							// k-mer length
	int p_t;							// no. of threads
	int p_sf;							// no. of reading threads
	int p_sp;							// no. of splitting threads
	int p_so;							// no. of OpenMP threads for sorting
	int p_sr;							// no. of sorting threads	
	int p_ci;							// do not count k-mers occurring less than
	int p_cx;							// do not count k-mers occurring more than
	int p_cs;							// maximal counter value
	bool p_quake;						// use Quake-compatibile counting
	bool p_mem_mode;					// use RAM instead of disk
	int p_quality;						// lowest quality
	input_type p_file_type;				// input in FASTA format
	bool p_verbose;						// verbose mode
	bool p_both_strands;				// compute canonical k-mer representation
	int p_p1;							// signature length	

	// File names
	vector<string> input_file_names;
	string output_file_name;
	string working_directory;
	input_type file_type;
	
	uint32 lut_prefix_len;

	uint32 KMER_T_size;

	// Memory sizes
	int64 max_mem_size;				// maximum amount of memory to be used in GBs;	default: 30GB
	int64 max_mem_storer;			// maximum amount of memory for internal buffers of KmerStorer
	int64 max_mem_stage2;			// maximum amount of memory in stage 2
	int64 max_mem_storer_pkg;		// maximum amount of memory for single package

	int64 mem_tot_pmm_bins;			// maximal amount of memory per pool memory manager (PMM) of bin parts
	int64 mem_part_pmm_bins;		// maximal amount of memory per single part of memory maintained by PMM of bin parts
	int64 mem_tot_pmm_fastq;
	int64 mem_part_pmm_fastq;
	int64 mem_part_pmm_reads;
	int64 mem_tot_pmm_reads;
	int64 mem_part_pmm_radix_buf;
	int64 mem_tot_pmm_radix_buf;
	int64 mem_part_pmm_prob;
	int64 mem_tot_pmm_prob;
	int64 mem_part_pmm_cnts_sort;	
	int64 mem_tot_pmm_stats;
	int64 mem_part_pmm_stats;
	
	int64 mem_tot_pmm_epxand;
	int64 mem_part_pmm_epxand;

	bool verbose;	

	int kmer_len;			// kmer length
	int signature_len;
	int cutoff_min;			// exclude k-mers occurring less than times
	int cutoff_max;			// exclude k-mers occurring more than times
	int counter_max;		// maximal counter value
	bool use_quake;			// use Quake's counting based on qualities
	int lowest_quality;		// lowest quality value	    
	bool both_strands;		// find canonical representation of each k-mer
	bool mem_mode;			// use RAM instead of disk

	int n_bins;				// number of bins; fixed: 448
	int bin_part_size;		// size of a bin part; fixed: 2^15
	int fastq_buffer_size;	// size of FASTQ file buffer; fixed: 2^23

	int n_threads;			// number of cores
	int n_readers;			// number of FASTQ readers; default: 1
	int n_splitters;		// number of splitters; default: 1
	int n_sorters;			// number of sorters; default: 1
	vector<int> n_omp_threads;// number of OMP threads per sorters
	uint32 max_x;					//k+x-mers will be counted

	uint32 gzip_buffer_size;
	uint32 bzip2_buffer_size;

	CKMCParams()
	{
		p_m = 12;
		p_k = 25;
		p_t = 0;
		p_sf = 0;
		p_sp = 0;
		p_so = 0;
		p_sr = 0;
		p_ci = 2;
		p_cx = 1000000000;
		p_cs = 255;
		p_quake = false;
		p_mem_mode = false;
		p_quality = 33;
		p_file_type = fastq;
		p_verbose = false;
		p_both_strands = true;
		p_p1 = 7;		

		gzip_buffer_size  = 64 << 20;
		bzip2_buffer_size = 64 << 20;
	}
};

// Structure for passing KMC queues and monitors to threads
struct CKMCQueues 
{
	//Signature mapper
	CSignatureMapper* s_mapper;
	// Memory monitors
	CMemoryMonitor *mm;

	// Queues
	CInputFilesQueue *input_files_queue;
	CPartQueue *part_queue;
	CStatsPartQueue* stats_part_queue;

	CBinPartQueue *bpq;
	CBinDesc *bd;
	CBinQueue *bq;
	CKmerQueue *kq;
	CMemoryPool *pmm_bins, *pmm_fastq, *pmm_reads, *pmm_radix_buf, *pmm_prob, *pmm_stats, *pmm_expand;
	CMemoryBins *memory_bins;

	CKMCQueues() {}
};

#endif

// ***** EOF
