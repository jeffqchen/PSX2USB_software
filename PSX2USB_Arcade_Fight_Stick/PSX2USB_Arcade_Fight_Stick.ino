/*
Adapted from the PSX2USB example from PsxNewLib.
Supports Asciiware fight stick
 */

#include <PsxControllerBitBang.h>
#include <Joystick.h>

/* We must use the bit-banging interface, as SPI pins are only available on the
 * ICSP header on the Leonardo.
 *
 * Note that we use pins 9-12 so that 13 can be used with the built-in LED.
 */
const byte PIN_PS2_ATT = 10;
const byte PIN_PS2_CMD = 16;
const byte PIN_PS2_DAT = 14;
const byte PIN_PS2_CLK = 15;

const unsigned long POLLING_INTERVAL = 1000U / 500U;

// Send debug messages to serial port
//~ #define ENABLE_SERIAL_DEBUG

#define USB_PRODUCT "Jeff Stick"

// Button Mapping
#define JOY_CROSS       1
#define JOY_CIRCLE      2
#define JOY_SQUARE      0
#define JOY_TRIANGLE    3

#define JOY_L1          4
#define JOY_R1          5
#define JOY_L2          6
#define JOY_R2          7

#define JOY_SELECT      8
#define JOY_START       9

PsxControllerBitBang<PIN_PS2_ATT, PIN_PS2_CMD, PIN_PS2_DAT, PIN_PS2_CLK> psx;

Joystick_ usbStick (
	JOYSTICK_DEFAULT_REPORT_ID,
	JOYSTICK_TYPE_JOYSTICK,
	10,			// buttonCount
	1,			// hatSwitchCount (0-2)
	true,		// includeXAxis
	true,		// includeYAxis
	false,		// includeZAxis
	false,		// includeRxAxis
	false,		// includeRyAxis
	false,		// includeRzAxis
	false,		// includeRudder
	false,		// includeThrottle
	false,		// includeAccelerator
	false,		// includeBrake
	false		// includeSteering
);


#ifdef ENABLE_SERIAL_DEBUG
	#define dstart(spd) do {Serial.begin (spd); while (!Serial) {digitalWrite (LED_BUILTIN, (millis () / 500) % 2);}} while (0);
	#define debug(...) Serial.print (__VA_ARGS__)
	#define debugln(...) Serial.println (__VA_ARGS__)
#else
	#define dstart(...)
	#define debug(...)
	#define debugln(...)
#endif

boolean haveController = false;

#define	toDegrees(rad) (rad * 180.0 / PI)

#define deadify(var, thres) (abs (var) > thres ? (var) : 0)


/** \brief Dead zone for analog sticks
 *  
 * If the analog stick moves less than this value from the center position, it
 * is considered still.
 * 
 * \sa ANALOG_IDLE_VALUE
 */
const byte ANALOG_DEAD_ZONE = 50U;

int currentStickState;
int lastStickState;

const int dpadToAngle[16] = { 
                          JOYSTICK_HATSWITCH_RELEASE,   //0   center
                          0,                            //1   up
                          90,                           //2   right
                          45,                           //3   up-right
                          180,                          //4   down
                          JOYSTICK_HATSWITCH_RELEASE,   //5   *undefined
                          135,                          //6   down-right
                          JOYSTICK_HATSWITCH_RELEASE,   //7   *undefined
                          270,                          //8   left
                          315,                          //9   up-left
                          JOYSTICK_HATSWITCH_RELEASE,   //10  *undefined
                          JOYSTICK_HATSWITCH_RELEASE,   //11  *undefined
                          225,                          //12  down-left
                          JOYSTICK_HATSWITCH_RELEASE,   //    *undefined
                          JOYSTICK_HATSWITCH_RELEASE,   //    *undefined
                          JOYSTICK_HATSWITCH_RELEASE    //    *undefined
                        };

void setup () {
	// Lit the builtin led whenever buttons are pressed
	//pinMode (LED_BUILTIN, OUTPUT);

	// Init Joystick library
	usbStick.begin (false);		// We'll call sendState() manually to minimize lag

	// This way we can output the same range of values we get from the PSX controller
	usbStick.setXAxisRange (ANALOG_MIN_VALUE, ANALOG_MAX_VALUE);
	usbStick.setYAxisRange (ANALOG_MIN_VALUE, ANALOG_MAX_VALUE); 
  usbStick.setHatSwitch(0, JOYSTICK_HATSWITCH_RELEASE);

  // D-pad to hat switch angle conversion preparation
  currentStickState = 0;
  lastStickState = 0;

	dstart (115200);

	debugln (F("Ready!"));
}

void loop () {
	static unsigned long last = 0;
	
	if (millis () - last >= POLLING_INTERVAL) {
		last = millis ();
		
		if (!haveController) {
			if (psx.begin ()) {
				debugln (F("Controller found!"));
				if (!psx.enterConfigMode ()) {
					debugln (F("Cannot enter config mode"));
				} else {
					// Try to enable analog sticks
					if (!psx.enableAnalogSticks ()) {
						debugln (F("Cannot enable analog sticks"));
					}
									
					if (!psx.exitConfigMode ()) {
						debugln (F("Cannot exit config mode"));
					}
				}
				
				haveController = true;
			}
		} else {
			if (!psx.read ()) {
				debugln (F("Controller lost :("));
				haveController = false;
			} else {
				byte x, y;
				
				/* Flash led with buttons, I like this but it introduces a bit of
				 * lag, so let's keep it disabled by default
				 */
				//~ digitalWrite (LED_BUILTIN, !!psx.getButtonWord ());

// Read was successful, so let's make up data for Joystick

				// Buttons first!
				usbStick.setButton (JOY_SQUARE,   psx.buttonPressed (PSB_SQUARE));
				usbStick.setButton (JOY_CROSS,    psx.buttonPressed (PSB_CROSS));
				usbStick.setButton (JOY_CIRCLE,   psx.buttonPressed (PSB_CIRCLE));
				usbStick.setButton (JOY_TRIANGLE, psx.buttonPressed (PSB_TRIANGLE));
				usbStick.setButton (JOY_L1,       psx.buttonPressed (PSB_L1));
				usbStick.setButton (JOY_R1,       psx.buttonPressed (PSB_R1));
				usbStick.setButton (JOY_L2,       psx.buttonPressed (PSB_L2));
				usbStick.setButton (JOY_R2,       psx.buttonPressed (PSB_R2));
				usbStick.setButton (JOY_SELECT,   psx.buttonPressed (PSB_SELECT));
				usbStick.setButton (JOY_START,    psx.buttonPressed (PSB_START));
				//usbStick.setButton (10, psx.buttonPressed (PSB_L3));		// Only available on DualShock and later controllers
				//usbStick.setButton (11, psx.buttonPressed (PSB_R3));		// Ditto

// D-Pad makes up the X/Y axes

        currentStickState = 0;
        
				if (psx.buttonPressed (PSB_PAD_UP)) {
					usbStick.setYAxis (ANALOG_MIN_VALUE);
          currentStickState |= 1L << 0;
				} else if (psx.buttonPressed (PSB_PAD_DOWN)) {
					usbStick.setYAxis (ANALOG_MAX_VALUE);
          currentStickState |= 1L << 2;
				} else {
					usbStick.setYAxis (ANALOG_IDLE_VALUE);
				}
				
				if (psx.buttonPressed (PSB_PAD_LEFT)) {
					usbStick.setXAxis (ANALOG_MIN_VALUE);
          currentStickState |= 1L << 3;
				} else if (psx.buttonPressed (PSB_PAD_RIGHT)) {
					usbStick.setXAxis (ANALOG_MAX_VALUE);
          currentStickState |= 1L << 1;
				} else {
					usbStick.setXAxis (ANALOG_IDLE_VALUE);
				}
       
        if (currentStickState == lastStickState) {
          usbStick.sendState ();
          return;
        }

        //hat switch state changed
        lastStickState = currentStickState;        
        usbStick.setHatSwitch(0, dpadToAngle[lastStickState]);

				// All done, send data for real!
				usbStick.sendState ();
			}
		}
	}
}
