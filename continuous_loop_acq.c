// simple_loop.c
//
// Acquire's data from the RedPitaya Channel A in a continuous
// loop. The user may specify on the command line the desired
// sampling rate (decimation).
//
// Send SIGINT (CTRL+C) to quit safely. 
//
// PL  v0.2 (5-8-14)

#define _BSD_SOURCE // Needed for usleep() to work
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stddef.h>
#include <sys/param.h>
#include <time.h>
#include "fpga_osc.h"

//RedPitaya's sample buffer length
const int BUF = 16*1024;

// Output step factor. If it's, e.g., 2, then the
// output will skip every 2nd sample, etc. Set to
// 1 to keep all samples in a trace.
const int step = 1;

// This must be of type `volatile` to prevent
// the compiler from optimizing away the
// while loop condition.
volatile sig_atomic_t stop;

// Called when we recieve SIGINT (CTRL+C)
// which then stops the infinite loop in main().
void inthand(int signum) 
{
   stop=1;
}

int main(int argc, char *argv[]) 
{

   signal(SIGINT, inthand);

   int start = osc_fpga_init();

   if(start) 
      {
      printf("osc_fpga_init() didn't work, retval = %d",start);
      return -1;
      }

   // Get desired decimation from command line. Make sure it's valid.
   int decimation;
   if(argc > 1) 
      {
         int t = atoi(argv[1]);
         if(t == 1 || t== 8 || t == 64 || t == 1024 || t == 8192 || t == 65536)
            decimation = t;
         else 
            {
               printf("Invalid decimation %d; set to 1.\n"         \
                     "Valid decimations are {1, 8, 64, 1024, 8192, 65536}\n");
               usleep(1e6);
               decimation = 1; 
            }
      }
   else
      decimation = 1;

   // Length of time after the trigger the FPGA should write
   float after_trigger = OSC_FPGA_SIG_LEN * c_osc_fpga_smpl_period * decimation;

   // Convert that time value into some number of samples
    uint32_t fpga_delay = osc_fpga_cnv_time_to_smpls(after_trigger, decimation);

   osc_fpga_set_trigger_delay(fpga_delay);

   g_osc_fpga_reg_mem->data_dec = decimation; // set decimation level
   g_osc_fpga_reg_mem->other = 0;             // disable averaging at decimation
step

   osc_fpga_reset();

   int * cha_signal;   // pointer to channel A's signal buffer
   int * chb_signal;   // don't actually care about this yet
   int trig_ptr;       // index in sig. buffer where trigger occurs 
   int curr_ptr;       // where current write pointer is (not used)
   int idx;
   osc_fpga_get_sig_ptr(&cha_signal, &chb_signal);
   osc_fpga_get_wr_ptr(&curr_ptr,&trig_ptr);

   while(!stop)
      {   
         osc_fpga_arm_trigger();       // Start acquiring
         osc_fpga_set_trigger(0x1);       // Trigger immediately

         osc_fpga_get_wr_ptr(&curr_ptr,&trig_ptr);

         // wait until done acquiring (trig_source becomes 0)
         while(g_osc_fpga_reg_mem->trig_source);

         // for sampling rate ~950Khz at max decimation
         // use step = 2
         for (int i=0; i < (BUF-1); i=i+step) 
            {
               idx = (trig_ptr+i)%BUF;
               printf("%e\n",osc_fpga_cnv_cnt_to_v(cha_signal[idx]));
            }
      }

   osc_fpga_exit();
   return 0;
}
