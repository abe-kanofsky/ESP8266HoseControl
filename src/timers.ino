/*
Daily and countdown timer functionality.
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

/*  Set the time of the daily timer.
    bool start_time: whether or not we are setting the daily start time
        (as opposed to the daily end time).
    time_t alarm_secs: daily time of the alarm, measure in seconds after midnight.
*/
void set_timer_time(bool start_time, time_t alarm_secs) {
  String var_name = start_time ? "start-time" : "end-time";
  webSocket.broadcastTXT( var_name + "/value/" + seconds_to_string(alarm_secs));
  
  // get pointer to the appropriate global variable 
  AlarmID_t *alarm_ptr = start_time ? &start_alarm : &end_alarm;
  OnTick_t callback = start_time ? []{timer_set_state(true);} : []{timer_set_state(false);};

  if(*alarm_ptr) {
    // free the old alarm
    Alarm.free(*alarm_ptr);
  }
  AlarmID_t new_alarm = Alarm.alarmRepeat(alarm_secs, callback);
  memcpy(alarm_ptr, &new_alarm, sizeof(AlarmID_t));


  int eeprom_addr = start_time ? EEPROM_start_time_addr : EEPROM_end_time_addr;
  EEPROM.write(eeprom_addr, hour(alarm_secs));
  EEPROM.write(eeprom_addr+1, minute(alarm_secs));
  EEPROM.commit();
}

// Overloaded function
void set_timer_time(bool start_time, String input) {
  char *t = const_cast<char*>(input.c_str());
  char *h = strtok( t, ":");
  char *m = strtok(NULL, ":");
  time_t alarm_secs = 3600 * atoi(h) + 60 * atoi(m);

  set_timer_time(start_time, alarm_secs);
}

// wrapper for set_state() that first performs weather checking, if weather checking is enabled
void timer_set_state(bool en) {
  if (daily_timer_enable && (!weather_checking || !will_rain)) {
    set_state(en);
  }
}

void set_timer_enable(bool en) {
  daily_timer_enable = en;
  if (en) {
    Alarm.enable(start_alarm);
    Alarm.enable(end_alarm);
  } else {
    Alarm.disable(start_alarm);
    Alarm.disable(end_alarm);
  }
  broadcast_checked("timer-enable", en);

  EEPROM.write(EEPROM_timer_enabled_addr, en);
  EEPROM.commit();
}

// Add time to the countdown timer. If countdown timer doesn't exist, create it.
void add_time(String mins) {
  set_state(true);
  Alarm.free(countdown_alarm);

  int secs = 60 * mins.toInt();
  // time left on existing countdown, if it exists; otherwise 0
  int leftover_time = countdown_end_time ? countdown_end_time - time(NULL): 0;
  int remaining_time = leftover_time + secs;
  update_countdown_end_time(time(NULL) + remaining_time);
  countdown_alarm = Alarm.timerOnce(remaining_time, end_countdown);
}

// update the global varialbe, and push changes to ui
void update_countdown_end_time(time_t end_time) {
  countdown_end_time = end_time;
  int time_left = end_time ? end_time - time(NULL) : 0;
  webSocket.broadcastTXT("countdown-time-left/" + String(time_left));
}

void end_countdown() {
  update_countdown_end_time(NULL);
  Alarm.free(countdown_alarm);
  countdown_alarm = NULL;
  set_state(false);
}

// pad time of day (given in seconds after midnight) to HH:MM format
String seconds_to_string(time_t secs) {
  String m = ((minute(secs) < 10) ? "0" : "") + String(minute(secs));
  String h = ((hour(secs) < 10) ? "0" : "") + String(hour(secs));
  return h + ":" + m;
}