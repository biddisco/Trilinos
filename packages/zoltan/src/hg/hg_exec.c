/*****************************************************************************
 * Zoltan Library for Parallel Applications                                  *
 * Copyright (c) 2000,2001,2002, Sandia National Laboratories.               *
 * For more info, see the README file in the top-level Zoltan directory.     *
 *****************************************************************************/


#include "hypergraph.h"
#include <math.h>

static double hcut_size_links (ZZ *zz, HGraph *hg, int p, Partition part);
static double hcut_size_total (HGraph *hg, Partition part);
int hg_readfile (ZZ*, HGraph*, char*, int*);


/* =========== TIME */
/* This is used to measure the time of the stand-alone program hg_test. */
#ifdef WITHTIME
static long     t=0, t_init=0, t_load=0, t_part=0, t_rest=0;
static void times_output ()
{ long t_all=t_load+t_part+t_rest;
  printf("TIME                : %d:%d:%.2f\n", (int)(t_all/3600000),
   (int)((t_all%3600000)/60000),(float)((float)(t_all%60000)/1000));
  printf("  Load/Check        : %.2f\n",(float)t_load/1000);
  printf("  Part              : %.2f\n",(float)t_part/1000);
  printf("  Rest              : %.2f\n",(float)t_rest/1000);
}
#ifdef CLOCK
#define INIT_TIME()     {if(!t_init)t=clock()/1000;t_init++;}
#define ADD_NEW_TIME(T) {T-=t; T+=(t=clock()/1000);}
#define END_TIME()      {t_init--;}
#else
#include <sys/time.h>
#include <sys/resource.h>
struct rusage    Rusage;
#define INIT_TIME()     {if(!t_init) \
                         { getrusage(RUSAGE_SELF,&Rusage); \
                           t=Rusage.ru_utime.tv_sec*1000+ \
                             Rusage.ru_utime.tv_usec/1000; } \
                         t_init++;}
#define ADD_NEW_TIME(T) {T-=t; getrusage(RUSAGE_SELF,&Rusage); \
                         T+=(t=Rusage.ru_utime.tv_sec*1000+ \
                               Rusage.ru_utime.tv_usec/1000);}
#define END_TIME()      {t_init--;}
#endif
#else
static void times_output () {}
#define INIT_TIME()     {}
#define ADD_NEW_TIME(T) {}
#define END_TIME()      {}
#endif



/* The main procedure for the executable hg_test */

int main (int argc, char **argv)
{
int    i, p = 2, *part, *part2, memory_graph;
char   hgraphfile[100] = "grid5x5.hg";
HGraph hg;
HGPartParams hgp;
ZZ     zz;
int    base;   /* minimum vertex number in input file; usually 0 or 1. */
int err;

/* Pre-set parameter values */
  hgp.check_graph = 1;
  hgp.bal_tol = 1.1;
  hgp.redl = 0;
  hgp.ews  = 1;
  hgp.output_level = HG_DEBUG_LIST;

hgp.fmswitch        = -1;
hgp.noimprove_limit = 0.25;
hgp.nlevelrepeat    = 0;
hgp.tollevelswitch  = 10000;
hgp.tolfactor       = 0.5;
hgp.cleanup         = 0;
hgp.cleanuprepeat   = 0;
hgp.tiestrategy     = 0;
hgp.hyperedge_limit = 10000;
hgp.orphan_flag = 0;

  strcpy(hgp.redm_str,   "grg");
  strcpy(hgp.redmo_str,  "aug2");
  strcpy(hgp.global_str, "gr0");
  strcpy(hgp.local_str,  "fm2");

  zz.Debug_Level = 1;
  zz.Proc = 0;
  zz.Num_Proc = 1;
  Zoltan_Memory_Debug(2);

/* Start of the time*/
  INIT_TIME();

/* Read parameters */
  if (argc == 1) {
     puts("hg_test [-flag value] [] [] ...");
     puts("-f      graphfile:        (grid5x5.hg)");
     puts("-p      # of parts:       (2)");
     puts("-bal    balance tolerance:(1.1)");
     puts("-redl   reduction limit:  (0)");
     puts("-redm   reduction method: {mxm,rem,rrm,rhm,grm,lhm,pgm,");
     puts("                           mxp,rep,rrp,rhp,grp,lhp,pgp,");
     puts("                           mxg,reg,rrg,rhg,(grg),");
     puts("                           no}");
     puts("-redmo  reduction augment:{no,aug1,(aug2)}");
     puts("-reds   reduction scaling:(1)");
     puts("-g      global method:    {ran,lin,bfs,rbfs,bfsh,rbfsh,");
     puts("                           (gr0),gr1,gr2,gr3,gr4}");
     puts("-l      local method:     no,(fm)");
     puts("-d      HG debug level:      (1)");
     puts("-zd     Zoltan debug level:  (1)");
     puts("default values are in brackets ():");
     return 0;
     }
  i = 0;
  while (++i<argc) {
    if     (!strcmp(argv[i],"-f")   &&i+1<argc)strcpy(hgraphfile,argv[++i]);
    else if(!strcmp(argv[i],"-p")   &&i+1<argc)p=atoi(argv[++i]);
    else if(!strcmp(argv[i],"-bal") &&i+1<argc)hgp.bal_tol=atof(argv[++i]);
    else if(!strcmp(argv[i],"-redl")&&i+1<argc)hgp.redl=atoi(argv[++i]);
    else if(!strcmp(argv[i],"-redm")&&i+1<argc)strcpy(hgp.redm_str,argv[++i]);
    else if(!strcmp(argv[i],"-redmo")&&i+1<argc)strcpy(hgp.redmo_str,argv[++i]);
    else if(!strcmp(argv[i],"-reds")&&i+1<argc)hgp.ews=atoi(argv[++i]);
    else if(!strcmp(argv[i],"-g")   &&i+1<argc)strcpy(hgp.global_str,argv[++i]);
    else if(!strcmp(argv[i],"-l")   &&i+1<argc)strcpy(hgp.local_str,argv[++i]);
    else if(!strcmp(argv[i],"-d")   &&i+1<argc)hgp.output_level=atoi(argv[++i]);
    else if(!strcmp(argv[i],"-zd")  &&i+1<argc)zz.Debug_Level=atoi(argv[++i]);
    else {
       fprintf(stderr,"ERR...option '%s' not legal or without value\n",argv[i]);
       return 1;
       }
    }
  zz.Debug_Level = ((hgp.output_level>zz.Debug_Level) ? hgp.output_level
                                                      : zz.Debug_Level);
  if (Zoltan_HG_Set_Part_Options(&zz, &hgp))
     return 1;
  ADD_NEW_TIME(t_rest);

  /* load hypergraph and print its info */
  if (hg_readfile(&zz,&hg,hgraphfile,&base))
     return 1;
  if (hgp.output_level >= HG_DEBUG_PRINT)
     Zoltan_HG_Print(&zz, &hg, stdout);
  memory_graph = Zoltan_Memory_Usage (ZOLTAN_MEM_STAT_TOTAL);
  printf("Initial Memory: %d %d\n", memory_graph,
   Zoltan_Memory_Usage (ZOLTAN_MEM_STAT_MAXIMUM) );
  printf ("local %s, global %s, redl %d\n", hgp.local_str, hgp.global_str,
   hgp.redl);

  if (hgp.output_level >= HG_DEBUG_LIST)
     if (Zoltan_HG_Info  (&zz, &hg))
        return 1;
  if (Zoltan_HG_Check (&zz, &hg))
     return 1;
  ADD_NEW_TIME(t_load);

  hgp.kway = (!strcasecmp(hgp.local_str, "fmkway")
   ||         !strcasecmp(hgp.local_str, "no")) ? 1 : 0;
  if (!((part) = (int*) calloc ((unsigned)(hg.nVtx), sizeof(int))))
     return 1;

  if (hgp.kway) {
     err = Zoltan_HG_HPart_Lib (&zz, &hg, p, part, &hgp, 0);
     if (err != ZOLTAN_OK)
         return err;
     }
  else {
     /* tighten balance tolerance for recursive bisection process */
     if (p > 2)
        hgp.bal_tol = pow (hgp.bal_tol, 1.0 / ceil (log((double)p) / log(2.0)));

     /* vmap associates original vertices to sub hypergraphs */
     hg.vmap = (int*) ZOLTAN_MALLOC (hg.nVtx * sizeof (int));
     if (hg.vmap == NULL)
       return 1;
     for (i = 0; i < hg.nVtx; i++)
        hg.vmap[i] = i;

     /* partition hypergraph */
     err = Zoltan_HG_rdivide (1, p, (Partition) part, &zz, &hg, &hgp, 0);
     if (err != ZOLTAN_OK)
        return err;
     ZOLTAN_FREE (&hg.vmap);
     }


if (zz.Proc == 0)
{
double subtotal[30];
double total, top;
int cuts, tcuts, temp;

for (i = 0; i < p; i++)
   subtotal[i] = 0.0;
total = 0.0;
for (i = 0; i < hg.nVtx; i++)
   subtotal[part[i]] += ((hg.vwgt == NULL) ? 1.0 : hg.vwgt[i]);
for (i = 0; i < p; i++)
   total += subtotal[i];
top = 0.0;
for (i = 0; i < p; i++)
   {
   subtotal[i] = subtotal[i]/total;
   if (subtotal[i] > top)
      top = subtotal[i];
   }
cuts = (int) hcut_size_links (&zz, &hg, p, part);
tcuts = (int) hcut_size_total (&hg, part);

printf ("RTHRTHp=%d, cuts %5d%c %5d tol %.3f:  ", p, cuts, hgp.orphan_flag ? '*' : ' ', tcuts, top*p);
temp = ((p > 8) ? 8 : p);
for (i = 0; i < temp; i++)
   printf ("%4.2f  ", subtotal[i]);
printf ("\n");
}

  ADD_NEW_TIME(t_part);

  /* partition info */
  if (hgp.output_level >= HG_DEBUG_LIST)
     if (Zoltan_HG_HPart_Info (&zz, &hg, p, part, &hgp))
        return 1;
  free(part);

  /* free hypergraph */
  if (Zoltan_HG_HGraph_Free (&hg))
     return 1;
  if (Zoltan_Memory_Usage (ZOLTAN_MEM_STAT_TOTAL) > 0) {
     printf("ERROR: remaining memory: %d\n",
      Zoltan_Memory_Usage (ZOLTAN_MEM_STAT_TOTAL));
     return 1;
     }

  ADD_NEW_TIME(t_rest);
  END_TIME();
  times_output();

  printf("Final Memory: %d %d  ratio:%f\n",
   Zoltan_Memory_Usage (ZOLTAN_MEM_STAT_TOTAL),
   Zoltan_Memory_Usage (ZOLTAN_MEM_STAT_MAXIMUM),
   (float)Zoltan_Memory_Usage(ZOLTAN_MEM_STAT_MAXIMUM) / memory_graph );

  Zoltan_Memory_Stats();
  return 0;
}


static double hcut_size_links (ZZ *zz, HGraph *hg, int p, Partition part)
{
int i, j, *parts, nparts;
double cut = 0.0;
char *yo = "hcut_size_links";

  if (!(parts = (int*) ZOLTAN_CALLOC (p, sizeof(int)))) {
     ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Insufficient memory.");
     return ZOLTAN_MEMERR;
     }

  for (i = 0; i < hg->nEdge; i++) {
     nparts = 0;
     for (j = hg->hindex[i]; j < hg->hindex[i+1]; j++) {
        if (parts[part[hg->hvertex[j]]] < i + 1)
           nparts++;
        parts[part[hg->hvertex[j]]] = i + 1;
        }
     cut += (nparts-1) * (hg->ewgt ? hg->ewgt[i] : 1.0);
     }
  ZOLTAN_FREE ((void**) &parts);
  return cut;
}

static double hcut_size_total (HGraph *hg, Partition part)
{
int i, j, hpart;
double cut = 0.0;

  for (i = 0; i < hg->nEdge; i++) {
     hpart = part[hg->hvertex[hg->hindex[i]]];
     for (j = hg->hindex[i] + 1; j < hg->hindex[i+1]
      && part[hg->hvertex[j]] == hpart; j++);
         if (j != hg->hindex[i+1])
            cut += (hg->ewgt ? hg->ewgt[i] : 1.0);
     }
  return cut;
}
