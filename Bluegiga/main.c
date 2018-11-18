// This is based on BlueGiga's code for the BLED112 USB dongle with BLE 4.0
// The author for the modofications is Bill Weiler
// The intent is to provide a Bluetooth Python module for Skoobot Python apps 
//
// This is free software distributed under the terms of the MIT license reproduced below.
#include <windows.h>
#include <Python.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "cmd_def.h"
#include "uart.h"

//#define DEBUG
//COMMAND SET FOR BLE
#define MOTORS_RIGHT_30     0x08
#define MOTORS_LEFT_30      0x09
#define MOTORS_RIGHT        0x10
#define MOTORS_LEFT         0x11
#define MOTORS_FORWARD      0x12
#define MOTORS_BACKWARD     0x13
#define MOTORS_STOP         0x14
#define STOP_TURNING        0x15
#define MOTORS_SLEEP        0x16
#define MOTORS_SPEED        0x50
#define PLAY_BUZZER         0x17
//The next two in Android yet
#define DEC_STEP_MODE       0x18
#define INC_STEP_MODE       0x19
#define GET_AMBIENT         0x21
#define GET_DISTANCE        0x22
#define CONNECT_DISCONNECT  0x23
#define RECORD_SOUND        0x30
#define INCREASE_GAIN       0x31
#define DECREASE_GAIN       0x32
#define RECORD_SOUND_PI     0x33
#define ROVER_MODE          0x40
#define FOTOV_MODE          0x41
#define ROVER_MODE_REV      0x42

#define CLARG_PORT 1
#define CLARG_ACTION 2

#define UART_TIMEOUT 1000

#define MAX_DEVICES 64

int found_devices_count = 0;
bd_addr found_devices[MAX_DEVICES];
uint8_t addr[6] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };

enum actions {
    action_none,
    action_scan,
    action_connect,
    action_info,
};
enum actions action = action_none;

typedef enum {
    state_disconnected,
    state_connecting,
    state_connected,
    state_finding_services,
    state_finding_attributes,
    state_listening_measurements,
    state_finish,
    state_last
} states;
states state = state_disconnected;

const char *state_names[state_last] = {
    "disconnected",
    "connecting",
    "connected",
    "finding_services",
    "finding_attributes",
    "listening_measurements",
    "finish"
};

#define FIRST_HANDLE 0x0001
#define LAST_HANDLE  0xffff
#define SKOOBOT_UUID_BASE        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, \
                              0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}
#define SKOOBOT_SERVICE_UUID  	0x1523
#define SKOOBOT_DATA_UUID	    0x1524
#define SKOOBOT_COMMAND_UUID    0x1525
#define SKOOBOT_BYTE2_UUID   	0x1526
#define SKOOBOT_BYTE128_UUID 	0x1527
#define SKOOBOT_BYTE4_UUID   	0x1528


#define SKOOBOT_MEASUREMENT_CONFIG_UUID 0x2902

uint8 group_uuid[] = { 0x00, 0x28 };
uint8 type_uuid[] = { 0x03, 0x28 };
uint8 primary_service_uuid[] = SKOOBOT_UUID_BASE;
uint16 connection_handle = 0;

uint16 skoobot_handle_start = 0,
       skoobot_handle_end = 0,
       skoobot_handle_cmd = 0,
       skoobot_handle_data = 0,       
       skoobot_handle_byte2 = 0,
       skoobot_handle_byte4 = 0,
       skoobot_handle_byte128 = 0;		//maybe actually just 20 bytes
	   
bd_addr connect_addr;

void output(uint8 len1, uint8* data1, uint16 len2, uint8* data2);
void change_state(states new_state);
int read_message(int timeout_ms);
void print_bdaddr(bd_addr bdaddr);
void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg);
void delay(int milliseconds);

DWORD   dwThreadIdArray;
HANDLE  hThread; 
DWORD WINAPI BLED112_Pump( LPVOID lpParam );

static PyObject *op_callback = NULL;
static PyObject *data_callback = NULL;
static PyObject *sendCMD(PyObject *self, PyObject *args );
static PyObject *openConnection(PyObject *self, PyObject *args);
static PyObject *send4Byte(PyObject *self, PyObject *args );
static PyObject *closeConnection(PyObject *self, PyObject *args );
static PyObject *exitLibrary(PyObject *self, PyObject *args );

static PyObject *sendCMD(PyObject *self, PyObject *args) 
{
    const char cmd;
	static uint8 cmd_send; //must persist
	int sts;

    if (!PyArg_ParseTuple(args, "b", &cmd))
        return NULL;
	
	cmd_send = cmd;
	//printf("Write %d %d %02x\n",connection_handle,skoobot_handle_cmd,cmd);
	//ble_cmd_attclient_write_command(connection_handle,skoobot_handle_cmd,1,&cmd);
	ble_cmd_attclient_attribute_write(connection_handle,skoobot_handle_cmd,1,&cmd_send);
	
	sts = cmd;

	return PyLong_FromLong(sts);;
}

static PyObject *send4Byte(PyObject *self, PyObject *args) 
{
    const char cmd;
	int sts;

    if (!PyArg_ParseTuple(args, "b", &cmd))
        return NULL;
	sts = cmd;

	return PyLong_FromLong(sts);;
}

static PyObject *closeConnection(PyObject *self, PyObject *args) 
{
	int sts;

    ble_cmd_connection_disconnect(connection_handle);
			
	sts = 0;

	return PyLong_FromLong(sts);;
}

static PyObject *exitLibrary(PyObject *self, PyObject *args) 
{
	int sts;

	change_state(state_finish);
	
	//if (!PyArg_ParseTuple(args, "b", &cmd))
      //  return NULL;
	sts = 0;

	return PyLong_FromLong(sts);;
}
	

static PyObject *openConnection(PyObject *self, PyObject *args) 
{
	char *uart_port, *arg_action, *arg_addr;
	int sts, i;
    PyObject *temp0, *temp1;
	unsigned short addr[6];

    if (PyArg_ParseTuple(args, "sssOO:set_callback", &uart_port,&arg_action,&arg_addr,&temp0,&temp1))
	{
        if (!PyCallable_Check(temp0) || !PyCallable_Check(temp1)) {
			sts = 1;
			return PyLong_FromLong(sts);
        }
        Py_XINCREF(temp0);         /* Add a reference to new callback */
        Py_XDECREF(op_callback);  /* Dispose of previous callback */
        op_callback = temp0;       /* Remember new callback */
        Py_XINCREF(temp1);         /* Add a reference to new callback */
        Py_XDECREF(data_callback);  /* Dispose of previous callback */
        data_callback = temp1;       /* Remember new callback */
	}
	else
	{
		sts = 3;
		//Exit back to Python, message pump is running
		return PyLong_FromLong(sts);
	
	}
	printf("%s\n",uart_port);
	
	if (strcmp("scan",arg_action) == 0)
	{
		action = action_scan;
	}
	if (strcmp("info",arg_action) == 0)
	{
		action = action_info;
	}
	if (strcmp("connect",arg_action) == 0)
	{
		action = action_connect;
	
		if (sscanf(arg_addr,
			"%02hx:%02hx:%02hx:%02hx:%02hx:%02hx",
			&addr[5],
			&addr[4],
			&addr[3],
			&addr[2],
			&addr[1],
			&addr[0]) == 6) {

			for (i = 0; i < 6; i++) {
				connect_addr.addr[i] = addr[i];
			}
		}
		action = action_connect;
		print_bdaddr(connect_addr);
	}
	
    bglib_output = output;

    if (uart_open(uart_port)) {
        printf("ERROR: Unable to open serial port\n");
        sts = 2;
		return PyLong_FromLong(sts);
    }

    // Reset dongle to get it into known state
    ble_cmd_system_reset(0);
    uart_close();
    do {
        usleep(500000); // 0.5s
    } while (uart_open(uart_port));

	// Execute action
    if (action == action_scan) {
        ble_cmd_gap_discover(gap_discover_observation);
    }
    else if (action == action_info) {
        ble_cmd_system_get_info();
    }
    else if (action == action_connect) {
        printf("Trying to connect\n");
        change_state(state_connecting);
        ble_cmd_gap_connect_direct(&connect_addr, gap_address_type_random, 60, 76, 100, 1);
    }
			
    hThread = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            BLED112_Pump,       // thread function name
            NULL,          // argument to thread function 
            0,                      // use default creation flags 
            &dwThreadIdArray);   // returns the thread identifier 

	sts = 0;

	//Exit back to Python, message pump is running
	return PyLong_FromLong(sts);
}

DWORD WINAPI BLED112_Pump( LPVOID lpParam ) 
{ 
	
    // Message loop
    while (state != state_finish) {
		if (read_message(UART_TIMEOUT) > 0) break;
    }

    uart_close();
	printf("C Library closed\n");
	
	PyObject *arglist;
	PyObject *result;
	PyGILState_STATE gstate;
	int val = 1;
	
	gstate = PyGILState_Ensure();
	arglist = Py_BuildValue("(i)", val);
	result = PyEval_CallObject(op_callback, arglist);
	Py_DECREF(arglist);
	PyGILState_Release(gstate);

	return 0;
}

static PyMethodDef module_methods[] = {
   { "sendCMD", (PyCFunction)sendCMD, METH_VARARGS, NULL },
   { "openConnection", (PyCFunction)openConnection, METH_VARARGS, NULL },   
   { "closeConnection", (PyCFunction)closeConnection, METH_VARARGS, NULL },   
   { "send4Byte", (PyCFunction)send4Byte, METH_VARARGS, NULL },
   { "exitLibrary",(PyCFunction)exitLibrary, METH_VARARGS, NULL },
   { NULL, NULL, 0, NULL }
};

static struct PyModuleDef bluetooth =
{
    PyModuleDef_HEAD_INIT,
    "bluetooth", /* name of module */
    "usage: Combinations.uniqueCombinations(lstSortableItems, comboSize)\n", /* module documentation, may be NULL */
    -1,   /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    module_methods
};

PyMODINIT_FUNC PyInit_bluetooth(void)
{
    return PyModule_Create(&bluetooth);
}

void usage(char *exe)
{
    printf("%s <COMx|list> <scan|address>\n", exe);
}

void change_state(states new_state)
{
#ifdef DEBUG
    printf("DEBUG: State changed: %s --> %s\n", state_names[state], state_names[new_state]);
#endif
    state = new_state;
}

/**
 * Compare Bluetooth addresses
 *
 * @param first First address
 * @param second Second address
 * @return Zero if addresses are equal
 */
int cmp_bdaddr(bd_addr first, bd_addr second)
{
    int i;
    for (i = 0; i < sizeof(bd_addr); i++) {
        if (first.addr[i] != second.addr[i]) return 1;
    }
    return 0;
}

void print_bdaddr(bd_addr bdaddr)
{
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
            bdaddr.addr[5],
            bdaddr.addr[4],
            bdaddr.addr[3],
            bdaddr.addr[2],
            bdaddr.addr[1],
            bdaddr.addr[0]);
}

void print_raw_packet(struct ble_header *hdr, unsigned char *data)
{
    printf("Incoming packet: ");
    int i;
    for (i = 0; i < sizeof(*hdr); i++) {
        printf("%02x ", ((unsigned char *)hdr)[i]);
    }
    for (i = 0; i < hdr->lolen; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

void output(uint8 len1, uint8* data1, uint16 len2, uint8* data2)
{
    if (uart_tx(len1, data1) || uart_tx(len2, data2)) {
        printf("ERROR: Writing to serial port failed\n");
        exit(1);
    }
}

int read_message(int timeout_ms)
{
    unsigned char data[256]; // enough for BLE
    struct ble_header hdr;
    int r;

    r = uart_rx(sizeof(hdr), (unsigned char *)&hdr, UART_TIMEOUT);
    if (!r) {
        return -1; // timeout
    }
    else if (r < 0) {
        printf("ERROR: Reading header failed. Error code:%d\n", r);
        return 1;
    }

    if (hdr.lolen) {
        r = uart_rx(hdr.lolen, data, UART_TIMEOUT);
        if (r <= 0) {
            printf("ERROR: Reading data failed. Error code:%d\n", r);
            return 1;
        }
    }

    const struct ble_msg *msg = ble_get_msg_hdr(hdr);

#ifdef DEBUG
    print_raw_packet(&hdr, data);
#endif

    if (!msg) {
        printf("ERROR: Unknown message received\n");
        exit(1);
    }

    msg->handler(data);

    return 0;
}

void enable_notifications(uint8 con_handle, uint16 client_configuration_handle)
{
    uint8 configuration[] = {0x01, 0x00}; // enable notifications
	printf("Enable notify %d\n",client_configuration_handle+1);
    ble_cmd_attclient_attribute_write(con_handle, client_configuration_handle+1, 2, &configuration);
}

void ble_rsp_system_get_info(const struct ble_msg_system_get_info_rsp_t *msg)
{
    printf("Build: %u, protocol_version: %u, hardware: ", msg->build, msg->protocol_version);
    switch (msg->hw) {
    case 0x01: printf("BLE112"); break;
    case 0x02: printf("BLED112"); break;
    default: printf("Unknown");
    }
    printf("\n");

    if (action == action_info) change_state(state_finish);
}

void ble_evt_gap_scan_response(const struct ble_msg_gap_scan_response_evt_t *msg)
{
    if (found_devices_count >= MAX_DEVICES) change_state(state_finish);

    int i;
    char *name = NULL;

    // Check if this device already found
    for (i = 0; i < found_devices_count; i++) {
        if (!cmp_bdaddr(msg->sender, found_devices[i])) return;
    }
    found_devices_count++;
    memcpy(found_devices[i].addr, msg->sender.addr, sizeof(bd_addr));

    // Parse data
    for (i = 0; i < msg->data.len; ) {
        int8 len = msg->data.data[i++];
        if (!len) continue;
        if (i + len > msg->data.len) break; // not enough data
        uint8 type = msg->data.data[i++];
        switch (type) {
        case 0x09:
            name = malloc(len);
            memcpy(name, msg->data.data + i, len - 1);
            name[len - 1] = '\0';
        }

        i += len - 1;
    }

    print_bdaddr(msg->sender);
    printf(" RSSI:%d", msg->rssi);

    printf(" Name:");
    if (name) printf("%s", name);
    else printf("Unknown");
    printf("\n");

    free(name);
}

void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg)
{
	
	// New connection
    if (msg->flags & connection_connected) {
        change_state(state_connected);
        printf("Connected\n");
		connection_handle = msg->connection;
		if (skoobot_handle_cmd == 0)
		{
			change_state(state_finding_services);
			ble_cmd_attclient_read_by_group_type(msg->connection, FIRST_HANDLE, LAST_HANDLE, 2, group_uuid);
		}
	}
	//printf("Connection status %d\n",msg->flags);
}

void ble_evt_attclient_group_found(const struct ble_msg_attclient_group_found_evt_t *msg)
{
	//uint16 uuid;
	int found_skoobot_svc;
	
	found_skoobot_svc = 0;
	//printf("uuid len is %d\n",msg->uuid.len);
    if (msg->uuid.len == 0) return;
	//if (msg->uuid.len == 2)
		//uuid = (msg->uuid.data[7] << 8) | msg->uuid.data[6];
	if (msg->uuid.len == 16)
	{
		found_skoobot_svc = 1;
	}
	
    // First skoobot service found
    if (state == state_finding_services && found_skoobot_svc == 1 && skoobot_handle_start == 0) {
        skoobot_handle_start = msg->start;
        skoobot_handle_end = msg->end;
		//printf("start %d end %d\n",skoobot_handle_start,skoobot_handle_end);
    }
}

void ble_evt_attclient_procedure_completed(const struct ble_msg_attclient_procedure_completed_evt_t *msg)
{
	static int next_notify = 0;
	
    //printf("Finding services and attributes\n");
    if (state == state_finding_services) {
        // skoobot service not found
        if (skoobot_handle_start == 0) {
            printf("No Skoobot service found\n");
            change_state(state_finish);
        }
        // Find skoobot service attributes
        else {
            change_state(state_finding_attributes);
            ble_cmd_attclient_find_information(msg->connection, skoobot_handle_start, skoobot_handle_end);
			//ble_cmd_attclient_read_by_type(msg->connection, skoobot_handle_start, skoobot_handle_end, 2, type_uuid);
        }
    }
    else if (state == state_finding_attributes) {
        // Client characteristic configuration not found
        if (skoobot_handle_data == 0) {
            printf("No Client Characteristic Configuration found for Skoobot service\n");
            change_state(state_finish);
        }
        // Enable notifications
        else {
			switch(next_notify)
			{
				case 0:
					enable_notifications(msg->connection, skoobot_handle_data);
					break;
				case 1:
					if (skoobot_handle_byte2 !=0)
						enable_notifications(msg->connection, skoobot_handle_byte2);
					else
						printf("Error skoobot_handle_byte2 == 0");
					break;
				case 2:
					if (skoobot_handle_byte128 !=0)
						enable_notifications(msg->connection, skoobot_handle_byte128);
					else
						printf("Error skoobot_handle_byte128 == 0");
					break;
				case 3:
					change_state(state_listening_measurements);
					PyObject *arglist;
					PyObject *result;
					PyGILState_STATE gstate;
					int val = 1;
					
					gstate = PyGILState_Ensure();
					arglist = Py_BuildValue("(i)", val);
					result = PyEval_CallObject(op_callback, arglist);
					Py_DECREF(arglist);
					PyGILState_Release(gstate);
					break;
				default:
					break;
			}
			++next_notify;
			
        }
    }
}
	   
void ble_evt_attclient_find_information_found(const struct ble_msg_attclient_find_information_found_evt_t *msg)
{
	uint16 uuid = 0;
	//if (msg->uuid.len == 2) 
	//{
        //uint16 uuid = (msg->uuid.data[1] << 8) | msg->uuid.data[0];
		//printf("found uuid len 2 %x\n",uuid);
	//}	
	if (msg->uuid.len == 16)
	{	
        uuid = (msg->uuid.data[13] << 8) | msg->uuid.data[12];
		//printf("found uuid len 16 %x\n",uuid);
		
        if (uuid == SKOOBOT_COMMAND_UUID) {
            skoobot_handle_cmd = msg->chrhandle;
	      }
        else if (uuid == SKOOBOT_DATA_UUID) {
            skoobot_handle_data = msg->chrhandle;
        }
        else if (uuid == SKOOBOT_BYTE2_UUID) {
            skoobot_handle_byte2 = msg->chrhandle;
        }
        else if (uuid == SKOOBOT_BYTE4_UUID) {
            skoobot_handle_byte4 = msg->chrhandle;
        }
        else if (uuid == SKOOBOT_BYTE128_UUID) {
            skoobot_handle_byte128 = msg->chrhandle;
        }
    }
}

void ble_evt_attclient_attribute_value(const struct ble_msg_attclient_attribute_value_evt_t *msg)
{
    PyObject *arglist = NULL;
    PyObject *result = NULL;
	PyGILState_STATE gstate;
	char data[20];
	const char *p;
	int i;
	uint32 val = 0;
	
    //printf("len = %d\n",msg->value.len);
	if (msg->value.len < 1) {
        printf("Not enough fields in value");
        change_state(state_finish);
		return;
    }

	if (msg->value.len == 20)
	{		
		for(i=0;(i<20);i++)	
			data[i] = msg->value.data[i];
		p = data;
		gstate = PyGILState_Ensure();
		arglist = Py_BuildValue("(y#)", p, 20);
		if (arglist != NULL)
		{
			result = PyEval_CallObject(data_callback, arglist);
			Py_DECREF(arglist);
		}
		else
		{
			printf("arglist returned null\n");
		}
		PyGILState_Release(gstate);

		return;
	}
    if (msg->value.len == 1) 
	{
		val = msg->value.data[0];
	}
    if (msg->value.len == 2)
	{		
		data[0] = msg->value.data[0];
		data[1] = msg->value.data[1];
		val = data[0]<<8|data[1];
    }
  
	gstate = PyGILState_Ensure();
    arglist = Py_BuildValue("(i)", val);
    result = PyEval_CallObject(data_callback, arglist);
    Py_DECREF(arglist);
	PyGILState_Release(gstate);

	return;
}

void ble_evt_connection_disconnected(const struct ble_msg_connection_disconnected_evt_t *msg)
{
    change_state(state_disconnected);
    printf("Disconnected\n");
    //change_state(state_connecting);
    //ble_cmd_gap_connect_direct(&connect_addr, gap_address_type_public, 10, 40, 4000,0);
}

void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}