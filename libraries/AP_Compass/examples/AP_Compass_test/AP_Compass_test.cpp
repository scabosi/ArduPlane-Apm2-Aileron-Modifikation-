/*
 *       Example of APM_Compass library (HMC5843 sensor).
 *       Code by Jordi MuÒoz and Jose Julio. DIYDrones.com
 */

#include <AP_Common/AP_Common.h>
#include <AP_Progmem/AP_Progmem.h>
#include <AP_Param/AP_Param.h>
#include <StorageManager/StorageManager.h>
#include <AP_HAL/AP_HAL.h>
#include <AP_HAL_AVR/AP_HAL_AVR.h>
#include <AP_HAL_PX4/AP_HAL_PX4.h>
#include <AP_HAL_Linux/AP_HAL_Linux.h>
#include <AP_HAL_FLYMAPLE/AP_HAL_FLYMAPLE.h>
#include <AP_HAL_Empty/AP_HAL_Empty.h>
#include <AP_HAL_VRBRAIN/AP_HAL_VRBRAIN.h>

#include <AP_Math/AP_Math.h>    // ArduPilot Mega Vector/Matrix math Library
#include <AP_Declination/AP_Declination.h>
#include <AP_Compass/AP_Compass.h> // Compass Library
#include <GCS_MAVLink/GCS_MAVLink.h>
#include <AP_Scheduler/AP_Scheduler.h>
#include <DataFlash/DataFlash.h>
#include <AP_GPS/AP_GPS.h>
#include <AP_Vehicle/AP_Vehicle.h>
#include <AP_InertialSensor/AP_InertialSensor.h>
#include <Filter/Filter.h>
#include <AP_Baro/AP_Baro.h>
#include <AP_AHRS/AP_AHRS.h>
#include <AP_Airspeed/AP_Airspeed.h>
#include <AP_NavEKF/AP_NavEKF.h>
#include <AP_ADC/AP_ADC.h>
#include <AP_ADC_AnalogSource/AP_ADC_AnalogSource.h>
#include <AP_Notify/AP_Notify.h>
#include <AP_Mission/AP_Mission.h>
#include <AP_Terrain/AP_Terrain.h>
#include <AP_Rally/AP_Rally.h>
#include <AP_BattMonitor/AP_BattMonitor.h>
#include <AP_RangeFinder/AP_RangeFinder.h>

const AP_HAL::HAL& hal = AP_HAL_BOARD_DRIVER;

static Compass compass;

uint32_t timer;

void setup() {
    hal.console->println("Compass library test");

    if (!compass.init()) {
        hal.console->println("compass initialisation failed!");
        while (1) ;
    }
    hal.console->printf("init done - %u compasses detected\n", compass.get_count());

    compass.set_and_save_offsets(0,0,0,0); // set offsets to account for surrounding interference
    compass.set_declination(ToRad(0.0f)); // set local difference between magnetic north and true north

    hal.scheduler->delay(1000);
    timer = hal.scheduler->micros();
}

void loop()
{
    static float min[3], max[3], offset[3];

    compass.accumulate();

    if((hal.scheduler->micros()- timer) > 100000L)
    {
        timer = hal.scheduler->micros();
        compass.read();
        unsigned long read_time = hal.scheduler->micros() - timer;
        float heading;

        if (!compass.healthy()) {
            hal.console->println("not healthy");
            return;
        }
	Matrix3f dcm_matrix;
	// use roll = 0, pitch = 0 for this example
	dcm_matrix.from_euler(0, 0, 0);
        heading = compass.calculate_heading(dcm_matrix);
        compass.learn_offsets();

        // capture min
        const Vector3f &mag = compass.get_field();
        if( mag.x < min[0] )
            min[0] = mag.x;
        if( mag.y < min[1] )
            min[1] = mag.y;
        if( mag.z < min[2] )
            min[2] = mag.z;

        // capture max
        if( mag.x > max[0] )
            max[0] = mag.x;
        if( mag.y > max[1] )
            max[1] = mag.y;
        if( mag.z > max[2] )
            max[2] = mag.z;

        // calculate offsets
        offset[0] = -(max[0]+min[0])/2;
        offset[1] = -(max[1]+min[1])/2;
        offset[2] = -(max[2]+min[2])/2;

        // display all to user
        hal.console->printf("Heading: %.2f (%3d,%3d,%3d) i2c error: %u",
			    ToDeg(heading),
			    (int)mag.x,
			    (int)mag.y,
			    (int)mag.z, 
			    (unsigned)hal.i2c->lockup_count());

        // display offsets
        hal.console->printf(" offsets(%.2f, %.2f, %.2f)",
                      offset[0], offset[1], offset[2]);

        hal.console->printf(" t=%u", (unsigned)read_time);

        hal.console->println();
    } else {
	    hal.scheduler->delay(1);
    }
}

AP_HAL_MAIN();
