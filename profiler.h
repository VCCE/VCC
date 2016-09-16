#ifndef __PROFILER_H__
#define __PROFILER_H__

#ifdef __cplusplus
extern "C"
{
#endif

int profiler_reset_hook (void);
int profiler_init_hook (void);
int profiler_deinit_hook (void);
int profiler_fetch_hook (unsigned short pc, unsigned char opcode, unsigned char opcode_post_byte);
int profiler_post_fetch_hook (unsigned short pc, int cycles);
int profiler_firq_hook (unsigned short pc);
int profiler_irq_hook (unsigned short pc);
int profiler_nmi_hook (unsigned short pc);

#ifdef __cplusplus
}
#endif

#endif
