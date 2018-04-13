#ifndef __BLUETOOTH_EVENT_H
#define __BLUETOOTH_EVENT_H
#include "bluetooth_common.h"

void initializeBluetoothEvent();
void cleanupBluetoothEvent();
int startEventLoop();
void stopEventLoop();
int isEventLoopRunning();

#endif