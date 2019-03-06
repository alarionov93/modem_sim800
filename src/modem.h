#ifndef MODEM_H
#define MODEM_H

#include <stdint.h>

typedef enum {
	M_STATE_OFF, // 0
	M_STATE_ON, // 1 if module is turned on
	M_STATE_ERROR, // 2 if module is not answering or smth else
	M_STATE_INIT, // 3 while turning on pwr on module
	M_STATE_ONLINE, // 4 if module answered to AT with OK and registered to network
	M_STATE_OFFLINE, // 5 if module NOT answered to AT with OK or NOT registered to network
} modem_state_t;

typedef enum {
	MTASK_INIT,
	MTASK_CHECK_REG,
	MTASK_CONF_SMS,
	MTASK_WORK,
	MTASK_CHECK_PING,
	MTASK_CHECK_SMS_IS_SENT,
} mtask_state_t;

typedef enum {
	SMS_TASK_INIT,
	SMS_TASK_WORK,
	SMS_TASK_IDLE,
} sms_task_state_t;

// move this to sbus.h too
typedef enum {
	SBUS_TASK_INIT,
	SBUS_TASK_WORK,
	SBUS_TASK_IDLE,
} sbus_task_state_t;

void init_sim_800(void);
void modem_task(void);

#endif
