
#include <stdint.h>
#include "pointdrv.h"


#define STANDARD_STEPS 420  /* 5.625*(1/64) per step, 4096 steps is 360° */
#define STEP_DELAY  3 


#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

uint8_t half_step_sequence[8][4] = {{ HIGH,  LOW,  LOW, HIGH},
                                    { HIGH,  LOW,  LOW,  LOW},
                                    { HIGH, HIGH,  LOW,  LOW},
                                    {  LOW, HIGH,  LOW,  LOW},
                                    {  LOW, HIGH, HIGH,  LOW},
                                    {  LOW,  LOW, HIGH,  LOW},
                                    {  LOW,  LOW, HIGH, HIGH},
                                    {  LOW,  LOW,  LOW, HIGH}};

static struct point_drv_cfg p_drv_cfg;

static int point_configure_interface(struct point_drv_cfg* cfg);
static void point_move(struct point_drv_cfg* cfg, int direction, int force);
static int point_get_position(struct point_drv_cfg* cfg);
static int point_motor_disable(struct point_drv_cfg* cfg);

/** Public Interface **/

int pointdrv_init(struct point_drv_cfg cfg)
{
    /* do some error checking and return 0 on failure */
    p_drv_cfg = cfg;

    return 1;
};

void pointdrv_move(int direction, int force)
{
    if(direction == 0)
    {
        return;
    }

    if (!bcm2835_init() 
        && (!point_configure_interface(&p_drv_cfg)))
    {
        bcm2835_close();
        return 0;
    }

    if (force == 0)
    {
        if(   ((direction < 0) && (point_get_position(&p_drv_cfg) <= 0)) 
            || ((direction > 0) && (point_get_position(&p_drv_cfg) >= 0))  )
        {
            printf("Error, point not in end postion. Use force = 1 to override.\n");
            bcm2835_close();
            return;
        }
    }

    point_move(&p_drv_cfg, int direction, int force);

    bcm2835_close();
}

int pointdrv_get_postion()
{
    int retval = 0;
    if (!bcm2835_init() 
        && (!point_configure_interface(&p_drv_cfg)))
    {
        bcm2835_close();
        return 0;
    }
    retval = point_get_position(&p_drv_cfg);
    bcm2835_close();
    return retval;
}

/** Private Functions **/

static int point_configure_interface(struct point_drv_cfg* cfg)
{
    /* configure output pins*/
    bcm2835_gpio_fsel(cfg->drv_pin_1, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(cfg->drv_pin_2, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(cfg->drv_pin_3, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(cfg->drv_pin_4, BCM2835_GPIO_FSEL_OUTP);

    /* configure input pins */
    bcm2835_gpio_fsel(cfg->pos_pin_1, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(cfg->pos_pin_2, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(cfg->pos_pin_1, BCM2835_GPIO_PUD_DOWN);
    bcm2835_gpio_set_pud(cfg->pos_pin_2, BCM2835_GPIO_PUD_DOWN);

    point_motor_disable(cfg);

return 1;
}

static void point_move(struct point_drv_cfg* cfg, int direction, int force)
{
    int motor_step_counter = 0;

    uint32_t mask =   (1 << cfg->drv_pin_1) 
                    | (1 << cfg->drv_pin_2)
                    | (1 << cfg->drv_pin_3)
                    | (1 << cfg->drv_pin_4);

    for (uint32_t i = 0; i < STANDARD_STEPS; i++)
    {
        if((direction < 0) && (point_get_position(cfg) == -1))
        {
            printf("Endlage Links erreicht\n");
            break;
        }
        else if((direction > 0) && (point_get_position(cfg) == 1))
        {
            printf("Endlage Rechts erreicht\n");
            break;
        }

        uint32_t value =  (half_step_sequence[motor_step_counter][0] << cfg->drv_pin_1 ) 
                        | (half_step_sequence[motor_step_counter][1] << cfg->drv_pin_2 ) 
                        | (half_step_sequence[motor_step_counter][2] << cfg->drv_pin_3 ) 
                        | (half_step_sequence[motor_step_counter][3] << cfg->drv_pin_4 );
        
        if((cfg->half_steps == 1) || ((motor_step_counter % 2) == 1 ) )
        {
            bcm2835_gpio_write_mask(value, mask);
        }
        bcm2835_delay(STEP_DELAY);
        
        motor_step_counter = (motor_step_counter + (1 * direction)+ 8) % 8 ;
    }

    point_drv_disable(cfg);
}

static int point_get_position(struct point_drv_cfg* cfg)
{
    if(bcm2835_gpio_lev(cfg->pos_pin_1) == HIGH)
    {
        return -1;
    }
    else if(bcm2835_gpio_lev(cfg->pos_pin_2) == HIGH)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/**
 * @brief Sets all GPIOs to low to disable engine
 * 
 * @param cfg 
 * @return int 
 */
static int point_motor_disable(struct point_drv_cfg* cfg)
{
    bcm2835_gpio_write(cfg->drv_pin_1, LOW);
    bcm2835_gpio_write(cfg->drv_pin_2, LOW);
    bcm2835_gpio_write(cfg->drv_pin_3, LOW);
    bcm2835_gpio_write(cfg->drv_pin_4, LOW);
    return 1;
}