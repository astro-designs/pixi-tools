#ifndef libpixi_pixi_pwm_h__included
#define libpixi_pixi_pwm_h__included


#include <libpixi/common.h>

LIBPIXI_BEGIN_DECLS

///@defgroup PixiPWM PiXi-200 PWM interface
///@{

///	Set the state of the PiXi PWM controller
///	@param pwm PWM device [0,7]
///	@param dutyCycle PWM duty cycle [0,1023]
///	@return 0 on success, -errno on error
int pixi_pwmWritePin (uint pwm, uint dutyCycle);

///	Set the state of the PiXi PWM controller
///	@param pwm PWM device [0,7]
///	@param dutyCycle PWM duty cycle percent [0,100]
///	@return 0 on success, -errno on error
int pixi_pwmWritePinPercent (uint pwm, double dutyCycle);

///@} defgroup

LIBPIXI_END_DECLS

#endif // !defined libpixi_pixi_pwm_h__included
