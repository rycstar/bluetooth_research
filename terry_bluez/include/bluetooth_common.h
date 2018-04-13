#ifndef __BLUETOOTH_COMMON_H
#define __BLUETOOTH_COMMON_H

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/poll.h>

#include <dbus/dbus.h>
#include <bluetooth/bluetooth.h>

#define BLUEZ_DBUS_BASE_PATH      "/org/bluez"
#define BLUEZ_DBUS_BASE_IFC       "org.bluez"
#define BLUEZ_ERROR_IFC           "org.bluez.Error"

// It would be nicer to retrieve this from bluez using GetDefaultAdapter,
// but this is only possible when the adapter is up (and hcid is running).
// It is much easier just to hardcode bluetooth adapter to hci0
#define BLUETOOTH_ADAPTER_HCI_NUM 0
#define BLUEZ_ADAPTER_OBJECT_NAME BLUEZ_DBUS_BASE_PATH "/hci0"

#define BTADDR_SIZE 18   // size of BT address character array (including null)
#define DEFAULT_INITIAL_POLLFD_COUNT 8

#define LOG_AND_FREE_DBUS_ERROR_WITH_MSG(err, msg) \
    {   printf("%s: D-Bus error in %s: %s (%s)", __FUNCTION__, \
        dbus_message_get_member((msg)), (err)->name, (err)->message); \
         dbus_error_free((err)); }
#define LOG_AND_FREE_DBUS_ERROR(err) \
    {   printf("%s: D-Bus error: %s (%s)", __FUNCTION__, \
        (err)->name, (err)->message); \
        dbus_error_free((err)); }

typedef struct _properties{
    char name[32];
    int  type;
}Properties;

typedef union {
    char *str_val;
    int int_val;
    char **array_val;
} u_property_value;

typedef struct{
	char name[32];
	int len;/*this len is the length of u_property_value.array_val*/
	int type;/*the data type in dbus*/
	u_property_value val;
}t_property_value;

typedef struct{
    t_property_value * head;
    int num;
}t_property_value_array;

typedef struct event_loop_native_data_t {
    DBusConnection *conn;
    const char *adapter;

    /* protects the thread */
    pthread_mutex_t thread_mutex;
    pthread_t thread;
    /* our comms socket */
    /* mem for the list of sockets to listen to */
    struct pollfd *pollData;
    int pollMemberCount;
    int pollDataSize;
    /* mem for matching set of dbus watch ptrs */
    DBusWatch **watchData;
    /* pair of sockets for event loop control, Reader and Writer */
    int controlFdR;
    int controlFdW;
    /* flag to indicate if the event loop thread is running */
    int running;
}tBluetoothEvent;

dbus_bool_t dbus_func_args_async(DBusConnection *conn,
                                 int timeout_ms,
                                 void (*reply)(DBusMessage *, void *, void *),
                                 void *user,
                                 void *nat,
                                 const char *path,
                                 const char *ifc,
                                 const char *func,
                                 int first_arg_type,
                                 ...);

DBusMessage * dbus_func_args(DBusConnection *conn,
                             const char *path,
                             const char *ifc,
                             const char *func,
                             int first_arg_type,
                             ...);

DBusMessage * dbus_func_args_error(DBusConnection *conn,
                                   DBusError *err,
                                   const char *path,
                                   const char *ifc,
                                   const char *func,
                                   int first_arg_type,
                                   ...);

DBusMessage * dbus_func_args_timeout(DBusConnection *conn,
                                     int timeout_ms,
                                     const char *path,
                                     const char *ifc,
                                     const char *func,
                                     int first_arg_type,
                                     ...);

DBusMessage * dbus_func_args_timeout_valist(DBusConnection *conn,
                                            int timeout_ms,
                                            DBusError *err,
                                            const char *path,
                                            const char *ifc,
                                            const char *func,
                                            int first_arg_type,
                                            va_list args);

/*following functions is to get the value received from the dbus*/
int dbus_returns_int32(DBusMessage *reply);
unsigned int dbus_returns_uint32(DBusMessage *reply);
int dbus_returns_unixfd(DBusMessage *reply);
int dbus_returns_boolean(DBusMessage *reply);
/*for the string, we will strdup a new string, users should free the string*/
char * dbus_returns_string(DBusMessage *reply);
/*
* we will return a array[char *] accroding to the dbus msg, users should free the array and the values in the array
*/
char ** dbus_returns_array_of_strings(DBusMessage *reply, int * array_len);
char ** dbus_returns_array_of_object_path(DBusMessage *reply, int * array_len);
/*
* we will alloc a array[char] accroding to the dbus msg, users should free the array
*/
char * dbus_returns_array_of_bytes(DBusMessage *reply, int * array_len);

void free_array_of_strings(char * strArray[], int len);
void free_array_of_bytes(char * byteArray);

/*following is the parse function for dbus message*/
int parse_properties(DBusMessageIter *iter, Properties *properties,
                              const int max_num_properties,t_property_value_array *array);
							  
int parse_property_change(DBusMessage *msg,
                                   Properties *properties, int max_num_properties,t_property_value_array *array);
int parse_adapter_properties( DBusMessageIter *iter,t_property_value_array *array);
int parse_remote_device_properties(DBusMessageIter *iter,t_property_value_array *array);
/*jobjectArray parse_remote_device_property_change(JNIEnv *env, DBusMessage *msg);
jobjectArray parse_adapter_property_change(JNIEnv *env, DBusMessage *msg);*/
void print_property_value(t_property_value_array *array);
void free_property_value(t_property_value_array *array);

/*following is the functions that we append variant and send to dbus*/
void append_dict_args(DBusMessage *reply, const char *first_key, ...);
void append_variant(DBusMessageIter *iter, int type, void *val);

/*translation between bdaddr and string(mac)*/
int get_bdaddr(const char *str, bdaddr_t *ba);
void get_bdaddr_as_string(const bdaddr_t *ba, char *str);



// Result codes from Bluez DBus calls
#define BOND_RESULT_ERROR                      -1
#define BOND_RESULT_SUCCESS                     0
#define BOND_RESULT_AUTH_FAILED                 1
#define BOND_RESULT_AUTH_REJECTED               2
#define BOND_RESULT_AUTH_CANCELED               3
#define BOND_RESULT_REMOTE_DEVICE_DOWN          4
#define BOND_RESULT_DISCOVERY_IN_PROGRESS       5
#define BOND_RESULT_AUTH_TIMEOUT                6
#define BOND_RESULT_REPEATED_ATTEMPTS           7

#endif
