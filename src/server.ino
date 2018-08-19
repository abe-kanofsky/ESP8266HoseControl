/*
Server functionality to serve main HTML page and communicate via Web Sockets.
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

// This method is called each time a new WebSocket is opened,
// and it updates all attached Web Sockets.
void initialize_page() {
  broadcast_checked("state", current_state);
  broadcast_checked("timer-enable", daily_timer_enable);
  broadcast_checked("weather-enabled", weather_checking);

  webSocket.broadcastTXT( "start-time/value/" + seconds_to_string(Alarm.read(start_alarm))); 
  webSocket.broadcastTXT( "end-time/value/" + seconds_to_string(Alarm.read(end_alarm))); 
  webSocket.broadcastTXT("countdown-time-left/" + String(countdown_end_time - time(NULL)));
  webSocket.broadcastTXT("weather-days/value/" + String(days_to_check_weather));
  webSocket.broadcastTXT("max-rain/value/" + String(rain_limit));
}

// Send the main control page
void handleRoot() {
  if(server.args() == 0){
    String page = PAGE;
    server.sendHeader("Access-Control-Allow-Origin", "*", false);
    server.send(200, "text/html", page);
    return;
  }
}

// Send certain pieces of data when requested. Usefult for debugging.
void sendStatus() {
  String response = ""
  "Current time: " + seconds_to_string(time(NULL)) + ":" + String(second()) + "\n"
  "Start time: " + seconds_to_string(Alarm.read(start_alarm)) + "\n"
  "End time: " + seconds_to_string(Alarm.read(end_alarm)) + "\n"
  "Days to check weather: " + String(days_to_check_weather) + "\n"
  "Rain limit: " + String(rain_limit) + "\n"
  "Last forecast :" + String(last_forecast) + "mm";
  
  server.send(200, "text/text", response);
}

// Handle Web Socket event
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
      {
        initialize_page();
        IPAddress ip = webSocket.remoteIP(num);
      }
      break;
    case WStype_TEXT:
      if (strcmp((char *) payload, "ping") == 0) {
        webSocket.sendTXT(num, "pong");
      } else {
       process_websocket_update(payload); 
      }
      break;
  }
}

// Process an update sent by a WebSocket. Messages should be
// of the form field/value
void process_websocket_update(uint8_t *text) {
  char *token = strtok((char *) text, "/");
  char *value = strtok(NULL, "/");
  if (strcmp(token, "state") == 0) {
    if (strcmp(value, "true") == 0) {
      set_state(true);
    } else if (strcmp(value, "false") == 0) {
      set_state(false);
    }
  } else if (strcmp(token, "timer-enable") == 0) {
    if (strcmp(value, "true") == 0) {
      set_timer_enable(true);
    } else if (strcmp(value, "false") == 0) {
      set_timer_enable(false);
    }
  } else if (strcmp(token, "weather-enabled") == 0) {
    if (strcmp(value, "true") == 0) {
      set_weather_checking(true);
    } else if (strcmp(value, "false") == 0) {
      set_weather_checking(false);
    }
  } else if (strcmp(token, "start-time") == 0) {
    set_timer_time(true, value);
  } else if (strcmp(token, "end-time") == 0) {
    set_timer_time(false, value);
  } else if (strcmp(token, "add-time") == 0) {
    add_time(value);
  } else if(strcmp(token, "weather-days") == 0) {
    days_to_check_weather = atoi(value);
    webSocket.broadcastTXT("weather-days/value/" + String(days_to_check_weather));
    EEPROM.write(EEPROM_weather_days_addr, atoi(value));
    EEPROM.commit();
    update_weather();
  } else if(strcmp(token, "max-rain") == 0) {
    rain_limit = atoi(value);
    webSocket.broadcastTXT("max-rain/value/" + String(rain_limit));
    EEPROM.write(EEPROM_rain_limit_addr, atoi(value));
    EEPROM.commit();
    update_weather();
  }
}

// Broadcast the state of a boolean variable.
void broadcast_checked(String name, bool checked) {
  String s = checked ? "true" : "";
  webSocket.broadcastTXT(name + "/checked/" + s);
}