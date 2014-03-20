#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <stack>
#include "instruction.h"
#include "addressmap.h"
#include "module_map.h"

using namespace std;

#define FILE_NAME_TO_DEBUG "/sql/log.cc"
#define DEBUG_LINE_NO_START 568
#define DEBUG_LINE_NO_END 761

#define FILE_NAME_TO_DEBUG_1 "/sql/sql_insert.cc"
#define DEBUG_LINE_NO_START_1 890
#define DEBUG_LINE_NO_END_1 930

/* =================================================================== */
// Constants
/* =================================================================== */

const int BUF_SIZE = 512;

/* =================================================================== */
// Global Variables 
/* =================================================================== */

fstream conf_file, **in_file, **out_file, sym_file, malloc_file, free_file,
        old_modules_file, translated_modules_file;
//fstream addr_of_interest_file;
int total_threads;

static string LOCK("nova_lock");
static string UNLOCK("nova_unlock");
static string FUNC_BEGIN("FUNC_BEGIN");
static string FUNC_END("FUNC_END");

AddressMarkerVectorHandler amv_handler;
ModuleTranslator m_translator;

bool dbg_flag = false;

/* =================================================================== */

// Read all the profile file and parse them
void ReadAtomicSections() {
  for (int i = 0; i < total_threads; i++) {
    string str;

   cout<< "Now Processing thread " << i << endl; 
    while (getline(*(in_file[i]), str)) {
      // Format:
      // c # pc # [#, name] (r|w) # #
      // c # pc # [#, name] r # # r # # w # #
#ifdef PRINT_LOCK
      //if (str.find("lock") != string::npos || 
      //    str.find("unlock") != string::npos) {
      if (str.find(LOCK) != string::npos || 
          str.find(UNLOCK) != string::npos ||
          str.find(FUNC_BEGIN) != string::npos ||
          str.find(FUNC_END) != string::npos) {
        //cout << str << " found" << endl;
        (*out_file[i]) << str << endl;
        continue;
      }
#endif
      //cout << str << " thread " << i << endl;
      size_t c_start = str.find("c");
      assert(c_start != string::npos);
//      if(c_start != string::npos)
//         continue;  /// just skip the garbage lines :( mejbah
      size_t pc_start = str.find("pc");
      assert(pc_start != string::npos);
//      if(pc_start != string::npos)
//         continue;  /// just skip the garbage lines :( mejbah

      stringstream c_ss(str.substr(c_start + 2, pc_start - c_start - 3));
      access_count_type access_count;
      c_ss >> access_count;
      //cout << access_count;
      
      size_t left_bracket = str.find("[");
      assert( left_bracket != string::npos);
      stringstream pc_ss(str.substr(pc_start + 3, left_bracket - pc_start - 4));
      pc_type pc;
      pc_ss >> hex >> pc;
      //cout << " " << hex << pc;
      
      size_t comma_pos = str.find(",");
      assert( comma_pos != string::npos);
      stringstream line_no_ss(str.substr(left_bracket + 1, 
                                         comma_pos - left_bracket -1));
      line_no_type line_no;
      line_no_ss >> line_no;
      //cout << " " <<dec << line_no;
      
      size_t right_bracket = str.find("]");
      assert( right_bracket != string::npos);
      string file_name = str.substr(comma_pos + 2, right_bracket - comma_pos -2);
      //cout << " " << file_name;

      string record_without_addr_str = str.substr(0, right_bracket + 1);
      
      addr_type read_addr, read_addr2, write_addr;
      read_addr = read_addr2 = write_addr = INVALID_ADDR;
      size_t read_len, write_len;
      read_len = write_len = 0;
      size_t access_pos;
      if ((access_pos = str.find("r", right_bracket)) != string::npos) {
        size_t end = str.find(" ", access_pos + 2);
        stringstream read_addr_ss(str.substr(access_pos + 2, 
                                             end - access_pos - 2));
        read_addr_ss >> read_addr;        
        stringstream read_len_ss(str.substr(end + 1));
        read_len_ss >> read_len;
        size_t access_pos2 = string::npos;
        if ((access_pos2 = str.find("r", access_pos + 2)) != string::npos) {
          size_t end2 = str.find(" ", access_pos2 + 2);
          stringstream read_addr2_ss(str.substr(access_pos2 + 2, 
                                              end2 - access_pos2 - 2));
          read_addr2_ss >> read_addr2;        
        }
      }
      if ((access_pos = str.find("w", right_bracket)) != string::npos) {
        size_t end = str.find(" ", access_pos + 2);
        stringstream write_addr_ss(str.substr(access_pos + 2, 
                                             end - access_pos - 2));
        write_addr_ss >> write_addr;        
        stringstream write_len_ss(str.substr(end + 1));
        write_len_ss >> write_len;
      }

      if (read_len == 0 && write_len == 0) continue;
     
      //cout << " r " << read_addr << " r2 " << read_addr2 << " " << read_len;
      //cout << " w " << write_addr << " " << write_len << endl;
      
      addr_type translated_read_addr, translated_write_addr, translated_read_addr2;
      bool ret_r = amv_handler.IsPresent(read_addr, translated_read_addr, 
                                         access_count);
      bool ret_r2 = amv_handler.IsPresent(read_addr2, translated_read_addr2, 
                                          access_count);
      bool ret_w = amv_handler.IsPresent(write_addr, translated_write_addr, 
                                         access_count);
//      if (ret_r || ret_r2 || ret_w) {
        //(*out_file[i]) << record_without_addr_str;
        (*out_file[i]) << "c " << access_count << " pc " << hex << m_translator.GetTranslated(pc)
                       << dec << " [" << line_no << ", " << file_name
                       << "]";  
        if (translated_read_addr != INVALID_ADDR)
          (*out_file[i]) << " r " << translated_read_addr << " " << read_len;
        if (translated_read_addr2 != INVALID_ADDR)
          (*out_file[i]) << " r " << translated_read_addr2 << " " << read_len;
        if (translated_write_addr != INVALID_ADDR)
          (*out_file[i]) << " w " << translated_write_addr << " " << write_len;
        (*out_file[i]) << endl;


//      }
//      mejbah added the following part
/*
        if(file_name.find(FILE_NAME_TO_DEBUG)!= string::npos ) {
          if( (line_no >= DEBUG_LINE_NO_START && line_no <= DEBUG_LINE_NO_END) || (line_no >= 2530 && line_no <= 2537 ) || (line_no >= 1743 && line_no <= 1750)) {
            addr_of_interest_file << "Line: "<< line_no << " " <<" PC: "<< pc <<" "<< m_translator.GetTranslated(pc)<<" Thread: " << i << endl;

          }
        }
        else if(file_name.find(FILE_NAME_TO_DEBUG_1)!= string::npos ) {
          if( (line_no >= DEBUG_LINE_NO_START_1 && line_no <= DEBUG_LINE_NO_END_1)) {
            addr_of_interest_file << "Line: "<< line_no << " " <<" PC: "<< pc <<" "<< m_translator.GetTranslated(pc)<<" Thread: " << i << endl;

          }
        }
*/
    }
  }
}

/* =================================================================== */

int main(int argc, char* args[]) {
  /////////////////////////////////

  /////////////////////////////////
  if (argc != 9) {
    cerr << "USAGE: filter file_conf file_in file_sym file_malloc file_free file_modules_old file_modules_translated file_out" 
         << endl;
    return -1;
  }
  
  //if (argc != 3) {
  //  cerr << "USAGE: filter file_malloc file_free" 
  //       << endl;
  //  return -1;
  //}
  //malloc_file.open(args[1], fstream::in);
  //free_file.open(args[2], fstream::in);
 
  conf_file.open(args[1], fstream::in);
  string str;
  getline(conf_file, str);
  stringstream ss(str);
  ss >> total_threads;
  cout << "total threads " << total_threads << endl;

  char *file_name = new char[BUF_SIZE];
  in_file = new fstream*[total_threads];
  for (int i = 0; i < total_threads; i++) {
    sprintf(file_name, "%s.%d", args[2], i);
    in_file[i] = new fstream(file_name, fstream::in);
  }

  char *log_file_name =  new char[BUF_SIZE];
  sprintf( log_file_name, "%s.trans.pc", args[2]);
//  addr_of_interest_file.open( log_file_name, fstream::out ); //mejbah added
  
  out_file = new fstream*[total_threads];
  for (int i = 0; i < total_threads; i++) {
    sprintf(file_name, "%s.%d", args[8], i);
    out_file[i] = new fstream(file_name, fstream::out);
  }

  sym_file.open(args[3], fstream::in);
  malloc_file.open(args[4], fstream::in);
  free_file.open(args[5], fstream::in);
  old_modules_file.open(args[6], fstream::in);
  translated_modules_file.open(args[7], fstream::in);
  delete[] file_name;

  while (getline(sym_file, str)) {
    stringstream ss(str);
    addr_type address;
    size_t size;
    ss >> address >> size;
    amv_handler.Allocate(address, size);
  }
  
  while (getline(malloc_file, str)) {
    stringstream ss(str);
    addr_type address;
    size_t size;
    ss >> address >> size;
    amv_handler.Allocate(address, size);
    //cout << "after Allocate(" << address << ", " << size << "):" << endl; 
    //amv_handler.Print();
  }
  // Since allocation splits lots of ranges, their translator
  // becomes inconsistent. So, we have to sanitize them.
  amv_handler.SanitizeTranslator();

  while (getline(free_file, str)) {
    stringstream ss(str);
    access_count_type access_counter;
    addr_type address;
    ss >> access_counter >> address;
    amv_handler.Free(address, access_counter);
    //cout << "after Free(" << address << ", " << access_counter << "):" << endl; 
    //amv_handler.Print();
  }

  while (getline(old_modules_file, str)) {
    //cout << "str " << str << endl;
    stringstream ss(str);
    int mod;
    long long base, upper_address;
    string name;
    ss >> hex >> mod >> base >> upper_address >> name ;
    m_translator.InsertModule(base, upper_address, &name);
  }

  while (getline(translated_modules_file, str)) {
    //cout << "str " << str << endl;
    stringstream ss(str);
    int mod;
    long long base, upper_address;
    string name;
    ss >> hex >> mod >> base >> upper_address >> name;
    //cout << hex << mod << " " << base << " " << upper_address << " " << dec << name << endl;
    m_translator.SetTranslatedModule(base, upper_address, &name);
  }

  ReadAtomicSections();
  
  for (int i = 0; i < total_threads; i++) {
    in_file[i]->close();
    out_file[i]->close();
    delete in_file[i];
    delete out_file[i];
  }
  delete[] in_file;
  delete[] out_file;
  
  conf_file.close();
  malloc_file.close();
  free_file.close();
  sym_file.close();
  old_modules_file.close();
  translated_modules_file.close();
//  addr_of_interest_file.close(); //mejbah added
  return 0;
}

/* =================================================================== */
// EOF 
/* =================================================================== */
