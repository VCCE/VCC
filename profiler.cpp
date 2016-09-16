#include <stdio.h>
#include <unistd.h>
#include <map>
#include "profiler.h"

static FILE *fp = NULL;
static bool enabled = false;
static unsigned short fetch_pc;
static unsigned char fetch_opcode;
static unsigned char fetch_post_byte;
static unsigned long cycles = 0;

// key=addr
static std::map <unsigned short int, int> routine_count;
static std::map <unsigned short int, int> routine_cycles;
static std::map <unsigned short int, char *> symbol;
// key=cycle count
// - a cheap (code-wise) way of sorting?
static std::map <unsigned long int, unsigned short int> sorted_cycles;

typedef struct
{
  unsigned short  pc;
  unsigned long   cycles;
  unsigned int    rts_levels;
  
} CALL_ENTRY, *PCALL_ENTRY;
static CALL_ENTRY call_stack[1024];
static int call_stack_depth = 0;

int profiler_reset_hook (void)
{
  return (0);
}

static char symbol_name[4096][32];
static unsigned n_symbols;

static char *extract_symbol (char *buf, unsigned i_name, unsigned i_addr)
{
  char *p;
  unsigned i = 0;
  unsigned short addr = 0;
  
  if (strlen(buf) > (i_addr+5) && buf[i_addr+5] == 'R')
  {
    // extract name
    p = buf + i_name;
    while (!isspace(*p))
      symbol_name[n_symbols][i++] = *(p++);
    symbol_name[n_symbols][i] = '\0';
    // extract address
    p = buf + i_addr;
    while (!isspace(*p))
    {
      addr <<= 4;
      if (isdigit(*p))
        addr |= (*p - '0');
      else
        addr |= (toupper(*p) - 'A' + 10);
      p++;
    }
    // insert into map
    symbol[addr] = symbol_name[n_symbols];
    n_symbols++;
  }
}

static int read_symbols (void)
{
  std::map <int, char *>::iterator it;

  FILE  *fp_sym = fopen ("kl.sym", "rt");
  char  line[256];
  
  if (!fp_sym)
  {
    fprintf (fp, "Unable to open .sym file!\n");
    fflush (fp);
    return (-1);
  }

  n_symbols = 0;
  
  fgets (line, 256-1, fp_sym);
  while (!feof (fp_sym))
  {
    fgets (line, 256, fp_sym);

    extract_symbol (line,  4, 23);
    extract_symbol (line, 38, 57);
  }

  // dump the symbols
  //for (it = symbol.begin (); it != symbol.end (); it++)
  //  fprintf (fp, "0x%04X: %s\n", it->first, it->second);

  fprintf (fp, "n_symbols=%d\n", n_symbols);
  fflush (fp);
  
  fclose (fp_sym);
}

int profiler_init_hook (void)
{
#if 1
	int					fd, fd2;

	fd = fileno (stderr);
  fd2 = dup (fd);
  fp = fdopen (fd2, "a+");
#else    
  fp = fopen ("profiler.txt", "wt");
#endif

  if (!fp)
    return (-1);

  if (symbol.size () == 0)
    read_symbols ();

  enabled = false;
      
  return (0);
}

int profiler_deinit_hook (void)
{
  std::map <unsigned short int, int>::iterator it;
  std::map <unsigned long int, unsigned short int>::reverse_iterator rit;
    
  enabled = false;

  // update the current context with cycle count
  if (call_stack_depth)
  {
    routine_cycles[call_stack[call_stack_depth-1].pc] +=
      call_stack[call_stack_depth-1].cycles;
    call_stack_depth--;
  }

  // build a map of sorted cycles
  for (it = routine_cycles.begin (); it != routine_cycles.end (); it++)
    sorted_cycles[it->second] = it->first;
        
  // dump the routines
  fprintf (fp, "\n");
  fprintf (fp, "Addr   Routine              Count    Cycles\n");
  fprintf (fp, "----   ----------------     -----    ------\n");
  for (rit = sorted_cycles.rbegin (); rit != sorted_cycles.rend (); ++rit)
  {
    fprintf (fp, "0x%04X %-16s %9d %9d (%3.0lf%%)\n",
              rit->second, 
              (symbol.find(rit->second) == symbol.end() 
                ? "unknown" 
                : symbol[rit->second]),
              routine_count[rit->second],
              rit->first,
              (double)rit->first/(double)cycles*100.0);
  }
  fprintf (fp, "----------------------            ---------\n", cycles);
  fprintf (fp, "Total Cycles                      %9d\n", cycles);
  
  if (fp)
    fclose (fp);
}

static int profiler_add_context (unsigned short pc, char *symbol_override=NULL)
{
  if (routine_count.find(pc) == routine_count.end())
  {
    // add a new symbol if not found
    if (symbol.find(pc) == symbol.end())
      if (symbol_override)
        symbol[pc] = symbol_override;
        
    fprintf (fp, "@0x%04X: ->0x%04X (%s)\n", fetch_pc, pc,
              (symbol.find(pc) == symbol.end() ? "unknown" : symbol[pc]));
    
    fflush (fp);
    routine_count[pc] = 0;
    routine_cycles[pc] = 0;
  }
  routine_count[pc]++;
        
  // add context to call stack
  call_stack[call_stack_depth].pc = pc;
  call_stack[call_stack_depth].cycles = 0;
  call_stack[call_stack_depth].rts_levels = 1;
  call_stack_depth++;

  return (0);
}

static int profiler_exit_context (void)
{
  if (!call_stack_depth)
  {
    fprintf (fp, "RTS/RTI/LEAS - call_stack_depth=0");
    fflush (fp);
    return (-1);
  }
  if (call_stack_depth)
  {
    call_stack_depth--;
    routine_cycles[call_stack[call_stack_depth].pc] +=
      call_stack[call_stack_depth].cycles;
  }
  cycles += call_stack[call_stack_depth].cycles;
  
  return (0);
}

// knightlore hacks
bool kl_pushed_ret = false;

int profiler_fetch_hook (unsigned short pc, unsigned char opcode, unsigned char opcode_post_byte)
{
  // store the current opcode [prefix]
  fetch_pc = pc;
  fetch_opcode = opcode;
  fetch_post_byte = opcode_post_byte;
    
  if (!enabled)
  {
    //if (symbol.find (pc) != symbol.end() && !strcmp (symbol[pc], "start"))
    if (pc == 0xC2C5)
    {
      fprintf (fp, "%s (@0x%x:0x%x,0x%x)\n", 
                __FUNCTION__, pc, fetch_opcode, fetch_post_byte);
      fprintf (fp, "- enabling profiling...\n");
      fflush (fp);

      call_stack_depth = 0;
      profiler_add_context (pc);

      enabled = true;
      cycles = 0;
    }
  }

  if (!enabled)
    return (0);

  // Knight Lore hacks    
  if (pc == 0xC2E7)
    kl_pushed_ret = true;

  return (0);
}

int profiler_post_fetch_hook (unsigned short pc, int cycles)
{
  PCALL_ENTRY   context = NULL;
  signed char   post_byte = (signed char)fetch_post_byte;
  unsigned      i;
    
  if (!enabled)
    return (0);
    
  // add to cycle count for current routine
  if (call_stack_depth)
    call_stack[call_stack_depth-1].cycles += cycles;

  switch (fetch_opcode)
  {
    case 0x10 :
      // only interested in LDS #nnnn
      if (fetch_post_byte != 0xCE)
        break;
      // reset all contexts
      call_stack_depth = 0;
      profiler_add_context (pc, (char *)"_LDS_");
      break;
      
    case 0x17 : // lbsr
    case 0x8D : // bsr
    case 0xBD : // jsr
      profiler_add_context (pc);
      break;

    // strictly speaking, this is not a subroutine call
    // but in _some_ cases it will
    // - need to hack for case-by-case basis
    case 0x6E : // JMP []
      if (kl_pushed_ret)
        profiler_add_context (pc);
      kl_pushed_ret = false;
      break;
      
    case 0x39 : // RTS
      profiler_exit_context ();
      break;

    case 0x3B : // RTI
      profiler_exit_context ();
      break;
      
    // again, this could have nothing to do with subroutines
    // but in Knight Lore it's used to return higher up the stack
    case 0x32 : // LEAS
      // only interested in LEAS n,S
      if (((post_byte>>5)&3) != 0x03)
        break;
      post_byte &= 0x1f;
      // only interested in +ve offsets
      if (post_byte & 0x10)
        break;
      // number of calls = post_byte/2
      post_byte >>= 1;
      fprintf (fp, "LEAS (0x%02X->%d)\n", fetch_post_byte, post_byte);
      fprintf (fp, "context = %s, rts_levels=%d\n", 
                symbol[call_stack[call_stack_depth-1].pc], post_byte);
      while (post_byte--)
      {
        fprintf (fp, "exiting context(%d) = %s\n", 
                  post_byte, symbol[call_stack[call_stack_depth-1].pc]);
        profiler_exit_context ();
      }
      fprintf (fp, "restoring context = %s\n", 
                symbol[call_stack[call_stack_depth-1].pc]);
      fflush (fp);
      break;
                              
    default :
      break;
  }
    
  return (0);
}

static int profiler_generic_irq_hook (unsigned short pc, char *type)
{
  if (!enabled)
    return (0);

  // no information on cycles!!!

  profiler_add_context (pc, type);
    
  return (0);
}

int profiler_firq_hook (unsigned short pc)
{
  return (profiler_generic_irq_hook (pc, (char *)"_FIRQ_"));
}

int profiler_irq_hook (unsigned short pc)
{
  return (profiler_generic_irq_hook (pc, (char *)"_IRQ_"));
}

int profiler_nmi_hook (unsigned short pc)
{
  return (profiler_generic_irq_hook (pc, (char *)"_NMI_"));
}
