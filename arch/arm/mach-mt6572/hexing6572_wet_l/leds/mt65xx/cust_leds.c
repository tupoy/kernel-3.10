#include <cust_leds.h>
#include <cust_leds_def.h>
#include <mach/mt_pwm.h>

#include <linux/kernel.h>
#include <mach/pmic_mt6329_hw_bank1.h> 
#include <mach/pmic_mt6329_sw_bank1.h> 
#include <mach/pmic_mt6329_hw.h>
#include <mach/pmic_mt6329_sw.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>

//extern int mtkfb_set_backlight_level(unsigned int level);
//extern int mtkfb_set_backlight_pwm(int div);
extern int disp_bls_set_backlight(unsigned int level);

#define ERROR_BL_LEVEL 0xFFFFFFFF
/*
unsigned int brightness_mapping(unsigned int level)
{  
	return ERROR_BL_LEVEL;
}
*/
unsigned int brightness_mapping(unsigned int level)
{
#if 0  
	if(level) {
		return (level<<3) + 1 ;    //256/32 for PWM duty 32 levels
	}else {
		return level;
	}
#else
       if(level >= 30 && level <=255)
       {
           return ((level - 26)/4 + 6);
       }
       else if(level >=0 && level < 30)
       {
           return 6;

       }
       return ERROR_BL_LEVEL;
#endif
}

unsigned int Cust_SetBacklight(int level, int div)
{
    //mtkfb_set_backlight_pwm(div);
    //mtkfb_set_backlight_level(brightness_mapping(level));
    disp_bls_set_backlight(brightness_mapping(level));
    return 0;
}


static struct cust_mt65xx_led cust_led_list[MT65XX_LED_TYPE_TOTAL] = {
	{"red",               MT65XX_LED_MODE_NONE, -1, {0}},
	{"green",             MT65XX_LED_MODE_NONE, -1, {0}},
	{"blue",              MT65XX_LED_MODE_NONE, -1, {0}},
	{"jogball-backlight", MT65XX_LED_MODE_NONE, -1, {0}},
	{"keyboard-backlight",MT65XX_LED_MODE_NONE, -1, {0}},
	{"button-backlight",  MT65XX_LED_MODE_NONE, -1, {0}},
	//{"lcd-backlight",     MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_LCD_ISINK, {0}},
	{"lcd-backlight",     MT65XX_LED_MODE_PWM, PWM1,{1,0,38,38}},  //by Jason.
};

struct cust_mt65xx_led *get_cust_led_list(void)
{
	return cust_led_list;
}

