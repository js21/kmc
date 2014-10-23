/*
  This file is a part of KMC software distributed under GNU GPL 3 licence.
  The homepage of the KMC project is http://sun.aei.polsl.pl/kmc
  
  Authors: Sebastian Deorowicz, Agnieszka Debudaj-Grabysz, Marek Kokot
  
  Version: 2.0
  Date   : 2014-07-04
*/

#ifndef _QUEUES_H
#define _QUEUES_H

#include "defs.h"
#include <stdio.h>
#include <iostream>
#include <tuple>
#include <queue>
#include <list>
#include <map>
#include <string>
#include "mem_disk_file.h"

using namespace std;

#ifdef THREADS_NATIVE			// C++11 threads
#include <thread>
#include <mutex>
#include <condition_variable>

using std::thread;
#else							// Boost threads
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

using namespace boost;
#endif

//************************************************************************************************************
class CInputFilesQueue {
	typedef string elem_t;
	typedef queue<elem_t, list<elem_t>> queue_t;

	queue_t q;
	bool is_completed;

	mutable mutex mtx;								// The mutex to synchronise on

public:
	CInputFilesQueue(const vector<string> &file_names) {
		unique_lock<mutex> lck(mtx);

		for(vector<string>::const_iterator p = file_names.begin(); p != file_names.end(); ++p)
			q.push(*p);

		is_completed = false;
	};
	~CInputFilesQueue() {};

	bool empty() {
		lock_guard<mutex> lck(mtx);
		return q.empty();
	}
	bool completed() {
		lock_guard<mutex> lck(mtx);
		return q.empty() && is_completed;
	}
	void mark_completed() {
		lock_guard<mutex> lck(mtx);
		is_completed = true;
	}
	bool pop(string &file_name) {
		lock_guard<mutex> lck(mtx);

		if(q.empty())
			return false;

		file_name = q.front();
		q.pop();

		return true;
	}
};

//************************************************************************************************************
class CPartQueue {
	typedef pair<uchar *, uint64> elem_t;
	typedef queue<elem_t, list<elem_t>> queue_t;

	queue_t q;
	bool is_completed;
	int n_readers;

	mutable mutex mtx;								// The mutex to synchronise on
	condition_variable cv_queue_empty;

public:
	CPartQueue(int _n_readers) {
		unique_lock<mutex> lck(mtx);
		is_completed    = false;
		n_readers       = _n_readers;
	};
	~CPartQueue() {};

	bool empty() {
		lock_guard<mutex> lck(mtx);
		return q.empty();
	}
	bool completed() {
		lock_guard<mutex> lck(mtx);
		return q.empty() && !n_readers;
	}
	void mark_completed() {
		lock_guard<mutex> lck(mtx);
		n_readers--;
		if(!n_readers)
			cv_queue_empty.notify_all();
	}
	void push(uchar *part, uint64 size) {
		unique_lock<mutex> lck(mtx);
		
		bool was_empty = q.empty();
		q.push(make_pair(part, size));

		if(was_empty)
			cv_queue_empty.notify_all();
	}
	bool pop(uchar *&part, uint64 &size) {
		unique_lock<mutex> lck(mtx);
		cv_queue_empty.wait(lck, [this]{return !this->q.empty() || !this->n_readers;}); 

		if(q.empty())
			return false;

		part = q.front().first;
		size = q.front().second;
		q.pop();

		return true;
	}
};

//************************************************************************************************************
class CStatsPartQueue
{
	typedef pair<uchar *, uint64> elem_t;
	typedef queue<elem_t, list<elem_t>> queue_t;

	queue_t q;

	mutable mutex mtx;
	condition_variable cv_queue_empty;
	int n_readers;
	int64 bytes_to_read;
public:
	CStatsPartQueue(int _n_readers, int64 _bytes_to_read)
	{
		unique_lock<mutex> lck(mtx);
		n_readers = _n_readers;
		bytes_to_read = _bytes_to_read;
	}

	~CStatsPartQueue() {};

	void mark_completed() {
		lock_guard<mutex> lck(mtx);
		n_readers--;
		if (!n_readers)
			cv_queue_empty.notify_all();
	}

	bool completed() {
		lock_guard<mutex> lck(mtx);
		return q.empty() && !n_readers;
	}

	bool push(uchar *part, uint64 size) {
		unique_lock<mutex> lck(mtx);

		if (bytes_to_read <= 0)
			return false;

		bool was_empty = q.empty();
		q.push(make_pair(part, size));
		bytes_to_read -= size;
		if (was_empty)
			cv_queue_empty.notify_one();

		return true;
	}

	bool pop(uchar *&part, uint64 &size) {
		unique_lock<mutex> lck(mtx);
		cv_queue_empty.wait(lck, [this]{return !this->q.empty() || !this->n_readers; });

		if (q.empty())
			return false;

		part = q.front().first;
		size = q.front().second;
		q.pop();

		return true;
	}


};

//************************************************************************************************************
class CBinPartQueue {
	typedef tuple<int32, uchar *, uint32, uint32> elem_t;
	typedef queue<elem_t, list<elem_t>> queue_t;
	queue_t q;

	int n_writers;
	bool is_completed;

	mutable mutex mtx;						// The mutex to synchronise on
	condition_variable cv_queue_empty;

public:
	CBinPartQueue(int _n_writers) {
		lock_guard<mutex> lck(mtx);

		n_writers       = _n_writers;
		is_completed    = false;
	}
	~CBinPartQueue() {}

	bool empty() {
		lock_guard<mutex> lck(mtx);
		return q.empty();
	}
	bool completed() {
		lock_guard<mutex> lck(mtx);
		return q.empty() && !n_writers;
	}
	void mark_completed() {
		lock_guard<mutex> lck(mtx);
		n_writers--;
		if(!n_writers)
			cv_queue_empty.notify_all();
	}
	void push(int32 bin_id, uchar *part, uint32 true_size, uint32 alloc_size) {
		unique_lock<mutex> lck(mtx);

		bool was_empty = q.empty();
		q.push(std::make_tuple(bin_id, part, true_size, alloc_size));
		if(was_empty)
			cv_queue_empty.notify_all();
	}
	bool pop(int32 &bin_id, uchar *&part, uint32 &true_size, uint32 &alloc_size) {
		unique_lock<mutex> lck(mtx);
		cv_queue_empty.wait(lck, [this]{return !q.empty() || !n_writers;}); 

		if(q.empty())
			return false;

		bin_id     = get<0>(q.front());
		part       = get<1>(q.front());
		true_size  = get<2>(q.front());
		alloc_size = get<3>(q.front());
		q.pop();

		return true;
	}
};

//************************************************************************************************************
class CBinDesc {
	typedef tuple<string, int64, uint64, uint32, uint32, CMemDiskFile*, uint64, uint64> desc_t;
	typedef map<int32, desc_t> map_t;

	map_t m;
	int32 bin_id;

	vector<int32> random_bins;

	mutable mutex mtx;

public:
	CBinDesc() {
		lock_guard<mutex> lck(mtx);
		bin_id = -1;
	}
	~CBinDesc() {}

	void reset_reading() {
		lock_guard<mutex> lck(mtx);
		bin_id = -1;
	}

	bool empty() {
		lock_guard<mutex> lck(mtx);
		return m.empty();
	}

	void init_random()
	{
		lock_guard<mutex> lck(mtx);
		vector<pair<int32, int64>> bin_sizes;

		for (auto& p : m)
			bin_sizes.push_back(make_pair(p.first, get<2>(p.second)));

		sort(bin_sizes.begin(), bin_sizes.end(), [](const pair<int32, int64>& l, const pair<int32, int64>& r){
			return l.second > r.second;
		});

		uint32 no_sort_start = uint32(0.6 * bin_sizes.size());
		uint32 no_sort_end = uint32(0.8 * bin_sizes.size());

		for (uint32 i = 0; i < no_sort_start; ++i)
			random_bins.push_back(bin_sizes[i].first);

		for (uint32 i = no_sort_end; i < bin_sizes.size(); ++i)
			random_bins.push_back(bin_sizes[i].first);

		random_shuffle(random_bins.begin(), random_bins.end());

		for (uint32 i = no_sort_start; i < no_sort_end; ++i)
			random_bins.push_back(bin_sizes[i].first);
	}

	int32 get_next_random_bin()
	{
		lock_guard<mutex> lck(mtx);
		if (bin_id == -1)
			bin_id = 0;
		else
			++bin_id;

		if (bin_id >= (int32)m.size())
			return -1000;
		return random_bins[bin_id];
	}

	int32 get_next_bin()
	{
		lock_guard<mutex> lck(mtx);
		map_t::iterator p;
		if(bin_id == -1)
			p = m.begin();
		else
		{
			p = m.find(bin_id);
			if(p != m.end())
				++p;
		}

		if(p == m.end())
			bin_id = -1000;
		else
			bin_id = p->first;		

		return bin_id;
	}
	void insert(int32 bin_id, CMemDiskFile *file, string desc, int64 size, uint64 n_rec, uint64 n_plus_x_recs, uint64 n_super_kmers, uint32 buffer_size = 0, uint32 kmer_len = 0) {
		lock_guard<mutex> lck(mtx);

		map_t::iterator p = m.find(bin_id);
		if(p != m.end())
		{
			if(desc != "")
			{
				get<0>(m[bin_id]) = desc;
				get<5>(m[bin_id]) = file;
			}
			get<1>(m[bin_id]) += size;
			get<2>(m[bin_id]) += n_rec;
			get<6>(m[bin_id]) += n_plus_x_recs;
			get<7>(m[bin_id]) += n_super_kmers;
			if(buffer_size)
			{
				get<3>(m[bin_id]) = buffer_size;
				get<4>(m[bin_id]) = kmer_len;
			}
		}
		else
			m[bin_id] = std::make_tuple(desc, size, n_rec, buffer_size, kmer_len, file, n_plus_x_recs, n_super_kmers);
	}
	void read(int32 bin_id, CMemDiskFile *&file, string &desc, uint64 &size, uint64 &n_rec, uint64 &n_plus_x_recs, uint32 &buffer_size, uint32 &kmer_len) {
		lock_guard<mutex> lck(mtx);

		desc			= get<0>(m[bin_id]);
		file			= get<5>(m[bin_id]);
		size			= (uint64) get<1>(m[bin_id]);
		n_rec			= get<2>(m[bin_id]);
		buffer_size		= get<3>(m[bin_id]);
		kmer_len		= get<4>(m[bin_id]);
		n_plus_x_recs	= get<6>(m[bin_id]);
	}
	void read(int32 bin_id, CMemDiskFile *&file, string &desc, uint64 &size, uint64 &n_rec, uint64 &n_plus_x_recs, uint64 &n_super_kmers) {
		lock_guard<mutex> lck(mtx);

		desc			= get<0>(m[bin_id]);
		file			= get<5>(m[bin_id]);
		size			= (uint64) get<1>(m[bin_id]);
		n_rec			= get<2>(m[bin_id]);
		n_plus_x_recs	= get<6>(m[bin_id]);
		n_super_kmers		= get<7>(m[bin_id]);
	}
};

//************************************************************************************************************
class CBinQueue {
	typedef tuple<int32, uchar *, uint64, uint64> elem_t;
	typedef queue<elem_t, list<elem_t>> queue_t;
	queue_t q;

	int n_writers;

	mutable mutex mtx;								// The mutex to synchronise on
	condition_variable cv_queue_empty;

public:
	CBinQueue(int _n_writers) {
		lock_guard<mutex> lck(mtx);
		n_writers = _n_writers;
	}
	~CBinQueue() {}

	bool empty() {
		lock_guard<mutex> lck(mtx);
		return q.empty();
	}
	bool completed() {
		lock_guard<mutex> lck(mtx);
		return q.empty() && !n_writers;
	}
	void mark_completed() {
		lock_guard<mutex> lck(mtx);
		n_writers--;
		if(n_writers == 0)
			cv_queue_empty.notify_all();
	}
	void push(int32 bin_id, uchar *part, uint64 size, uint64 n_rec) {
		lock_guard<mutex> lck(mtx);
		bool was_empty = q.empty();
		q.push(std::make_tuple(bin_id, part, size, n_rec));
		if(was_empty)
			cv_queue_empty.notify_all();
	}
	bool pop(int32 &bin_id, uchar *&part, uint64 &size, uint64 &n_rec) {
		unique_lock<mutex> lck(mtx);

		cv_queue_empty.wait(lck, [this]{return !q.empty() || !n_writers;}); 

		if(q.empty())
			return false;

		bin_id = get<0>(q.front());
		part   = get<1>(q.front());
		size   = get<2>(q.front());
		n_rec  = get<3>(q.front());
		q.pop();

		return true;
	}
};

//************************************************************************************************************
class CKmerQueue {
	typedef tuple<int32, uchar*, uint64, uchar*, uint64, uint64, uint64, uint64, uint64> data_t;
	typedef list<data_t> list_t;

	int n_writers;
	mutable mutex mtx;								// The mutex to synchronise on
	condition_variable cv_queue_empty;

	list_t l;
	int32 n_bins;
public:
	CKmerQueue(int32 _n_bins, int _n_writers) {
		lock_guard<mutex> lck(mtx);
		n_bins = _n_bins;
		n_writers = _n_writers;
	}
	~CKmerQueue() {
	}

	bool empty() {
		lock_guard<mutex> lck(mtx);
		return l.empty() && !n_writers;
	}
	void mark_completed() {
		lock_guard<mutex> lck(mtx);
		n_writers--;
		if (!n_writers)
			cv_queue_empty.notify_all();
	}
	void push(int32 bin_id, uchar *data, uint64 data_size, uchar *lut, uint64 lut_size, uint64 n_unique, uint64 n_cutoff_min, uint64 n_cutoff_max, uint64 n_total) {
		lock_guard<mutex> lck(mtx);
		l.push_back(std::make_tuple(bin_id, data, data_size, lut, lut_size, n_unique, n_cutoff_min, n_cutoff_max, n_total));
		cv_queue_empty.notify_all();
	}
	bool pop(int32 &bin_id, uchar *&data, uint64 &data_size, uchar *&lut, uint64 &lut_size, uint64 &n_unique, uint64 &n_cutoff_min, uint64 &n_cutoff_max, uint64 &n_total) {
		unique_lock<mutex> lck(mtx);
		cv_queue_empty.wait(lck, [this]{return !l.empty() || !n_writers; });
		if (l.empty())
			return false;

		bin_id = get<0>(l.front());
		data = get<1>(l.front());
		data_size = get<2>(l.front());
		lut = get<3>(l.front());
		lut_size = get<4>(l.front());
		n_unique = get<5>(l.front());
		n_cutoff_min = get<6>(l.front());
		n_cutoff_max = get<7>(l.front());
		n_total = get<8>(l.front());

		l.pop_front();

		if (l.empty())
			cv_queue_empty.notify_all();

		return true;
	}
};




//************************************************************************************************************
class CMemoryMonitor {
	uint64 max_memory;
	uint64 memory_in_use;

	mutable mutex mtx;								// The mutex to synchronise on
	condition_variable cv_memory_full;				// The condition to wait for

public:
	CMemoryMonitor(uint64 _max_memory) {
		lock_guard<mutex> lck(mtx);
		max_memory    = _max_memory;
		memory_in_use = 0;
	}
	~CMemoryMonitor() {
	}

	void increase(uint64 n) {
		unique_lock<mutex> lck(mtx);
		cv_memory_full.wait(lck, [this, n]{return memory_in_use + n <= max_memory;});
		memory_in_use += n;
	}
	void force_increase(uint64 n) {
		unique_lock<mutex> lck(mtx);
		cv_memory_full.wait(lck, [this, n]{return memory_in_use + n <= max_memory || memory_in_use == 0;});
		memory_in_use += n;
	}
	void decrease(uint64 n) {
		lock_guard<mutex> lck(mtx);
		memory_in_use -= n;
		cv_memory_full.notify_all();
	}
	void info(uint64 &_max_memory, uint64 &_memory_in_use)
	{
		lock_guard<mutex> lck(mtx);
		_max_memory    = max_memory;
		_memory_in_use = memory_in_use;
	}
};

//************************************************************************************************************
class CMemoryPool {
	int64 total_size;
	int64 part_size;
	int64 n_parts_total;
	int64 n_parts_free;

	uchar *buffer, *raw_buffer;
	uint32 *stack;

	mutable mutex mtx;							// The mutex to synchronise on
	condition_variable cv;						// The condition to wait for

public:
	CMemoryPool(int64 _total_size, int64 _part_size) {
		raw_buffer = NULL;
		buffer = NULL;
		stack  = NULL;
		prepare(_total_size, _part_size);
	}
	~CMemoryPool() {
		release();
	}

	void prepare(int64 _total_size, int64 _part_size) {
		release();

		n_parts_total = _total_size / _part_size;
		part_size     = (_part_size + 15) / 16 * 16;			// to allow mapping pointer to int*
		n_parts_free  = n_parts_total;

		total_size = n_parts_total * part_size;

		raw_buffer = new uchar[total_size+64];
		buffer     = raw_buffer;
		while(((uint64) buffer) % 64)
			buffer++;

		stack = new uint32[n_parts_total];
		for(uint32 i = 0; i < n_parts_total; ++i)
			stack[i] = i;
	}

	void release(void) {
		if(raw_buffer)
			delete[] raw_buffer;
		raw_buffer = NULL;
		buffer     = NULL;

		if(stack)
			delete[] stack;
		stack = NULL;
	}

	// Allocate memory buffer - uchar*
	void reserve(uchar* &part)
	{
		unique_lock<mutex> lck(mtx);
		cv.wait(lck, [this]{return n_parts_free > 0;});

		part = buffer + stack[--n_parts_free]*part_size;
	}
	// Allocate memory buffer - char*
	void reserve(char* &part)
	{
		unique_lock<mutex> lck(mtx);
		cv.wait(lck, [this]{return n_parts_free > 0;});

		part = (char*) (buffer + stack[--n_parts_free]*part_size);
	}
	// Allocate memory buffer - uint32*
	void reserve(uint32* &part)
	{
		unique_lock<mutex> lck(mtx);
		cv.wait(lck, [this]{return n_parts_free > 0;});

		part = (uint32*) (buffer + stack[--n_parts_free]*part_size);
	}
	// Allocate memory buffer - uint64*
	void reserve(uint64* &part)
	{
		unique_lock<mutex> lck(mtx);
		cv.wait(lck, [this]{return n_parts_free > 0;});

		part = (uint64*) (buffer + stack[--n_parts_free]*part_size);
	}
	// Allocate memory buffer - double*
	void reserve(double* &part)
	{
		unique_lock<mutex> lck(mtx);
		cv.wait(lck, [this]{return n_parts_free > 0;});

		part = (double*) (buffer + stack[--n_parts_free]*part_size);
	}

	// Deallocate memory buffer - uchar*
	void free(uchar* part)
	{
		lock_guard<mutex> lck(mtx);

		stack[n_parts_free++] = (uint32) ((part - buffer) / part_size);
		
		cv.notify_all();
	}
	// Deallocate memory buffer - char*
	void free(char* part)
	{
		lock_guard<mutex> lck(mtx);

		stack[n_parts_free++] = (uint32) (((uchar*) part - buffer) / part_size);
		cv.notify_all();
	}
	// Deallocate memory buffer - uint32*
	void free(uint32* part)
	{
		lock_guard<mutex> lck(mtx);

		stack[n_parts_free++] = (uint32) ((((uchar *) part) - buffer) / part_size);
		cv.notify_all();
	}
	// Deallocate memory buffer - uint64*
	void free(uint64* part)
	{
		lock_guard<mutex> lck(mtx);

		stack[n_parts_free++] = (uint32) ((((uchar *) part) - buffer) / part_size);
		cv.notify_all();
	}
	// Deallocate memory buffer - double*
	void free(double* part)
	{
		lock_guard<mutex> lck(mtx);

		stack[n_parts_free++] = (uint32) ((((uchar *) part) - buffer) / part_size);
		cv.notify_all();
	}
};


class CMemoryBins {
	int64 total_size;
	int64 free_size;

	uint32 n_bins;

	typedef tuple<uchar*, uchar*, uchar*, uchar*, uchar*, uchar*, uchar*, int64> bin_ptrs_t;

public:
	typedef enum{ mba_input_file, mba_input_array, mba_tmp_array, mba_suffix, mba_kxmer_counters, mba_lut } mba_t;

private:
	uchar *buffer, *raw_buffer;
	bin_ptrs_t *bin_ptrs;

	list<pair<uint64, uint64>> list_reserved;
	list<pair<uint32, uint64>> list_insert_order;

	mutable mutex mtx;							// The mutex to synchronise on
	condition_variable cv;						// The condition to wait for

public:
	CMemoryBins(int64 _total_size, uint32 _n_bins) {
		raw_buffer = NULL;
		buffer = NULL;
		bin_ptrs = NULL;
		prepare(_total_size, _n_bins);
	}
	~CMemoryBins() {
		release();
	}

	int64 round_up_to_alignment(int64 x)
	{
		return (x + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;
	}

	void prepare(int64 _total_size, uint32 _n_bins) {
		release();

		n_bins = _n_bins;
		bin_ptrs = new bin_ptrs_t[n_bins];

		total_size = round_up_to_alignment(_total_size - n_bins * sizeof(bin_ptrs_t));
		free_size = total_size;

		raw_buffer = (uchar*)malloc(total_size + ALIGNMENT);
		buffer = raw_buffer;
		while (((uint64)buffer) % ALIGNMENT)
			buffer++;

		list_reserved.clear();
		list_insert_order.clear();
		list_reserved.push_back(make_pair(total_size, 0));		// guard
	}

	void release(void) {
		if (raw_buffer)
			::free(raw_buffer);
		raw_buffer = NULL;
		buffer = NULL;

		if (bin_ptrs)
			delete[] bin_ptrs;
		bin_ptrs = NULL;
	}

	// Prepare memory buffer for bin of given id
	void init(uint32 bin_id, uint32 sorting_phases, int64 file_size, int64 kxmers_size, int64 out_buffer_size, int64 kxmer_counter_size, int64 lut_size)
	{
		unique_lock<mutex> lck(mtx);
		int64 part1_size;
		int64 part2_size;

		if (sorting_phases % 2 == 0)
		{
			part1_size = kxmers_size + kxmer_counter_size;
			part2_size = max(max(file_size, kxmers_size), out_buffer_size + lut_size);
		}
		else
		{
			part1_size = max(kxmers_size + kxmer_counter_size, file_size);
			part2_size = max(kxmers_size, out_buffer_size + lut_size);
		}
		int64 req_size = part1_size + part2_size;
		uint64 found_pos;
		uint64 last_found_pos;

		// Look for space to insert
		cv.wait(lck, [&]() -> bool{
			found_pos = total_size;
			if (!list_insert_order.empty())
			{
				last_found_pos = list_insert_order.back().second;
				for (auto p = list_reserved.begin(); p != list_reserved.end(); ++p)
					if (p->first == last_found_pos)
					{
						uint64 last_end_pos = p->first + p->second;
						++p;
						if (last_end_pos + req_size <= p->first)
						{
							found_pos = last_end_pos;
							return true;
						}
						else
							break;
					}
			}

			uint64 prev_end_pos = 0;

			for (auto p = list_reserved.begin(); p != list_reserved.end(); ++p)
			{
				if (prev_end_pos + req_size <= p->first)
				{
					found_pos = prev_end_pos;
					return true;
				}
				prev_end_pos = p->first + p->second;
			}

			// Reallocate memory for buffer if necessary
			if (list_insert_order.empty() && req_size > (int64)list_reserved.back().first)
			{
				::free(raw_buffer);
				total_size = round_up_to_alignment(req_size);
				free_size = total_size;

				raw_buffer = (uchar*)malloc(total_size + ALIGNMENT);
				buffer = raw_buffer;
				while (((uint64)buffer) % ALIGNMENT)
					buffer++;

				list_reserved.back().first = total_size;
				found_pos = 0;
				return true;
			}

			return false;
		});

		// Reserve found free space
		list_insert_order.push_back(make_pair(bin_id, found_pos));
		for (auto p = list_reserved.begin(); p != list_reserved.end(); ++p)
			if (found_pos < p->first)
			{
				list_reserved.insert(p, make_pair(found_pos, req_size));
				break;
			}

		uchar *base_ptr = get<0>(bin_ptrs[bin_id]) = buffer + found_pos;

		if (sorting_phases % 2 == 0)				// the result of sorting is in the same place as input
		{
			get<1>(bin_ptrs[bin_id]) = base_ptr + part1_size;
			get<2>(bin_ptrs[bin_id]) = base_ptr;
			get<3>(bin_ptrs[bin_id]) = base_ptr + part1_size;
		}
		else
		{
			get<1>(bin_ptrs[bin_id]) = base_ptr;
			get<2>(bin_ptrs[bin_id]) = base_ptr + part1_size;
			get<3>(bin_ptrs[bin_id]) = base_ptr;
		}
		get<4>(bin_ptrs[bin_id]) = base_ptr + part1_size;									// data
		get<5>(bin_ptrs[bin_id]) = get<4>(bin_ptrs[bin_id]) + out_buffer_size;
		if (kxmer_counter_size)
			get<6>(bin_ptrs[bin_id]) = base_ptr + kxmers_size;								//kxmers counter
		else
			get<6>(bin_ptrs[bin_id]) = NULL;
		free_size -= req_size;
		get<7>(bin_ptrs[bin_id]) = req_size;
	}

	void reserve(uint32 bin_id, uchar* &part, mba_t t)
	{
		unique_lock<mutex> lck(mtx);
		if (t == mba_input_file)
			part = get<1>(bin_ptrs[bin_id]);
		else if (t == mba_input_array)
			part = get<2>(bin_ptrs[bin_id]);
		else if (t == mba_tmp_array)
			part = get<3>(bin_ptrs[bin_id]);
		else if (t == mba_suffix)
			part = get<4>(bin_ptrs[bin_id]);
		else if (t == mba_lut)
			part = get<5>(bin_ptrs[bin_id]);
		else if (t == mba_kxmer_counters)
			part = get<6>(bin_ptrs[bin_id]);
	}

	// Deallocate memory buffer - uchar*
	void free(uint32 bin_id, mba_t t)
	{
		unique_lock<mutex> lck(mtx);
		if (t == mba_input_file)
			get<1>(bin_ptrs[bin_id]) = NULL;
		else if (t == mba_input_array)
			get<2>(bin_ptrs[bin_id]) = NULL;
		else if (t == mba_tmp_array)
			get<3>(bin_ptrs[bin_id]) = NULL;
		else if (t == mba_suffix)
			get<4>(bin_ptrs[bin_id]) = NULL;
		else if (t == mba_lut)
			get<5>(bin_ptrs[bin_id]) = NULL;
		else if (t == mba_kxmer_counters)
			get<6>(bin_ptrs[bin_id]) = NULL;

		if (!get<1>(bin_ptrs[bin_id]) && !get<2>(bin_ptrs[bin_id]) && !get<3>(bin_ptrs[bin_id]) && !get<4>(bin_ptrs[bin_id]) && !get<5>(bin_ptrs[bin_id]) && !get<6>(bin_ptrs[bin_id]))
		{
			for (auto p = list_reserved.begin(); p != list_reserved.end() && p->second != 0; ++p)
			{
				if ((int64)p->first == get<0>(bin_ptrs[bin_id]) - buffer)
				{
					list_reserved.erase(p);
					break;
				}
			}
			for (auto p = list_insert_order.begin(); p != list_insert_order.end(); ++p)
			if (p->first == bin_id)
			{
				list_insert_order.erase(p);
				break;
			}

			get<0>(bin_ptrs[bin_id]) = NULL;
			free_size += get<7>(bin_ptrs[bin_id]);
			cv.notify_all();
		}
	}
};


#endif

// ***** EOF
