/*
Provides ability to check the weather forecast using the OpenWeather API.
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

// Forecast is obtained in 3-hour intervals.
const int intervals_to_check = days_to_check_weather * 8;

/*  Check forecast and update the will_rain variable. Assumes that
    weather_url a global variable, set to a valid url for the free
    OpenWeatherMap (including a valid API key). This function is set
    up to parse the 5-day/3-hour forecast, delivered in XML format.
     */
void update_weather() {
  HTTPClient http;
  http.begin(weather_url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    return;
  }

  WiFiClient * stream = http.getStreamPtr();
  char buffer[64] = {};
  int len = http.getSize();
  float total_rain = 0;
  int intervals_checked = 0;

  while (http.connected() && (intervals_checked < intervals_to_check)) {
    if(stream->available()) {
      int c = stream->readBytesUntil('>', buffer, 64);
      if(strncmp(buffer, "<precipitation", 14) == 0) {
        intervals_checked++;
        char *value_ptr = strstr(buffer, "value=\"");
        if (value_ptr) {
          total_rain += atof(value_ptr + 7);
        }
      }
    }
    Alarm.delay(0);
  }
  http.end();

  will_rain = total_rain >= rain_limit;
  last_forecast = total_rain;
}

void set_weather_checking(bool en) {
  weather_checking = en;
  broadcast_checked("weather-enabled", en);

  EEPROM.write(EEPROM_weather_checking_addr, en);
  EEPROM.commit();
}