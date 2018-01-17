#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SIZE 1
#define NUMELEM 13

#define ACCEL_MG_LSB_2G 	(0.000244F)
#define ACCEL_MG_LSB_4G 	(0.000488F)
#define ACCEL_MG_LSB_8G 	(0.000976F)

#define ACCEL_RANGE				ACCEL_RANGE_4G

/* conversion factor between mag and uTesla */
#define MAG_UT_LSB      (0.1F)

/**
 * typedefs
 */
typedef enum {												// DEFAULT    TYPE
	FXOS8700_REGISTER_STATUS = 0x00,
	FXOS8700_REGISTER_OUT_X_MSB = 0x01,
	FXOS8700_REGISTER_OUT_X_LSB = 0x02,
	FXOS8700_REGISTER_OUT_Y_MSB = 0x03,
	FXOS8700_REGISTER_OUT_Y_LSB = 0x04,
	FXOS8700_REGISTER_OUT_Z_MSB = 0x05,
	FXOS8700_REGISTER_OUT_Z_LSB = 0x06,
	FXOS8700_REGISTER_WHO_AM_I = 0x0D,   // 11000111   r
	FXOS8700_REGISTER_XYZ_DATA_CFG = 0x0E,
	FXOS8700_REGISTER_CTRL_REG1 = 0x2A,   // 00000000   r/w
	FXOS8700_REGISTER_CTRL_REG2 = 0x2B,   // 00000000   r/w
	FXOS8700_REGISTER_CTRL_REG3 = 0x2C,   // 00000000   r/w
	FXOS8700_REGISTER_CTRL_REG4 = 0x2D,   // 00000000   r/w
	FXOS8700_REGISTER_CTRL_REG5 = 0x2E,   // 00000000   r/w
	FXOS8700_REGISTER_MSTATUS = 0x32,
	FXOS8700_REGISTER_MOUT_X_MSB = 0x33,
	FXOS8700_REGISTER_MOUT_X_LSB = 0x34,
	FXOS8700_REGISTER_MOUT_Y_MSB = 0x35,
	FXOS8700_REGISTER_MOUT_Y_LSB = 0x36,
	FXOS8700_REGISTER_MOUT_Z_MSB = 0x37,
	FXOS8700_REGISTER_MOUT_Z_LSB = 0x38,
	FXOS8700_REGISTER_MCTRL_REG1 = 0x5B,   // 00000000   r/w
	FXOS8700_REGISTER_MCTRL_REG2 = 0x5C,   // 00000000   r/w
	FXOS8700_REGISTER_MCTRL_REG3 = 0x5D,   // 00000000   r/w
} fxos8700Registers_t;

typedef struct fxos8700RawData_s {
	/*! Value for x-axis */
	int16_t x;
	/*! Value for y-axis */
	int16_t y;
	/*! Value for z-axis */
	int16_t z;
} fxos8700RawData_t;

typedef enum {
	/*! ±0.244 mg/LSB */
	ACCEL_RANGE_2G = 0x00,
	/*! ±0.488 mg/LSB */
	ACCEL_RANGE_4G = 0x01,
	/*! ±0.976 mg/LSB */
	ACCEL_RANGE_8G = 0x02
} fxos8700AccelRange_t;

#define SENSORS_GRAVITY_EARTH             (9.80665F)              /**< Earth's gravity in m/s^2 */
#define SENSORS_GRAVITY_MOON              (1.6F)                  /**< The moon's gravity in m/s^2 */
#define SENSORS_GRAVITY_SUN               (275.0F)                /**< The sun's gravity in m/s^2 */
#define SENSORS_GRAVITY_STANDARD          (SENSORS_GRAVITY_EARTH)
#define SENSORS_MAGFIELD_EARTH_MAX        (60.0F)                 /**< Maximum magnetic field on Earth's surface */
#define SENSORS_MAGFIELD_EARTH_MIN        (30.0F)                 /**< Minimum magnetic field on Earth's surface */
#define SENSORS_PRESSURE_SEALEVELHPA      (1013.25F)              /**< Average sea level pressure is 1013.25 hPa */
#define SENSORS_DPS_TO_RADS               (0.017453293F)          /**< Degrees/s to rad/s multiplier */
#define SENSORS_GAUSS_TO_MICROTESLA       (100)                   /**< Gauss to micro-Tesla multiplier */

int main(void) {
	FILE* fd = NULL;
	char values[13];
	int i = 0;

	uint8_t axhi = 0x00;
	uint8_t axlo = 0x00;
	uint8_t ayhi = 0x00;
	uint8_t aylo = 0x00;
	uint8_t azhi = 0x00;
	uint8_t azlo = 0x00;
	uint8_t mxhi = 0x00;
	uint8_t mxlo = 0x00;
	uint8_t myhi = 0x00;
	uint8_t mylo = 0x00;
	uint8_t mzhi = 0x00;
	uint8_t mzlo = 0x00;

	float acceleration_x = 0.0f;
	float acceleration_y = 0.0f;
	float acceleration_z = 0.0f;
	float magnetic_x = 0.0f;
	float magnetic_y = 0.0f;
	float magnetic_z = 0.0f;

	memset(values, 0, sizeof(values));

	fd = fopen("/dev/fxos8700", "r");

	if (NULL == fd) {
		fprintf(stderr, "fopen() Error!!!\n");
		return 1;
	}

	fprintf(stdout, "fxos8700 opened successfully.\n");

	for (i = 0; i < 10; i++) {

		if (SIZE * NUMELEM != fread(values, SIZE, NUMELEM, fd)) {
			fprintf(stderr, "fread() failed\n");
			return 1;
		}

		/* convert raw data to acceleration and nagnetometer data vectors */
		axhi = values[1];
		axlo = values[2];
		ayhi = values[3];
		aylo = values[4];
		azhi = values[5];
		azlo = values[6];
		mxhi = values[7];
		mxlo = values[8];
		myhi = values[9];
		mylo = values[10];
		mzhi = values[11];
		mzlo = values[12];

		/* determine acceleration data out of raw sensor data */
		acceleration_x = (int16_t) ((axhi << 8) | axlo) >> 2;
		acceleration_y = (int16_t) ((ayhi << 8) | aylo) >> 2;
		acceleration_z = (int16_t) ((azhi << 8) | azlo) >> 2;

		acceleration_x *= ACCEL_MG_LSB_8G * SENSORS_GRAVITY_STANDARD;
		acceleration_y *= ACCEL_MG_LSB_8G * SENSORS_GRAVITY_STANDARD;
		acceleration_z *= ACCEL_MG_LSB_8G * SENSORS_GRAVITY_STANDARD;

		/* determine acceleration data out of raw sensor data */
		magnetic_x = (int16_t) ((mxhi << 8) | mxlo);
		magnetic_y = (int16_t) ((myhi << 8) | mylo);
		magnetic_z = (int16_t) ((mzhi << 8) | mzlo);

		/* convert mag values to uTesla */
		magnetic_x *= MAG_UT_LSB;
		magnetic_y *= MAG_UT_LSB;
		magnetic_z *= MAG_UT_LSB;

		printf("%03.4f;%03.4f;%03.4f;%03.4f;%03.4f;%03.4f\n",
				acceleration_x, acceleration_y, acceleration_z,
				magnetic_x, magnetic_y, magnetic_z);
		sleep(1);
	}

	fclose(fd);

	return 0;
}

