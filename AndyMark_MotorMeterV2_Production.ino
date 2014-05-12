//http://playground.arduino.cc/Code/Button
#include <Button.h>
//http://playground.arduino.cc/Code/Potentiometer
//#include <Potentiometer.h>
#include <Servo.h>
#include <Wire.h>
////CSK 4/15/2014 had to add call to SPI.h library when I updated to latest ssd1306 library and the display is really responsive now.
#include <SPI.h>
//http://www.adafruit.com/products/938
//CSK 4/16/2014 Updated to latest GFX library and the display is really responsive now.
//https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_GFX.h>
//CSK 4/15/2014 Updated to latest ssd1306 library and the display is really responsive now.
//https://github.com/adafruit/Adafruit_SSD1306
#include "Adafruit_SSD1306_AndyMark_May2014/Adafruit_SSD1306.h"


#define FIRMWARE_VERSION 2.0

//CSK 10/31/2013 Misc. menu alignment cleanup before product release

//*** I/0 Setup ***//
//    Analog I/O   //
#define SPEED_POT                  A0
#define SELECT_SWITCH			   A1
//Available                        A2
#define CURRENT_SENSOR             A3
//I2C_DATA                         4
//I2C_CLOCK                        5

//    Digital I/0   //
//Digital I/O Pin Definitions
//USB RX                           0
//USB TX                           1
//Available                        2
#define SPEED_CONTROLLER		   3	//CSK 3/19/2014 I had to move this to pin 3 because pin 10 and 11 don't generate clean PWMs - A lot of random spikes. It seems to be related to changing the display from I2C to SPI

//Pins 3, 5, 6, 9, 10, 11 are PWM
//CSK 3/3/2014 Changes for V2 SPI Control
//Available                        4
#define OLED_CS                    5
#define OLED_RESET                 6
#define OLED_DC                    7
#define OLED_CLK                   8
#define OLED_MOSI                  9  //This is the Data pin

//CSK 10/14/2013 Changed to match Arduino Pro Mini custom interface PCB
//#define MODE_INCREMENT_SWITCH      6 //10
//Available         			       11
#define MODE_SWITCH				   10
//Available                        12
//Available                        13
//*** END I/O Setup ***//

Servo myservo;  // create servo object to control a servo

//CSK 9/6/2013 This is an alias for the servo range selection.  Standard means 1000ms to 2000ms
#define STANDARD_SERVO_RANGE 800	//CSK 3/4/2014 Change this to get button hold time to set servo range - replaces separate jumper selector
#define POT_VALUE_IS_SERVO_POSITION 1
#define POT_VALUE_IS_SERVO_SPEED    0
//CSK 4/29/2014 These were measured from with HiTech Servo Test pulse measure feature
#define ARDUINO_SERVO_EXTENDED_START	500
#define ARDUINO_SERVO_EXTENDED_END		2500
#define ARDUINO_SERVO_CENTER			1500
#define ARDUINO_SERVO_START				1000
#define ARDUINO_SERVO_END				2000
//CSK 4/22/2014 Changed from 170 to 180 to make uS display come closer to 2000us
#define SERVO_END_FUDGE_FACTOR 2
int glb_servo_start  =  ARDUINO_SERVO_START;
int glb_servo_end    =  ARDUINO_SERVO_END;
int glb_servo_center =  ARDUINO_SERVO_CENTER;

//http://playground.arduino.cc/Code/Button
Button select_button = Button(SELECT_SWITCH,PULLUP);
Button mode_increment_button = Button(MODE_SWITCH,PULLUP);
//http://playground.arduino.cc/Code/Potentiometer


int glb_int_present_pwm_setting = glb_servo_center;


float glb_smooth_current_in_amps;
float glb_previous_current_in_amps;
float glb_current_calibration_offset = 0.25;

//CSK 11/26/2013 New equation for 100A sensor
#define A2D_TO_AMPS(X) ( ( ( ( ( 100.0/(1023.0 - 122.76) ) * X ) - 13.63 )  <= 0.3 )?0:( ( ( 100.0/(1023.0 - 122.76) ) * X ) - 13.63 ) )

#define INSTRUCTION_MODE   0
//#define RAMP_TO_SPEED	   1
#define SET_SPEED          1
#define SWEEP              2
#define TALON_CALIBRATION  3

#define HIGHEST_MODE       3

int Mode_State = 0;

////***Polou QTR Sensor Setup***//
////http://www.pololu.com/catalog/product/2455
//#define NUM_SENSORS   1     // number of sensors used
//#define TIMEOUT       2500  // waits for 2500 microseconds for sensor outputs to go low
//QTRSensorsRC qtrrc((unsigned char[]) {QTR_L_1RC_RPM_SENSOR}, NUM_SENSORS, TIMEOUT, QTR_EMITTER_PIN);
unsigned int glb_RPM_Value = 0;

//CSK 3/3/2014 Changes for V2 SPI Control
//Adafruit_SSD1306 SSD1306_display(OLED_RESET);
//This is the software SSD1306 constructor???
Adafruit_SSD1306 SSD1306_display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
//This is the hardware SSD1306 constructor???
//Adafruit_SSD1306 SSD1306_display(OLED_DC, OLED_RESET, OLED_CS);


#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 96
//CSK 9/4/2013 Note: Fonts are not fixed width so width numbers are approx.
//Things like "i" and "l" take up less space that say "B".  This makes my centering function less than perfect.
#define CHAR_WIDTH_IN_PIXELS_FOR_FONTSIZE1  6
#define CHAR_WIDTH_FOR_FONTSIZE1            21
#define CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 8
#define CHAR_WIDTH_IN_PIXELS_FOR_FONTSIZE2  12
#define CHAR_WIDTH_FOR_FONTSIZE2            12
#define CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 18

boolean glb_extended_servo_range_selected = false;

#define INCREMENT_RATE ( labs(glb_int_present_pwm_setting - read_potentiometer(POT_VALUE_IS_SERVO_POSITION) )>100?10:10 )
#define OVERWRITE_GLOBAL_PWM	true
#define PRESERVE_GLOBAL_PWM		false
#define RAMP_DOWN				true
#define RAMP_UP					false
#define ENABLE_HOLD_TIME		true
#define DISABLE_HOLD_TIME		false

//Uncomment to turn on serial debug data
//#define SERIAL_DEBUG 1

void setup()
{
	#ifdef SERIAL_DEBUG
	//***USB serial config***//
	Serial.begin(115200);
	Serial.print(F("Firmware Version "));
	Serial.println(FIRMWARE_VERSION, 1);
	#endif


	//***IO Setup***//
	pinMode(SELECT_SWITCH, INPUT);
	digitalWrite(SELECT_SWITCH, HIGH);
	pinMode(MODE_SWITCH, INPUT);
	digitalWrite(MODE_SWITCH, HIGH);
	//	pinMode(SERVO_RANGE_SELECT, INPUT_PULLUP);
	//CSK 10/30/2013 Added lines to disable pullup on current sensor
	pinMode(CURRENT_SENSOR, INPUT);
	digitalWrite(CURRENT_SENSOR, LOW);
	
	pinMode( SPEED_POT, INPUT );
	
	//***Adafruit display setup***//
	// by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
	SSD1306_display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
	//SSD1306_display.begin(SSD1306_SWITCHCAPVCC);
	//With display.setTextSize(2) you can fit 10 characters in a row, good line spacing is 18pixels (e.g. Row 1 > 0,0; Row 2 > 0,18)
	//
	// init done
	
	//CSK 8/20/2013 Get really weird behavior if I don't show the splash screen, so just make it fast
	//
	SSD1306_display.clearDisplay();
	SSD1306_display.display(); // show splashscreen
	SSD1306_display.setTextWrap(false);
	//delay(1);
	SSD1306_display.setTextSize(2);
	SSD1306_display.setTextColor(WHITE);
	for (int position = 128; position >= 24;position -= 8)
	{
		SSD1306_display.clearDisplay(); // clears the screen and buffer		
		#define ANDYMARK_STARTING_ROW 10
		SSD1306_display.setCursor(position - 3, ANDYMARK_STARTING_ROW);
		SSD1306_display.print(F("AndyMark"));
		SSD1306_display.setCursor(position + 14, ANDYMARK_STARTING_ROW + CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2);
		SSD1306_display.print(F("Motor"));
		SSD1306_display.setCursor(position - 18, ANDYMARK_STARTING_ROW + (CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 * 2));
		//CSK 2/4/2014 Changed the word Tester to Meter to match the product name change
		SSD1306_display.print(F("Meter V"));
		SSD1306_display.print(FIRMWARE_VERSION,1);
		// Looks like LCD_generator tool doesn't work with the chipset in the 128X96 module???
		//   http://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
		//   display.drawBitmap(0,0,AMlogo_Cropped_BW_26X32,26,32,WHITE);
		SSD1306_display.display();
	}
	delay(1300);
	SSD1306_display.clearDisplay();

	myservo.attach(SPEED_CONTROLLER);  // attaches the servo object to PWM pin 3.
}

void loop()
{
	//CSK 9/6/2013 Read the servo range jumper and set the value to corresponding degrees
	static boolean printed = false;
	//    Serial.print(glb_servo_start);
	//    Serial.print(" ");
	//    Serial.println(glb_servo_end);
	
	switch (Mode_State)
	{
		case INSTRUCTION_MODE:
		//      Serial.println("main menu");
		print_mode_dialogs();
		instruction_mode();
		break;
		
		case SET_SPEED:
		//***Servo setup***/
		//		myservo.attach(SPEED_CONTROLLER);  // attaches the servo object to PWM pin 3.
		//      Serial.println("set pwm menu");
		print_mode_dialogs();
		set_speed_requested();
		//CSK 8/23/2013 Set PWM to Center so motor stops.
		motor_damping( glb_servo_center, 2000 );
		//		myservo.detach();  // attaches the servo object to PWM pin 3.
		break;

		case SWEEP:
		//      Serial.println("sweep menu");
		//***Servo setup***/
		//		myservo.attach(SPEED_CONTROLLER);  // attaches the servo object to PWM pin 3.
		print_mode_dialogs();
		sweep_requested();
		//CSK 8/23/2013 Set PWM to Center so motor stops.
		motor_damping( glb_servo_center, 2000 );
		//		myservo.detach();  // attaches the servo object to PWM pin 3.
		break;
		
		case TALON_CALIBRATION:
		//		myservo.attach(SPEED_CONTROLLER);  // attaches the servo object to PWM pin 3.
		print_mode_dialogs();
		talon_calibration();
		//      Serial.println("talon menu");
		//		myservo.detach();  // attaches the servo object to PWM pin 3.
		motor_damping( glb_servo_center, 2000 );
		break;
		
		default:
		//CSK 8/23/2013 Set PWM to Center so motor stops.
		motor_damping( glb_servo_center, 2000 );
		break;
	}
	#ifdef SERIAL_DEBUG
	if (!printed)
	{
		Serial.println(freeRam());
		printed = true;
	}
	#endif // SERIAL_DEBUG

	//delay(100);
}

void instruction_mode()
{
	if ( mode_increment_is_pressed() )
	{
		increment_mode_state();
		SSD1306_display.clearDisplay();
	}
	return;
}

void print_mode_dialogs()
{
	if ( Mode_State == INSTRUCTION_MODE)
	{
		SSD1306_display.setTextSize(1);
		SSD1306_display.setTextColor(WHITE);
		#define INSTRUCTION_STARTING_ROW 19
		SSD1306_display.setCursor(22,INSTRUCTION_STARTING_ROW);
		//http://playground.arduino.cc/Learning/Memory
		//CSK 8/23/2013 F() macro puts strings in flash memory rather than RAM
		SSD1306_display.print(F("Press Mode for"));
		SSD1306_display.setCursor(22, CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 + INSTRUCTION_STARTING_ROW );
		SSD1306_display.print(F("Next Function."));
		SSD1306_display.setCursor(22, (CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 2) + INSTRUCTION_STARTING_ROW );
		SSD1306_display.print(F("Press Select to"));
		SSD1306_display.setCursor(22, (CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 3) + INSTRUCTION_STARTING_ROW );
		SSD1306_display.println(F("Enter Function."));
		SSD1306_display.display();
	}
	else
	{
		SSD1306_display.setTextSize(2);
		SSD1306_display.setTextColor(WHITE);
		#define MENU_STARTING_ROW 7
		SSD1306_display.setCursor(34, MENU_STARTING_ROW);
		SSD1306_display.print(F("Press"));
		SSD1306_display.setCursor(10,CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 + MENU_STARTING_ROW);
		SSD1306_display.print(F("Select To"));
		switch (Mode_State)
		{
			case SET_SPEED:
			SSD1306_display.setCursor(17, (CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 * 2) + MENU_STARTING_ROW );
			SSD1306_display.print(F("Set Spd>"));
			break;
			
			case SWEEP:
			SSD1306_display.setCursor(17,(CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 * 2) + MENU_STARTING_ROW );
			SSD1306_display.print(F(" Sweep>"));
			break;
			
			case TALON_CALIBRATION:
			SSD1306_display.setCursor(6,(CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 * 2) + MENU_STARTING_ROW );
			SSD1306_display.print(F("CAL Talon>"));
			break;
			
			default:
			break;
		}
		SSD1306_display.display();
	}
	return;
}
void increment_mode_state()
{
	SSD1306_display.clearDisplay();
	Mode_State++;
	if ( Mode_State > HIGHEST_MODE )
	{
		Mode_State = 1;
	}
	return;
}

boolean mode_increment_is_pressed(void)
{
	boolean mode_button_pressed = false;
	if( mode_increment_button.isPressed() )
	{
		//sweep_requested_LED.toggle();
		//Don't actually change states till they let go of the button
		while(mode_increment_button.isPressed())
		{
		};
		delay(10);
		mode_button_pressed = true;
	}
	return mode_button_pressed;
}

boolean select_is_pressed(boolean bool_enable_hold_time)
{
	boolean button_pressed = false;
	uint16_t glb_button_hold_time = 0;	//CSK 3/4/2014 Added to use button hold time to set servo range - replaces separate jumper selector
	
	if( select_button.isPressed() )
	{
		//Don't actually change states till they let go of the button
		while(select_button.isPressed())
		{
			if ( bool_enable_hold_time ) //CSK 5/7/2014 Only count hold time when asked to. Prevents toggling of hold time when select is pressed inside set_speed_requested()
			{
				glb_button_hold_time++;	//CSK 3/4/2014 Added to get button hold time to set servo range - replaces separate jumper selector
				delay(1);				//CSK 3/4/2014 Added to get button hold time to set servo range - replaces separate jumper selector
					if ( glb_button_hold_time > STANDARD_SERVO_RANGE )
					{
						SSD1306_display.setTextSize(1);
						SSD1306_display.setCursor(0,0);
						//CSK 4/15/2014 Use an asterisk to indicate we are in extended servo range mode
						SSD1306_display.print(F("*"));
						SSD1306_display.display();
					}				
			}
		}
		delay(10);
		button_pressed = true;
		
		//CSK 3/4/2014 Moved this here to use button hold time to set servo range - replaces separate jumper selector
		//CSK 5/7/2014 Trying to distinguish between a select press from outside a function to those inside a function
		if ( bool_enable_hold_time )
		{
			if ( glb_button_hold_time <= STANDARD_SERVO_RANGE )
			{
				glb_extended_servo_range_selected = false;
				//CSK 9/30/2013 Standard servo range is 1000us - 2000us
				glb_servo_start  =  ARDUINO_SERVO_START;
				glb_servo_end    =  ARDUINO_SERVO_END;
			}
			else
			{
				//Extended Servo Range
				glb_extended_servo_range_selected = true;
				//Arduino full range is ~500us to ~2500us
				glb_servo_start  =  ARDUINO_SERVO_EXTENDED_START;
				glb_servo_end    =  ARDUINO_SERVO_EXTENDED_END;
			}
		}
		else
		{
			if ( !glb_extended_servo_range_selected )
			{
				//CSK 9/30/2013 Standard servo range is 1000us - 2000us
				glb_servo_start  =  ARDUINO_SERVO_START;
				glb_servo_end    =  ARDUINO_SERVO_END;
			}
			else
			{
				//Extended Servo Range
				//Arduino full range is ~500us to ~2500us
				glb_servo_start  =  ARDUINO_SERVO_EXTENDED_START;
				glb_servo_end    =  ARDUINO_SERVO_EXTENDED_END;
			}
		}
		// 		Serial.println(glb_servo_start);
		// 		Serial.println(glb_servo_end);
		// 		Serial.println(glb_button_hold_time);

	}
	return button_pressed;
}

void set_speed_requested()
{
	//CSK 11/26/2013 Trying to speed up behavior by reducing screen writing to just what changes
	boolean bool_speed_set_requested = false;
	int requested_pwm_setting = glb_servo_center;
	double dbl_peak_amps = 0;
	double dbl_amps = 0;
	int int_previous_pwm_setting = ARDUINO_SERVO_CENTER;
	
	SSD1306_display.clearDisplay();
	
	if ( select_is_pressed( ENABLE_HOLD_TIME ) )
	{
		#pragma region collapse
		while( !mode_increment_is_pressed() )
		{
			//Read current
			//CSK 5/7/2014 For some reason the current sometime displays -0.0.  Trying fabs() to stop that
			dbl_amps = fabs((readCurrent() + dbl_amps)/2);
			if ( dbl_peak_amps < dbl_amps )
			{
				dbl_peak_amps = dbl_amps;
			}
			
			requested_pwm_setting  = read_potentiometer(POT_VALUE_IS_SERVO_POSITION);

			if ( select_is_pressed( DISABLE_HOLD_TIME ) )
			{
				//Toggle speed set flag
				bool_speed_set_requested = !bool_speed_set_requested;
			}
			
			if ( bool_speed_set_requested )
			{
				//glb_int_present_pwm_setting = requested_pwm_setting;
				//myservo.writeMicroseconds( glb_int_present_pwm_setting );
				motor_damping( requested_pwm_setting, 125 );
			}
			else
			{
				motor_damping( glb_servo_center, 500 );
			}
			#ifdef SERIAL_DEBUG
			Serial.println(bool_speed_set_requested);
			#endif // SERIAL_DEBUG


			int_previous_pwm_setting = glb_int_present_pwm_setting;
			
			#define SPEED_STARTING_ROW	14

			SSD1306_display.clearDisplay();
			if ( glb_extended_servo_range_selected )
			{
				SSD1306_display.setTextSize(1);
				SSD1306_display.setCursor(0,0);
				//CSK 4/15/2014 Use an asterisk to indicate we are in extended servo range mode
				SSD1306_display.print(F("*"));
			}
					
			//Set up basic display items
			SSD1306_display.setTextSize(1);
			SSD1306_display.setTextColor(BLACK,WHITE);
			SSD1306_display.setCursor(32,2);
			{
				SSD1306_display.print(F("SPEED MODE"));
			}
			SSD1306_display.setTextSize(2);
			SSD1306_display.setTextColor(WHITE);
			SSD1306_display.setCursor(7, SPEED_STARTING_ROW);
			SSD1306_display.print(F("PWM=    uS"));
			SSD1306_display.setCursor( 7, CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 + SPEED_STARTING_ROW );
			SSD1306_display.print(F("  I=     A"));

			SSD1306_display.setCursor(7, ( CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 * 2 ) + SPEED_STARTING_ROW );
			SSD1306_display.print(F("MAX=     A"));
			SSD1306_display.setTextSize(1);
				
			//Setup to display PWM data	
			SSD1306_display.setCursor(7 +  strlen("PWM=") * CHAR_WIDTH_FOR_FONTSIZE2, SPEED_STARTING_ROW);
								
			//CSK 5/1/2014 Invert text when not set
			if ( !bool_speed_set_requested )
			{
				SSD1306_display.setTextColor(BLACK, WHITE);
			}
			else
			{
				SSD1306_display.setTextColor(WHITE);
			}
				
			//CSK 5/1/2014 size was getting set if I held the set button too long
			SSD1306_display.setTextSize(2);
				
			//Keep value right justified
			//CSK 5/7/2014 This comparison looks wrong but
			//The spacing was getting fooled by round function in the read_potentiometer()
			if (glb_int_present_pwm_setting < 999 || requested_pwm_setting < 999)	SSD1306_display.print(F(" "));
							
			SSD1306_display.print(requested_pwm_setting); //map(glb_byte_present_pwm_setting, 0, 180, ARDUINO_SERVO_START, ARDUINO_SERVO_END));
				
			//CSK 8/21/2013 This device uses this extended ascii table set http://www.asciitable.pro/ascii_table.htm
			//http://forums.adafruit.com/viewtopic.php?f=47&t=30092
			////247 is the degree symbol from this table
			//SSD1306_display.write(247);
				
			//Setup to display Amps data
			SSD1306_display.setTextColor(WHITE);
			SSD1306_display.setCursor( 7 +  strlen("I=  ") * CHAR_WIDTH_FOR_FONTSIZE2, (CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 + SPEED_STARTING_ROW) );
				
			//Keep value right justified
			if (dbl_amps < 10.0)									SSD1306_display.print(F(" "));
//			else if (dbl_amps >= 10.0 && dbl_amps < 100)			SSD1306_display.print(F(""));
			//CSK 5/7/2014 For some reason the current sometime displays -0.0.  Trying fabs() to stop that
			SSD1306_display.print( dbl_amps,1 );
				
			SSD1306_display.setCursor( 7 +  strlen("MAX=") * CHAR_WIDTH_FOR_FONTSIZE2, (( CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 * 2 ) + SPEED_STARTING_ROW) );

			//Setup to display peak current data
			//Keep value right justified
			//If value is one digit
			if (dbl_peak_amps < 10)									SSD1306_display.print(F(" "));
			//if value is 2 digits
//			else if (dbl_peak_amps >= 10.0 && dbl_peak_amps < 100)	SSD1306_display.print(F(""));
			SSD1306_display.print(dbl_peak_amps, 1);
			SSD1306_display.display();
		}//end while !mode_increment_is_pressed()
		#pragma endregion collapse

		SSD1306_display.clearDisplay();
	}
	if ( mode_increment_is_pressed() )
	{
		increment_mode_state();
	}
	return;
}



int read_potentiometer(boolean bool_pot_value_is_servo_position)
{
	int	byte_pot_value_scaled	= ARDUINO_SERVO_CENTER;

	//If we are in a simple servo position state then map pot A2D values to servo degrees
	if (bool_pot_value_is_servo_position)
	{
		//http://playground.arduino.cc/Code/Potentiometer
		//CSK 11/14/2013 Swapped to match potentiometer mounting on PC board.
		//byte_pot_value_scaled = map(Speed_Pot.getValue(), 0, 1023, glb_servo_start, glb_servo_end);
		//CSK 5/6/2014 Modified to make potentiometer pwm count by 10s because this is the minimum value that effects speed. Plus it make system response faster.
		byte_pot_value_scaled = round(map(analogRead(SPEED_POT), 0, 1023, glb_servo_end, glb_servo_start)/10.0) * 10;
	}
	//If we are in servo sweep state then map pot A2D values to time Xms/degree
	else
	{
		//http://playground.arduino.cc/Code/Potentiometer
		//byte_pot_value_scaled = map(Speed_Pot.getValue(), 0, 1023, 100, 0);
		//CSK 11/18/2013 reversed to match the potentiometer mounting on PC board
		byte_pot_value_scaled = map(analogRead( SPEED_POT ), 0, 1023, 0, 100);
	}
	
	return byte_pot_value_scaled;
}

#define NUM_READS 50
#define NUM_TO_AVERAGE 10
float readCurrent(void)
{
	//http://www.elcojacobs.com/eleminating-noise-from-sensor-readings-on-arduino-with-digital-filtering/
	// read multiple values and sort them to take the mode
	//CSK 9/9/2013 eliminate float calculations inside loop to try to speed up this function
	//float sortedValues[NUM_READS];
	int sortedValues[NUM_READS];
	for( byte read_counter = 0; read_counter < NUM_READS; read_counter++ )
	{
		//CSK 9/9/2013 eliminate float calculations inside loop to try to speed up this function
		//float A2D_TO_AMPS( analogRead(CURRENT_SENSOR) );
		int value = analogRead(CURRENT_SENSOR);
		byte insert_position;
		if( value < sortedValues[0] || read_counter == 0 )
		{
			insert_position = 0; //insert at first position
		}
		else
		{
			for( insert_position = 1; insert_position < read_counter; insert_position++ )
			{
				if( sortedValues[insert_position-1] <= value && sortedValues[insert_position] >= value )
				{
					break;
				}
			}
		}
		for( byte k = read_counter; k > insert_position; k-- )
		{
			// move all values higher than current reading up one position
			sortedValues[k] = sortedValues[k-1];
		}
		sortedValues[insert_position] = value; //insert current reading
	}

	//return average of NUM_TO_AVERAGE values
	float filtered_current = 0;
	for( byte read_counter = NUM_READS/2 - NUM_TO_AVERAGE/2; read_counter < NUM_READS/2 + NUM_TO_AVERAGE/2; read_counter++ )
	{
		filtered_current += sortedValues[read_counter];
	}
	
	filtered_current = filtered_current/NUM_TO_AVERAGE;
	
	//CSK 6/25/2013 Average this result with the last value we displayed for a little more smoothing
	filtered_current = (filtered_current + glb_previous_current_in_amps)/2;

	glb_previous_current_in_amps = filtered_current;
	//CSK 6/25/2013 Keep the display from bouncing when it should be 0.0A
	//CSK 10/25/2013 Modified so it doesn't run calculation when we force current to zero
	//CSK 11/20/2013 Modified to handle A2D from Allegro 100A sensor ACS758LCB-100U-PFF
	//Serial.println(filtered_current, 1);
	return A2D_TO_AMPS( filtered_current );
}

void readRPM(void)
{
	//Need to make app/ for IR RPM to analog output so we don't have to suffer delays trying to read pulses here
	//http://www.pololu.com/catalog/product/2455
	// read raw sensor values
	//delayMicroseconds(100);
	//  pinMode(QTR_DETECTOR_PIN, INPUT);
	//for ( int milliseconds = 0; milliseconds <= 250; milliseconds++)
	//{
	//glb_RPM_Value = glb_RPM_Value + digitalRead(QTR_DETECTOR_PIN);
	//delayMicroseconds(800);
	//}
	//glb_RPM_Value = glb_RPM_Value/2;
	return;
}


void sweep_requested(void)
{
	#pragma region sweep_selected
	SSD1306_display.clearDisplay();
	
	glb_int_present_pwm_setting = glb_servo_center;
	
	if (select_is_pressed( ENABLE_HOLD_TIME ))
	{
		SSD1306_display.setTextSize(2);
		while( !mode_increment_is_pressed() )
		{
			int int_requested_speed = 0;  
			
			//Sweep one way each time
			if (glb_int_present_pwm_setting == glb_servo_end)
			{
				while(glb_int_present_pwm_setting != glb_servo_start)
				{
					if ( mode_increment_is_pressed() )
					{
						goto EndSweep;
					}
					int_requested_speed = read_potentiometer(POT_VALUE_IS_SERVO_SPEED);
					myservo.writeMicroseconds(glb_int_present_pwm_setting);
					display_pwm("SWEEP MODE");
					delay(int_requested_speed);
					glb_int_present_pwm_setting -= 10;
				}
			}
			else
			{
				while(glb_int_present_pwm_setting != glb_servo_end)
				{
					if ( mode_increment_is_pressed() )
					{
						goto EndSweep;
					}
					int_requested_speed = read_potentiometer(POT_VALUE_IS_SERVO_SPEED);
					myservo.writeMicroseconds(glb_int_present_pwm_setting);
					display_pwm("SWEEP MODE");
					delay(int_requested_speed);
					glb_int_present_pwm_setting += 10;
				}
			}
			//SSD1306_display.display();
		}
	}
	EndSweep:
	SSD1306_display.clearDisplay();
	if ( mode_increment_is_pressed() )
	{
		increment_mode_state();
	}
	return;
	#pragma endregion sweep_selected
}

void talon_calibration(void)
{
	SSD1306_display.clearDisplay();
	#define TALON_STARTING_ROW	14
	if ( select_is_pressed( ENABLE_HOLD_TIME ) )
	{
		//     SSD1306_display.clearDisplay();
		while( !mode_increment_is_pressed() )
		{
			//Set to 0 when pot value represents speed
			int int_pot_value_is_servo_position = 0;
			int int_requested_speed             = 0;
			SSD1306_display.setTextSize(1);
			//Serial.println(center_string_in_screen_width("Press and hold Red", 1));
			//SSD1306_display.setCursor( center_string_in_screen_width("Press and hold Red", 1), CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 );
			//CSK 4/15/2014 Use an asterisk to indicate we are in extended servo range mode
			SSD1306_display.setCursor( 23, TALON_STARTING_ROW );
			SSD1306_display.print(F("Press and hold"));

			//SSD1306_display.setCursor(center_string_in_screen_width("CAL Button on Talon", 1), CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 2);
			SSD1306_display.setCursor( 23, ( CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 ) + TALON_STARTING_ROW );
			SSD1306_display.print(F("Red CAL Button"));
			SSD1306_display.setCursor( 40, ( CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 2 ) + TALON_STARTING_ROW );
			SSD1306_display.print(F("on Talon"));
			SSD1306_display.setCursor( 32, ( CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 3 ) + TALON_STARTING_ROW );
			SSD1306_display.print(F("Push SEL to"));
			SSD1306_display.setCursor( 38, ( CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 4 ) + TALON_STARTING_ROW );
			SSD1306_display.print(F("start CAL"));
			SSD1306_display.display();
			
			//VVV CSK 11/18/2013 Added extra "press SEL" step to allow users to get set up
			int int_elapsed_time = 0;
			while( !select_is_pressed( DISABLE_HOLD_TIME ) )
			{
				if( mode_increment_is_pressed() )
				{
					//CSK 5/1/2014 Drop out of Talon CAL if mode is selected
					goto EndCAL;
				}
				else
				{
					//Wait for user to press Select button again to start CAL or check for timeout
					int_elapsed_time++;
					delay(250);
					if (int_elapsed_time >= 40 )
					{
						goto UserTimout;
					}
				}
			}
			//^^^ CSK 11/18/2013
			
			//delay(3000);
			
			myservo.writeMicroseconds(glb_servo_start);
			SSD1306_display.clearDisplay();
			SSD1306_display.setTextSize(2);
			//Serial.println(center_string_in_screen_width("Done", 2));
			//SSD1306_display.setCursor(center_string_in_screen_width("Setting", 2),CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2);
			SSD1306_display.setCursor( 22, CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 );
			SSD1306_display.print(F("Setting"));
			SSD1306_display.setCursor(34,CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 * 2);
			SSD1306_display.print(F("Start"));
			SSD1306_display.display();
			delay(1000);
			
			myservo.writeMicroseconds(glb_servo_end);
			
			SSD1306_display.clearDisplay();
			SSD1306_display.setTextSize(2);
			SSD1306_display.setCursor(22, CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2);
			SSD1306_display.print(F("Setting"));
			SSD1306_display.setCursor(46,CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 * 2);
			SSD1306_display.print(F("End"));
			SSD1306_display.display();
			delay(1000);
			////Center the servo
			
			myservo.writeMicroseconds(glb_servo_center);
			
			SSD1306_display.clearDisplay();
			SSD1306_display.setTextSize(2);
			SSD1306_display.setCursor(22, CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2);
			SSD1306_display.print(F("Setting"));
			SSD1306_display.setCursor(28,CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 * 2);
			SSD1306_display.print(F("Center"));
			SSD1306_display.display();
			delay(1000);
			SSD1306_display.clearDisplay();
			SSD1306_display.setTextSize(2);
			SSD1306_display.setCursor(40,CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 2);
			SSD1306_display.print(F("Done"));
			SSD1306_display.setTextSize(1);
			SSD1306_display.setCursor(19,CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 5);
			SSD1306_display.print(F("Release Red Cal"));
			SSD1306_display.setCursor(19,(CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 6) );
			SSD1306_display.print(F("Button on Talon"));
			SSD1306_display.setCursor(4,(CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE1 * 7) );
			SSD1306_display.print(F("Press MODE to Finish"));
			SSD1306_display.display();
			
			SSD1306_display.display();
			while( !mode_increment_is_pressed() );
			goto EndCAL;
		}
		//VVV CSK 11/18/2013 Added extra "press SEL" stuff to allow users to get set up
		UserTimout:
		SSD1306_display.clearDisplay();
		SSD1306_display.setTextSize(2);
		SSD1306_display.setCursor(12, CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2);
		SSD1306_display.print(F("Timed out"));
		SSD1306_display.display();
		delay(3000);
		//^^^ CSK 11/18/2013
		
	}
	
	EndCAL:
	SSD1306_display.clearDisplay();
	
	if ( mode_increment_is_pressed() )
	{
		increment_mode_state();
	}
	return;
}

void display_pwm(String function_name_string)
{
	//SSD1306_display.clearDisplay();
	#define SWEEP_STARTING_ROW 6
	SSD1306_display.setTextSize(1);
	SSD1306_display.setCursor( center_string_in_screen_width(function_name_string, 1), 2 );
	SSD1306_display.setTextColor(BLACK,WHITE);
	SSD1306_display.print( function_name_string);
	SSD1306_display.setTextSize(2);
	SSD1306_display.setCursor(6,26);
	SSD1306_display.setTextColor(WHITE);
	SSD1306_display.print(F("PWM="));
	//Keep value right justified

	//This is data that is changing
	//CSK 5/1/2014 Trying to speed up behavior by reducing screen writing to just what changes
	//Instead of clearing and rewriting whole screen, just clear a rectangle where data goes
	SSD1306_display.setCursor( 7 +  strlen("PWM=") * CHAR_WIDTH_FOR_FONTSIZE2, (CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 + SWEEP_STARTING_ROW) );
	SSD1306_display.fillRect( 7 +  strlen("PWM=") * CHAR_WIDTH_FOR_FONTSIZE2, (CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2 + SWEEP_STARTING_ROW), (5 * CHAR_WIDTH_FOR_FONTSIZE2), CHAR_HEIGHT_IN_PIXELS_FOR_FONTSIZE2, BLACK );

	//CSK 5/7/2014 This comparison looks wrong but
	//The spacing was getting fooled by round function in the read_potentiometer()
	if (glb_int_present_pwm_setting < 999)  SSD1306_display.print(F(" "));

	SSD1306_display.print( glb_int_present_pwm_setting );

	//CSK 8/21/2013 This device uses this extended ascii table set http://www.asciitable.pro/ascii_table.htm
	//http://forums.adafruit.com/viewtopic.php?f=47&t=30092
	//SSD1306_display.write(247);

	SSD1306_display.print(F("uS"));
	SSD1306_display.display();
	return;
}

void motor_damping( int int_requested_pwm, uint16_t uint_delay_time )
{
	//CSK 5/7/2014 If we are not at the target motor speed then gradually go to that speed to prevent motor/gear slamming
	if ( glb_int_present_pwm_setting < int_requested_pwm )
	{
		while( glb_int_present_pwm_setting++ != int_requested_pwm)
		{
			myservo.writeMicroseconds( glb_int_present_pwm_setting );
			delayMicroseconds(uint_delay_time);
			//glb_int_present_pwm_setting++;
		}
	}
	else
	{
		while( glb_int_present_pwm_setting-- != int_requested_pwm )
		{
			myservo.writeMicroseconds( glb_int_present_pwm_setting );
			delayMicroseconds(uint_delay_time);
			//glb_int_present_pwm_setting--;
		}
	}
	return;
}

int center_string_in_screen_width(String string2print, int fontsize)
{
	if ( ( fontsize == 1 && string2print.length() < CHAR_WIDTH_FOR_FONTSIZE1 ) || ( fontsize == 2 && string2print.length() < CHAR_WIDTH_FOR_FONTSIZE2 ) )
	{
		return ( SCREEN_WIDTH - ( string2print.length() * (fontsize == 1?CHAR_WIDTH_IN_PIXELS_FOR_FONTSIZE1:CHAR_WIDTH_IN_PIXELS_FOR_FONTSIZE2) ) )/2;
	}
	else return 0;
}

//Added this function to tell me how much RAM is left. CSK 3/12/2012
//http://jeelabs.org/2011/05/22/atmega-memory-use/
int freeRam ()
{
	//When freeRam reported 202 this code crashed every time.  Reporting 404 as of 8/23/2013 and crashes stopped
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

