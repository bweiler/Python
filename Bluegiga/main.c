// This is based on BlueGiga's code for the BLED112 USB dongle with BLE 4.0
// The author for the modofications is Bill Weiler
// The intent is to provide a Bluetooth Python module for Skoobot Python apps 
//
// This is free software distributed under the terms of the MIT license reproduced below.

#include <Python.h>
#include <unistd.h>
#include <stdlib.h>
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

uint8 primary_service_uuid[] = {0x00, 0x28};

uint16 skoobot_handle_start = 0,
       skoobot_handle_end = 0,
       skoobot_handle_measurement = 0,
       skoobot_handle_configuration = 0;

bd_addr connect_addr;

void output(uint8 len1, uint8* data1, uint16 len2, uint8* data2);
void change_state(states new_state);
int read_message(int timeout_ms);

static PyObject *sendCMD(PyObject *self, PyObject *args );
static PyObject *openConnection(PyObject *self, PyObject *args );

static PyObject *sendCMD(PyObject *self, PyObject *args) 
{
    const char cmd;
	int sts;

    if (!PyArg_ParseTuple(args, "b", &cmd))
        return NULL;
	sts = cmd;

	return PyLong_FromLong(sts);;
}

static PyObject *openConnection(PyObject *self, PyObject *args) 
{
	char *uart_port;
	int sts, i;

    if (!PyArg_ParseTuple(args, "s", &uart_port))
        return NULL;

	printf("COM %s\n",uart_port);
	
	action = action_info;
	action = action_connect;

	for (i = 0; i < 6; i++) {
		connect_addr.addr[i] = addr[i];
	}
	
    bglib_output = output;

    if (uart_open(uart_port)) {
        printf("ERROR: Unable to open serial port\n");
        sts = 1;
		return PyLong_FromLong(sts);
    }

    // Reset dongle to get it into known state
    ble_cmd_system_reset(0);
    uart_close();
    do {
        usleep(500000); // 0.5s
    } while (uart_open(uart_port));

	action = action_scan;
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
        ble_cmd_gap_connect_direct(&connect_addr, gap_address_type_public, 40, 60, 100,0);
    }

    // Message loop
    while (state != state_finish) {
        if (read_message(UART_TIMEOUT) > 0) break;
    }

    uart_close();

	sts = 0;

	return PyLong_FromLong(sts);
}

static PyMethodDef module_methods[] = {
   { "sendCMD", (PyCFunction)sendCMD, METH_VARARGS, NULL },
   { "openConnection", (PyCFunction)openConnection, METH_VARARGS, NULL },
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
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
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

void enable_indications(uint8 connection_handle, uint16 client_configuration_handle)
{
    uint8 configuration[] = {0x02, 0x00}; // enable indications
    ble_cmd_attclient_attribute_write(connection_handle, skoobot_handle_configuration, 2, &configuration);
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
    printf(" RSSI:%u", msg->rssi);

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

        // Handle for Temperature Measurement configuration already known
        if (skoobot_handle_configuration) {
            change_state(state_listening_measurements);
            enable_indications(msg->connection, skoobot_handle_configuration);
        }
        // Find primary services
        else {
            change_state(state_finding_services);
            ble_cmd_attclient_read_by_group_type(msg->connection, FIRST_HANDLE, LAST_HANDLE, 2, primary_service_uuid);
        }
    }
}

void ble_evt_attclient_group_found(const struct ble_msg_attclient_group_found_evt_t *msg)
{
    if (msg->uuid.len == 0) return;
    uint16 uuid = (msg->uuid.data[1] << 8) | msg->uuid.data[0];

    // First skoobot service found
    if (state == state_finding_services && uuid == SKOOBOT_SERVICE_UUID && skoobot_handle_start == 0) {
        skoobot_handle_start = msg->start;
        skoobot_handle_end = msg->end;
    }
}

void ble_evt_attclient_procedure_completed(const struct ble_msg_attclient_procedure_completed_evt_t *msg)
{
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
        }
    }
    else if (state == state_finding_attributes) {
        // Client characteristic configuration not found
        if (skoobot_handle_configuration == 0) {
            printf("No Client Characteristic Configuration found for Skoobot service\n");
            change_state(state_finish);
        }
        // Enable notifications
        else {
            change_state(state_listening_measurements);
            enable_indications(msg->connection, skoobot_handle_configuration);
        }
    }
}

void ble_evt_attclient_find_information_found(const struct ble_msg_attclient_find_information_found_evt_t *msg)
{
    if (msg->uuid.len == 2) {
        uint16 uuid = (msg->uuid.data[1] << 8) | msg->uuid.data[0];

        if (uuid == SKOOBOT_COMMAND_UUID) {
            skoobot_handle_measurement = msg->chrhandle;
        }
        else if (uuid == SKOOBOT_DATA_UUID) {
            skoobot_handle_configuration = msg->chrhandle;
        }
    }
}

void ble_evt_attclient_attribute_value(const struct ble_msg_attclient_attribute_value_evt_t *msg)
{
    if (msg->value.len < 1) {
        printf("Not enough fields in value");
        change_state(state_finish);
    }

    uint8 data = msg->value.data[0];
    //msg->value.data[4];
    //(msg->value.data[3] << 16) | (msg->value.data[2] << 8) | msg->value.data[1];

  
    printf("Data is %u\n",data);
}

void ble_evt_connection_disconnected(const struct ble_msg_connection_disconnected_evt_t *msg)
{
    change_state(state_disconnected);
    printf("Connection terminated, trying to reconnect\n");
    change_state(state_connecting);
    ble_cmd_gap_connect_direct(&connect_addr, gap_address_type_public, 40, 60, 100,0);
}

