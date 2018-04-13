#include "bluetooth_service.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>

static DBusConnection * g_dbus_conn = NULL;
static char g_default_adapter_path[128] = {0};
extern DBusHandlerResult agent_event_filter(DBusConnection *conn,
                                            DBusMessage *msg,
                                            void *data);
const char *get_adapter_path(){
	return g_default_adapter_path;
}

DBusConnection * get_dbus_conn(){
	return g_dbus_conn;
}

// This function is called when the adapter is enabled.
static int setupRemoteAgent(DBusConnection *conn) {
    // Register agent for remote devices. 
    const char *device_agent_path = REMOTE_AGENT_PATH;
    static const DBusObjectPathVTable agent_vtable = {
                 NULL, agent_event_filter, NULL, NULL, NULL, NULL };

    if (!dbus_connection_register_object_path(conn, device_agent_path,
                                              &agent_vtable, NULL)) {
        printf("%s: Can't register object path %s for remote device agent!",
                               __FUNCTION__, device_agent_path);
        return -1;
    }
    return 0;
}

static int tearDownRemoteAgent(DBusConnection *conn) {

    const char *device_agent_path = REMOTE_AGENT_PATH;
    dbus_connection_unregister_object_path (conn, device_agent_path);
    return 0;
}

/*
* bluetooth service init
*/
int initServices(){
	g_dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if(!g_dbus_conn) return -1;
	if(getDefaultAdapter(g_default_adapter_path,sizeof(g_default_adapter_path)) < 0) return -1;
    setupRemoteAgent(g_dbus_conn);
	return 0;
}

/*
* bluetooth service destory
*/
int destoryServices(){
    tearDownRemoteAgent(g_dbus_conn);
	if(g_dbus_conn){
		dbus_connection_unref(g_dbus_conn);
		g_dbus_conn = NULL;
	}
	memset(g_default_adapter_path,0,sizeof(g_default_adapter_path));
	return 0;
}



/*
* get the default adapter path
*/
static int _getDefaultAdapter(DBusConnection *conn,char *path, int len){
	DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    DBusError err;
    int ret = -1;
    const char *reply_path;

    dbus_error_init(&err);
    msg = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC,"/",
					"org.bluez.Manager", "DefaultAdapter");

    if (msg == NULL) {
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, msg);
        }
        goto done;
    }

    reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    if (dbus_error_is_set(&err)) {
         LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, msg);
         goto done;
    }

	if (!dbus_message_get_args(reply, &err,
					DBUS_TYPE_OBJECT_PATH, &reply_path,
					DBUS_TYPE_INVALID)) {
		LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err,msg);
		goto done;
	}

	snprintf(path,len,"%s",reply_path);
    ret = 0;
done:
	if (reply) dbus_message_unref(reply);
    if (msg) dbus_message_unref(msg);
    return ret;
}

int getDefaultAdapter(char *path, int len){
	return _getDefaultAdapter(get_dbus_conn(),path, len);
}
/*
* start bluetooth discovery
*/
static int _startDiscovery(DBusConnection *conn){
	DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    DBusError err;
    int ret = -1;

    dbus_error_init(&err);
    printf("%s-------------%s(%s)\n",__FUNCTION__,get_adapter_path(),DBUS_ADAPTER_IFACE);
    /* Compose the command */
    msg = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC,
                                       get_adapter_path(),
                                       DBUS_ADAPTER_IFACE, "StartDiscovery");

    if (msg == NULL) {
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, msg);
        }
        goto done;
    }

    /* Send the command. */
    reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    if (dbus_error_is_set(&err)) {
         LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, msg);
         goto done;
    }
    printf("%s==========%d\n",__FUNCTION__,ret);
    ret = 0;
done:
    if (reply) dbus_message_unref(reply);
    if (msg) dbus_message_unref(msg);
    return ret;
}

int startDiscovery(){
	return _startDiscovery(get_dbus_conn());
}

/*
* stop bluetooth discovery
*/
static int _stopDiscovery(DBusConnection *conn){
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    DBusError err;
    int ret = -1;

    dbus_error_init(&err);

    /* Compose the command */
    msg = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC,
                                       get_adapter_path(),
                                       DBUS_ADAPTER_IFACE, "StopDiscovery");
    if (msg == NULL) {
        if (dbus_error_is_set(&err))
            LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, msg);
        goto done;
    }

    /* Send the command. */
    reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    if (dbus_error_is_set(&err)) {
        if(strncmp(err.name, BLUEZ_DBUS_BASE_IFC ".Error.NotAuthorized",
                   strlen(BLUEZ_DBUS_BASE_IFC ".Error.NotAuthorized")) == 0) {
            // hcid sends this if there is no active discovery to cancel
            printf("%s: There was no active discovery to cancel", __FUNCTION__);
            dbus_error_free(&err);
        } else {
            LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, msg);
        }
        goto done;
    }

    ret = 0;
done:
    if (msg) dbus_message_unref(msg);
    if (reply) dbus_message_unref(reply);
    return ret;
}

int stopDiscovery(){
	return _stopDiscovery(get_dbus_conn());
}

/*
*paired to the remote headset or other device
*/
void onCreatePairedDeviceResult(DBusMessage *msg, void *user, void *n) {
    DBusError err;
    dbus_error_init(&err);

    printf("%s (%s)\n",__FUNCTION__,(char*) user);
    if (dbus_set_error_from_message(&err, msg)) {
        LOG_AND_FREE_DBUS_ERROR(&err);
    }
    if(user) free(user);
}

static int _createPairedDevice(DBusConnection *conn,const char * addr, int timeout_ms){
	int len = strlen(addr) + 1;
	char * context_path = (char*)calloc(len,sizeof(char));
	const char *capabilities = "DisplayYesNo";
	const char *agent_path = REMOTE_AGENT_PATH;
    snprintf(context_path,len,"%s",addr);
	int ret = dbus_func_args_async(conn, (int)timeout_ms,
                                        onCreatePairedDeviceResult, // callback
                                        context_path,
                                        NULL,
                                        get_adapter_path(),
                                        DBUS_ADAPTER_IFACE,
                                        "CreatePairedDevice",
                                        DBUS_TYPE_STRING, &addr,
                                        DBUS_TYPE_OBJECT_PATH, &agent_path,
                                        DBUS_TYPE_STRING, &capabilities,
                                        DBUS_TYPE_INVALID);

    return ret ? 0 : -1;
}

int createPairedDevice(const char * addr, int timeout){
	return _createPairedDevice(get_dbus_conn(),addr,timeout);
}

/*
* discover the specify service profile (local or remote)
*/
static void onDiscoverServicesResult(DBusMessage *msg, void *user, void *n) {
	DBusError err;
    dbus_error_init(&err);

	printf("%s (%s)\n",__FUNCTION__,(char*) user);
	if (dbus_set_error_from_message(&err, msg)) {
        LOG_AND_FREE_DBUS_ERROR(&err);
    }
	if(user) free(user);
}

static int _discoverServices(DBusConnection *conn,const char * path, const char * pattern){
	int len = strlen(path) + 1;
	char * context_path = (char*)calloc(len,sizeof(char));
	snprintf(context_path,len,"%s",path);
	int ret = dbus_func_args_async(conn, -1,
                                        onDiscoverServicesResult,//callback
                                        context_path,
                                        NULL,
                                        path,
                                        DBUS_DEVICE_IFACE,
                                        "DiscoverServices",
                                        DBUS_TYPE_STRING, &pattern,
                                        DBUS_TYPE_INVALID);
	return ret ? 0 : -1;
}

int discoverServices(const char * path, const char * pattern){
	return _discoverServices(get_dbus_conn(),path,pattern);
}

/*
*create device
*/
static void onCreateDeviceResult(DBusMessage *msg, void *user, void *n) {
	DBusError err;
    dbus_error_init(&err);
    int result = 0;

	printf("%s (%s)\n",__FUNCTION__,(char*) user);
	if (dbus_set_error_from_message(&err, msg)) {
		if (dbus_error_has_name(&err, "org.bluez.Error.AlreadyExists")) {
            result = 1;
        } else {
            result = -1;
        }
        LOG_AND_FREE_DBUS_ERROR(&err);
    }
	if(user) free(user);
}

int _createDevice(DBusConnection *conn,const char * addr){
	char *context_address = (char *)calloc(BTADDR_SIZE, sizeof(char));
    snprintf(context_address, BTADDR_SIZE,"%s",addr);  // for callback
    int ret = dbus_func_args_async(conn, -1,
                                        onCreateDeviceResult,
                                        context_address,
                                        NULL,
                                        get_adapter_path(),
                                        DBUS_ADAPTER_IFACE,
                                        "CreateDevice",
                                        DBUS_TYPE_STRING, &addr,
                                        DBUS_TYPE_INVALID);
     return ret ? 0 : -1;
}

int createDevice(const char * addr){
	return _createDevice(get_dbus_conn(),addr);
}


/*
* get device properties
*/
static int _getDeviceProperties(DBusConnection *conn,const char *path,t_property_value_array *array)
{
    DBusMessage *reply;
    DBusError err;
    dbus_error_init(&err);

    reply = dbus_func_args_timeout(conn, -1, path,
                               DBUS_DEVICE_IFACE, "GetProperties",
                               DBUS_TYPE_INVALID);

    if (!reply) {
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR(&err);
        } else{
        	printf("DBus reply is NULL in function %s", __FUNCTION__);
        }
        return -1;
    }

    DBusMessageIter iter;
    if (dbus_message_iter_init(reply, &iter))
       parse_remote_device_properties(&iter,array);
    dbus_message_unref(reply);

    return 0;
}

int getDeviceProperties(const char *path,t_property_value_array *array){
	return _getDeviceProperties(get_dbus_conn(),path,array);
}

/*
* get adapter properties
*/
static int _getAdapterProperties(DBusConnection *conn,t_property_value_array *array)
{
    DBusMessage *reply;
    DBusError err;
    dbus_error_init(&err);

    reply = dbus_func_args_timeout(conn, -1, get_adapter_path(),
                               DBUS_ADAPTER_IFACE, "GetProperties",
                               DBUS_TYPE_INVALID);
    if (!reply) {
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR(&err);
        } else
            printf("DBus reply is NULL in function %s", __FUNCTION__);
        return -1;
    }

    DBusMessageIter iter;
    if (dbus_message_iter_init(reply, &iter))
        parse_adapter_properties(&iter,array);
    dbus_message_unref(reply);

    return 0;
}

int getAdapterProperties(t_property_value_array *array){
	return _getAdapterProperties(get_dbus_conn(),array);
}

/*
* set device propetry
*/
int _setDeviceProperty(DBusConnection *conn,const char * path,const char *key, void * value,int type){
	DBusMessage *msg;
    DBusMessageIter iter;
    dbus_bool_t reply = FALSE;
    msg = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC,
                                      path, DBUS_DEVICE_IFACE, "SetProperty");
    if (!msg) {
        printf("%s: Can't allocate new method call for device SetProperty!", __FUNCTION__);
        return -1;
    }

    dbus_message_append_args(msg, DBUS_TYPE_STRING, &key, DBUS_TYPE_INVALID);
    dbus_message_iter_init_append(msg, &iter);
    append_variant(&iter, type, value);

    // Asynchronous call - the callbacks come via Device propertyChange
    reply = dbus_connection_send_with_reply(conn, msg, NULL, -1);
    dbus_message_unref(msg);
    return 0;
}

int setDevicePropertyBoolean(const char * path,const char *key, int value,int type){
	return _setDeviceProperty(get_dbus_conn(),path,key,(void*)&value,DBUS_TYPE_BOOLEAN);
}

int setDevicePropertyString(const char * path,const char *key, const char * value,int type){
	return _setDeviceProperty(get_dbus_conn(),path,key,(void*)&value,DBUS_TYPE_STRING);
}

/*
* set adapter propetry
*/
int _setAdapterProperty(DBusConnection *conn,const char *key, void * value,int type){
	DBusMessage *msg;
    DBusMessageIter iter;
    dbus_bool_t reply = FALSE;
    msg = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC,
                                      get_adapter_path(), DBUS_ADAPTER_IFACE, "SetProperty");
    if (!msg) {
        printf("%s: Can't allocate new method call for adapter SetProperty!", __FUNCTION__);
        return -1;
    }

    dbus_message_append_args(msg, DBUS_TYPE_STRING, &key, DBUS_TYPE_INVALID);
    dbus_message_iter_init_append(msg, &iter);
    append_variant(&iter, type, value);

    // Asynchronous call - the callbacks come via Device propertyChange
    reply = dbus_connection_send_with_reply(conn, msg, NULL, -1);
    dbus_message_unref(msg);
    return 0;

}

int setAdapterPropertyBoolean(const char *key, int value,int type){
	return _setAdapterProperty(get_dbus_conn(),key,(void*)&value,DBUS_TYPE_BOOLEAN);
}

int setAdapterPropertyString(const char *key, const char * value,int type){
	return _setAdapterProperty(get_dbus_conn(),key,(void*)&value,DBUS_TYPE_STRING);
}

int setAdapterPropertyInteger(const char *key, int value,int type){
	return _setAdapterProperty(get_dbus_conn(),key,(void*)&value,DBUS_TYPE_UINT32);
}