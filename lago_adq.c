
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 			
#include "redpitaya/rp.h"
#include "math.h"
#include <inttypes.h>

#define VERSION "0.1"


int main (int argc, char **argv) {
	
	/* Syntax analysis */
	if (argc != 5)
	{
		fprintf(stderr,"\tError: Incorrect syntax.\n Syntax:  LD_LIBRARY_PATH=/opt/redpitaya/lib ./%s [Filename] [Trigger Level] [Measure time] [HV 0] \nExample: LD_LIBRARY_PATH=/opt/redpitaya/lib ./%s data.dat -0.2 10 1.2 \n",argv[0],argv[0]);
		return EXIT_FAILURE;
	}
	
	if (atof(argv[4]) < 0 || atof(argv[4]) > 2.5)
	{
		fprintf(stderr,"\tError: Incorrect input value.\n HV must satisfy the condition 0 [V] <= HV <= 2.5 [V]");
		return EXIT_FAILURE;
	}
	
	/* Definition of the input parameters */
	float trigger_level = atof(argv[2]);
	float measure_time = atof(argv[3]);
	float HV = atof(argv[4]);
	
	
	
	/* General information */
	fprintf(stdout,"\t%s version %s\n",argv[0],VERSION);
	fprintf(stdout,"\tThe HV value sent is %f (V)\n",1.4*HV);
	   
    /* Open file and test for error */
    FILE *fd;
    fd = fopen(argv[1], "wb");
    if (fd == NULL) 
	{ 
    	fprintf(stderr, "\tError: Unable to input file %s \n", argv[1]);
    	return EXIT_FAILURE;
	}
    
    /* Create buffer */
    uint32_t buff_size = 16384;
    float *buff = (float *)malloc(buff_size * sizeof(float));
    
    /* Initialization of the RP */
    if(rp_Init() != RP_OK)
	{
        fprintf(stderr, "\tError: Rp api init failed!\n");
        return EXIT_FAILURE;
    }
    
	/* HV setting */
	rp_AOpinReset();
	float step = 0.001;
	float base = 0;
	float tiempo = 5;
	
	/* Definition of the base value (may be ommited) */
	if(rp_AOpinGetValue(0, &base) != RP_OK)
	{
        fprintf(stderr, "\tError: HV base value establishment failed\n");
        return EXIT_FAILURE;
    }
	
	/* 0 to HV setting */ 
	sleep(1);
	float HV_points =  (HV-base)/step;
	float period = tiempo/HV_points;
	int i = 0;
	float ramp = base;
	fprintf(stderr, "\t0 to HV value establishment started\n");
	for (i=0; i<HV_points; i++)
	{
		ramp = ramp + step;
		if (rp_AOpinSetValue(0, ramp) != RP_OK) 
       	{
   			fprintf(stderr, "\tError: 0 to HV value establishment failed\n");
        	return EXIT_FAILURE;
		}
		usleep(period*pow(10.0,6));	
	}
		
	/* HV corrovoration */ 
	float HV_value = 0;
	if(rp_AOpinGetValue(0, &HV_value) != RP_OK)
	{
        fprintf(stderr, "\tError: HV value corrovoration failed\n");
        return EXIT_FAILURE;
    }
    
    float HV_value_input = 0;
    if(rp_AIpinGetValue(0, &HV_value_input) != RP_OK)
	{
        fprintf(stderr, "\tError: HV value corrovoration failed\n");
        return EXIT_FAILURE;
    }
	fprintf(stdout, "\tEstablished HV value: %f\n", HV_value_input);
	
	
	/*** Start of the data adquisition process ***/
	
	fprintf(stdout, "\tTo start the data adquisition process, press ENTER");
	getchar();
	
	/* Definition of the acquire parameters */
	rp_acq_decimation_t acq_decimation = RP_DEC_1;
	float ADC_trigger_level = -0.01;
	int32_t trigger_delay = (int32_t)(buff_size-16384);
	
	/* Reset of the acquire writing state machine */
	if(rp_AcqReset() != RP_OK)
	{
        fprintf(stderr, "\tError: Reset of the acquire writing state machine failed\n");
        return EXIT_FAILURE;
    }
	
	/* Set of the adquisition decimation */
	if(rp_AcqSetDecimation(acq_decimation) != RP_OK)
	{
        fprintf(stderr, "\tError: Set of the adquisition decimation failed\n");
        return EXIT_FAILURE;
    }
    
    /* Set of the acquisition trigger value */
    if(rp_AcqSetTriggerLevel(ADC_trigger_level) != RP_OK)
	{
        fprintf(stderr, "\tError: Set of the adquisition trigger value failed\n");
        return EXIT_FAILURE;
    }
    
    /* Set of the ADC buffer length */
    if(rp_AcqSetTriggerDelay(trigger_delay) != RP_OK)
	{
        fprintf(stderr, "\tError: Set of the adquisition trigger length failed\n");
        return EXIT_FAILURE;
    }
    
	//struct timespec tim;
	//tim.tv_nsec = impulse_period*pow(10,9);
	
	/* Start of the adquisition loop*/
	clock_t start, end;
    volatile double elapsed;
    volatile double elapsed_flag;
    int elapsed_counter = 0;
    start = clock();
    
    if(rp_AcqStart() != RP_OK)
	{
       	fprintf(stderr, "\tError: Start of the Acquisition failed\n");
       	return EXIT_FAILURE;
    }
    
    usleep(131.07);
    
    while(1)
    {
    	/* Set of the adquisition trigger source */
    	if(rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHB_NE) != RP_OK)
		{
        	fprintf(stderr, "\tError: Set of the adquisition trigger source failed\n");
        	return EXIT_FAILURE;
    	}
    	
    	rp_acq_trig_state_t state = RP_TRIG_STATE_WAITING;
    	
	    while(1)
		{	
			end = clock();
        	elapsed_flag = ((double) (end-start)) / (double) CLOCKS_PER_SEC;
        	elapsed = 2130*elapsed_counter + elapsed_flag;
        	
        	if (elapsed_flag>=2130)
        	{
        		start = clock();
        		elapsed_counter = elapsed_counter + 1;
			}
						
			if(elapsed >= measure_time)
        	{
            	goto timeover;
        	}
        	
            rp_AcqGetTriggerState(&state);
            
        	if(state == RP_TRIG_STATE_TRIGGERED)
			{
            	usleep(65.5);
            	break;
        	}
    	}
        	
		if(rp_AcqGetOldestDataV(RP_CH_2, &buff_size, buff) != RP_OK)
		{
	    	fprintf(stderr, "\tError: Buffer filling failed\n");
	    	return EXIT_FAILURE;
	    }
	    	
        bool trigger_reached = 0;
        
		for(i = 0; i < buff_size; i++)
		{
			if (buff[i] <= trigger_level)
			{
			trigger_reached = 1;
			break;
			}
		}
			
		if (trigger_reached == 1)
		{
			fprintf(stdout,"\tElapsed time:%f\n",elapsed);
			for(i = 0; i < buff_size; i++)
			{
				int half_diff=20;
				int sup_limit=16384;
				int inf_limit=0;
					
				if (i-half_diff>inf_limit)
				{
					inf_limit=i-half_diff;
				}
					
				if (sup_limit-i>half_diff)
				{
					sup_limit=i+half_diff;
				}
					
				int j=0;
									
				for (j=inf_limit;j<sup_limit;j++)
				{
					if (buff[j]<=trigger_level)
					{
						fprintf(fd,"%f\n", buff[i]);
						break;
					}	
				}
			}
		}      	
	}
    
	timeover:
	fprintf(stdout, "\tThe data adquisition process is over\n");
	/*** End of the data adquisition process ***/
	
	/* HV to 0 setting */ 
	sleep(1);
	i = 0;
	ramp = HV_value;
	fprintf(stderr, "\tHV to 0 value establishement started\n");
	
	for (i=0; i<HV_points; i++)
	{
		ramp = ramp - step;
		
		if (rp_AOpinSetValue(0, ramp) != RP_OK) 
       	{
   			fprintf(stderr, "\tError: HV to 0 value establishment failed\n");
        	return EXIT_FAILURE;
		}
		
		usleep(period*pow(10.0,6));		
	}
	
	/* Final HV corrovoration */
	float HV_final_value = 0;
	
	if(rp_AIpinGetValue(0, &HV_final_value) != RP_OK)
	{
        fprintf(stderr, "\tError: final HV value corrovoration failed\n");
        return EXIT_FAILURE;
    }
    
	fprintf(stdout, "\tEstablished final HV value: %f \n", HV_final_value);
	
    /* Close of the file */
    fclose(fd);
    
    /* Reset of the RP
    if(rp_Reset() != RP_OK)
	{
        fprintf(stderr, "Error: Rp api res failed!\n");
        return EXIT_FAILURE;
    }
    */
    
	/* Release of the RP */
    if(rp_Release() != RP_OK)
	{
        fprintf(stderr, "\tError: Rp api release failed!\n");
        return EXIT_FAILURE;
    }
    
    free(buff);
    fprintf(stdout, "\tThe application has finished working properly\n");
    return EXIT_SUCCESS;
}



