/* SPDX-FileCopyrightText: 2026 Core Devices LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#include <pbl/services/notifications/notification_cooldown.h>
#include <pbl/drivers/rtc.h>
#include <string.h>

#define COOLDOWN_SLOTS 8
#define SMART_WINDOW_TICKS (10 * RTC_TICKS_HZ)
#define SMART_BURST_THRESHOLD 4

typedef struct {
  char key[64];
  bool used;
  RtcTicks last_buzz;
  RtcTicks first_arrival;
  uint8_t arrivals;
  RtcTicks last_used;
} Slot;

static Slot s_slots[COOLDOWN_SLOTS];
static NotifCooldownMode s_mode = NotifCooldownModeOff;
static NotifCooldownScope s_scope = NotifCooldownScopeEverything;
static uint32_t s_secs = 60;

static Slot *prv_find_or_alloc(const char *key) {
  int free_idx = -1;
  RtcTicks oldest = 0;
  bool have_oldest = false;
  for (int i = 0; i < COOLDOWN_SLOTS; i++) {
    if (s_slots[i].used && strcmp(s_slots[i].key, key) == 0) {
      s_slots[i].last_used = rtc_get_ticks();
      return &s_slots[i];
    }
    if (!s_slots[i].used && free_idx < 0) free_idx = i;
    if (s_slots[i].used && (!have_oldest || s_slots[i].last_used < oldest)) {
      oldest = s_slots[i].last_used; have_oldest = true;
      if (free_idx < 0) free_idx = i;
    }
  }
  if (free_idx < 0) free_idx = 0;
  memset(&s_slots[free_idx], 0, sizeof(Slot));
  s_slots[free_idx].used = true;
  strncpy(s_slots[free_idx].key, key, sizeof(s_slots[free_idx].key) - 1);
  s_slots[free_idx].last_used = rtc_get_ticks();
  return &s_slots[free_idx];
}

void notification_cooldown_init(void) {
  memset(s_slots, 0, sizeof(s_slots));
}

void notification_cooldown_configure(NotifCooldownMode mode, NotifCooldownScope scope, uint32_t secs) {
  s_mode = mode;
  s_scope = scope;
  if (secs < 5) secs = 5;
  if (secs > 600) secs = 600;
  s_secs = secs;
}

void notification_cooldown_reset(void) {
  notification_cooldown_init();
}

bool notification_cooldown_is_suppressed(const char *key) {
  if (s_mode == NotifCooldownModeOff || !key) return false;
  // find slot
  for (int i = 0; i < COOLDOWN_SLOTS; i++) {
    if (s_slots[i].used && strcmp(s_slots[i].key, key) == 0) {
      if (s_slots[i].last_buzz == 0) return false;
      RtcTicks now = rtc_get_ticks();
      uint32_t dur_ticks = s_secs * RTC_TICKS_HZ;
      return (now - s_slots[i].last_buzz) < dur_ticks;
    }
  }
  return false;
}

bool notification_cooldown_should_buzz(const char *key) {
  if (s_mode == NotifCooldownModeOff || !key) return true;
  Slot *sl = prv_find_or_alloc(key);
  RtcTicks now = rtc_get_ticks();
  uint32_t dur_ticks = s_secs * RTC_TICKS_HZ;
  if (sl->last_buzz != 0 && (now - sl->last_buzz) < dur_ticks) {
    return false;
  }
  if (s_mode == NotifCooldownModeSmart) {
    if (sl->first_arrival == 0 || (now - sl->first_arrival) > SMART_WINDOW_TICKS) {
      sl->first_arrival = now;
      sl->arrivals = 1;
    } else {
      sl->arrivals++;
    }
    if (sl->arrivals < SMART_BURST_THRESHOLD) {
      // buzz, but don't start cooldown
      return true;
    }
  }
  return true;
}

void notification_cooldown_record_buzz(const char *key) {
  if (s_mode == NotifCooldownModeOff || !key) return;
  Slot *sl = prv_find_or_alloc(key);
  sl->last_buzz = rtc_get_ticks();
}
