/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <asm/msr.h>

/* tics per second (this is constant for my 1GHz athlon) */
#define TPS 1000000000ull
#define CNT 8

unsigned long long timers[CNT][3];

static __inline__ void start_timer(int t) {
  timers[t][2] = 0;
  rdtscll(timers[t][0]);
}

static __inline__ void reset_timer(int t) {
  timers[t][2] = 0;
}

static __inline__ void continue_timer(int t) {
  rdtscll(timers[t][0]);
}

static __inline__ void stop_timer(int t) {
  rdtscll(timers[t][1]);  
  timers[t][2] += timers[t][1] - timers[t][0];
}

static __inline__ double get_timer(int t) {
  return (double)(timers[t][1]-timers[t][0])/TPS; 
}

void print_timer(int t, char* msg)
{
  double d = (double)timers[t][2]/TPS;

  if (d < 1e-6)
    printf("timer[%d]: %s = %1.1lf ns\n", t, msg?msg:"", d*1e9);
  else if (d < 1e-3)
    printf("timer[%d]: %s = %1.4lf us\n", t, msg?msg:"", d*1e6);
  else if (d < 1)
    printf("timer[%d]: %s = %1.7lf ms\n", t, msg?msg:"", d*1e3);
  else
    printf("timer[%d]: %s = %1.10lf s\n", t, msg?msg:"", d);
}
