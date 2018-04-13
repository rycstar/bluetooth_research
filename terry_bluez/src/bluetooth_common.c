#include "bluetooth_common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static Properties remote_device_properties[] = {
    {"Address",  DBUS_TYPE_STRING},
    {"Name", DBUS_TYPE_STRING},
    {"Icon", DBUS_TYPE_STRING},
    {"Class", DBUS_TYPE_UINT32},
    {"UUIDs", DBUS_TYPE_ARRAY},
    {"Services", DBUS_TYPE_ARRAY},
    {"Paired", DBUS_TYPE_BOOLEAN},
    {"Connected", DBUS_TYPE_BOOLEAN},
    {"Trusted", DBUS_TYPE_BOOLEAN},
    {"Blocked", DBUS_TYPE_BOOLEAN},
    {"Alias", DBUS_TYPE_STRING},
    {"Nodes", DBUS_TYPE_ARRAY},
    {"Adapter", DBUS_TYPE_OBJECT_PATH},
    {"LegacyPairing", DBUS_TYPE_BOOLEAN},
    {"RSSI", DBUS_TYPE_INT16},
    {"TX", DBUS_TYPE_UINT32},
    {"Broadcaster", DBUS_TYPE_BOOLEAN}
};

static Properties adapter_properties[] = {
    {"Address", DBUS_TYPE_STRING},
    {"Name", DBUS_TYPE_STRING},
    {"Class", DBUS_TYPE_UINT32},
    {"Powered", DBUS_TYPE_BOOLEAN},
    {"Discoverable", DBUS_TYPE_BOOLEAN},
    {"DiscoverableTimeout", DBUS_TYPE_UINT32},
    {"Pairable", DBUS_TYPE_BOOLEAN},
    {"PairableTimeout", DBUS_TYPE_UINT32},
    {"Discovering", DBUS_TYPE_BOOLEAN},
    {"Devices", DBUS_TYPE_ARRAY},
    {"UUIDs", DBUS_TYPE_ARRAY},
};

typedef struct {
    void (*user_cb)(DBusMessage *, void *, void *);
    void *user;
    void *nat;
} dbus_async_call_t;

void dbus_func_args_async_callback(DBusPendingCall *call, void *data) {

    dbus_async_call_t *req = (dbus_async_call_t *)data;
    DBusMessage *msg;

    /* This is guaranteed to be non-NULL, because this function is called only
       when once the remote method invokation returns. */
    msg = dbus_pending_call_steal_reply(call);

    if (msg) {
        if (req->user_cb) {
            // The user may not deref the message object.
            req->user_cb(msg, req->user, req->nat);
        }
        dbus_message_unref(msg);
    }

    //dbus_message_unref(req->method);
    dbus_pending_call_cancel(call);
    dbus_pending_call_unref(call);
    free(req);
}

static dbus_bool_t dbus_func_args_async_valist(DBusConnection *conn,
                                        int timeout_ms,
                                        void (*user_cb)(DBusMessage *,
                                                        void *,
                                                        void*),
                                        void *user,
                                        void *nat,
                                        const char *path,
                                        const char *ifc,
                                        const char *func,
                                        int first_arg_type,
                                        va_list args) {
    DBusMessage *msg = NULL;
    dbus_async_call_t *pending;
    dbus_bool_t reply = FALSE;

    /* Compose the command */
    msg = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC, path, ifc, func);

    if (msg == NULL) {
        printf("Could not allocate D-Bus message object!");
        goto done;
    }

    /* append arguments */
    if (!dbus_message_append_args_valist(msg, first_arg_type, args)) {
        printf("Could not append argument to method call!");
        goto done;
    }

    /* Make the call. */
    pending = (dbus_async_call_t *)malloc(sizeof(dbus_async_call_t));
    if (pending) {
        DBusPendingCall *call;

        pending->user_cb = user_cb;
        pending->user = user;
        pending->nat = nat;
        //pending->method = msg;

        reply = dbus_connection_send_with_reply(conn, msg,
                                                &call,
                                                timeout_ms);
        if (reply == TRUE) {
            dbus_pending_call_set_notify(call,
                                         dbus_func_args_async_callback,
                                         pending,
                                         NULL);
        }
    }

done:
    if (msg) dbus_message_unref(msg);
    return reply;
}


dbus_bool_t dbus_func_args_async(DBusConnection *conn,
                                 int timeout_ms,
                                 void (*reply)(DBusMessage *, void *, void*),
                                 void *user,
                                 void *nat,
                                 const char *path,
                                 const char *ifc,
                                 const char *func,
                                 int first_arg_type,
                                 ...) {
    dbus_bool_t ret;
    va_list lst;
    va_start(lst, first_arg_type);

    ret = dbus_func_args_async_valist(conn,
                                      timeout_ms,
                                      reply, user, nat,
                                      path, ifc, func,
                                      first_arg_type, lst);
    va_end(lst);
    return ret;
}

// If err is NULL, then any errors will be LOGE'd, and free'd and the reply
// will be NULL.
// If err is not NULL, then it is assumed that dbus_error_init was already
// called, and error's will be returned to the caller without logging. The
// return value is NULL iff an error was set. The client must free the error if
// set.
DBusMessage * dbus_func_args_timeout_valist(DBusConnection *conn,
                                            int timeout_ms,
                                            DBusError *err,
                                            const char *path,
                                            const char *ifc,
                                            const char *func,
                                            int first_arg_type,
                                            va_list args) {

    DBusMessage *msg = NULL, *reply = NULL;
    int return_error = (err != NULL);

    if (!return_error) {
        err = (DBusError*)malloc(sizeof(DBusError));
        dbus_error_init(err);
    }

    /* Compose the command */
    msg = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC, path, ifc, func);

    if (msg == NULL) {
        printf("Could not allocate D-Bus message object!");
        goto done;
    }

    /* append arguments */
    if (!dbus_message_append_args_valist(msg, first_arg_type, args)) {
        printf("Could not append argument to method call!");
        goto done;
    }

    /* Make the call. */
    reply = dbus_connection_send_with_reply_and_block(conn, msg, timeout_ms, err);
    if (!return_error && dbus_error_is_set(err)) {
        LOG_AND_FREE_DBUS_ERROR_WITH_MSG(err, msg);
    }

done:
    if (!return_error) {
        free(err);
    }
    if (msg) dbus_message_unref(msg);
    return reply;
}

DBusMessage * dbus_func_args_timeout(DBusConnection *conn,
                                     int timeout_ms,
                                     const char *path,
                                     const char *ifc,
                                     const char *func,
                                     int first_arg_type,
                                     ...) {
    DBusMessage *ret;
    va_list lst;
    va_start(lst, first_arg_type);
    ret = dbus_func_args_timeout_valist(conn, timeout_ms, NULL,
                                        path, ifc, func,
                                        first_arg_type, lst);
    va_end(lst);
    return ret;
}

DBusMessage * dbus_func_args(DBusConnection *conn,
                             const char *path,
                             const char *ifc,
                             const char *func,
                             int first_arg_type,
                             ...) {
    DBusMessage *ret;
    va_list lst;
    va_start(lst, first_arg_type);
    ret = dbus_func_args_timeout_valist(conn, -1, NULL,
                                        path, ifc, func,
                                        first_arg_type, lst);
    va_end(lst);
    return ret;
}

DBusMessage * dbus_func_args_error(DBusConnection *conn,
                                   DBusError *err,
                                   const char *path,
                                   const char *ifc,
                                   const char *func,
                                   int first_arg_type,
                                   ...) {
    DBusMessage *ret;
    va_list lst;
    va_start(lst, first_arg_type);
    ret = dbus_func_args_timeout_valist(conn, -1, err,
                                        path, ifc, func,
                                        first_arg_type, lst);
    va_end(lst);
    return ret;
}

int dbus_returns_int32(DBusMessage *reply) {

    DBusError err;
    int ret = -1;

    dbus_error_init(&err);
    if (!dbus_message_get_args(reply, &err,
                               DBUS_TYPE_INT32, &ret,
                               DBUS_TYPE_INVALID)) {
        LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
    }
    dbus_message_unref(reply);
    return ret;
}

unsigned int dbus_returns_uint32( DBusMessage *reply) {

    DBusError err;
    int ret = -1;

    dbus_error_init(&err);
    if (!dbus_message_get_args(reply, &err,
                               DBUS_TYPE_UINT32, &ret,
                               DBUS_TYPE_INVALID)) {
        LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
    }
    dbus_message_unref(reply);
    return ret;
}

int dbus_returns_unixfd(DBusMessage *reply){
    DBusError err;
    int ret = -1;

    dbus_error_init(&err);
    if (!dbus_message_get_args(reply, &err,
                               DBUS_TYPE_UNIX_FD, &ret,
                               DBUS_TYPE_INVALID)) {
        LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
    }
    dbus_message_unref(reply);
    return ret;
}

int dbus_returns_boolean(DBusMessage *reply){
    DBusError err;
    int ret = 0;
    dbus_bool_t val = FALSE;

    dbus_error_init(&err);

    /* Check the return value. */
    if (dbus_message_get_args(reply, &err,
                               DBUS_TYPE_BOOLEAN, &val,
                               DBUS_TYPE_INVALID)) {
        ret = val == TRUE ? 1 : 0;
    } else {
        LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
    }

    dbus_message_unref(reply);
    return ret;
}

char * dbus_returns_string(DBusMessage *reply){
    DBusError err;
    char * ret = NULL;
    const char *name;

    dbus_error_init(&err);
    if (dbus_message_get_args(reply, &err,
                               DBUS_TYPE_STRING, &name,
                               DBUS_TYPE_INVALID)) {
        ret = strdup(name);
    } else {
        LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
    }
    dbus_message_unref(reply);

    return ret;
}

/*
* Notice : User must free the array and the value in the array
*/
char **  dbus_returns_array_of_strings(DBusMessage *reply, int * array_len){
    DBusError err;
    char **list;
    int i, len;
    char ** str_array = NULL;

    dbus_error_init(&err);
    if (dbus_message_get_args (reply,
                               &err,
                               DBUS_TYPE_ARRAY, DBUS_TYPE_STRING,
                               &list, &len,
                               DBUS_TYPE_INVALID)) {
        str_array = malloc(len * sizeof(char *));
        if(str_array){
            for (i = 0; i < len; i++)
                str_array[i] = strdup(list[i]);
            *array_len = len;
        }
    } else {
        LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
    }

    dbus_message_unref(reply);
    return str_array;
}


/*
* Notice : User must free the array and the value in the array
*/
char ** dbus_returns_array_of_object_path(DBusMessage *reply,int * array_len){
    DBusError err;
    char **list;
    int i, len;
    char ** str_array = NULL;

    dbus_error_init(&err);
    if (dbus_message_get_args (reply,
                               &err,
                               DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH,
                               &list, &len,
                               DBUS_TYPE_INVALID)) {
        str_array = (char **)malloc(len * sizeof(char *));
        if(str_array){
            for (i = 0; i < len; i++)
                str_array[i] = strdup(list[i]);
            *array_len = len;
        }
    } else {
        LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
    }

    dbus_message_unref(reply);
    return str_array;
}

/*
* Notice : User must free the array
*/
char * dbus_returns_array_of_bytes(DBusMessage *reply, int * array_len){
    DBusError err;
    int len;
    char * list;
    char * byteArray = NULL;

    dbus_error_init(&err);
    if (dbus_message_get_args(reply, &err,
                              DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &list, &len,
                              DBUS_TYPE_INVALID)) {
        byteArray = malloc(len * sizeof(char));
        if (byteArray){
            memcpy(byteArray,list,len*sizeof(char));
	}

    } else {
        LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
    }

    dbus_message_unref(reply);
    return byteArray;
}

void free_array_of_strings(char * strArray[], int len){
    int i;
    if(strArray){
        for(i = 0; i < len; i++)
            free(strArray[i]);
        free(strArray);
    }
}

void free_array_of_bytes(char * byteArray){
    if(byteArray)
        free(byteArray);
}

/*******************parse functions*********************************************/
int get_property(DBusMessageIter iter, Properties *properties,
                  int max_num_properties, int *prop_index, u_property_value *value, int *len){

    DBusMessageIter prop_val, array_val_iter;
    char *property = NULL;
    uint32_t array_type;
    int i, j, type, int_val;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
        return -1;
    dbus_message_iter_get_basic(&iter, &property);
    if (!dbus_message_iter_next(&iter))
        return -1;
    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT)
        return -1;
    for (i = 0; i <  max_num_properties; i++) {
        if (!strncmp(property, properties[i].name, strlen(property)))
            break;
    }
    *prop_index = i;
    if (i == max_num_properties)
        return -1;
    dbus_message_iter_recurse(&iter, &prop_val);
    type = properties[*prop_index].type;

    if (dbus_message_iter_get_arg_type(&prop_val) != type) {
        printf("Property type mismatch in get_property: %d, expected:%d, index:%d",
             dbus_message_iter_get_arg_type(&prop_val), type, *prop_index);
        return -1;
    }

    switch(type){
    case DBUS_TYPE_STRING:
    case DBUS_TYPE_OBJECT_PATH:
        dbus_message_iter_get_basic(&prop_val, &value->str_val);
        *len = 1;
        break;
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_BOOLEAN:
        dbus_message_iter_get_basic(&prop_val, &int_val);
        value->int_val = int_val;
        *len = 1;
        break;
    case DBUS_TYPE_ARRAY:
        dbus_message_iter_recurse(&prop_val, &array_val_iter);
        array_type = dbus_message_iter_get_arg_type(&array_val_iter);
        *len = 0;
        value->array_val = NULL;
        if (array_type == DBUS_TYPE_OBJECT_PATH ||
            array_type == DBUS_TYPE_STRING){
            j = 0;
            do {
               j ++;
            } while(dbus_message_iter_next(&array_val_iter));
            dbus_message_iter_recurse(&prop_val, &array_val_iter);
            // Allocate  an array of char *
            *len = j;
            char **tmp = (char **)malloc(sizeof(char *) * *len);
            if (!tmp)
                return -1;
            j = 0;
            do {
               dbus_message_iter_get_basic(&array_val_iter, &tmp[j]);
               j ++;
            } while(dbus_message_iter_next(&array_val_iter));
            value->array_val = tmp;
        }
        break;
    default:
        return -1;
    }
    return 0;
}

void create_prop_array(t_property_value* p_value, Properties *property,
                       u_property_value *value, int len){
				
	int array_index = 0;
	memset(p_value,0,sizeof(t_property_value));
	snprintf(p_value->name,32,"%s",property->name);
	p_value->type = property->type;
	p_value->len = len;
	switch(p_value->type){
		case DBUS_TYPE_UINT32:
		case DBUS_TYPE_INT16:
		case DBUS_TYPE_BOOLEAN:
			p_value->val.int_val = value->int_val;
			break;
		case DBUS_TYPE_STRING:
		case DBUS_TYPE_OBJECT_PATH:
			p_value->val.str_val = strdup(value->str_val);
			break;
		case DBUS_TYPE_ARRAY:
			p_value->val.array_val = malloc(p_value->len * sizeof(char*));
			if(p_value->val.array_val){
				memset(p_value->val.array_val,0,p_value->len * sizeof(char*));
				for(array_index = 0;array_index < p_value->len;array_index++){
					p_value->val.array_val[array_index] = strdup(value->array_val[array_index]);
				}
			}
			break;
		default:
			break;
	}
}

int parse_properties(DBusMessageIter *iter, Properties *properties,
                              const int max_num_properties, t_property_value_array *array){
    DBusMessageIter dict_entry, dict;
    u_property_value value;
    int i, j = 0;
    int len = 0, prop_index = -1;
    struct {
        u_property_value value;
        int len;
        int used;
    } values[max_num_properties];

    DBusError err;
    dbus_error_init(&err);

    for (i = 0; i < max_num_properties; i++) {
        values[i].used = 0;
    }

    if(dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY)
        goto failure;
    dbus_message_iter_recurse(iter, &dict);
    do {
        len = 0;
        if (dbus_message_iter_get_arg_type(&dict) != DBUS_TYPE_DICT_ENTRY)
            goto failure;
        dbus_message_iter_recurse(&dict, &dict_entry);

        if (!get_property(dict_entry, properties, max_num_properties, &prop_index,
                          &value, &len)) {
            values[prop_index].value = value;
            values[prop_index].len = len;
            values[prop_index].used = 1;
        } else {
            goto failure;
        }
    } while(dbus_message_iter_next(&dict));

    for (i = 0; i < max_num_properties; i++) {
		if (values[i].used) i++;
    }
	array->num = i;
	array->head = malloc(array->num *sizeof(t_property_value));
	if(array->head){
		memset(array->head,0,array->num * sizeof(t_property_value));
		for (i = 0; i < max_num_properties; i++) {
			if (values[i].used && j < array->num){
				create_prop_array(&(array->head[j]),&(properties[i]),&(values[i].value),values[i].len);
				j++;
				if (properties[i].type == DBUS_TYPE_ARRAY && values[i].used
                   && values[i].value.array_val != NULL)
					free(values[i].value.array_val);
			}
		}
	}

    return 0;

failure:
    if (dbus_error_is_set(&err))
        LOG_AND_FREE_DBUS_ERROR(&err);
    for (i = 0; i < max_num_properties; i++)
        if (properties[i].type == DBUS_TYPE_ARRAY && values[i].used == 1
                                        && values[i].value.array_val != NULL)
            free(values[i].value.array_val);
    return -1;
}

int parse_property_change(DBusMessage *msg,
                                   Properties *properties, int max_num_properties, t_property_value_array *array){
	DBusMessageIter iter;
    DBusError err;
    int len = 0, prop_index = -1;
    u_property_value value;

    dbus_error_init(&err);
    if (!dbus_message_iter_init(msg, &iter))
        goto failure;
	
	array->num = 1;
	array->head = malloc(array->num*sizeof(t_property_value));
    if (!get_property(iter, properties, max_num_properties,
                      &prop_index, &value, &len)) {
		if(array->head)
			create_prop_array(&(array->head[0]),&(properties[prop_index]),&value,len);
        if (properties[prop_index].type == DBUS_TYPE_ARRAY && value.array_val != NULL)
             free(value.array_val);
		
        return 0;
    }
failure:
    LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, msg);
    return -1;							   
}

int parse_adapter_properties(DBusMessageIter *iter,t_property_value_array *array){
    return parse_properties(iter, (Properties *) &adapter_properties,
                            sizeof(adapter_properties) / sizeof(Properties),array);
}

int parse_remote_device_properties(DBusMessageIter *iter,t_property_value_array *array){
    return parse_properties(iter, (Properties *) &remote_device_properties,
                            sizeof(remote_device_properties) / sizeof(Properties),array);
}

void print_property_value(t_property_value_array *array){
	int i,j;
	t_property_value* tmp = array->head;
	printf("-------------print start------------\n");
	for(i = 0; i < array->num; i++){
		printf("key[%d] name:%s\n",i,tmp->name);
		if(tmp){
			if(tmp->type == DBUS_TYPE_ARRAY){
				printf("-------------array start------------\n");
				for(j = 0; j < tmp->len; j++){
					printf("array[%d]:%s\t",j,tmp->val.array_val[j]);
				}
				printf("\n-------------array end------------\n");
			}else if(tmp->type == DBUS_TYPE_OBJECT_PATH || tmp->type == DBUS_TYPE_STRING){
				printf("string:%s\n",tmp->val.str_val);
			}else{
				printf("int:%d\n",tmp->val.int_val);
			}
			tmp++;
		}
	}
	printf("-------------print end------------\n");
}

void free_property_value(t_property_value_array *array){
	int i,j;
	t_property_value* tmp = array->head;

	for(i = 0; i < array->num; i++){
		if(tmp){
			if(tmp->type == DBUS_TYPE_ARRAY){
				for(j = 0; j < tmp->len; j++){
					if(tmp->val.array_val[j]) free(tmp->val.array_val[j]);
				}
				if(tmp->val.array_val)	free(tmp->val.array_val);
			}else if(tmp->type == DBUS_TYPE_OBJECT_PATH || tmp->type == DBUS_TYPE_STRING){
				if(tmp->val.str_val) free(tmp->val.str_val);
			}
			tmp++;
		}
	}
	if(array->head) free(array->head);
}

/*******************combine variants for dbus struction*************************/
void append_variant(DBusMessageIter *iter, int type, void *val){
    DBusMessageIter value_iter;
    char var_type[2] = { type, '\0'};
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, var_type, &value_iter);
    dbus_message_iter_append_basic(&value_iter, type, val);
    dbus_message_iter_close_container(iter, &value_iter);
}

static void dict_append_entry(DBusMessageIter *dict,
                        const char *key, int type, void *val)
{
        DBusMessageIter dict_entry;
        dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
                                                        NULL, &dict_entry);

        dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &key);
        append_variant(&dict_entry, type, val);
        dbus_message_iter_close_container(dict, &dict_entry);
}

static void append_dict_valist(DBusMessageIter *iterator, const char *first_key,
                                va_list var_args)
{
        DBusMessageIter dict;
        int val_type;
        const char *val_key;
        void *val;

        dbus_message_iter_open_container(iterator, DBUS_TYPE_ARRAY,
                        DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                        DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
                        DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

        val_key = first_key;
        while (val_key) {
                val_type = va_arg(var_args, int);
                val = va_arg(var_args, void *);
                dict_append_entry(&dict, val_key, val_type, val);
                val_key = va_arg(var_args, char *);
        }

        dbus_message_iter_close_container(iterator, &dict);
}

void append_dict_args(DBusMessage *reply, const char *first_key, ...){
    DBusMessageIter iter;
    va_list var_args;

    dbus_message_iter_init_append(reply, &iter);

    va_start(var_args, first_key);
    append_dict_valist(&iter, first_key, var_args);
    va_end(var_args);
}

int get_bdaddr(const char *str, bdaddr_t *ba) {
    char *d = ((char *)ba) + 5, *endp;
    int i;
    for(i = 0; i < 6; i++) {
        *d-- = strtol(str, &endp, 16);
        if (*endp != ':' && i != 5) {
            memset(ba, 0, sizeof(bdaddr_t));
            return -1;
        }
        str = endp + 1;
    }
    return 0;
}

void get_bdaddr_as_string(const bdaddr_t *ba, char *str) {
    const uint8_t *b = (const uint8_t *)ba;
    sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
            b[5], b[4], b[3], b[2], b[1], b[0]);
}
