#ifndef __BLUETOOTH_SERVICE_H
#define __BLUETOOTH_SERVICE_H

#include "bluetooth_common.h"

#define REMOTE_AGENT_PATH "/terry/bluetooth/remote_device_agent"
#define LOCAL_AGENT_PATH "/terry/bluetooth/agent"
#define DBUS_ADAPTER_IFACE BLUEZ_DBUS_BASE_IFC ".Adapter"
#define DBUS_DEVICE_IFACE BLUEZ_DBUS_BASE_IFC ".Device"

/*
* base headset operation:
* 1) startDiscovery
* 2) choose one headset and createDevice & createPairedDevice
* 3) discoverServices to get the rfcomm channel.
* 4) rfcomm connect (service level connection)
* 5) sco sconnect
*/

/*
* bluetooth service init
*/
int initServices();

/*
* bluetooth service destory
*/
int destoryServices();
/*
* get the dbus connection point
*/
DBusConnection * get_dbus_conn();
/*
* get the default adapter path
*/
const char *get_adapter_path();
/*
* get the default adapter path
*/
int getDefaultAdapter(char *path, int len);
/*
* start bluetooth discovery
*/
int startDiscovery();
/*
* stop bluetooth discovery
*/
int stopDiscovery();
/*
* discover the specify service profile (local or remote)
*/
int discoverServices(const char * path, const char * pattern);
/*
*paired to the remote headset or other device
*/
int createPairedDevice(const char * addr, int timeout);

/*
* create device
*/
int createDevice(const char * addr);

/*
* get device properties
*/
int getDeviceProperties(const char *path,t_property_value_array *array);

/*
* get adapter properties
*/
int getAdapterProperties(t_property_value_array *array);

/*
* set device property
*/
int setDevicePropertyBoolean(const char * path,const char *key, int value,int type);
int setDevicePropertyString(const char * path,const char *key, const char * value,int type);
/*
* set Adapter property
*/
int setAdapterPropertyBoolean(const char *key, int value,int type);
int setAdapterPropertyString(const char *key, const char * value,int type);
int setAdapterPropertyInteger(const char *key, int value,int type);
#endif
