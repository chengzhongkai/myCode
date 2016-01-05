#include "fsl_platform_common.h"
#include "bootloader_common.h"

#include <assert.h>
#include <string.h>

#include "bootloader/bootloader_led.h"

#define LED_PWM_INSTANCE	0
#define LED_PWM_CHANNEL		HW_CHAN4

/* blink parameter for normal */
const ftm_pwm_param_t	stParamNormal = {

	.mode                   = kFtmEdgeAlignedPWM,
    .edgeMode               = kFtmLowTrue,
    .uFrequencyHZ           = 4,
    .uDutyCyclePercent      = 80,
    .uFirstEdgeDelayPercent = 0,
};

/* blink parameter for error */
const ftm_pwm_param_t	stParamError = {

	.mode                   = kFtmEdgeAlignedPWM,
    .edgeMode               = kFtmLowTrue,
    .uFrequencyHZ           = 10,
    .uDutyCyclePercent      = 50,
    .uFirstEdgeDelayPercent = 0,
};

void led_init(void)
{
    uint32_t channel;

	/* output pin setting */
	BW_PORT_PCRn_MUX(HW_PORTD, 4, 4);
	
    /* clock setting initialization*/
    BW_SIM_SCGC6_FTM0(1);

	FTM_HAL_Reset(0, LED_PWM_INSTANCE);

    /* Use FTM mode */
	FTM_HAL_Enable(0, true);
	FTM_HAL_SetClockPs(0, kFtmDividedBy2);
    /* Use the Enhanced PWM synchronization method */
	FTM_HAL_SetPwmSyncModeCmd(0, true);
    /* Enable needed bits for software trigger to update registers with its buffer value */
	FTM_HAL_SetCounterSoftwareSyncModeCmd(0, true);
	FTM_HAL_SetModCntinCvSoftwareSyncModeCmd(0, true);
	FTM_HAL_SetCntinPwmSyncModeCmd(0, true);
	for (channel = 0; channel < FSL_FEATURE_FTM_CHANNEL_COUNTn(LED_PWM_INSTANCE); channel = channel + 2)
	{
	    FTM_HAL_SetDualChnPwmSyncCmd(0, channel, true);
	}

	FTM_HAL_SetTofFreq(0, 0);
	FTM_HAL_SetWriteProtectionCmd(0, false);

	return;  
}

void led_stop(ftm_pwm_param_t *param)
{
	assert((param->mode == kFtmEdgeAlignedPWM) || (param->mode == kFtmCenterAlignedPWM) ||
		   (param->mode == kFtmCombinedPWM));

	/* Stop the FTM counter */
	FTM_HAL_SetClockSource(0, kClock_source_FTM_None);

	FTM_HAL_DisablePwmMode(0, param, HW_CHAN4);

	MCG->C1 &= ~(MCG_C1_IRCLKEN_MASK);
	
    /* Clear out the registers */
	FTM_HAL_SetMod(0, 0);
	FTM_HAL_SetCounter(0, 0);
}

void led_start(ftm_pwm_param_t *param)
{
	uint32_t uFTMhz = 32768 * 10;
	uint16_t uMod, uCnv, uCnvFirstEdge = 0;

	assert(param->uDutyCyclePercent <= 100);

    /* Clear the overflow flag */
	FTM_HAL_ClearTimerOverflow(0);

	FTM_HAL_EnablePwmMode(0, param, HW_CHAN4);

    /* Based on Ref manual, in PWM mode CNTIN is to be set 0*/
	FTM_HAL_SetCounterInitVal(0, 0);
	uFTMhz = uFTMhz / (1 << FTM_HAL_GetClockPs(0));

	switch(param->mode)
	{
    	case kFtmEdgeAlignedPWM:
        	uMod = uFTMhz / (param->uFrequencyHZ) - 1;
        	uCnv = uMod * param->uDutyCyclePercent / 100;
        	/* For 100% duty cycle */
        	if(uCnv >= uMod)
        	{
        	    uCnv = uMod + 1;
        	}
        	FTM_HAL_SetMod(0, uMod);
        	FTM_HAL_SetChnCountVal(0, HW_CHAN4, uCnv);
        	break;
    	case kFtmCenterAlignedPWM:
        	uMod = uFTMhz / (param->uFrequencyHZ * 2);
        	uCnv = uMod * param->uDutyCyclePercent / 100;
        	/* For 100% duty cycle */
        	if(uCnv >= uMod)
        	{
        	    uCnv = uMod + 1;
        	}
        	FTM_HAL_SetMod(0, uMod);
        	FTM_HAL_SetChnCountVal(0, HW_CHAN4, uCnv);
        	break;
    	case kFtmCombinedPWM:
        	uMod = uFTMhz / (param->uFrequencyHZ) - 1;
        	uCnv = uMod * param->uDutyCyclePercent / 100;
        	uCnvFirstEdge = uMod * param->uFirstEdgeDelayPercent / 100;
        	/* For 100% duty cycle */
        	if(uCnv >= uMod)
        	{
        	    uCnv = uMod + 1;
        	}
        	FTM_HAL_SetMod(0, uMod);
        	FTM_HAL_SetChnCountVal(0, FTM_HAL_GetChnPairIndex(HW_CHAN4), uCnvFirstEdge);
        	FTM_HAL_SetChnCountVal(0, FTM_HAL_GetChnPairIndex(HW_CHAN4) + 1,
                               uCnv + uCnvFirstEdge);
        	break;
    	default:
        	assert(0);
        	break;
	}

	/* Issue a software trigger to update registers */
	FTM_HAL_SetSoftwareTriggerCmd(0, true);
	
	/* enable fixed clock */
	MCG->C1 |= MCG_C1_IREFS_MASK | MCG_C1_IRCLKEN_MASK;
	
    /* Set clock source to start counter */
	FTM_HAL_SetClockSource(0, kClock_source_FTM_FixedClk);
}

void led_deinit(void)
{
	MCG->C1 &= ~(MCG_C1_IRCLKEN_MASK);
	
	FTM_HAL_Enable(0, false);
	FTM_HAL_Reset(0, LED_PWM_INSTANCE);
	BW_SIM_SCGC6_FTM0(0);

	BW_PORT_PCRn_MUX(HW_PORTD, 4, 1);
}