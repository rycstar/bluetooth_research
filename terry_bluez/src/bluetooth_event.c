#include "bluetooth_service.h"
#include "bluetooth_event.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

static tBluetoothEvent * g_bluetooth_evt = NULL;
static DBusHandlerResult event_filter(DBusConnection *conn, DBusMessage *msg,
                                      void *data);
DBusHandlerResult agent_event_filter(DBusConnection *conn,
                                     DBusMessage *msg,
                                     void *data);
static int register_agent(tBluetoothEvent *nat,
                          const char *agent_path, const char *capabilities);

static const DBusObjectPathVTable agent_vtable = {
    NULL, agent_event_filter, NULL, NULL, NULL, NULL
};

static unsigned int unix_events_to_dbus_flags(short events) {
    return (events & DBUS_WATCH_READABLE ? POLLIN : 0) |
           (events & DBUS_WATCH_WRITABLE ? POLLOUT : 0) |
           (events & DBUS_WATCH_ERROR ? POLLERR : 0) |
           (events & DBUS_WATCH_HANGUP ? POLLHUP : 0);
}

static short dbus_flags_to_unix_events(unsigned int flags) {
    return (flags & POLLIN ? DBUS_WATCH_READABLE : 0) |
           (flags & POLLOUT ? DBUS_WATCH_WRITABLE : 0) |
           (flags & POLLERR ? DBUS_WATCH_ERROR : 0) |
           (flags & POLLHUP ? DBUS_WATCH_HANGUP : 0);
}

/*
* local agent for listen the remote device pair
*/
int register_agent(tBluetoothEvent * nat,
                          const char *agent_path, const char *capabilities){
    DBusMessage *msg, *reply;
    DBusError err;

    if (!dbus_connection_register_object_path(nat->conn, agent_path,
            &agent_vtable, NULL)) {
        printf("%s: Can't register object path %s for agent!",
              __FUNCTION__, agent_path);
        return -1;
    }

    nat->adapter = get_adapter_path();
    if (nat->adapter == NULL) {
        return -1;
    }
    msg = dbus_message_new_method_call("org.bluez", nat->adapter,
          "org.bluez.Adapter", "RegisterAgent");
    if (!msg) {
        printf("%s: Can't allocate new method call for agent!",
              __FUNCTION__);
        return -1;
    }
    dbus_message_append_args(msg, DBUS_TYPE_OBJECT_PATH, &agent_path,
                             DBUS_TYPE_STRING, &capabilities,
                             DBUS_TYPE_INVALID);

    dbus_error_init(&err);
    reply = dbus_connection_send_with_reply_and_block(nat->conn, msg, -1, &err);
    dbus_message_unref(msg);

    if (!reply) {
        printf("%s: Can't register agent!", __FUNCTION__);
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR(&err);
        }
        return -1;
    }

    dbus_message_unref(reply);
    dbus_connection_flush(nat->conn);

    return 0;
}

DBusHandlerResult agent_event_filter(DBusConnection *conn,
                                     DBusMessage *msg, void *data){
	if (dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_METHOD_CALL) {
        printf("%s: not interested (not a method call).", __FUNCTION__);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (dbus_message_is_method_call(msg,
            "org.bluez.Agent", "Cancel")){

    }else if (dbus_message_is_method_call(msg,
            "org.bluez.Agent", "Authorize")) {

    }else if (dbus_message_is_method_call(msg,
            "org.bluez.Agent", "OutOfBandAvailable")){

    }else if (dbus_message_is_method_call(msg,
            "org.bluez.Agent", "RequestPinCode")) {

    }else if (dbus_message_is_method_call(msg,
            "org.bluez.Agent", "RequestPasskey")) {

    }else if (dbus_message_is_method_call(msg,
            "org.bluez.Agent", "RequestOobData")) {

    } else if (dbus_message_is_method_call(msg,
            "org.bluez.Agent", "DisplayPasskey")) {

    } else if (dbus_message_is_method_call(msg,
            "org.bluez.Agent", "RequestConfirmation")) {

    }else if (dbus_message_is_method_call(msg,
            "org.bluez.Agent", "RequestPairingConsent")) {

    }else if (dbus_message_is_method_call(msg,
                  "org.bluez.Agent", "Release")) {

    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult event_filter(DBusConnection *conn, DBusMessage *msg,
                                      void *data){
	DBusError err;
    //DBusHandlerResult ret;
    //tBluetoothEvent * nat = (tBluetoothEvent *)data;
    dbus_error_init(&err);

	if (dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_SIGNAL) {
        printf("%s: not interested (not a signal).", __FUNCTION__);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    printf("%s: Received signal %s:%s from %s", __FUNCTION__,
        dbus_message_get_interface(msg), dbus_message_get_member(msg),
        dbus_message_get_path(msg));

    if (dbus_message_is_signal(msg,
                               "org.bluez.Adapter",
                               "DeviceFound")) {
        printf("%s-----------DeviceFound\n",__FUNCTION__);

    }else if (dbus_message_is_signal(msg,
                                     "org.bluez.Adapter",
                                     "DeviceDisappeared")){


    }else if (dbus_message_is_signal(msg,
                                     "org.bluez.Adapter",
                                     "DeviceCreated")){


    }else if (dbus_message_is_signal(msg,
                                     "org.bluez.Adapter",
                                     "DeviceRemoved")) {

    }else if (dbus_message_is_signal(msg,
                                      "org.bluez.Adapter",
                                      "PropertyChanged")) {

    }else if (dbus_message_is_signal(msg,
                                      "org.bluez.Device",
                                      "PropertyChanged")) {


    }else if (dbus_message_is_signal(msg,
                                      "org.bluez.Device",
                                      "DisconnectRequested")) {

    }

	return DBUS_HANDLER_RESULT_HANDLED;
}

#define EVENT_LOOP_EXIT 1
#define EVENT_LOOP_ADD  2
#define EVENT_LOOP_REMOVE 3
#define EVENT_LOOP_WAKEUP 4

dbus_bool_t dbusAddWatch(DBusWatch *watch, void *data) {
    tBluetoothEvent * nat = (tBluetoothEvent *)data;

    if (dbus_watch_get_enabled(watch)) {
        // note that we can't just send the watch and inspect it later
        // because we may get a removeWatch call before this data is reacted
        // to by our eventloop and remove this watch..  reading the add first
        // and then inspecting the recently deceased watch would be bad.
        char control = EVENT_LOOP_ADD;
        write(nat->controlFdW, &control, sizeof(char));

        /*remove a warning, if you want to see the warning ,open the mark*/
        int fd = dbus_watch_get_fd(watch);
        //int fd = dbus_watch_get_unix_fd(watch);
        write(nat->controlFdW, &fd, sizeof(int));

        unsigned int flags = dbus_watch_get_flags(watch);
        write(nat->controlFdW, &flags, sizeof(unsigned int));

        write(nat->controlFdW, &watch, sizeof(DBusWatch*));
    }
    return 1;
}

void dbusRemoveWatch(DBusWatch *watch, void *data) {
    tBluetoothEvent * nat = (tBluetoothEvent *)data;

    char control = EVENT_LOOP_REMOVE;
    write(nat->controlFdW, &control, sizeof(char));

    /*remove a warning, if you want to see the warning ,open the mark*/
    int fd = dbus_watch_get_fd(watch);
    //int fd = dbus_watch_get_unix_fd(watch);
    write(nat->controlFdW, &fd, sizeof(int));

    unsigned int flags = dbus_watch_get_flags(watch);
    write(nat->controlFdW, &flags, sizeof(unsigned int));
}

void dbusToggleWatch(DBusWatch *watch, void *data) {
    if (dbus_watch_get_enabled(watch)) {
        dbusAddWatch(watch, data);
    } else {
        dbusRemoveWatch(watch, data);
    }
}

void dbusWakeup(void *data) {
    tBluetoothEvent * nat = (tBluetoothEvent *)data;

    char control = EVENT_LOOP_WAKEUP;
    write(nat->controlFdW, &control, sizeof(char));
}

static void handleWatchAdd(tBluetoothEvent *nat) {
    DBusWatch *watch;
    int newFD,y;
    unsigned int flags;

    read(nat->controlFdR, &newFD, sizeof(int));
    read(nat->controlFdR, &flags, sizeof(unsigned int));
    read(nat->controlFdR, &watch, sizeof(DBusWatch *));
    short events = dbus_flags_to_unix_events(flags);

    for (y = 0; y<nat->pollMemberCount; y++) {
        if ((nat->pollData[y].fd == newFD) &&
                (nat->pollData[y].events == events)) {
            printf("DBusWatch duplicate add");
            return;
        }
    }
    if (nat->pollMemberCount == nat->pollDataSize) {
        printf("Bluetooth EventLoop poll struct growing");
        struct pollfd *temp = (struct pollfd *)malloc(
                sizeof(struct pollfd) * (nat->pollMemberCount+1));
        if (!temp) {
            return;
        }
        memcpy(temp, nat->pollData, sizeof(struct pollfd) *
                nat->pollMemberCount);
        free(nat->pollData);
        nat->pollData = temp;
        DBusWatch **temp2 = (DBusWatch **)malloc(sizeof(DBusWatch *) *
                (nat->pollMemberCount+1));
        if (!temp2) {
            return;
        }
        memcpy(temp2, nat->watchData, sizeof(DBusWatch *) *
                nat->pollMemberCount);
        free(nat->watchData);
        nat->watchData = temp2;
        nat->pollDataSize++;
    }
    nat->pollData[nat->pollMemberCount].fd = newFD;
    nat->pollData[nat->pollMemberCount].revents = 0;
    nat->pollData[nat->pollMemberCount].events = events;
    nat->watchData[nat->pollMemberCount] = watch;
    nat->pollMemberCount++;
}

static void handleWatchRemove(tBluetoothEvent *nat) {
    int removeFD,y;
    unsigned int flags;

    read(nat->controlFdR, &removeFD, sizeof(int));
    read(nat->controlFdR, &flags, sizeof(unsigned int));
    short events = dbus_flags_to_unix_events(flags);

    for (y = 0; y < nat->pollMemberCount; y++) {
        if ((nat->pollData[y].fd == removeFD) &&
                (nat->pollData[y].events == events)) {
            int newCount = --nat->pollMemberCount;
            // copy the last live member over this one
            nat->pollData[y].fd = nat->pollData[newCount].fd;
            nat->pollData[y].events = nat->pollData[newCount].events;
            nat->pollData[y].revents = nat->pollData[newCount].revents;
            nat->watchData[y] = nat->watchData[newCount];
            return;
        }
    }
    printf("WatchRemove given with unknown watch");
}

int setUpEventLoop(tBluetoothEvent *nat){
	DBusError err;
	const char *agent_path = LOCAL_AGENT_PATH;
    const char *capabilities = "DisplayYesNo";
	if(nat != NULL && nat->conn != NULL){
		dbus_error_init(&err);
		if (register_agent(nat, agent_path, capabilities) < 0) {
            dbus_connection_unregister_object_path (nat->conn, agent_path);
            return -1;
        }

        // Add a filter for all incoming messages
        if (!dbus_connection_add_filter(nat->conn, event_filter, nat, NULL)){
            return -1;
        }

        // Set which messages will be processed by this dbus connection
        dbus_bus_add_match(nat->conn,
                "type='signal',interface='org.freedesktop.DBus'",
                &err);
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR(&err);
            return -1;
        }

        printf("%s-----%s\n",__FUNCTION__,"type='signal',interface='"BLUEZ_DBUS_BASE_IFC".Adapter'");
        dbus_bus_add_match(nat->conn,
                "type='signal',interface='"BLUEZ_DBUS_BASE_IFC".Adapter'",
                &err);
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR(&err);
            return -1;
        }
        dbus_bus_add_match(nat->conn,
                "type='signal',interface='"BLUEZ_DBUS_BASE_IFC".Device'",
                &err);
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR(&err);
            return -1;
        }
	}
	return 0;
}

static void tearDownEventLoop(tBluetoothEvent *nat){
    if (nat != NULL && nat->conn != NULL) {
        DBusMessage *msg, *reply;
        DBusError err;
        dbus_error_init(&err);
        const char * agent_path = LOCAL_AGENT_PATH;
        msg = dbus_message_new_method_call("org.bluez",
                                           nat->adapter,
                                           "org.bluez.Adapter",
                                           "UnregisterAgent");
        if (msg != NULL) {
            dbus_message_append_args(msg, DBUS_TYPE_OBJECT_PATH, &agent_path,
                                     DBUS_TYPE_INVALID);
            reply = dbus_connection_send_with_reply_and_block(nat->conn,
                                                              msg, -1, &err);

            if (!reply) {
                if (dbus_error_is_set(&err)) {
                    LOG_AND_FREE_DBUS_ERROR(&err);
                    dbus_error_free(&err);
                }
            } else {
                dbus_message_unref(reply);
            }
            dbus_message_unref(msg);
        } else {
             printf("%s: Can't create new method call!", __FUNCTION__);
        }

        dbus_connection_flush(nat->conn);
        dbus_connection_unregister_object_path(nat->conn, agent_path);

        dbus_bus_remove_match(nat->conn,
                "type='signal',interface='"BLUEZ_DBUS_BASE_IFC".Device'",
                &err);
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR(&err);
        }

        dbus_bus_remove_match(nat->conn,
                "type='signal',interface='"BLUEZ_DBUS_BASE_IFC".Adapter'",
                &err);
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR(&err);
        }

        dbus_bus_remove_match(nat->conn,
                "type='signal',interface='org.freedesktop.DBus'",
                &err);
        if (dbus_error_is_set(&err)) {
            LOG_AND_FREE_DBUS_ERROR(&err);
        }

        dbus_connection_remove_filter(nat->conn, event_filter, nat);
    }
}
/*
* init functions for bluetooth event
*/

void initializeBluetoothEvent(){
	g_bluetooth_evt = (tBluetoothEvent *)calloc(1, sizeof(tBluetoothEvent));
    tBluetoothEvent *nat = g_bluetooth_evt;
    if (NULL == nat) {
        printf("%s: out of memory!", __FUNCTION__);
        return;
    }
    memset(nat, 0, sizeof(tBluetoothEvent));

    pthread_mutex_init(&(nat->thread_mutex), NULL);

    {
        DBusError err;
        dbus_error_init(&err);
        dbus_threads_init_default();
        nat->conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
        if (dbus_error_is_set(&err)){
            printf("%s: Could not get onto the system bus!", __FUNCTION__);
            dbus_error_free(&err);
        }
        dbus_connection_set_exit_on_disconnect(nat->conn, FALSE);
    }
}

void cleanupBluetoothEvent() {
    tBluetoothEvent *nat = g_bluetooth_evt;
    pthread_mutex_destroy(&(nat->thread_mutex));

    if (nat) {
    	//dbus_connection_unref(nat->conn);
        free(nat);
        g_bluetooth_evt = NULL;
    }
}

static void *eventLoopMain(void *ptr) {
    int i = 0;
    tBluetoothEvent *nat = (tBluetoothEvent *)ptr;

    dbus_connection_set_watch_functions(nat->conn, dbusAddWatch,
            dbusRemoveWatch, dbusToggleWatch, ptr, NULL);
    dbus_connection_set_wakeup_main_function(nat->conn, dbusWakeup, ptr, NULL);

    nat->running = 1;

    while (1) {
        for (i = 0; i < nat->pollMemberCount; i++) {
            if (!nat->pollData[i].revents) {
                continue;
            }
            if (nat->pollData[i].fd == nat->controlFdR) {
                char data;
                while (recv(nat->controlFdR, &data, sizeof(char), MSG_DONTWAIT)
                        != -1) {
                    switch (data) {
                    case EVENT_LOOP_EXIT:
                    {
                        dbus_connection_set_watch_functions(nat->conn,
                                NULL, NULL, NULL, NULL, NULL);
                        tearDownEventLoop(nat);

                        int fd = nat->controlFdR;
                        nat->controlFdR = 0;
                        close(fd);
                        return NULL;
                    }
                    case EVENT_LOOP_ADD:
                    {
                        handleWatchAdd(nat);
                        break;
                    }
                    case EVENT_LOOP_REMOVE:
                    {
                        handleWatchRemove(nat);
                        break;
                    }
                    case EVENT_LOOP_WAKEUP:
                    {
                        // noop
                        break;
                    }
                    }
                }
            } else {
                short events = nat->pollData[i].revents;
                unsigned int flags = unix_events_to_dbus_flags(events);
                dbus_watch_handle(nat->watchData[i], flags);
                nat->pollData[i].revents = 0;
                // can only do one - it may have caused a 'remove'
                break;
            }
        }
        while (dbus_connection_dispatch(nat->conn) ==
                DBUS_DISPATCH_DATA_REMAINS) {
        }

        poll(nat->pollData, nat->pollMemberCount, -1);
    }
}

int startEventLoop(){
	int result = -1;

    tBluetoothEvent *nat = g_bluetooth_evt;

    pthread_mutex_lock(&(nat->thread_mutex));

    nat->running = 0;

    if (nat->pollData) {
        printf("trying to start EventLoop a second time!");
        pthread_mutex_unlock( &(nat->thread_mutex) );
        return result;
    }

    nat->pollData = (struct pollfd *)malloc(sizeof(struct pollfd) *
            DEFAULT_INITIAL_POLLFD_COUNT);
    if (!nat->pollData) {
        printf("out of memory error starting EventLoop!");
        goto done;
    }

    nat->watchData = (DBusWatch **)malloc(sizeof(DBusWatch *) *
            DEFAULT_INITIAL_POLLFD_COUNT);
    if (!nat->watchData) {
        printf("out of memory error starting EventLoop!");
        goto done;
    }

    memset(nat->pollData, 0, sizeof(struct pollfd) *
            DEFAULT_INITIAL_POLLFD_COUNT);
    memset(nat->watchData, 0, sizeof(DBusWatch *) *
            DEFAULT_INITIAL_POLLFD_COUNT);
    nat->pollDataSize = DEFAULT_INITIAL_POLLFD_COUNT;
    nat->pollMemberCount = 1;

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, &(nat->controlFdR))) {
        printf("Error getting BT control socket");
        goto done;
    }
    nat->pollData[0].fd = nat->controlFdR;
    nat->pollData[0].events = POLLIN;

    if (setUpEventLoop(nat) < 0) {
        printf("failure setting up Event Loop!");
        goto done;
    }

    printf("%s success create event thread(%d/%d)\n", __FUNCTION__,nat->controlFdW,nat->controlFdR);
    pthread_create(&(nat->thread), NULL, eventLoopMain, nat);
    result = 0;

done:
    if (-1 == result) {
        if (nat->controlFdW) {
            close(nat->controlFdW);
            nat->controlFdW = 0;
        }
        if (nat->controlFdR) {
            close(nat->controlFdR);
            nat->controlFdR = 0;
        }
        if (nat->pollData) free(nat->pollData);
        nat->pollData = NULL;
        if (nat->watchData) free(nat->watchData);
        nat->watchData = NULL;
        nat->pollDataSize = 0;
        nat->pollMemberCount = 0;
    }

    pthread_mutex_unlock(&(nat->thread_mutex));

    return result;
}

void stopEventLoop(){
	tBluetoothEvent *nat = g_bluetooth_evt;

    pthread_mutex_lock(&(nat->thread_mutex));
    printf("%s come in...\n",__FUNCTION__);
    if (nat->pollData) {
        char data = EVENT_LOOP_EXIT;
        write(nat->controlFdW, &data, sizeof(char));
        void *ret;
        printf("%s waiting for thread end\n",__FUNCTION__);
        pthread_join(nat->thread, &ret);

        free(nat->pollData);
        nat->pollData = NULL;
        free(nat->watchData);
        nat->watchData = NULL;
        nat->pollDataSize = 0;
        nat->pollMemberCount = 0;

        int fd = nat->controlFdW;
        nat->controlFdW = 0;
        close(fd);
    }
    nat->running = 0;
    pthread_mutex_unlock(&(nat->thread_mutex));
}

int isEventLoopRunning(){
	tBluetoothEvent *nat = g_bluetooth_evt;
	int ret = 0;
	pthread_mutex_lock(&(nat->thread_mutex));
	ret = nat->running;
	pthread_mutex_unlock(&(nat->thread_mutex));
	return ret;
}