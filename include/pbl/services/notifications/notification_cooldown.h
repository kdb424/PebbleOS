/* SPDX-FileCopyrightText: 2026 Core Devices LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum NotifCooldownMode {
  NotifCooldownModeOff = 0,
  NotifCooldownModeAlways = 1,
  NotifCooldownModeSmart = 2,
} NotifCooldownMode;

typedef enum NotifCooldownScope {
  NotifCooldownScopeEverything = 0,
  NotifCooldownScopePerApp = 1,
  NotifCooldownScopePerConversation = 2,
} NotifCooldownScope;

void notification_cooldown_init(void);
void notification_cooldown_configure(NotifCooldownMode mode, NotifCooldownScope scope, uint32_t secs);
bool notification_cooldown_should_buzz(const char *key);
bool notification_cooldown_is_suppressed(const char *key);
void notification_cooldown_record_buzz(const char *key);
void notification_cooldown_reset(void);
