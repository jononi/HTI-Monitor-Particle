/* THI: temperature, humidity and illuminance monitoring
Phase 1: real time monitoring on the cloud without logging
Phase 2: add storage (logging) on the cloud
Phase 3: clean up the code, account counters on the cloud *
Phase 4: add display when Nextion HMI is connected
hardware:
Particle Photon IoT Dev board
HTU21DF I2C temperature and humidity sensor in Adafruit's breakout board
TSL2561 I2C illuminance sensor in Adafruit's breakout board
Nextion HMI LCD+touch 2.4" panel and UART interface from Itead Studios

Software:
Libraries: HTU21D, TSL2561, http request
IoT platform: Ubidots & Particle....

Jaafar Ben-Abdallah July - October 2015

Progress:
testing cloud on/off function and http counter

*/

// imports, definitions and global variables go here:
#include "HTImonitor.h"

void setup()
{
  // initialize sensors and get status
  tsl_opsOn = enableTSL();
  htu_opsOn = enableHTU();
  sprintf(status_c,"TSL: %i, HTU: %i",tsl_opsOn,htu_opsOn);

  // publish initialization status report: always enabled
  if (tsl_opsOn && htu_opsOn) {
    Particle.publish("Status","On");
  }
  else {
    Particle.publish("Status","Init. Problem");
  }

  // by default, publish to particle cloud is on, http requests are off
  ubidotsOn = false;
  particleOn = true;
  //sprintf(cloud_settings,"ubidots: %d, particle: %d",ubidotsOn,particleOn);
  sprintf(cloud_settings,"free mem= %i",System.freeMemory());
  // cloud variables (max 10)
  Particle.variable("illuminance", &illuminance, DOUBLE);
  Particle.variable("humidity", &humidity, DOUBLE);
  Particle.variable("temperature", &temperatureC, DOUBLE);
  Particle.variable("status", status_c, STRING);
  Particle.variable("tsl_settings",tslSettings, STRING);
  Particle.variable("dots_count",response_c,STRING);
  Particle.variable("cloud_status",cloud_settings,STRING);

  // function on the cloud: change light sensor exposure settings (max 4)
  Particle.function("setExposure", setExposure);
  Particle.function("setCloud", setActiveCloud);

  // ubidots connection params
  ubiRequest.hostname = ubiHostName;
  ubiRequest.port = 80;
  ubiRequest.path = "/api/v1.6/collections/values";

  // future: make it switched by compiler directive
  if (serial_on){
    // debug: serial for ubihttp response debug
    Serial.begin(115200);
    Serial.println("start");
  }

  // start timer
  startTime = millis();
}


void loop()
{
  // get illuminance value if sensor is on
  if(tsl_opsOn){
    // get raw data
    if (tsl.getData(broadband,ir,autoGainOn)){
      // now get the illuminance value
      tsl.getLux(integrationTime,broadband,ir,illuminance);
      // print current settings to cloud variable
      //printTSLsettings(tslSettings);
    }
    // problem with reading, restart sensor, one attempt only
    else {
      illuminance = -1;
      sprintf(status_c,"tsl get data:%i",tsl.getError());
      tsl.setPowerDown();
      delay(100);

      //try a restart
      if (enableTSL()){
        // good: clear sensor for operation again
        tsl_opsOn = true;
        sprintf(status_c,"tsl restored:%i",tsl.getError());
      }
      // didn't work, disable sensor
      else if (auto_reset_on == true ) {
        // publish this event first
        Particle.publish("THI","reset tsl");
        delay(2000);
        // restart the Photon
        System.reset();
      }
      else {
        // just stop operating this sensor
        tsl_opsOn = false;
        sprintf(status_c,"tsl abort");
      }
    }
  }


  // get T/H values if sensor is operational

  if (htu_opsOn){
    // get data
    humidity = htu.readHumidity();
    temperatureC = htu.readTemperature();
    //check if there  was I2C comm errors
    if (htu.getError()){
      // display error code
      sprintf(status_c,"htu get data:%i",htu.getError());
      // try a fix
      if (enableHTU()) {
        // good: clear sensor for operation again
        htu_opsOn = true;
        sprintf(status_c,"htu restored:%i",htu.getError());
      }
      else if (auto_reset_on == true){
        // publish this event first
        Particle.publish("THI","reset htu");
        delay(2000);
        System.reset();
      }
      else {
        //didn't work, disable sensor
        htu_opsOn = false;
        sprintf(status_c,"htu abort");
      }
    }
  }

  // update sensor status
  sprintf(status_c,"TSL: %i, HTU: %i",tsl_opsOn,htu_opsOn);

  //publish values every publishPeriod/1000 seconds

  if ((millis() - startTime) > publishPeriod) {
    // if sensors operational publish measurements
    // else publish status if a sensor has a problem

    startTime = millis();//reset timer

    // as of current firmware,sprintf doesn't format float correctly
    // following line is not formatted correctly
    // sprintf(data_c,"%.1f,%.1f,%.1f",temperatureC,humidity,illuminance);
    // in the meantime, I use String class for a workaround
    // to convert float to ascii ten store
    if (htu_opsOn) {
      strcpy(temp_c,String(temperatureC,1).c_str());
      strcpy(hum_c,String(humidity,1).c_str());
    }
    else {
      // always publish a down event ()
      Particle.publish("THI","htu error");
    }

    if (tsl_opsOn){
      strcpy(ill_c,String(illuminance,1).c_str());
    }
    else {
      // always publish a down event ()
      Particle.publish("THI","tsl error");
    }
    // only if publish to Particle cloud is enabled
    if (tsl_opsOn && htu_opsOn && particleOn){
      // now append it to the published string
      strcpy(data_c,"Celsius=");
      strcat(data_c,temp_c);
      strcat(data_c,", RH=");
      strcat(data_c,hum_c);
      strcat(data_c,", Lux=");
      strcat(data_c,ill_c);

      Particle.publish("THI",data_c);

      if (serial_on){
        Serial.println(data_c);
      }
    }

    // send http POST request to ubidots api:

    // 1- construct the data section of the request
    char ubiReqData[168];

    /* example with two variables:
      [{"variable": "55cb5ab87625425251017c77", "value":26.3},
      {"variable": "55cb5ab87625425251017c77", "value":43.7}]
    */

    strcpy(ubiReqData,"[");
    // post requests only if sensor is on
    if (htu_opsOn){
      buildVarData(ubiTempVarId, temp_c, data_c );//re-using data_c
      strcat(ubiReqData,data_c);
      strcat(ubiReqData,",");
      buildVarData(ubiHumVarId, hum_c, data_c );
      strcat(ubiReqData,data_c);
    }

    if (htu_opsOn && tsl_opsOn) {
      strcat(ubiReqData,",");
    }

    if (tsl_opsOn){
      buildVarData(ubiIllVarId, ill_c, data_c );
      strcat(ubiReqData,data_c);
    }

    strcat(ubiReqData,"]");

    ubiRequest.body = ubiReqData;

    // 2- POST
    http.post(ubiRequest,ubiResponse,ubiHeaders);
    dots_sent++;

    // 3- get server response code
    if (ubiResponse.status==200)
      dots_successful++;

    sprintf(response_c,"%d/%d",dots_successful,dots_sent);

    // send server response it to Serial
    if (serial_on){
      // let's see this hippo
      Serial.println(ubiReqData);
      // let's see server response
      Serial.println(ubiResponse.status);
      Serial.println(ubiResponse.body);
    }
  }
  // do all this every sensingPeriod ms
  delay(sensingPeriod);
}


bool enableTSL(){

  // (re-)start Wire periph and check sensor is connected
  if(!tsl.begin()){
    sprintf(status_c,"tsl Begin:%i",tsl.getError());
    return false;
  }

  // x1 gain, 101ms integration time
  if(!tsl.setTiming(false,1,integrationTime))
  {
    sprintf(status_c,"tsl SetTiming:%i",tsl.getError());
    return false;
  }

  if (!tsl.setPowerUp()){
    sprintf(status_c,"tsl PowerUp:%i",tsl.getError());
    return false;
  }

  // enable auto gain by default
  autoGainOn = true;
  return true;
}

bool enableHTU(){
  if(!htu.begin()){
    sprintf(status_c,"htu Begin:%i",htu.getError());
    return false;
  }
  return true;
}

void printTSLsettings(char *buffer)
{
  if (!tsl._gain)
    sprintf(buffer,"G:x1, IT: %i ms, %i",integrationTime,autoGainOn);
  else if (tsl._gain)
    sprintf(buffer,"G:x16, IT: %i ms,%i",integrationTime,autoGainOn);

}

// cloud function to change exposure settings (gain and integration time)
int setExposure(String command)
//command is expected to be "gain={0,1,2},integrationTimeSwitch={0,1,2}"
// gain = 0:x1, 1: x16, 2: auto
// integrationTimeSwitch: 0: 14ms, 1: 101ms, 2:402ms
{
    // private vars
    char gainInput;
    uint8_t itSwitchInput;
    boolean setTimingReturn = false;

    // extract gain as char and integrationTime swithc as byte
    gainInput = command.charAt(0);//we expect 0, 1 or 2
    itSwitchInput = command.charAt(2) - '0';//we expect 0,1 or 2

    if (itSwitchInput >= 0 && itSwitchInput < 3){
      // acceptable integration time value, now check gain value
      if (gainInput=='0'){
        setTimingReturn = tsl.setTiming(false,itSwitchInput,integrationTime);
        autoGainOn = false;
      }
      else if (gainInput=='1') {
        setTimingReturn = tsl.setTiming(true,itSwitchInput,integrationTime);
        autoGainOn = false;
      }
      else if (gainInput=='2') {
        autoGainOn = true;
        // when auto gain is enabled, set starting gain to x16
        setTimingReturn = tsl.setTiming(true,itSwitchInput,integrationTime);
      }
      else{
        // no valid settings, raise error flag
        setTimingReturn = false;
      }
    }
    else{
      setTimingReturn = false;
    }

    // setTiming has an error
    if(!setTimingReturn){
        //disable getting illuminance value
        tsl_opsOn = false;
        return -1;
    }
    else {
      // all is good
      tsl_opsOn = true;
      return 0;
    }
}

//cloud function to control whee data is sent
int setActiveCloud(String command)
// valid values: "ubidots", "particle", "both" or any other value for no publishing
{
  // by default disable all
  ubidotsOn = false;
  particleOn = false;

  if (command.equals("ubidots") || command.equals("both"))
    ubidotsOn = true;
  if (command.equals("particle") || command.equals("both"))
    particleOn = true;

  // set the related variable
  sprintf(cloud_settings,"ubidots: %i, particle: %i",ubidotsOn,particleOn);
  return 0;
}

// build one leg of the http request data
void buildVarData(const char *varId, char *value, char *result)
{
  /*
  what it looks like:
  {"variable":"ab4538762542525fec77", "value":26.3}
  */

  strcpy(result,"{\"variable\":\"");
  strcat(result, varId);
  strcat(result,"\", \"value\":");
  strcat(result, value);
  strcat(result,"}");
}
