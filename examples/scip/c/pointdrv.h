#ifndef POINTDRV_HEADER_
#define POINTDRV_HEADER_ 1

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif

#include <bcm2835.h>

enum point_drv_direction
{
    DIRECTION_LEFT = -1,
    DIRECTION_RIGHT = 1
};

enum point_drv_position
{
    POSITION_LEFT = -1,
    POSITION_UNKNOWN = 0,
    POSITION_RIGHT = 1
};

struct point_drv_cfg
{
    RPiGPIOPin drv_pin_1;
    RPiGPIOPin drv_pin_2;
    RPiGPIOPin drv_pin_3;
    RPiGPIOPin drv_pin_4;
    RPiGPIOPin pos_pin_1;
    RPiGPIOPin pos_pin_2;
    int        half_steps;
};


int pointdrv_init(struct point_drv_cfg cfg);
void pointdrv_move(enum point_drv_direction, int force);
enum point_drv_position pointdrv_get_postion();


#ifdef __cplusplus
}
#endif

#endif /* POINTDRV_HEADER_ */