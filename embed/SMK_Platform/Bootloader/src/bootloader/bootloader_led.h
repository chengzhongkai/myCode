#ifndef BOOTLOADER_LED_H_
#define BOOTLOADER_LED_H_

#include "flextimer/hal/fsl_ftm_hal.h"

extern const ftm_pwm_param_t	stParamNormal;
extern const ftm_pwm_param_t	stParamError;

/* Funtion prototypes */
extern void led_init(void);

/*See fsl_ftm_driver.h for documentation of this function.*/
extern void led_stop(ftm_pwm_param_t *param);

/*See fsl_ftm_driver.h for documentation of this function.*/
extern void led_start(ftm_pwm_param_t *param);

extern void led_deinit(void);


#endif /* BOOTLOADER_EEPROM_H_ */