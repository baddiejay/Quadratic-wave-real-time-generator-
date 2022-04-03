#include <linux/module.h>
#include <asm/io.h>
#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_shm.h>
#include "parametri.h"

//Descriptors of the 3 generator and recognizer tasks
static RT_TASK thread[NTASKS];
static RT_TASK rico;

//Pointer to the SHM
static struct data_str *data;

//Timing variables
static RTIME tick_period1;
static RTIME tick_period2;

//Code that is produced by the generators and read by the recognizer
static int codice[3];

//Semi-period and phase entered as parameters by the user
static int semiperiodi[3] = { 10, 20, 40 };
static int arr_argc = 0;
module_param_array(semiperiodi, int, &arr_argc, 0000);
MODULE_PARM_DESC(semiperiodi, "Semiperiodi in input dall'utente");


static int fasi[3] = { 0, 1, 2 };
static int arr_argc2 = 0;
module_param_array(fasi, int, &arr_argc2, 0000);
MODULE_PARM_DESC(fasi, "fasi in input dall'utente");

static int seq[4] = { 0, 3, 6, 5 };
static int arr_argc3 = 0;
module_param_array(seq, int, &arr_argc3, 0000);
MODULE_PARM_DESC(seq, "Sequenza in input dall'utente");

//Conversion of code vector to decimal 
static int conversione(int v []){
	int i,d = 0;

	for(i = 2; i >= 0; i--)
		d=d*2+v[i];

	return d;
}

/* GENERATOR: Each period generates a semi-period of the square wave by switching the bits from zero to one or vice versa
and prints on the screen the task followed by the generated bit.
The generated bit is inserted in the position of the task in the global vector. */

static void generatore(long t)
{
	int count = 0;
	int val = 0;

	while(1) {
		if((count % 10)==0){
			val = val + 1;
			val = val % 2;
		}
        //Ogni task t posizione il suo bit prodotto nel vettore globale codice
		codice[t] = val;
		rt_printk("VAL %d %d\n", t, val); 
		rt_task_wait_period();
	}
}

/* RECOGNIZER: Its task is to recognize the sequence by converting the generating values (present in the global vector) into decimal. 
It does this by invoking a conversion function after which it compares. If the comparison is true it continues to compare
the other digits of the sequence otherwise it suspends. */

static void riconoscitore(long t){
	int trovato = 0;

	while(1){
		data->ok = 0;
		switch(trovato){
			case 0: 
                if(conversione(codice) == seq[0])
					trovato++;
				else
					break;
			case 1: 
				if(conversione(codice) == seq[1])
					trovato++;
				else
					break;

			case 2: 
				if(conversione(codice) == seq[2])
					trovato++;
				else
					break;
			case 3: 
				if(conversione(codice) == seq[3]){
					data->ok = 1;
					data->count++;
				}
		}
		rt_task_wait_period();
	}
}

//Body of the module
int init_module(void){
	RTIME now, now2;
	int i;

	data = rtai_kmalloc(SHMNAM, sizeof(struct data_str));
	data->count = 0;
	//inizializzazione dei task 
	for (i = 0; i < NTASKS; i++) {
		rt_task_init_cpuid(&thread[i], generatore, i, STACK_SIZE, NTASKS - i - 1, 0, 0, 0);
	}
	rt_task_init_cpuid(&rico, riconoscitore, 1, STACK_SIZE, 2, 0, 0, 0);

	tick_period1 = nano2count(TICK_PERIOD1);
	tick_period2 = nano2count(TICK_PERIOD2);
	for (i = 0; i < NTASKS; i++) {
		now = rt_get_time() + fasi[i]*tick_period2;
		rt_task_make_periodic(&thread[i], now, semiperiodi[i]*tick_period1); 	
	}

	now2 = now + nano2count(5000);
	rt_task_make_periodic(&rico, now2, tick_period2);
    
    	//Schedulation with RM
	rt_spv_RMS(0);
	return 0;
}


void cleanup_module(void){
	int i;

	for (i = 0; i < NTASKS; i++) {
		rt_task_delete(&thread[i]);
	}
	rt_task_delete(&rico);
	rtai_kfree(SHMNAM);
	return;
}


