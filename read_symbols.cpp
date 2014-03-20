#include "read_symbols.h"

char *
rs_get_executable(int argc, char * argv[])
{
  int i;
  assert(argc>0);

  for(i=0; i<argc; i++)
    if(0==strcmp(argv[i], "--"))
      break;

  if((i==argc) || (NULL==argv[i+1])){
    fprintf(stderr, "Error parsing command line\n");
    exit(1);
  }
  return argv[i+1];
}
/*
bfd *
rs_symbol_table_init(const char *file_name)
{
  bfd *abfd = NULL;
  char **matching;

  bfd_init ();
//  bfd_set_default_target ();
  abfd = bfd_openr (file_name,  target NULL);
  
  if(NULL == abfd ){
    fprintf(stderr, "Error opening the symbol table");
    exit(1);
  }

  if (bfd_check_format (abfd, bfd_archive)){
    fprintf(stderr, "Error checking the symbol table");
    exit(1);
  }

  if (! bfd_check_format_matches (abfd, bfd_object, &matching)){
    fprintf(stderr, "Error checking the format of the symbol table");
    exit (1);
  }
  return abfd;
} */

int compar_addr(const void *n0, const void *n1){
	assert(n0!=NULL && n1!=NULL);

	return (((const tp_node *)n0)->addr < ((const tp_node *)n1)->addr ? -1 : 
			(((const tp_node *)n0)->addr == ((const tp_node *)n1)->addr ? 0 : 1));
}

void
rs_read_symbol_table(const char *file_name, tp_node **ptr_node, unsigned int *n_nodes)
{
	pid_t pid;
	int desc[2];
	tp_node *_prt = NULL;
	unsigned int _n_nodes = 0, _max_nodes=100;
	tp_addr _addr;
	char line[512];
	char _var_name[3*MAX_NAME_VARIABLE], _sect; /* buffer overflow risk */
	int _size;
	size_t len;
	char _v[3*MAX_NAME_VARIABLE]; /* buffer overflow risk */
	char _w[3*MAX_NAME_VARIABLE]; /* buffer overflow risk */

	if(-1==pipe(desc)){
    fprintf(stderr, "Error creating pipes\n");
    exit(1);
  }

	pid = fork();
	
	if(-1==pid){
		fprintf(stderr, "Error executing fork\n");
    exit(1);
	}
	/* child */
	if(0==pid){
    if(-1==dup2(desc[1], STDOUT_FILENO)){
      fprintf(stderr, "Error executing dup2\n");
      exit(1);
    }
    close(desc[0]);
		if(-1==execl("/usr/bin/nm", "nm", "-S", "-C", file_name, NULL)){
      perror("Error executing execlp");
      exit(1);
    }
	}else{
		if(-1==dup2(desc[0], STDIN_FILENO)){
      fprintf(stderr, "Error executing dup2\n");
      exit(1);
    }
		close(desc[1]);
		if((_prt=(tp_node*)calloc(_max_nodes, sizeof(tp_node)))==NULL){
			perror("Error allocating virtual memory");
			exit(1);
		}
		/* read a complete line into a buffer */
		while(fgets(line, 512, stdin)!=NULL)
			if(sscanf(line, " %s %s %c %s\n", _v, _w, &_sect, _var_name)==4){
				_sect = toupper(_sect);
				if((len=strlen(_var_name))>MAX_NAME_VARIABLE){
					fprintf(stderr, "Increase MAX_NAME_VARIABLE at least to: %d\n", (int)len);
					exit(1);
				}
				/* lower section name means local and upper case  global, for the moment
					 we add both */
				if(_sect == 'B' || _sect == 'D'){
					/* single scanf didn't work. I don't know why ?? */
					sscanf(_v, "%lx", &_addr);
					sscanf(_w, "%x", &_size);
					_prt[_n_nodes].addr=_addr;
					_prt[_n_nodes].size=_size;
					if(strcpy(_prt[_n_nodes].var_name, _var_name)==NULL){
						fprintf(stderr, "Error copying var_name string\n");
						exit(1);
					}
#ifdef VERYVERBOSE
	fprintf(stderr, "Added var: %s, 0x%lx, size: %u\n", _var_name, _addr, _size);
#endif
					_n_nodes++;
					if(_n_nodes==_max_nodes){
						if((_prt=(tp_node*)realloc(_prt, _max_nodes*2*sizeof(tp_node)))==NULL){
							perror("Error realocating memory");
							exit(1);
						}
						if(memset(&_prt[_n_nodes], 0, _max_nodes)==NULL){
							perror("Error clearing memory");
							exit(1);
						}
						_max_nodes*=2;
					}
				}
			}

		wait(NULL);
	}

	if((_n_nodes>0) && (_n_nodes<_max_nodes)){
		if((_prt=(tp_node*)realloc(_prt, _n_nodes*sizeof(tp_node)))==NULL){
			perror("Error resizing variable list");
			exit(1);
		}
	}
	/* sort the array based on the addesses */
	qsort((void *)_prt, _n_nodes, sizeof(tp_node), compar_addr);
	*n_nodes=_n_nodes;
	*ptr_node = _prt;
#ifdef VERBOSE
	fprintf(stderr, "Added %u nodes in %s\n", *n_nodes, __PRETTY_FUNCTION__);
#endif	
}

char *
rs_get_source_and_line(tp_addr PC, char *file_name)
{
  pid_t pid;
  char args[MAX_LENGHT];
  char *source_and_line;
  int desc[2];

  sprintf(args, "%lx", PC);
  if((source_and_line=(char*)calloc(MAX_LENGHT, sizeof(char)))==NULL){
    fprintf(stderr, "Error allocating virtual memory");
    exit(1);
  }

  if(-1==pipe(desc)){
    fprintf(stderr, "Error creating pipes\n");
    exit(1);
  }

  pid = fork();
  if(-1==pid){
    fprintf(stderr, "Error executing fork\n");
    exit(1);
  }
  
  if(0==pid){
    /* child */
    if(-1==dup2(desc[1], STDOUT_FILENO)){
      fprintf(stderr, "Error executing dup2\n");
      exit(1);
    }
    close(desc[0]);
    if(-1==execl("/usr/bin/addr2line", "addr2line", args, "-e", file_name, NULL)){
      perror("Error executing execlp");
      exit(1);
    }
  }else{
    if(-1==dup2(desc[0], STDIN_FILENO)){
      fprintf(stderr, "Error executing dup2\n");
      exit(1);
    }
    close(desc[1]);
    scanf("%s", source_and_line);
    wait(NULL);
  }
  return source_and_line;
}

unsigned int
rs_addr_in_nodes(const tp_node *nodes, const unsigned int n_nodes,
	const tp_addr addr)
{
	unsigned int i;
	/* the vector is ordered!!! */

/*#ifdef VERYVERBOSE
	fprintf(stderr, "%s:0x%lx:%u nodes\n", __PRETTY_FUNCTION__, addr, n_nodes);
#endif	*/

	if(0==n_nodes)
		return false;

	for(i=0; i<(n_nodes-1); i++){
//#ifdef VERYVERBOSE
//#endif	
		if(addr < nodes[i].addr)
			return false;
		if((addr >= nodes[i].addr) && (addr < nodes[i+1].addr)){
			return addr <= (nodes[i].addr+nodes[i].size); 
    }
	}
	/* it lacks the last element */
	return addr>= nodes[n_nodes-1].addr && 
		addr <= (nodes[n_nodes-1].addr + nodes[n_nodes-1].size);
}

const char *
rs_get_name_from_addr(const tp_node *nodes, const unsigned int n_nodes,
	const tp_addr addr)
{
	unsigned int i;

	assert(n_nodes!=0);

	for(i=0; i<(n_nodes-1); i++){
	
		if(addr < nodes[i].addr)
			return NULL;
	
		if((addr >= nodes[i].addr) && (addr < nodes[i+1].addr)){
			if(addr <= (nodes[i].addr+nodes[i].size)){
				return nodes[i].var_name;
			}else{
				return NULL;
			}
		}
	}
	
	if(addr>= nodes[n_nodes-1].addr && 
			addr <= (nodes[n_nodes-1].addr + nodes[n_nodes-1].size))
		return nodes[n_nodes-1].var_name;
	
	return NULL;
}

tp_addr
rs_get_addr_from_name(const tp_node *nodes, const unsigned int n_nodes,
		char *name)
{
	unsigned int i;

	assert(n_nodes!=0);

	for(i=0; i<n_nodes; i++){
		if(strcmp(name, nodes[i].var_name)==0)
			return nodes[i].addr;
	}

	fprintf(stderr, "Variable %s not found\n", name);
	exit(1);
	return 0;
}
#if 0
void
rs_init_nodes()
{
  unsigned int i;

  for(i=0;i<MAX_VARIABLES;i++){
    nodes[i].addr=0ll;
    nodes[i].size=0;
    nodes[i].var_name[0]='\0';
  }
  n_nodes = 0;
  root = NULL;
}
#endif


#if 0
rs_parse_section(bfd *abfd, asection *section,
    void *ignored ATTRIBUTE_UNUSED)
{

  /* only parse .data and .bss sections */
  if(!strcmp(section->name, ".data") || !strcmp(section->name, ".bss"))
    return;


}
void
rs_create_variable_tree(bfd *abfd)
{
  /* parse sections looking for .data and .bss */
/*  bfd_map_over_sections(abfd,
         void (*func) (bfd *abfd, asection *sect, void *obj),
         void *obj);
*/
}

#endif
