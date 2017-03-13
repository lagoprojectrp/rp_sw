#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "redpitaya/rp.h"
int main(int argc, char **argv){

				if ( argc < 6 ) {

								printf("Usage n_pulses(1-3) ampl(V) delta_t(ns) tau(ns) rate(Hz) \n");
								if (rp_Init() != RP_OK){
												fprintf(stderr, "Rp api init failed!\n");
								}
								rp_GenOutDisable(RP_CH_1);

								return -1;
				}
				/* Number of sub-pulses  argument parsing */
				int n =  atoi(argv[1]);
				if ( (n < 0) || (n > 3) ) {
								fprintf(stderr, "Invalid number of sub-pulses: %s\n", argv[1]);
								return -1;
				}

				/* Signal amplitude argument parsing */
				double ampl = strtod(argv[2], NULL);
				if ( (ampl < 0.0) || (ampl > 1.0) ) {
								fprintf(stderr, "Invalid amplitude: %s\n", argv[2]);
								return -1;
				}

				/* Signal delta_time argument parsing */
				double delta_t = (int)strtod(argv[3], NULL);

				/* Signal decay time argument parsing */
				double tau = strtod(argv[4], NULL);

				/* Signal rate argument parsing */
				double rate = strtod(argv[5], NULL);

				/* Print error, if rp_Init() function failed */
				if (rp_Init() != RP_OK){
								fprintf(stderr, "Rp api init failed!\n");
				}
				int buff_size = 16384; //this is the total number of bins
				int buff_zero = 0;
				double T;
				double freq;
				double step;
				int i1,i2,i3,i4,i5;
				T = 1e9/rate; //in ns
				step = T/(double)(buff_size-buff_zero) ; //  ns/bin
				if (n==1) {
								i1 = buff_size;
				} else if (n==2){
								i1 = (int)((delta_t)/step);
								i2 = (int)((delta_t+tau)/step);
								i3 = buff_size;
				} else if (n==3){
								i1 = (int)((delta_t)/step);
								i2 = (int)((delta_t+tau)/step);
								i3 = (int)((2*delta_t)/step);
								i4 = (int)((2*delta_t + tau)/step);
								i5 = (int)((2*delta_t + 2*tau)/step);
				}
				freq = rate; // in Hz

				fprintf(stderr,"SUMMARY:  \n");
				fprintf(stderr,"A pulse will be generated composed of %d sub-pulse(s). \n", n);
				if (n > 1) 
								fprintf(stderr,"The sub-pulses will be separated by %.0f ns. \n", delta_t);
				fprintf(stderr,"Each sub-pulse will have a decay time of %.0f ns \n", tau);
				fprintf(stderr,"accomodated in a buffer of %d bins with a step of %.2f ns/bin, \n",16384-buff_zero,step);
				fprintf(stderr,"and a burst repetition rate of %.1f Hz \n", rate);
				float *x = (float *)malloc(buff_size * sizeof(float));

				for (int i = 0; i < buff_zero; ++i){
								// avoid flaw of RP AWG module
								x[i] = 0;
				}
				//fprintf(stderr,"i1 = %d i2= %d i3=%d i4=%d i5=%d ns \n", i1,i2,i3,i4,i5);


				for (int i = 0; i < buff_size-buff_zero; ++i){

								if (i < i1 )                             // first pulse
												x[i+buff_zero] = -1.0*exp(-(i*step/tau));

								else if ( i >= i1 && i < i2 && n >1)    // first subpulse of second pulse
												x[i+buff_zero] = -1.0*exp(-(i-i1)*step/tau);

								else if ( i >= i2 && i < i3 && n > 1 )  // second subpulse of second pulse
												x[i+buff_zero] = -1.0*exp(-(i-i2)*step/tau);

								else if ( i > i3 && i <= i4 && n > 2)            // first subpulse of third pulse
												x[i+buff_zero] = -1.0*exp(-(i-i3)*step/tau);

								else if ( i > i4 && i <= i5 && n > 2)           // second subpulse of third pulse
												x[i+buff_zero] = -1.0*exp(-(i-i4)*step/tau);

								else if ( i> i5 && n > 2)                       // third subpulse of third pulse
												x[i+buff_zero] = -1.0*exp(-(i-i5)*step/tau);

								else
												x[i+buff_zero] = 0;
				}
				for (int i = 0; i<buff_size;i++){
								//printf("%f\n", x[i]);
				}

				rp_GenArbWaveform(RP_CH_1, x, buff_size);
				rp_GenWaveform(RP_CH_1, RP_WAVEFORM_ARBITRARY);
				rp_GenFreq(RP_CH_1, freq); // whole buffer corresponds to 10 us
				rp_GenAmp(RP_CH_1, ampl);
				rp_GenOutEnable(RP_CH_1);
				rp_Release();
}
