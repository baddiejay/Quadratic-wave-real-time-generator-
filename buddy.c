/* USER-LEVEL TASK BUDDY FOR PRINTING RECOGNITION RESULTS ON SCREEN */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "/usr/realtime/include/rtai_shm.h"
#include "parametri.h"

static int end;
static void endme(int dummy) { end=1; }

int main (void){

    struct data_str *data;

    signal(SIGINT, endme);
    data = rtai_malloc (SHMNAM,1);
    while (!end) {
        printf(" OK : %d Count : %d \n", data->ok, data->count);
	usleep(10000);
    }
    rtai_free (SHMNAM, &data);

    return 0;
}
