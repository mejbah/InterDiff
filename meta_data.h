#ifndef __META_DATA__
#define __META_DATA__

#include <cstring>
#include "instruction.h"



typedef struct meta_data
{	
	pc_type last_write_addr; /* last write PC */
	int last_write_tid;
	access_count_type access_count;
	
}MetaData;

#endif


