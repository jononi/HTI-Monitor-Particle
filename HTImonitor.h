/*

Header file with imports and definitions

*/

/*
      Libraries
*/

#include "HTU21D.h"
#include "tsl2561.h"
#include "HttpClient_mojtabacazi.h"//this is a faster http response version that also appends conten-length to header


/*
      Definitions
*/

#define sensingPeriod  2000  //defines frequency of measurements in ms
#define publishPeriod  30000 //defines frequency of publishing to the cloud (Particle, Ubidots...) in ms (min 1000)

// cloud services API params
// Ubidots
#define ubiHostName "things.ubidots.com"
#define ubiToken "your_ubidots_token_here"
#define ubiTempVarId "temperature_variable_id_ubidots"
#define ubiHumVarId "humidity_variable_id_ubidots"
#define ubiIllVarId "illuminance_variable_id_ubidots"




/*
      Variables
*/

// print info to Serial print switch for debug purpose
bool serial_on = false;
// allow auto-reset when sensor is stalled (experimental)
bool auto_reset_on = false;

// sensors objects

TSL2561 tsl = TSL2561(TSL2561_ADDR);
HTU21D htu = HTU21D();

// status report and execution control
// sensors controls
bool tsl_opsOn = false;
bool htu_opsOn = false;
// variables to display these settings
char status_c[43];
char cloud_settings[21]="NA";
// timer variable
uint32_t startTime;

// Publish data switches, make Particle cloud on by default, ubidots off
bool particleOn = true;
bool ubidotsOn = false;

// Light sensor vars
uint16_t integrationTime;
bool autoGainOn;
char tslSettings[21] = "NA";
uint16_t broadband, ir;
double illuminance;

// TH sensor vars
double temperatureC;
double humidity;

// data publishing vars
char data_c[63];
char response_c[21] = "0/0";
char temp_c[6];//i.e. 27.6
char hum_c[6];//i.e. 48.9
char ill_c[8];//i.e. 8645.5
// init dots counters
int dots_sent = 0;
int dots_successful=0;


// http request variables
HttpClient http;

// ubidots API header  (validated)

http_header_t ubiHeaders[] ={
  { "Content-Type", "application/json" },
  { "X-Auth-Token" , ubiToken },
  { NULL, NULL } // NOTE: Always terminate headers with NULL
};

http_request_t ubiRequest;
http_response_t ubiResponse;
