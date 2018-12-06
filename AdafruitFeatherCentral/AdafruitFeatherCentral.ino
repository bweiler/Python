/* Adafruit Feather nRF52 nRF52832
 *  
 * You have to go on Adafruit's site and load their JSON file into Arduino to load their library.
 * 
 * Then just load this Arduino code on the dongle and your good to go! 
 */
#include <bluefruit.h>

//This are packet tag bytes sent from host
//They are the first byte read from serial (at offset 0)
//currently just CMD and DATA4 are used
#define CMD    0
#define DATA1  1
#define DATA2  2
#define DATA4  3
#define DATA20 4
#define STR    5

//COMMAND SET FOR SKOOBOT
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
#define DO_DFT              0x51
#define PLAY_BUZZER         0x17

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

uint8_t SKOOBOT_SERVICE_UUID_128[] = { 0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x23, 0x15, 0x00, 0x00 };
uint8_t SKOOBOT_CMD_UUID_128[] = { 0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x25, 0x15, 0x00, 0x00 };
uint8_t SKOOBOT_DATA_UUID_128[] = { 0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x24, 0x15, 0x00, 0x00 };
uint8_t SKOOBOT_DATA20_UUID_128[] = { 0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x27, 0x15, 0x00, 0x00 };
uint8_t SKOOBOT_DATA2_UUID_128[] = { 0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x26, 0x15, 0x00, 0x00 };
uint8_t SKOOBOT_DATA4_UUID_128[] = { 0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x28, 0x15, 0x00, 0x00 };
uint8_t SKOOBOT_COMPLETE_LOCAL_NAME[] =  {'S','k','o','o','b','o','t' };  

//Put function prototypes here but Arduino doesn't need them
void scan_callback(ble_gap_evt_adv_report_t* report);
void connect_callback(uint16_t conn_handle);
void skoobot_data_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);

BLEClientService        skoobot_svc(SKOOBOT_SERVICE_UUID_128);
BLEClientCharacteristic skoobot_cmd(SKOOBOT_CMD_UUID_128);
BLEClientCharacteristic skoobot_data(SKOOBOT_DATA_UUID_128);
BLEClientCharacteristic skoobot_data20(SKOOBOT_DATA20_UUID_128);
BLEClientCharacteristic skoobot_data2(SKOOBOT_DATA2_UUID_128);
BLEClientCharacteristic skoobot_data4(SKOOBOT_DATA4_UUID_128);
uint16_t conn_handle_skoobot = 0;
uint8_t connected_and_ready = 0, sendbytes[4];
int32_t res;
uint8_t notify_result = 0;

void setup()
{
  //Python will talk to Feather over serial
  Serial.begin(115200);
  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  // SRAM usage required by SoftDevice will increase dramatically with number of connections
  
  Bluefruit.begin(0, 1);

  Bluefruit.setName("Skoobot Central"); //This is Feather's name

  // Initialize  client
  skoobot_svc.begin();

  // Initialize client characteristics.
  skoobot_cmd.begin();

  // set up callback for receiving distance
  skoobot_data.setNotifyCallback(skoobot_data_notify_callback);
  skoobot_data.begin();
  
  // set up callback for receiving 20 byte packets, sound file
  skoobot_data20.setNotifyCallback(skoobot_data_notify20_callback);
  skoobot_data20.begin();

  // set up callback for receiving 2 byte packets of ambient light in LUX
  skoobot_data2.setNotifyCallback(skoobot_data_notify2_callback);
  skoobot_data2.begin();
  
  skoobot_data4.begin();
  
  // Increase Blink rate to different from PrPh advertising mode
  Bluefruit.setConnLedInterval(60);

  // Callbacks for Central
  Bluefruit.Central.setConnInterval(40, 100);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);
  Bluefruit.Central.setConnectCallback(connect_callback);
  /* Start Central Scanning
   * - Enable auto scan if disconnected
   * - Start(timeout) with timeout = 0 will scan forever (until connected)
   */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80); // in unit of 0.625 ms
  //Bluefruit.Scanner.filterUuid(skoobot_svc.uuid);
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);                   // // 0 = Don't stop scanning after n seconds
}

//When powered up setup() connects to Skoobot immediately, then loop() watches for commands over serial
void loop()
{
  if (connected_and_ready == 1)
  {
      //This loop for recieving command on serial and then sending over BLE to Skoobot
      if (Serial.available() > 0) 
      {
        // read the incoming byte:
        Serial.readBytes(sendbytes,1);
        if (sendbytes[0] == CMD)
        {
           Serial.readBytes(sendbytes,1);
           notify_result = sendbytes[0]; //only used to tag distance or ambient
           skoobot_cmd.write8(sendbytes[0]);
        }        
        if (sendbytes[0] == DATA4)      //currently only setting speed is a 4 byte command
        {
           Serial.readBytes(sendbytes,4);
           notify_result = sendbytes[0]; //tag it here, but only distance or ambient are recognized
           skoobot_data4.write(sendbytes,4);
        }        
      }
  }
  else
  {
    //kind of hang out when not connected
    delay(50);
  }
}

/**
 * Callback invoked when scanner pick up an advertising data
 * Matches 'Skoobot' BLE name
 */
void scan_callback(ble_gap_evt_adv_report_t* report)
{ 

//  printReport( report ); 
  
  /* Choose a peripheral to connect with by searching for an advertisement packet with a 
  Complete Local Name matching our target device*/
  uint8_t buffer[BLE_GAP_ADV_SET_DATA_SIZE_MAX] = { 0 };


  if(Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, buffer, sizeof(buffer)))
  {
    if ( !memcmp( buffer, SKOOBOT_COMPLETE_LOCAL_NAME, sizeof(SKOOBOT_COMPLETE_LOCAL_NAME)) )
    {
      Bluefruit.Central.connect(report);
    }
    else
    {
      Bluefruit.Scanner.resume(); // continue scanning
    } 
  } 
  else
  {
    // For Softdevice v6: after received a report, scanner will be paused
    // We need to call Scanner resume() to continue scanning
    Bluefruit.Scanner.resume();
  }  
}


/**
 * Callback invoked when an connection is established
 * Will connect up service and characteristics, turn on notifications
 */
void connect_callback(uint16_t conn_handle)
{
  /* Complete Local Name */
  uint8_t buffer[BLE_GAP_ADV_SET_DATA_SIZE_MAX] = { 0 };
  
  conn_handle_skoobot = conn_handle;
  // If Skoobot is not found, disconnect and return
  if ( !skoobot_svc.discover(conn_handle) )
  {
    // disconect since we couldn't find Skoobot service
    Bluefruit.Central.disconnect(conn_handle);
    return;
  }

  if ( !skoobot_cmd.discover() )
  {
    Bluefruit.Central.disconnect(conn_handle);
    return;
  }

  if ( skoobot_data.discover() )
  {
    // Reaching here means we are ready to go, let's enable notification on distance
    skoobot_data.enableNotify();
  }
  if ( skoobot_data2.discover() )
  {
    // Reaching here means we are ready to go, let's enable notification on ambient
    skoobot_data2.enableNotify();
  }
  if ( !skoobot_data4.discover() )
  {
    //Is messed up
  }
  if ( skoobot_data20.discover() )
  {
    // Reaching here means we are ready to go, let's enable notification on 20 byte
    skoobot_data20.enableNotify();
  }
  connected_and_ready = 1;
}

/**
 * Callback invoked when a connection is dropped
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  //Serial.println("Disconnected");
}


//Used for distance (sound file upload flags currently not used)
void skoobot_data_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
   int res_sent;
   
   if (len == 1 && notify_result == GET_DISTANCE)
     res_sent = Serial.write(data,len);
}

//Used for uploading sound file, eventually dft too
void skoobot_data_notify20_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
   int res_sent;
   
   if (len == 20 && notify_result == RECORD_SOUND)
     res_sent = Serial.write(data,len);
}

//Used for uploading ambient light value
void skoobot_data_notify2_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
   int res_sent;
   
   if (len == 2 && notify_result == GET_AMBIENT)
     res_sent = Serial.write(data,len);
}

//Not used, good for debugging
void printHexList(uint8_t* buffer, uint8_t len)
{  
  // print forward order
  for(int i=0; i<len; i++)
  { 
    Serial.printf("%02X-", buffer[i]);
  } 
  Serial.println();  
}

//Not used, good for debugging
void printReport( const ble_gap_evt_adv_report_t* report )
{
  Serial.print( "  rssi: " );
  Serial.println( report->rssi );
  Serial.print( "  scan_rsp: " );
  Serial.println( report->type.scan_response );
  Serial.print( "  type: " );
  //Serial.println( report->type. );
  Serial.print( "  dlen: " );
  Serial.println( report->data.len );  
  Serial.print( "  data: " );
  for( int i = 0; i < report->data.len; i+= sizeof(uint8_t) )
  {
    Serial.printf( "%02X-", report->data.p_data[ i ] );
  }
  Serial.println(""); 
}

