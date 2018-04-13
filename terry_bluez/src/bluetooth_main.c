#include "bluetooth_common.h"
#include "bluetooth_service.h"
#include "bluetooth_event.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>

int main(int argc, char *argv[]){
	char cmd[64] = {0};
	initServices();
	initializeBluetoothEvent();
	startEventLoop();
	while(1){
		printf("input cmd:\n");
		memset(cmd,0,64);
		scanf("%s",cmd);
		if(strstr(cmd,"quit")){
			break;
		}else if(strstr(cmd,"startDis")){
			printf("\nstartDiscovery\n");
			startDiscovery();
		}else if(strstr(cmd,"stopDis")){
			printf("\nstopDiscovery\n");
			stopDiscovery();
		}else if(strstr(cmd,"pair")){
			printf("\npair\n");
		}else{
			printf("\n%s\n",cmd);
		}
	}
	stopEventLoop();
	cleanupBluetoothEvent();
	destoryServices();
	return 0;
}