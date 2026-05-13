#ifndef _XY_SENSOR_H_
#define _SENSOR_H_
enum xy_sensor_type {

};

enum xy_sensor_trigger_type {

};

enum xy_sensor_attribute {

};

enum sensor_stream_data_opt {
    SENSOR_STREAM_DATA_INCLUDE = 0,
    SENSOR_STREAM_DATA_NOP     = 1,
    SENSOR_STREAM_DATA_DROP    = 2
}

typedef struct {
	int32_t x;
	int32_t y;
	int32_t z;
}sensor_3_axis_acce_t;							/* Accelerometer.       unit: mG          */

typedef struct {
	int32_t x;
	int32_t y;
	int32_t z;
}sensor_3_axis_gyro_t;							 /* Gyroscope.           unit: mdps        */

typedef struct {
	int32_t x;
	int32_t y;
	int32_t z;
}sensor_3_axis_mag_t;							 /* Magnetometer.        unit: mGauss      */

typedef struct {
    int32_t sbp; /**<  systolic pressure. */
    int32_t dbp; /**< diastolic pressure*/
} sensor_bp_data_t                                /* BloodPressure.       unit: mmHg        */

typedef struct{
	double longtitude;
	double latitude;
}sensort_coordinate_t;                            /* Coordinates          unit: degrees     */

typedef int32_t           sensor_temp_t;          /* Temperature.         unit: dCelsius    */
typedef int32_t           sensor_humi_t;          /* Relative humidity.   unit: permillage  */
typedef int32_t           sensor_baro_t;          /* Pressure.            unit: pascal (Pa) */
typedef int32_t           sensor_light_t;         /* Light.               unit: lux         */
typedef int32_t           sensor_proximity_t;     /* Distance.            unit: centimeters */
typedef int32_t           sensor_hr_t;            /* Heart rate.          unit: bpm         */
typedef int32_t           sensor_tvoc_t;          /* TVOC.                unit: permillage  */
typedef int32_t           sensor_noise_t;         /* Noise Loudness.      unit: HZ          */
typedef uint32_t          sensor_step_t;          /* Step sensor.         unit: 1           */
typedef int32_t           sensor_force_t;         /* Force sensor.        unit: mN          */
typedef uint32_t          sensor_dust_t;          /* Dust sensor.         unit: ug/m3       */
typedef uint32_t          sensor_eco2_t;          /* eCO2 sensor.         unit: ppm         */
typedef uint32_t          sensor_spo2_t;          /* SpO2 sensor.         unit: permillage  */
typedef uint32_t          sensor_iaq_t;           /* IAQ sensor.          unit: 1           */
typedef uint32_t          sensor_etoh_t;          /* EtOH sensor.         unit: ppm         */
typedef float             sensor_mv_t;            /* Voltage sensor.      unit: mv          */
typedef float             sensor_ma_t;            /* Current sensor.      unit: ma          */
typedef float             sensor_mw_t;            /* Power sensor.        unit: mw          */

// more ref  https://docs.zephyrproject.org/latest/doxygen/html/sensor_8h.html
// https://docs.zephyrproject.org/latest/doxygen/html/group__sensor__interface.html#gaaa1b502bc029b10d7b23b0a25ef4e934
// sensor_value from, to https://docs.zephyrproject.org/latest/doxygen/html/group__sensor__interface.html#gaf01bbb251ad0c7f6c55c5b702e8a4048
// or see rtthread
#endif