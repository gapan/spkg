/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#if __BENCH == 1

#include <stdio.h>
#include <asm/msr.h>

/* tics per second (this is constant for my 1GHz athlon) */
#define TPS 1000000000ull
#define CNT 16

/* [0] = start tics, [1] = accumulated tics, [2] = times stopped/continued */
static unsigned long long timers[CNT][3];

static __inline__ void reset_timer(int t)
{
  timers[t][1] = 0;
  timers[t][2] = 0;
}

static __inline__ void reset_timers()
{
  int i;
  for (i=0;i<CNT;i++)
    reset_timer(i);
}

static __inline__ void start_timer(int t)
{
  reset_timer(t);
  rdtscll(timers[t][0]);
}

static __inline__ void continue_timer(int t)
{
  rdtscll(timers[t][0]);
}

static __inline__ void stop_timer(int t)
{
  unsigned long long stop;
  rdtscll(stop);
  timers[t][2] += 1;
  timers[t][1] += stop - timers[t][0];
}

static __inline__ double get_timer(int t)
{
  return (double)(timers[t][1])/TPS;
}

static __inline__ void get_time_str(double d, char* buf)
{
  if (d < 1e-6)
    sprintf(buf, "%1.1lf ns", d*1e9);
  else if (d < 1e-3)
    sprintf(buf, "%1.4lf us", d*1e6);
  else if (d < 1)
    sprintf(buf, "%1.7lf ms", d*1e3);
  else
    sprintf(buf, "%1.10lf s", d);
}


static __inline__ void print_timer(int t, char* msg)
{
  char buf1[64];
  char buf2[64];
  unsigned int c = (unsigned int)timers[t][2];
  double d1 = get_timer(t);
  double d2 = get_timer(t)/c;
  get_time_str(d1,buf1);
  get_time_str(d2,buf2);
  printf("** timer: %-30s : %-15s : %-15s per cycle (%u cycles)\n", msg?msg:"", c?buf1:"---", c?buf2:"---", c);
}

#undef CNT
#undef TPS

#else

static __inline__ void start_timer(int t) { }
static __inline__ void reset_timers() { }
static __inline__ void reset_timer(int t) { }
static __inline__ void continue_timer(int t) { }
static __inline__ void stop_timer(int t) { }
static __inline__ double get_timer(int t) { return 0; }
static __inline__ void print_timer(int t, char* msg) { }

#endif
