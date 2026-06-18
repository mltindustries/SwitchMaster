
#ifndef MLT_CALIBRATION_H
#define MLT_CALIBRATION_H

// calibration data structure
typedef struct calibration_t {
    float     calibration_gain;         // calibration data
    float     calibration_offset;       // calibration data
    uint32_t  magic;                    // magic pattern
}calibration_t;

extern calibration_t calibration_data;

void calibration_init(void);

void calibration_restore_defaults(void);

void calibration_load(void);

void calibration_save(void);

void calibration_run(void);

#endif //MLT_CALIBRATION_H
