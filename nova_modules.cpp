/* =================================================================== */
//  @AUTHOR: ABDULLAH A MUZAHID
//  @EMAIL: muzahid2@illinois.edu
/* =================================================================== */

/*==================================================================== */
                              //NOTES//
// It ignores functions outside application code.
// It also ignores function name starting with '.'.
// The purpose of this filtering is to avoid as much of
// unbalanced call/ret as possible.
/*==================================================================== */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <vector>
#include <utility>
#include <ext/hash_set>
#include "read_symbols.h"
#include "modules.h"

#define MALLOC "malloc"
#define CALLOC "calloc"
#define REALLOC "realloc"
#define FREE "free"
#define LOCK "nova_lock"
#define UNLOCK "nova_unlock"
//#define ANNOTATION

using namespace std;
using __gnu_cxx::hash_set;
using __gnu_cxx::hash;

/* =================================================================== */
// Constants
/* =================================================================== */

// Output file name buffer size.
const UINT16 BUF_SIZE = 512;
// Maximum number of modules this tool can handle
const UINT16 MAX_MODULES = 32;

const CHAR *excluded_lib_names[] = { "lib64",
                                     "libpthread",
                                     "libstc++",
                                     "libm",
                                     "libgcc_s",
                                     "libc",
                                     "ld_linux",
                                     "ld-linux",
                                     "libphp",
				    "libaprutil","libexpat", "libapr-0", "librt", "libfreebl3", "libcrypt", "libns", "libdl" // added for apache3
					  };
const UINT16 n_excluded_lib_names = 16;                                   

/* =================================================================== */
// Global Variables 
/* =================================================================== */

// There is one separate output file of each thread. The rest are one for the 
// whole program
// out_file - actual trace file
// conf_file - configuration file
// sync_file - synchronization variables file
// malloc_file - malloced addresses file
// free_file - freed addresses file
// symbol_file - file for global symbols
// others_file - file containing total dynamic instruction count 
//               and possibly other things
// modules_file - file for module id, address range & name
// loop_file - for loop information

fstream **out_file, conf_file, sync_file, malloc_file, free_file, symbol_file, 
        others_file, modules_file;
PIN_LOCK malloc_lock, free_lock;
PIN_LOCK ins_lock;

INT32 total_threads;

typedef UINT32 MEM_ACCESS_STAT;
volatile MEM_ACCESS_STAT mem_access_counter = 1;
//MEM_ACCESS_STAT mem_access_counter;
BOOL *is_write;
BOOL  *is_read;
BOOL *has_read2;
ADDRINT *waddr;
UINT32 *wlen;
ADDRINT *raddr;
ADDRINT *raddr2;
UINT32 *rlen;

// Initial value is 1 for main thread
volatile INT16 active_threads = 1;
volatile INT16 start_profile = 0;

Module *modules[MAX_MODULES];

// This variables are for reading symbols from binary
static	char *fname = NULL;
static tp_node *nodes = NULL;
static unsigned int n_nodes = 0;
#ifdef ANNOTATION
static int *flag_dummy_start;
bool dummy_check = false;
#endif

/* =================================================================== */
// Classes
/* =================================================================== */

class ADDRINTEq {
 public:
  bool operator() (const ADDRINT a, const ADDRINT b) const {
    return a == b;
  }
};

/* =================================================================== */
// Commandline Switches 
/* =================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
                            "o", "nova.out", 
                            "specify the output file name" );
KNOB<string> KnobConfFile(KNOB_MODE_WRITEONCE, "pintool",
                            "c", "nova.conf", 
                            "specify the file name for configuration ");
KNOB<string> KnobStdinFile(KNOB_MODE_WRITEONCE,  "pintool", "stdin", "", "STDIN file for the application");
#ifdef ANNOTATION
KNOB<string> KnobFuncAnnot(KNOB_MODE_WRITEONCE,  "pintool", "d", "", "y for dummy check n for no");
#endif
/* =================================================================== */

INT32 Usage() {
  cerr <<
      "This tool collects traces to extract atomic sections.\n";

  cerr << KNOB_BASE::StringKnobSummary();

  cerr << endl;

  return -1;
}
/* =================================================================== */
#ifdef ANNOTATION
VOID ProcessDummyStart( UINT32 thread_id ) {
  assert( flag_dummy_start[thread_id] == 0 );
  cout<<"DummyStart.."<< thread_id<< endl;
  flag_dummy_start[thread_id] = 1;  
  
}

VOID ProcessDummyEnd( UINT32 thread_id ) {
  assert( flag_dummy_start[thread_id] == 1 );
  cout<<"DummyEnd.."<< thread_id<<endl;
  flag_dummy_start[thread_id] = 0;  
}
#endif
/* =================================================================== */

VOID ProcessLock(ADDRINT mutex_address, UINT32 thread_id) {
  *(out_file[thread_id]) << LOCK << " " << mutex_address << endl;
}

/* =================================================================== */

VOID ProcessUnlock(ADDRINT mutex_address, UINT32 thread_id) {
  *(out_file[thread_id]) << UNLOCK << " " << mutex_address << endl;
}

/* =================================================================== */

VOID MemInsBefore(BOOL _is_write, BOOL _is_read, BOOL _has_read2,
                   ADDRINT _waddr, UINT32 _wlen,
                   ADDRINT _raddr, ADDRINT _raddr2, UINT32 _rlen,
                   UINT32 thread_id) {
  if (!start_profile) return;
  GetLock(&ins_lock, 1);
  is_write[thread_id] = _is_write;
  is_read[thread_id] = _is_read;
  has_read2[thread_id] = _has_read2;
  waddr[thread_id] = _waddr;
  wlen[thread_id] = _wlen;
  raddr[thread_id] = _raddr;
  raddr2[thread_id] = _raddr2;
  rlen[thread_id] = _rlen;
}

/* =================================================================== */

VOID MemInsAfter(UINT32 thread_id, ADDRINT inst_ptr/*, UINT32 mod, ADDRINT rva*/) {
  
  //if (active_threads < 2) return;
  if (!start_profile) return;
  MEM_ACCESS_STAT val = 1;
  __asm__ __volatile__("lock xaddl %3, %0"
                       : "=m"(mem_access_counter), "=r"(val)
                       :  "m"(mem_access_counter), "1"(val)
                      );
  ReleaseLock(&ins_lock);
  //*(out_file[thread_id]) << "c " << val << " mod " << mod << " rva " << hex 
  //                       << rva << dec;
#ifdef ANNOTATION

if(flag_dummy_start[thread_id] == 0 ) {
#endif
  *(out_file[thread_id]) << "c " << val << " pc " << hex << inst_ptr << dec;
  string file_name;
  INT32 line_num;
  PIN_LockClient();
  PIN_GetSourceLocation(inst_ptr, NULL, &line_num, &file_name);
  PIN_UnlockClient();
  *(out_file[thread_id]) << " [" << line_num << ", " << file_name << "]";
  if (is_read[thread_id])
    *(out_file[thread_id]) << " r " << raddr[thread_id] << " " << rlen[thread_id];
  if (has_read2[thread_id])
    *(out_file[thread_id]) << " r " << raddr2[thread_id] << " " << rlen[thread_id];
  if (is_write[thread_id])
    *(out_file[thread_id]) << " w " << waddr[thread_id] << " " << wlen[thread_id];
  *(out_file[thread_id]) << endl;
#ifdef ANNOTATION
}
#endif
  
}

/* =================================================================== */

VOID RecordSyncVar(ADDRINT sync_address) {
  sync_file << sync_address << endl;
}

/* =================================================================== */

VOID * New_Malloc( CONTEXT *context, AFUNPTR orgFuncptr, size_t size)
{
  VOID *ret;

  PIN_CallApplicationFunction( context, PIN_ThreadId(),
                                CALLINGSTD_DEFAULT, orgFuncptr,
                                PIN_PARG(void *), &ret,
                                PIN_PARG(size_t), size,
                                PIN_PARG_END() );

  GetLock(&malloc_lock, 1);
  malloc_file << (ADDRINT)ret << " " << size << endl;
  ReleaseLock(&malloc_lock);
  return ret;
}

/* =================================================================== */

VOID * New_Calloc( CONTEXT *context, AFUNPTR orgFuncptr, size_t nelem, size_t elesize)
{
  VOID *ret;

  PIN_CallApplicationFunction( context, PIN_ThreadId(),
                                CALLINGSTD_DEFAULT, orgFuncptr,
                                PIN_PARG(void *), &ret,
                                PIN_PARG(size_t), nelem,
                                PIN_PARG(size_t), elesize,
                                PIN_PARG_END() );

  GetLock(&malloc_lock, 1);
  malloc_file << (ADDRINT)ret << " " << (nelem * elesize) << endl;
  ReleaseLock(&malloc_lock);
  return ret;
}

/* =================================================================== */

VOID * New_Realloc( CONTEXT *context, AFUNPTR orgFuncptr, void *ptr, size_t size)
{
  VOID *ret;

  PIN_CallApplicationFunction( context, PIN_ThreadId(),
                                CALLINGSTD_DEFAULT, orgFuncptr,
                                PIN_PARG(void *), &ret,
                                PIN_PARG(void *), ptr,
                                PIN_PARG(size_t), size,
                                PIN_PARG_END() );

  if (size == 0 && ptr != NULL) {
    MEM_ACCESS_STAT val = 1;
    __asm__ __volatile__("lock xaddl %3, %0"
                        : "=m"(mem_access_counter), "=r"(val)
                        :  "m"(mem_access_counter), "1"(val)
                        );
    GetLock(&free_lock, 1);
    free_file << val << " " << (ADDRINT)ptr << endl;
    ReleaseLock(&free_lock);
  } else {
    GetLock(&malloc_lock, 1);
    malloc_file << (ADDRINT)ret << " " << size << endl;
    ReleaseLock(&malloc_lock);
  }
  return ret;
}

/* ===================================================================== */

VOID New_Free( CONTEXT *context, AFUNPTR orgFuncptr, void *ptr)
{
  if (start_profile) GetLock(&ins_lock, 1);
  PIN_CallApplicationFunction( context, PIN_ThreadId(),
                                CALLINGSTD_DEFAULT, orgFuncptr,
                                PIN_PARG(void),
                                PIN_PARG(void *), ptr,
                                PIN_PARG_END() );

  MEM_ACCESS_STAT val = 1;
  __asm__ __volatile__("lock xaddl %3, %0"
                      : "=m"(mem_access_counter), "=r"(val)
                      :  "m"(mem_access_counter), "1"(val)
                      );
  if (start_profile) ReleaseLock(&ins_lock);
  GetLock(&free_lock, 1);
  free_file << val << " " << (ADDRINT)ptr << endl;
  ReleaseLock(&free_lock);
}


/* =================================================================== */


VOID Image(IMG img, VOID *v) {
  if (IMG_Name(img).find("libpthread") != string::npos) {
    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
      for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)) {
        RTN_Open(rtn);
        //cout << "libpthread: " << RTN_Name(rtn) << endl;
        // This is so bizzare that I have to instrument pthread_cond_*
        // functions like this way.
        if (RTN_Name(rtn).find("pthread_cond_wait@@GLIBC") != string::npos) {
          RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)ProcessUnlock,
                          IARG_G_ARG1_CALLEE, IARG_THREAD_ID, IARG_END);
          RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)ProcessLock,
                          IARG_G_ARG1_CALLEE, IARG_THREAD_ID, IARG_END);
        }else if (RTN_Name(rtn).find("pthread_cond_init@GLIBC") != string::npos) {
          RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)RecordSyncVar,
                          IARG_G_ARG0_CALLEE, IARG_END);
        }
        RTN_Close(rtn);
      }
  }

  RTN mutex_init_rtn = RTN_FindByName(img, "pthread_mutex_init");
  if (RTN_Valid(mutex_init_rtn)) {
    RTN_Open(mutex_init_rtn);
    //cout << "****[" <<RTN_Name(mutex_init_rtn) << "]****" <<endl;
    RTN_InsertCall(mutex_init_rtn, IPOINT_AFTER, (AFUNPTR)RecordSyncVar,
                    IARG_G_ARG0_CALLEE, IARG_END);
    RTN_Close(mutex_init_rtn);
  }
  
  // For some reason, I have not found this function in the pthread
  // library. So, this instrumentation has no effect.
  RTN spin_init_rtn = RTN_FindByName(img, "pthread_spin_init");
  if (RTN_Valid(spin_init_rtn)) {
    RTN_Open(spin_init_rtn);
    //cout << "****[" <<RTN_Name(spin_init_rtn) << "]****" <<endl;
    RTN_InsertCall(spin_init_rtn, IPOINT_AFTER, (AFUNPTR)RecordSyncVar,
                    IARG_G_ARG0_CALLEE, IARG_END);
    RTN_Close(spin_init_rtn);
  }
  
  RTN rwlock_init_rtn = RTN_FindByName(img, "pthread_rwlock_init");
  if (RTN_Valid(rwlock_init_rtn)) {
    RTN_Open(rwlock_init_rtn);
    //cout << "****[" <<RTN_Name(rwlock_init_rtn) << "]****" <<endl;
    RTN_InsertCall(rwlock_init_rtn, IPOINT_AFTER, (AFUNPTR)RecordSyncVar,
                    IARG_G_ARG0_CALLEE, IARG_END);
    RTN_Close(rwlock_init_rtn);
  }
  
  //RTN cond_init_rtn = RTN_FindByName(img, "pthread_cond_init@@GLIBC");
  //if (RTN_Valid(cond_init_rtn)) {
  //  RTN_Open(cond_init_rtn);
  //  cout << "****[" <<RTN_Name(cond_init_rtn) << "]****" <<endl;
  //  RTN_InsertCall(cond_init_rtn, IPOINT_AFTER, (AFUNPTR)RecordSyncVar,
  //                  IARG_G_ARG0_CALLEE, IARG_END);
  //  RTN_Close(cond_init_rtn);
  //}

  RTN mutex_lock_rtn = RTN_FindByName(img, "pthread_mutex_lock");
  if (RTN_Valid(mutex_lock_rtn)) {
    RTN_Open(mutex_lock_rtn);
    //cout << "****[" <<RTN_Name(mutex_lock_rtn) << "]****" <<endl;
    RTN_InsertCall(mutex_lock_rtn, IPOINT_AFTER, (AFUNPTR)ProcessLock,
                    IARG_G_ARG0_CALLEE, IARG_THREAD_ID, IARG_END);
    RTN_Close(mutex_lock_rtn);
  }
  
  RTN mutex_unlock_rtn = RTN_FindByName(img, "pthread_mutex_unlock");
  if (RTN_Valid(mutex_unlock_rtn) &&
      (RTN_Name(mutex_unlock_rtn).find("pthread_mutex_unlock_usercnt") 
        == string::npos)) {
    RTN_Open(mutex_unlock_rtn);
    //cout << "****[" <<RTN_Name(mutex_unlock_rtn) << "]****" <<endl;
    RTN_InsertCall(mutex_unlock_rtn, IPOINT_BEFORE, (AFUNPTR)ProcessUnlock,
                    IARG_G_ARG0_CALLEE, IARG_THREAD_ID, IARG_END);
    RTN_Close(mutex_unlock_rtn);
  }
  
  RTN spin_lock_rtn = RTN_FindByName(img, "pthread_spin_lock");
  if (RTN_Valid(spin_lock_rtn)) {
    RTN_Open(spin_lock_rtn);
    //cout << "****[" <<RTN_Name(spin_lock_rtn) << "]****" <<endl;
    RTN_InsertCall(spin_lock_rtn, IPOINT_AFTER, (AFUNPTR)ProcessLock,
                    IARG_G_ARG0_CALLEE, IARG_THREAD_ID, IARG_END);
    RTN_Close(spin_lock_rtn);
  }
  
  RTN spin_unlock_rtn = RTN_FindByName(img, "pthread_spin_unlock");
  if (RTN_Valid(spin_unlock_rtn)) {
    RTN_Open(spin_unlock_rtn);
    //cout << "****[" <<RTN_Name(spin_unlock_rtn) << "]****" <<endl;
    RTN_InsertCall(spin_unlock_rtn, IPOINT_BEFORE, (AFUNPTR)ProcessUnlock,
                    IARG_G_ARG0_CALLEE, IARG_THREAD_ID, IARG_END);
    RTN_Close(spin_unlock_rtn);
  }
  
  RTN rw_rd_lock_rtn = RTN_FindByName(img, "pthread_rwlock_rdlock");
  if (RTN_Valid(rw_rd_lock_rtn)) {
    RTN_Open(rw_rd_lock_rtn);
    //cout << "****[" <<RTN_Name(rw_rd_lock_rtn) << "]****" <<endl;
    RTN_InsertCall(rw_rd_lock_rtn, IPOINT_AFTER, (AFUNPTR)ProcessLock,
                    IARG_G_ARG0_CALLEE, IARG_THREAD_ID, IARG_END);
    RTN_Close(rw_rd_lock_rtn);
  }
  
  RTN rw_wr_lock_rtn = RTN_FindByName(img, "pthread_rwlock_wrlock");
  if (RTN_Valid(rw_wr_lock_rtn)) {
    RTN_Open(rw_wr_lock_rtn);
    //cout << "****[" <<RTN_Name(rw_wr_lock_rtn) << "]****" <<endl;
    RTN_InsertCall(rw_wr_lock_rtn, IPOINT_AFTER, (AFUNPTR)ProcessLock,
                    IARG_G_ARG0_CALLEE, IARG_THREAD_ID, IARG_END);
    RTN_Close(rw_wr_lock_rtn);
  }
  
  RTN rw_unlock_rtn = RTN_FindByName(img, "pthread_rwlock_unlock");
  if (RTN_Valid(rw_unlock_rtn)) {
    RTN_Open(rw_unlock_rtn);
    //cout << "****[" <<RTN_Name(rw_unlock_rtn) << "]****" <<endl;
    RTN_InsertCall(rw_unlock_rtn, IPOINT_BEFORE, (AFUNPTR)ProcessUnlock,
                    IARG_G_ARG0_CALLEE, IARG_THREAD_ID, IARG_END);
    RTN_Close(rw_unlock_rtn);
  }

  //RTN cond_wait_rtn = RTN_FindByName(img, "pthread_cond_wait@@GLIBC");
  //if (RTN_Valid(cond_wait_rtn)) {
  //  RTN_Open(cond_wait_rtn);
  //  cout << "****[" <<RTN_Name(cond_wait_rtn) << "]****" <<endl;
  //  RTN_InsertCall(cond_wait_rtn, IPOINT_BEFORE, (AFUNPTR)ProcessCondUnlock,
  //                  IARG_G_ARG1_CALLEE, IARG_THREAD_ID, IARG_END);
  //  RTN_InsertCall(cond_wait_rtn, IPOINT_AFTER, (AFUNPTR)ProcessCondLock,
  //                  IARG_G_ARG1_CALLEE, IARG_THREAD_ID, IARG_END);
  //  RTN_Close(cond_wait_rtn);
  //}
  
  for (UINT16 i = 0; i < n_excluded_lib_names; i++)
    // Is this image suppossed to be excluded?
    if (IMG_Name(img).find(excluded_lib_names[i]) != string::npos)
      return;

  // This section iterates over all instructions of the image
  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)) {
      RTN_Open(rtn);
#ifdef ANNOTATION
      // check for dummy function annotated
      if( (RTN_Name(rtn).find("DummyStart") != std::string::npos) && dummy_check ) {
        RTN_InsertCall( rtn, IPOINT_BEFORE, (AFUNPTR)ProcessDummyStart,
                    IARG_THREAD_ID, IARG_END);
        RTN_Close(rtn);
        continue;
      }
      
      if( (RTN_Name(rtn).find("DummyEnd") != std::string::npos) && dummy_check ) {
        RTN_InsertCall( rtn, IPOINT_AFTER, (AFUNPTR)ProcessDummyEnd,
                    IARG_THREAD_ID, IARG_END);
        RTN_Close(rtn);
        continue;
      }
      // check for dummy function annotated..end
#endif
      for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
        // Avoid instrumenting the instrumentation
        if (!INS_IsOriginal(ins))
          continue;
        // In this version we are excluding stack accesses
        if (INS_IsStackRead(ins) || INS_IsStackWrite(ins)) {
          continue;
        }

        if ((INS_IsMemoryWrite(ins) || INS_IsMemoryRead(ins)) && INS_HasFallThrough(ins)) {
          if (INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) && 
              INS_HasMemoryRead2(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemInsBefore,
                          IARG_BOOL, true, 
                          IARG_BOOL, true,
                          IARG_BOOL, true,
                          IARG_MEMORYWRITE_EA,
                          IARG_MEMORYWRITE_SIZE,
                          IARG_MEMORYREAD_EA,
                          IARG_MEMORYREAD2_EA,
                          IARG_MEMORYREAD_SIZE,
                          IARG_THREAD_ID,
                          IARG_END);
            INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)MemInsAfter,
                           IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
          }
          else if (INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) && 
                  !INS_HasMemoryRead2(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemInsBefore,
                          IARG_BOOL, true, 
                          IARG_BOOL, true, 
                          IARG_BOOL, false,
                          IARG_MEMORYWRITE_EA,
                          IARG_MEMORYWRITE_SIZE,
                          IARG_MEMORYREAD_EA,
                          IARG_ADDRINT, 0,
                          IARG_MEMORYREAD_SIZE,
                          IARG_THREAD_ID,
                          IARG_END);
            INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)MemInsAfter,
                           IARG_THREAD_ID, IARG_INST_PTR,IARG_END);
          }
          else if (INS_IsMemoryWrite(ins) && !INS_IsMemoryRead(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemInsBefore,
                          IARG_BOOL, true, 
                          IARG_BOOL, false,
                          IARG_BOOL, false,
                          IARG_MEMORYWRITE_EA,
                          IARG_MEMORYWRITE_SIZE,
                          IARG_ADDRINT, 0,
                          IARG_ADDRINT, 0,
                          IARG_UINT32, 0,
                          IARG_THREAD_ID,
                          IARG_END);
            INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)MemInsAfter,
                           IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
          }
          else if (!INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) && 
                  INS_HasMemoryRead2(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemInsBefore,
                          IARG_BOOL, false, 
                          IARG_BOOL, true,
                          IARG_BOOL, true,
                          IARG_ADDRINT, 0,
                          IARG_UINT32, 0,
                          IARG_MEMORYREAD_EA,
                          IARG_MEMORYREAD2_EA,
                          IARG_MEMORYREAD_SIZE,
                          IARG_THREAD_ID,
                          IARG_END);
            INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)MemInsAfter,
                           IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
          }
          else if (!INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) && 
                  !INS_HasMemoryRead2(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemInsBefore,
                          IARG_BOOL, false, 
                          IARG_BOOL, true,
                          IARG_BOOL, false,
                          IARG_ADDRINT, 0,
                          IARG_UINT32, 0,
                          IARG_MEMORYREAD_EA,
                          IARG_ADDRINT, 0,
                          IARG_MEMORYREAD_SIZE,
                          IARG_THREAD_ID,
                          IARG_END);
            INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)MemInsAfter,
                           IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
          }
          else {
            ASSERTX(0);
          }
        } 
      }
      RTN_Close(rtn);
    }
}

/* =================================================================== */

VOID Fini(INT32 code, VOID *v) {
  for (INT32 i = 0; i < total_threads; i++) {
    out_file[i]->close();
    delete out_file[i];
  }
  delete[] out_file;

  conf_file.close();
  sync_file.close();
  malloc_file.close();
  free_file.close();
  others_file.close();
  modules_file.close();

  delete[] is_write;
  delete[] is_read;
  delete[] has_read2;
  delete[] waddr;
  delete[] raddr;
  delete[] raddr2;
  delete[] wlen;
  delete[] rlen;
}

/* =================================================================== */

// Format of conf file
//  1. Number of total threads
VOID ReadConfFile() {
  string str;
  getline(conf_file, str);
  stringstream ss(str);
  ss >> total_threads;
  cout << "total threads " << total_threads << endl;
}

/* =================================================================== */

VOID Initialize() {
  
  conf_file.open(KnobConfFile.Value().c_str(), fstream::in);
  ReadConfFile();
#ifdef ANNOTATION
  string str = KnobFuncAnnot.Value();
  if(str == "y") {
   dummy_check = true;
  }
  flag_dummy_start = new int[total_threads];
  for( int i=0; i<total_threads; i++) {
    flag_dummy_start[i] = 0;
  }
#endif
  char *out_file_name = new char[BUF_SIZE];
  out_file = new fstream*[total_threads];
  for (INT32 i = 0; i < total_threads; i++) {
    sprintf(out_file_name, "%s.%d", KnobOutputFile.Value().c_str(), i);
    out_file[i] = new fstream(out_file_name, fstream::out);
  }
  sprintf(out_file_name, "%s.sync", KnobOutputFile.Value().c_str());
  sync_file.open(out_file_name, fstream::out);
  
  sprintf(out_file_name, "%s.malloc", KnobOutputFile.Value().c_str());
  malloc_file.open(out_file_name, fstream::out);
  
  sprintf(out_file_name, "%s.free", KnobOutputFile.Value().c_str());
  free_file.open(out_file_name, fstream::out);
  
  sprintf(out_file_name, "%s.sym", KnobOutputFile.Value().c_str());
  symbol_file.open(out_file_name, fstream::out);
  
  sprintf(out_file_name, "%s.others", KnobOutputFile.Value().c_str());
  others_file.open(out_file_name, fstream::out);

  sprintf(out_file_name, "%s.modules", KnobOutputFile.Value().c_str());
  modules_file.open(out_file_name, fstream::out);
  
  delete[] out_file_name;

  InitLock(&malloc_lock);
  InitLock(&free_lock);
  InitLock(&ins_lock);
  
  is_write = new BOOL[total_threads];
  is_read = new BOOL[total_threads];
  has_read2 = new BOOL[total_threads];
  waddr = new ADDRINT[total_threads];
  wlen = new UINT32[total_threads];
  raddr = new ADDRINT[total_threads];
  raddr2 = new ADDRINT[total_threads];
  rlen = new UINT32[total_threads];
  for (INT32 i = 0; i < total_threads; i++) {
    is_write[i] = is_read[i] = has_read2[i] = false;
    waddr[i] = raddr[i] = raddr2[i] = 0;
    wlen[i] = rlen[i] = 0;
  }
}

/* ================================================================== */

VOID WriteSymbols() {
  for (UINT32 i = 0; i < n_nodes; i++) {
    symbol_file << nodes[i].addr << " " << nodes[i].size << " "
                << string(nodes[i].var_name) << endl;
  }
  symbol_file.close();
}

/* =================================================================== */

VOID ImageLoad(IMG img, VOID *v) {
  ASSERTX(IMG_Id(img) < MAX_MODULES);
  
  RTN mallocRtn = RTN_FindByName(img, MALLOC);
  if (RTN_Valid(mallocRtn))
  {
    PROTO proto_malloc = PROTO_Allocate( PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                          MALLOC, PIN_PARG(size_t), PIN_PARG_END() );
    
    
    RTN_ReplaceSignature(
        mallocRtn, AFUNPTR( New_Malloc ),
        IARG_PROTOTYPE, proto_malloc,
        IARG_CONTEXT,
        IARG_ORIG_FUNCPTR,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
        IARG_END);
    //malloc_file << "Replaced malloc() in:"  << IMG_Name(img) << endl;
  }
  
  RTN callocRtn = RTN_FindByName(img, CALLOC);
  if (RTN_Valid(callocRtn))
  {
    PROTO proto_calloc = PROTO_Allocate( PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                          CALLOC, PIN_PARG(size_t), PIN_PARG(size_t), 
                                          PIN_PARG_END() );
    
    
    RTN_ReplaceSignature(
        callocRtn, AFUNPTR( New_Calloc ),
        IARG_PROTOTYPE, proto_calloc,
        IARG_CONTEXT,
        IARG_ORIG_FUNCPTR,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
        IARG_END);
    //malloc_file << "Replaced calloc() in:"  << IMG_Name(img) << endl;
  }
  
  RTN reallocRtn = RTN_FindByName(img, REALLOC);
  if (RTN_Valid(reallocRtn))
  {
    PROTO proto_realloc = PROTO_Allocate( PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                          REALLOC, PIN_PARG(void *), PIN_PARG(size_t), 
                                          PIN_PARG_END() );
    
    
    RTN_ReplaceSignature(
        reallocRtn, AFUNPTR( New_Realloc ),
        IARG_PROTOTYPE, proto_realloc,
        IARG_CONTEXT,
        IARG_ORIG_FUNCPTR,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
        IARG_END);
    //malloc_file << "Replaced realloc() in:"  << IMG_Name(img) << endl;
  }

  RTN freeRtn = RTN_FindByName(img, FREE);
  if (RTN_Valid(freeRtn))
  {
    PROTO proto_free = PROTO_Allocate( PIN_PARG(void), CALLINGSTD_DEFAULT,
                                        FREE, PIN_PARG(void *), PIN_PARG_END() );
    
    
    RTN_ReplaceSignature(
        freeRtn, AFUNPTR( New_Free ),
        IARG_PROTOTYPE, proto_free,
        IARG_CONTEXT,
        IARG_ORIG_FUNCPTR,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
        IARG_END);
    //free_file << "Replaced free() in:"  << IMG_Name(img) << endl;
  }

  modules[IMG_Id(img)] = new Module(IMG_Name(img), IMG_LowAddress(img), 
                                    IMG_HighAddress(img), IMG_Id(img));
  modules_file << hex << IMG_Id(img) << " " << IMG_LowAddress(img) 
               << " " << IMG_HighAddress(img) << " " 
               << IMG_Name(img) << endl;
}

/* =================================================================== */

VOID ImageUnload(IMG img, VOID *v) {
  ASSERTX(modules[IMG_Id(img)] != NULL);
  delete modules[IMG_Id(img)];
}

/* =================================================================== */

VOID ThreadBegin(THREADID thread_id, CONTEXT *ctxt, INT32 flags, VOID *v)
{
  INT16 val = 1;
  __asm__ __volatile__("lock xaddw %3, %0"
                       : "=m"(active_threads), "=r"(val)
                       :  "m"(active_threads), "1"(val)
                      );
	ASSERTX(thread_id < (UINT32)total_threads);
  if (start_profile == 0 && active_threads > 1) start_profile = 1;
  cout << "Thread start " << thread_id << endl;
}

/* =================================================================== */

VOID ThreadEnd(THREADID thread_id, const CONTEXT *ctxt, INT32 code, VOID *v)
{
  INT16 val = -1;
  __asm__ __volatile__("lock xaddw %3, %0"
                       : "=m"(active_threads), "=r"(val)
                       :  "m"(active_threads), "1"(val)
                      );
  cout << "Thread end " << thread_id << endl;
}

/* =================================================================== */

int main(int argc, char *argv[]) {

  PIN_InitSymbols();
  
  if( PIN_Init(argc,argv) ) {
      return Usage();
  }
  
  Initialize();  
  
  	
  fname=rs_get_executable(argc, argv);
  rs_read_symbol_table(fname, &nodes, &n_nodes);
  WriteSymbols(); 

  string stdinFile = KnobStdinFile.Value();
  if(stdinFile.size() > 0) {
    assert(freopen(stdinFile.c_str(), "rt", stdin));
  }

	
  IMG_AddInstrumentFunction(ImageLoad, 0);
  IMG_AddUnloadFunction(ImageUnload, 0);
  
  PIN_AddThreadStartFunction(ThreadBegin,0);
  PIN_AddThreadFiniFunction(ThreadEnd,0);
  IMG_AddInstrumentFunction(Image, 0);
  PIN_AddFiniFunction(Fini, 0);

  // Never returns
  PIN_StartProgram();
    
  return 0;
}

/* ================================================================== */
// eof 
/* ================================================================== */
