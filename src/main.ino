/*
Control a garden hose sprinkler with an ESP8266.
Copyright (C) 2018 Abe Kanofsky

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <time.h>    //provides ntp
#include <TimeAlarms.h>

#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include "index.h"

/*  config.h must definte the following:
        const char *ssid
        const char *password 
        const char *weather_url 
    See weather.ino for more details of weather_url 
*/
#include "config.h"

const int signal_pin = 10; //SD3

// The max amount of rain in the forecase (given in mm) after which
// the daily timer will not be triggered if weather checking is enabled.
int rain_limit;
// The number of days to check for rain. Max 5.
int days_to_check_weather;
// Whether or not the forecast predicts the above defined amount of rain
// in the next two days.
bool will_rain;
// Keep track of last forecast, to include with status info.
float last_forecast;

bool current_state = false;
bool daily_timer_enable;
bool weather_checking;

const int EEPROM_start_time_addr = 0;
const int EEPROM_end_time_addr = EEPROM_start_time_addr + 2;
const int EEPROM_timer_enabled_addr = EEPROM_end_time_addr + 2;
const int EEPROM_weather_checking_addr = EEPROM_timer_enabled_addr + 1;
const int EEPROM_weather_days_addr = EEPROM_weather_checking_addr + 1;
const int EEPROM_rain_limit_addr = EEPROM_weather_days_addr + 1;
// const int EEPROM_next_thing_addr = EEPROM_weather_checking_addr + ???;
// start hours | start mins | end hours | end mins | timer enabled | weather enabled | weather days | rain limit
const int EEPROM_size = EEPROM_rain_limit_addr + 1;

// Global pointers to the various alarms
AlarmID_t start_alarm;
AlarmID_t end_alarm;
AlarmID_t countdown_alarm;
// Unlike alarmRepeat, we cannot find the time when timerOnce will trigger,
// so we store it separately. This info is needed to update the ui.
time_t countdown_end_time = NULL;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Sets the on/off state of the hose, as well as the built-in LED
void set_state(bool state){
  current_state = state;
  
  if (current_state) {
    digitalWrite(signal_pin, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    digitalWrite(signal_pin, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    // if the countdown timer is active, setting the state to OFF will end the countdown 
    if (countdown_end_time) {
      end_countdown();
    }
  }
  broadcast_checked("state", state);
}

// Wait until sntp actually syncs and changes the time from the initial value
void wait_for_ntp_sync() {
  time_t t;
  while (time(NULL) < 1000000) {
  // needed to prevent watchdog timeout
  delay(0);
  }
  return;
}

// Initialize various variables based on values stored in EEPROM.
// Assumes that valid values have been previously stored in EEPOM.
// The actual on/off state is not stored because that should be reset at each boot.
void load_eeprom() {
  EEPROM.begin(EEPROM_size);
  time_t start_time = 3600 * EEPROM.read(EEPROM_start_time_addr) + 60 * EEPROM.read(EEPROM_start_time_addr+1);
  set_timer_time(true, start_time);
  time_t end_time = 3600 * EEPROM.read(EEPROM_end_time_addr) + 60 * EEPROM.read(EEPROM_end_time_addr+1);
  set_timer_time(false, end_time);
  set_timer_enable(EEPROM.read(EEPROM_timer_enabled_addr));
  set_weather_checking(EEPROM.read(EEPROM_weather_checking_addr));
  days_to_check_weather = EEPROM.read(EEPROM_weather_days_addr);
  rain_limit = EEPROM.read(EEPROM_rain_limit_addr);
}

void setup() {  
  pinMode(signal_pin, OUTPUT);
  digitalWrite(signal_pin, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Serial.begin(115200); Serial.println ( "" );
  
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
  }

  configTime(-4 * 3600, 0, "pool.ntp.org", "time.nist.gov");    //time.h
  wait_for_ntp_sync();
  setSyncProvider([] () -> long int {return time(NULL);});   //Time.h, included with TimeAlarms.h
  setSyncInterval(3600);         //Time.h, included with TimeAlarms.h

  load_eeprom();

  server.on("/", handleRoot);
  server.on("/status", sendStatus);
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  update_weather();
  Alarm.alarmRepeat(0, update_weather);
}

void loop() {
  server.handleClient();
  webSocket.loop();
  Alarm.delay(0);
}
