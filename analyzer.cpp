#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <ext/hash_map>
//#include <ext/hash_set>
#include "instruction.h"
#include "atomicsection.h"
#include "meta_data.h"
#include <map>

//#define MULTI_CACHE
#ifdef MULTI_CAHCHE
#define OFFSET 4 // for 16B cache line
#endif
using namespace std;
//using __gnu_cxx::hash_set;
using __gnu_cxx::hash_map;
using __gnu_cxx::hash;

/* =================================================================== */
// Constants
/* =================================================================== */

const int BUF_SIZE = 512;
const size_t BATCH_SIZE = 50000;

string LOCK("nova_lock");
string UNLOCK("nova_unlock");


/* =================================================================== */
// Global Variables 
/* =================================================================== */

fstream conf_file, **in_file, report_file;

int total_threads;
/*
 * table for store and access  meta_data
 * mapped with mem address
 */
map<addr_type, MetaData> info_table; /* added by mejbah :: key = read ins addr, value = Meta Data*/
/*
 * Matrix No. of Thread x No. of Thread
 * Each [i][j] contains counter for Ri <-- Wj, where i, j are thread no for read and write respectively
 */
unsigned int **pair_counter;


/* =================================================================== */

void ParseAtomicSection(string* str, AtomicSection* atomic_section) {
  // Format:
  // c # pc # [#, name] (r|w) # #
  // c # pc # [#, name] r # # r # # w # #
  size_t c_start = str->find("c");
  assert(c_start != string::npos);
  size_t pc_start = str->find("pc");
  assert(pc_start != string::npos);
  stringstream c_ss(str->substr(c_start + 2, pc_start - c_start - 3));
  access_count_type access_count;
  c_ss >> access_count;
  //cout << access_count;
  
  size_t left_bracket = str->find("[");
  assert( left_bracket != string::npos);
  stringstream pc_ss(str->substr(pc_start + 3, left_bracket - pc_start - 4));
  pc_type pc;
  pc_ss >> hex >> pc;
  //cout << " " << hex << pc;
  
  size_t comma_pos = str->find(",");
  assert( comma_pos != string::npos);
  stringstream line_no_ss(str->substr(left_bracket + 1, 
                                      comma_pos - left_bracket -1));
  line_no_type line_no;
  line_no_ss >> line_no;
  //cout << " " <<dec << line_no;
  
  size_t right_bracket = str->find("]");
  assert( right_bracket != string::npos);
  string file_name = str->substr(comma_pos + 2, right_bracket - comma_pos -2);
  //cout << " " << file_name;
  
  addr_type read_addr, read_addr2, write_addr;
  read_addr = read_addr2 = write_addr = INVALID_ADDR;
  size_t read_len, write_len;
  read_len = write_len = 0;
  size_t access_pos;
  if ((access_pos = str->find("r", right_bracket)) != string::npos) {
    size_t end = str->find(" ", access_pos + 2);
    stringstream read_addr_ss(str->substr(access_pos + 2, 
                                          end - access_pos - 2));
    read_addr_ss >> read_addr;        
    stringstream read_len_ss(str->substr(end + 1));
    read_len_ss >> read_len;
    size_t access_pos2 = string::npos;
    if ((access_pos2 = str->find("r", access_pos + 2)) != string::npos) {
      size_t end2 = str->find(" ", access_pos2 + 2);
      stringstream read_addr2_ss(str->substr(access_pos2 + 2, 
                                          end2 - access_pos2 - 2));
      read_addr2_ss >> read_addr2;        
    }
  }
  if ((access_pos = str->find("w", right_bracket)) != string::npos) {
    size_t end = str->find(" ", access_pos + 2);
    stringstream write_addr_ss(str->substr(access_pos + 2, 
                                          end - access_pos - 2));
    write_addr_ss >> write_addr;        
    stringstream write_len_ss(str->substr(end + 1));
    write_len_ss >> write_len;
  }
  atomic_section->Set(pc, write_addr, write_len, read_addr,
                           read_addr2, read_len, access_count);
}

/* =================================================================== */

// Consider this relationship sync < positive num < EOF
// Needs to call GetNextLine(NULL, eof_reached, NULL, INIT) before first
// use.
// After the last use, call GetNextLine(NULL, NULL, NULL, END).
LINE_TYPE GetNextLine(AtomicSection** atomic_section, int* eof_reached,
                      int* thread_id, ITER_TYPE iter_type) {
  static AtomicSection** atomic_sections = NULL;
  static int lowest_i = -1, second_lowest_i = -1;
  static LINE_TYPE* line_type = NULL;
  if (iter_type == INIT) {
    atomic_sections = new AtomicSection*[total_threads];
    line_type = new LINE_TYPE[total_threads];
    string str;
    for (int i = 0; i < total_threads; i++) {
      atomic_sections[i] = new AtomicSection();
      if (getline(*(in_file[i]), str)) {
        //if (str.find("unlock") != string::npos) {
        if (str.find(UNLOCK) != string::npos) {
          atomic_sections[i]->Clear();
          line_type[i] = UNLOCK_TYPE;
        }
        //else if(str.find("lock") != string::npos) {
        else if(str.find(LOCK) != string::npos) {
          atomic_sections[i]->Clear();
          line_type[i] = LOCK_TYPE;
        }
        else {
          ParseAtomicSection(&str, atomic_sections[i]);
          line_type[i] = NORMAL_TYPE; 
        }
      } else {
        atomic_sections[i]->Clear();
        line_type[i] = EOF_TYPE;
        eof_reached[i] = 1;
      }
    }

    lowest_i = 0;
    for (int i = 1; i < total_threads; i++) {
      if (line_type[i] == EOF_TYPE) continue;
      // lowest - e, s, n
      // i - s, n
      if (line_type[lowest_i] == LOCK_TYPE || 
          line_type[lowest_i] == UNLOCK_TYPE) 
        break;
      // lowest - e, n
      // i - s, n
      if (line_type[i] == LOCK_TYPE || line_type[i] == UNLOCK_TYPE)
        lowest_i = i;
      else {
        // lowest - e, n
        // i - n
        if (line_type[lowest_i] == EOF_TYPE)
          lowest_i = i;
        else if (atomic_sections[lowest_i]->_access_counter >
                 atomic_sections[i]->_access_counter)
          lowest_i = i;
      }
    }

    if (lowest_i == 0 && total_threads > 1) second_lowest_i = 1;
    else second_lowest_i = 0;
    for (int i = second_lowest_i + 1; i < total_threads; i++) {
      if (i == lowest_i) continue;
      if (line_type[i] == EOF_TYPE) continue;
      // second_lowest - e, s, n
      // i - s, n
      if (line_type[second_lowest_i] == LOCK_TYPE || 
          line_type[second_lowest_i] == UNLOCK_TYPE) 
        break;
      // second_lowest - e, n
      // i - s, n
      if (line_type[i] == LOCK_TYPE || line_type[i] == UNLOCK_TYPE)
        second_lowest_i = i;
      else {
        // second_lowest - e, n
        // i - n
        if (line_type[second_lowest_i] == EOF_TYPE)
          second_lowest_i = i;
        else if (atomic_sections[second_lowest_i]->_access_counter >
                 atomic_sections[i]->_access_counter)
          second_lowest_i = i;
      }
    }
    return EOF_TYPE;
  }
  else if (iter_type == END) {
    for (int i = 0; i < total_threads; i++)
      delete atomic_sections[i];
    delete[] atomic_sections;
    delete[] line_type;
    atomic_sections = NULL;
    lowest_i = second_lowest_i = -1;
    LINE_TYPE* line_type = NULL;
    return EOF_TYPE;
  }
  if (line_type[lowest_i] != EOF_TYPE) {
    // return lowest_i entry
    **atomic_section = *atomic_sections[lowest_i];
    *thread_id = lowest_i;
    LINE_TYPE return_type = line_type[lowest_i];
   
    // read next entry
    string str;
    if (getline(*(in_file[lowest_i]), str)) {
      //if (str.find("unlock") != string::npos) {
      if (str.find(UNLOCK) != string::npos) {
        atomic_sections[lowest_i]->Clear();
        line_type[lowest_i] = UNLOCK_TYPE;
      }
      //else if(str.find("lock") != string::npos) {
      else if(str.find(LOCK) != string::npos) {
        atomic_sections[lowest_i]->Clear();
        line_type[lowest_i] = LOCK_TYPE;
      }
      else {
        ParseAtomicSection(&str, atomic_sections[lowest_i]);
        line_type[lowest_i] = NORMAL_TYPE; 
      }
    } else {
      atomic_sections[lowest_i]->Clear();
      line_type[lowest_i] = EOF_TYPE;
      eof_reached[lowest_i] = 1;
    } 

    if (((line_type[lowest_i] == LOCK_TYPE || 
         line_type[lowest_i] == UNLOCK_TYPE) || (line_type[lowest_i] ==
         NORMAL_TYPE && line_type[second_lowest_i] == EOF_TYPE) || 
         (line_type[lowest_i] == NORMAL_TYPE && line_type[second_lowest_i] ==
         NORMAL_TYPE && atomic_sections[lowest_i]->_access_counter < 
         atomic_sections[second_lowest_i]->_access_counter)) == false) {
      lowest_i = second_lowest_i;
      if (lowest_i == 0 && total_threads > 1) second_lowest_i = 1;
      else second_lowest_i = 0;
      for (int i = second_lowest_i + 1; i < total_threads; i++) {
        if (i == lowest_i) continue;
        if (line_type[i] == EOF_TYPE) continue;
        // second_lowest - e, s, n
        // i - s, n
        if (line_type[second_lowest_i] == LOCK_TYPE || 
            line_type[second_lowest_i] == UNLOCK_TYPE) 
          break;
        // second_lowest - e, n
        // i - s, n
        if (line_type[i] == LOCK_TYPE || line_type[i] == UNLOCK_TYPE)
          second_lowest_i = i;
        else {
          // second_lowest - e, n
          // i - n
          if (line_type[second_lowest_i] == EOF_TYPE)
            second_lowest_i = i;
          else if (atomic_sections[second_lowest_i]->_access_counter >
                  atomic_sections[i]->_access_counter)
            second_lowest_i = i;
        }
      }
    }
    return return_type;
  }
  else {
    // All files have reached eof
    *thread_id = lowest_i;
    return EOF_TYPE;
  }
  
}

/* =================================================================== */

bool IsFinished(int* eof_reached) {
  for (int i = 0; i < total_threads; i++)
    if (!eof_reached[i])
      return false;
  return true;
}

/* =================================================================== */

void UpdateCounter( AtomicSection* atomic_section, int thread_id ){
  
  map<addr_type, MetaData>::iterator it;
  MetaData *pt_data; 	/* pointer to meta data object ( second in info_table ) */ 
  MetaData temp;
  pc_type ins_addr =  atomic_section->_ins;
  addr_type write_addr = atomic_section->_write_addr;
  addr_type read_addr = atomic_section->_read_addr1;
  addr_type read_addr2 = atomic_section->_read_addr2;
  access_count_type  access_count = atomic_section->_access_counter;

//  pc_type readPC, writePC, prevPC; 


  if( write_addr == INVALID_ADDR) /* read instruction */
  {
#ifdef MULTI_CAHCHE
    read_addr = (read_addr >>  OFFSET) << OFFSET;
#endif
    it = info_table.find( read_addr );
    if(it != info_table.end())  /*found in the table: previous write occured */
    {
      pt_data = &(it->second);
      assert( access_count > pt_data->access_count ); 
      pair_counter[thread_id][pt_data->last_write_tid]++; 
    }
    if( read_addr2 != INVALID_ADDR )
    {
#ifdef MULTI_CAHCHE
      read_addr2 = (read_addr2 >>  OFFSET) << OFFSET;
#endif
      it = info_table.find( read_addr2 );
      if( it != info_table.end())
      {
        pt_data = &(it->second);
	assert( access_count > pt_data->access_count ); 
        pair_counter[thread_id][pt_data->last_write_tid]++;
       
      }
    }

  }
  else if( read_addr == INVALID_ADDR) /* write instruction */
  {
#ifdef MULTI_CAHCHE
    write_addr = ( write_addr >> OFFSET ) << OFFSET;
#endif
    temp.last_write_addr = ins_addr;
    temp.last_write_tid = thread_id;
    temp.access_count =  access_count;
    info_table.insert ( pair<addr_type,MetaData>(write_addr,temp) );
  }
}

void Report(){
  for( int i=0; i<total_threads; i++ ){
    for( int j=0; j<total_threads; j++) {
      report_file << pair_counter[i][j] << endl;
    }
  }
}

int main(int argc, char* args[]) {

  if (argc != 5) {
    cerr << "USAGE: analyzer profile.conf profile.out.compact output_file_with_path total_no_of_execution" <<endl;
    return -1;
  }

  conf_file.open(args[1], fstream::in);
  string str;
  getline(conf_file, str);
  stringstream ss(str);
  ss >> total_threads;
  cout << "total threads " << total_threads << endl;
  
  int exec_no;
  string no_argv(args[4]);
  stringstream ss1(no_argv);
  ss1 >> exec_no;
  
  char *in_file_name = new char[BUF_SIZE];
  char *out_file_name = new char[BUF_SIZE];
  in_file = new fstream*[total_threads];

  pair_counter = new unsigned int*[total_threads];
  sprintf(out_file_name, "%s/counter.report.%d", args[3], exec_no); // file in the APP directory for logging
  report_file.open(out_file_name, fstream::out);

  for (int i = 0; i < total_threads; i++) {
    sprintf(in_file_name, "%s.%d", args[2], i);
    in_file[i] = new fstream(in_file_name, fstream::in);   
    if(!in_file[i]->is_open()) {
      cout << in_file_name << " not found." << endl;
      return 1;
    }    
    pair_counter[i] = new unsigned int[total_threads];
    memset( pair_counter[i], 0, sizeof(pair_counter[i]));
  }
  delete[] in_file_name;
  delete[] out_file_name;

  int* eof_reached = new int[total_threads];
  AtomicSection* atomic_section_test = new AtomicSection();
  int thread_id_test;
  LINE_TYPE return_type;

  info_table.clear();

  GetNextLine(&atomic_section_test, eof_reached, &thread_id_test, INIT);
  while ((return_type = 
          GetNextLine(&atomic_section_test, eof_reached, 
          &thread_id_test, MIDDLE)) 
          != EOF_TYPE) 
  {
    if( return_type == NORMAL_TYPE ) 
    {
      UpdateCounter( atomic_section_test, thread_id_test );
    }
  }
  GetNextLine(&atomic_section_test, eof_reached, &thread_id_test, END);

  Report();

  for (int j = 0; j < total_threads; j++) 
  {
    in_file[j]->clear();
    in_file[j]->seekg(0, ios::beg);
    eof_reached[j] = 0;
  }

  delete atomic_section_test;
  delete[] eof_reached;
  
  for (int i = 0; i < total_threads; i++) {
    in_file[i]->close();
    delete in_file[i];
  }
  delete[] in_file;


  conf_file.close();
  report_file.close();
  
  return 0;
}

