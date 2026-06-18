
#define MAX_CHANNELS    10

#define MAX_SSID_LEN    20
#define MAX_PASS_LEN    32


#define DEFAULT_SSID    "MLT-Industriesv2"
#define DEFAULT_PASS    "ControlMe"
#define DEFAULT_PIN     123456

#define DEMO_SSID    "MLT-Demo"
#define DEMO_PASS    "ControlMe"
#define DEMO_PIN     123456

#define OVERRIDE_PIN    39

extern bool enabledebug;
extern bool enabletimerdebug;
extern bool enablepowerdebug;
extern bool enablelowcutdebug;
extern bool enablelockoutdebug;
extern bool enablemacdebug;
extern bool enableusb;
extern bool usbchangeflag;
extern bool tswchangeflag;
extern bool nvschangeflag;
extern bool enabletempdebug;
extern uint8_t hdwr_ver;
extern uint16_t vendor_id;

extern uint16_t v_adc; // measured voltage in mV
extern uint16_t a_adc; // measured voltage in mA

extern uint16_t  ch_cutout[MAX_CHANNELS];  // list of undervoltage cutout level, in mV for a channel. 0 means ignore
extern uint16_t  ch_timeout[MAX_CHANNELS]; // list of timeout counts, ticked every second. 0 = inactive
extern uint16_t  ch_lockout[MAX_CHANNELS];  // list of lockouts. 0 = unlock, 1 = locked
extern int ch_temps[3];  // list of temperatures
extern uint8_t override_active; //Is an override active? 0 = No, 1=Yes
extern int onboardTemp;
