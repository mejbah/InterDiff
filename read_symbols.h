#ifndef READ_SYMBOLS_H
#define READ_SYMBOLS_H

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* #include <bfd.h> */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#ifdef false
	#undef false
#endif
#define false 0

#ifdef true
	#undef true
#endif
#define true 1

/*#ifndef VERBOSE
	#define VERBOSE 1
#endif */

#define MAX_LENGHT 512
#define MAX_VARIABLES 4096
#define MAX_NAME_VARIABLE 256

#define RS_DEBUG_PRINT \
	fprintf(stderr, "%s:%s:%d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__);

typedef unsigned long tp_addr;

typedef struct {
  tp_addr addr;
  unsigned int size;
	char var_name[MAX_NAME_VARIABLE];
} tp_node;

/* return the first argument after -- 
   Executable file in Pin */
char *
rs_get_executable(int argc, char * argv[]);

/*bfd *
rs_symbol_table_init(const char *file_name); */

/* it returns a sortered array of global variables */
void
rs_read_symbol_table(const char *file_name, tp_node **ptr_node, unsigned int *n_nodes);

/* it returns true when the address belongs to the global variables array,
check for the sizes of the variables */
unsigned int
rs_addr_in_nodes(const tp_node *nodes, const unsigned int n_nodes, const tp_addr addr);

char *
rs_get_source_and_line(tp_addr PC, char *file_name);

const char *
rs_get_name_from_addr(const tp_node *nodes, const unsigned int n_nodes,
	const tp_addr addr);

tp_addr
rs_get_addr_from_name(const tp_node *nodes, const unsigned int n_nodes,
		char *name);

#endif
