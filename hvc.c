/* Programa para probar la tarjeta LEO + RP 2016 para tomar datos del PMT Auger
Marco Ramos 2016
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <iostream>
//#include <fstream>  
#include "redpitaya/rp.h"

int main (int argc, char **argv) {				// argc es la señal de control que ingresara como argumento
    float value[2];						//voltaje de control arreglo dim 2 por 2 canales
    float readout[2];						//voltaje de lectura del Vreadout del PMT
    uint32_t buff_size = 16384;					//declaracion del tamaño del buffer de la RP
    float *buff = (float *)malloc(buff_size * sizeof(float));   //reserva de memoria para el buffer
    int i;
    int status;

    for (i=0; i<2; i++) {					//Vamos a usar 2 canales por lo tanto usaremos la salida A0 y A1
        if (argc > (1+i)) {					
            value [i] = atof(argv[1+i]);
        } else {
            value [i] =0.0;				
        }
        printf("Voltage setting for AO[%i] = %1.1fV\n", i, value [i]);
    }

    // Initialization of API					
    if (rp_Init() != RP_OK) {
        fprintf(stderr, "Red Pitaya API init failed!\n");
        return EXIT_FAILURE;
    }

    // Setting a voltage for each ananlog output
    for (i=0; i<2; i++) {
        status = rp_AOpinSetValue(i, value[i]);
        if (status != RP_OK) {
            printf("Could not set AO[%i] voltage.\n", i);
        }
    }

    for (i=0; i<2; i++) {						//lectura del voltaje de readout del PMT
        rp_AIpinGetValue(i, &readout[i]);				
        printf("Measured voltage on AI[%i] = %1.2fV\n", i, readout[i]);
    }

/* Rutina logica de retroalimentacion del control del HV */


        rp_AcqReset();							//Preparar para obtener datos
        rp_AcqSetDecimation(1);
        rp_AcqSetTriggerLevel(0.1); //Trig level is set in Volts while in SCPI 
        rp_AcqSetTriggerDelay(0);

        rp_AcqStart();

        /* After acquisition is started some time delay is needed in order to acquire fresh samples in to buffer*/
        /* Here we have used time delay of one second but you can calculate exact value taking in to account buffer*/
        /*length and smaling rate*/

        sleep(1);

        rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);
        rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;

        while(1){
                rp_AcqGetTriggerState(&state);
                if(state == RP_TRIG_STATE_TRIGGERED){
                sleep(1);
                break;
                }
        }

        rp_AcqGetOldestDataV(RP_CH_1, &buff_size, buff);		//volcado de datos a la memoria

        for(i = 0; i < buff_size; i++){
                printf("%f\n", buff[i]);
        }
        
      /*  std::ofstream outfile ("RPCH1.cvs");				//creacion de archivo externo

        outfile << buff << std::endl;

        outfile.close();*/

    // wait for user input
    getchar();

    for (i=0; i<4; i++) {					//retorno a 0 de los voltajes de control delPMT
        status = rp_AOpinSetValue(i, 0);
        if (status != RP_OK) {
            printf("Could not set AO[%i] voltage.\n", i);
        }
    }

    // Releasing resources
    free(buff);
    rp_Release();

    return EXIT_SUCCESS;
}
