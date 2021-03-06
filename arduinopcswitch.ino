//Arduino PC Pwr Switch
boolean debugging = false;
//define pins
int pin_read_rst_switch = 2;
int pin_read_pwr_switch = 4;
int pin_mb_led_ide = 7;
int pin_mb_led_pwr = 8;
int pin_pwr = 9;
int pin_rst = 10;
int pin_led_pwr = 12;
int pin_led_ide = 13;

//vars
boolean cold_start = true;
boolean booting_up = true;
boolean computer_state_checked = false;
unsigned long bootup_time = millis();
unsigned long thetime = bootup_time;

int debounce_delay = 66;

boolean read_state = false;
boolean under_remote_command = false;
boolean under_local_command = false;

boolean state_pwr_switch = false;
boolean last_state_pwr_switch = state_pwr_switch;
unsigned long time_pwr_last_state = millis();
boolean debouncing_pwr_switch = false;
boolean switch_state_changed_pwr = false;

boolean state_rst_switch = false;
boolean last_state_rst_switch = state_rst_switch;
unsigned long time_rst_last_state = millis();
boolean debouncing_rst_switch = false;
boolean switch_state_changed_rst = false;

boolean state_mb_pwr_led = true; //INVERTED. When true, MB LED is off
boolean last_state_mb_pwr_led = state_mb_pwr_led;
boolean debouncing_mb_pwr_led = false;
unsigned long time_mb_pwr_led_last_state = millis();
boolean state_changed_pwr_led = false;

//com
String sBuffer = "";
String computerpowerstate = "";
boolean boolean_computer_power_state = false;
boolean sleeping = false;
boolean sleeping_count = 0;
boolean sleeping_count_max = 4;
String usbInstructionDataString = "";
int usbCommandVal = 0;
boolean USBcommandExecuted = true;
String usbCommand = "";
unsigned long lastcmdtime = millis();
//reporting states
boolean report_pwr_led = false;
boolean report_comp_state = true;
//heartbeat
unsigned long last_heartbeat_request_time = millis();
unsigned long heartbeat_wait = 900000; //15 minutes
int heartbeat_response_delay = 30000;   //wait this many milliseconds to hear a response from raspi
boolean heartbeat = false;
boolean heartbeat_requested = false;
boolean defibbing = false;


void setup() {
	Serial.begin(57600);

  	//set pinmodes
  	pinMode(pin_read_rst_switch, INPUT);
  	pinMode(pin_read_pwr_switch, INPUT);
	pinMode(pin_mb_led_ide, INPUT);
	pinMode(pin_mb_led_pwr, INPUT);
  	pinMode(pin_pwr, OUTPUT);
  	pinMode(pin_rst, OUTPUT);
  	pinMode(pin_led_pwr, OUTPUT);
  	pinMode(pin_led_ide, OUTPUT);

  	//set initial pin states
  	digitalWrite(pin_led_pwr, LOW);
  	digitalWrite(pin_led_ide, LOW);
  	digitalWrite(pin_pwr, LOW);
  	digitalWrite(pin_rst, LOW);

  	if(debugging) {
		heartbeat_wait = 6000; //6 seconds
		heartbeat_response_delay = 3000;   //three seconds
	}

  	delay(100);  //wait
}

/*functions*/
void printsbuffer () {
	//print sBuffer
	if(sBuffer != "") {
		Serial.println(sBuffer);
		sBuffer = "";
	}
}
void addtosbuffer (String param, String value) {
	if(sBuffer == "") {
		sBuffer = "t=" + (String)millis() + "&" + param + "=" + value;
	} else {
		sBuffer = sBuffer + "&" + param + "=" + value;
	}
}
void handle_heartbeat() {
	if(!cold_start) {
		thetime = millis();
		if(!heartbeat_requested && !defibbing) {
			if( thetime > (last_heartbeat_request_time + heartbeat_wait) ) {
				//heartbeat is positive but has become stale. request a new heartbeat
				last_heartbeat_request_time = thetime;
				addtosbuffer("rhb", "1");
				heartbeat_requested = true;
				heartbeat = false;
			}
		}
		if(!heartbeat && heartbeat_requested && !defibbing) {
			if ((thetime - last_heartbeat_request_time) > heartbeat_response_delay) {
				//haven't heard from the raspberry pi since we sent the heartbeat request
				//turn on the computer
				defibbing = true;
				addtosbuffer("actionstatus", "defibbing");
				if((computerpowerstate == "off") || (computerpowerstate == "sleeping")) {
					digitalWrite(pin_pwr, HIGH);
					delay(850);
					digitalWrite(pin_pwr, LOW);
				}
			}
		}
	}
}
void checkarduinobootstatus() {
	if(booting_up) {
		if(cold_start) {
			addtosbuffer("bootstatus", "cold_start");
			cold_start = false;
		} else {
			addtosbuffer("bootstatus", "warm_start");
		}
		computer_state_checked = false;
		booting_up=false;
	}
}
void reset_vars() {
	//a consequence of testing/development on leonardo
}
void handle_switches() {
	//handle switches
	if(!under_remote_command) {
		//handle power switch
		read_state = digitalRead(pin_read_pwr_switch);
		if((read_state != last_state_pwr_switch) && !debouncing_pwr_switch) {
			//state changed
			debouncing_pwr_switch = true;
			time_pwr_last_state = millis();
			under_local_command = true;
		} else if ((read_state != last_state_pwr_switch) && debouncing_pwr_switch) {
			if((millis() - debounce_delay) > time_pwr_last_state) {
				state_pwr_switch = read_state;
				last_state_pwr_switch = read_state;
				switch_state_changed_pwr = true;
				debouncing_pwr_switch = false;
			}
		}
		if(switch_state_changed_pwr) {
			if(state_pwr_switch) {
				digitalWrite(pin_pwr, HIGH);
				addtosbuffer("pwrbutton", "closed");
			} else {
				digitalWrite(pin_pwr, LOW);
				under_local_command = false;
				addtosbuffer("pwrbutton", "open");
			}
			switch_state_changed_pwr = false;
		}
		//handle reset switch
		read_state = digitalRead(pin_read_rst_switch);
		if((read_state != last_state_rst_switch) && !debouncing_rst_switch) {
			//state changed
			debouncing_rst_switch = true;
			time_rst_last_state = millis();
			under_local_command = true;
		} else if ((read_state != last_state_rst_switch) && debouncing_rst_switch) {
			if((millis() - debounce_delay) > time_rst_last_state) {
				state_rst_switch = read_state;
				last_state_rst_switch = read_state;
				switch_state_changed_rst = true;
				debouncing_rst_switch = false;
			}
		}
		if(switch_state_changed_rst) {
			if(state_rst_switch) {
				digitalWrite(pin_rst, HIGH);
				addtosbuffer("rstbutton", "closed");
			} else {
				digitalWrite(pin_rst, LOW);
				under_local_command = false;
				addtosbuffer("rstbutton", "open");
			}
			switch_state_changed_rst = false;
		}
	}
}
void read_mb_leds() {
	//read MB led signals	
	//pwr led
	thetime = millis();
	read_state = digitalRead(pin_mb_led_pwr);
	if((read_state != last_state_mb_pwr_led) && !debouncing_mb_pwr_led) {
		//state changed
		debouncing_mb_pwr_led = true;
		if((thetime - time_mb_pwr_led_last_state) <= 750) {
			sleeping_count++;
		}
		time_mb_pwr_led_last_state = thetime;
	} else if ((read_state != last_state_mb_pwr_led) && debouncing_mb_pwr_led) {
		if((thetime - debounce_delay) > time_mb_pwr_led_last_state) {
			state_mb_pwr_led = read_state;
			last_state_mb_pwr_led = read_state;
			state_changed_pwr_led = true;
			debouncing_mb_pwr_led = false;
		}
	}
	if(state_changed_pwr_led) {
		if(!state_mb_pwr_led) {
			if(report_pwr_led) {
				addtosbuffer("mbled", "on");
			}
		} else {
			if(report_pwr_led) {
				addtosbuffer("mbled", "off");
			}
		}
		state_changed_pwr_led = false;
	}
	//ide led
	/*
	read_state = digitalRead(pin_mb_led_ide);
	if(read_state) {
		digitalWrite(pin_led_ide, HIGH);
	} else {
		digitalWrite(pin_led_ide, LOW);
	}
	*/
}
//comp state
void assess_comp_state() {
	thetime = millis();
	if((thetime - time_mb_pwr_led_last_state) <= 750) {
		//power led changed state sometime in the last 750 milliseconds.
		if (!sleeping && (sleeping_count >= sleeping_count_max)) {
			computerpowerstate = "sleeping";
			addtosbuffer("computerpowerstate", computerpowerstate);
			sleeping = true;
			sleeping_count = 0;
		}
	} else if ( ((thetime - time_mb_pwr_led_last_state) > 750) ) {
		if((boolean_computer_power_state != state_mb_pwr_led) || !computer_state_checked || sleeping) {
			boolean_computer_power_state = state_mb_pwr_led;
			if(!boolean_computer_power_state) {
				computerpowerstate = "on";
				addtosbuffer("computerpowerstate", computerpowerstate);
			} else {
				computerpowerstate = "off";
				addtosbuffer("computerpowerstate", computerpowerstate);
			}
			computer_state_checked = true;
			sleeping = false;
			sleeping_count = 0;
		}
	}	
}
void delegate(String cmd, int cmdval) {
	if (cmd.equals("p")) {
		digitalWrite(pin_pwr, HIGH);
		delay(cmdval);
		digitalWrite(pin_pwr, LOW);
		addtosbuffer("actionstatus", "fpbp");
	}
	if (cmd.equals("r")) {
		if(cmdval == 1) {
			digitalWrite(pin_rst, HIGH);
			delay(200);
			digitalWrite(pin_rst, LOW);
			addtosbuffer("actionstatus", "frbp");
		}
	}
	if(cmd.equals("f")) {
		if(cmdval == 1) {
			report_pwr_led = true;
			addtosbuffer("comstatus", "pled_status_reporting_on");
		} else {
			report_pwr_led = false;
			addtosbuffer("comstatus", "pled_status_reporting_off");
		}
	}
	if(cmd.equals("c")) {
		if(cmdval == 1) {
			report_comp_state = true;
			addtosbuffer("comstatus", "computerpower_status_reporting_on");
		} else {
			report_comp_state = false;
			addtosbuffer("comstatus", "computerpower_status_reporting_off");
		}
	}

	if(cmd.equals("h")) {
		if(cmdval == 1) {
			//received a heartbeat
			addtosbuffer("comstatus", "received_heartbeat");
			last_heartbeat_request_time = millis();
			heartbeat = true;
			heartbeat_requested = false;
			defibbing = false;
		}
	}
	if(cmd.equals("d")) {
		if(cmdval == 1) {
			//stop defibbing
			heartbeat = false;
			heartbeat_requested = false;
			defibbing = false;
		}
	}
	if(cmd.equals("s")) {
		if(cmdval == 1) {
			//please report computerpowerstate
			addtosbuffer("computerpowerstate", computerpowerstate);
		}
	}

}
void serialListen()
{
  char arduinoSerialData; //FOR CONVERTING BYTE TO CHAR. here is stored information coming from the arduino.
  String currentChar = "";
  if(Serial.available() > 0) {
    arduinoSerialData = char(Serial.read());   //BYTE TO CHAR.
    currentChar = (String)arduinoSerialData; //incoming data equated to c.
    if(!currentChar.equals("1") && !currentChar.equals("2") && !currentChar.equals("3") && !currentChar.equals("4") && !currentChar.equals("5") && !currentChar.equals("6") && !currentChar.equals("7") && !currentChar.equals("8") && !currentChar.equals("9") && !currentChar.equals("0") && !currentChar.equals(".")) { 
      //the character is not a number, not a value to go along with a command,
      //so it is probably a command.
      if(!usbInstructionDataString.equals("")) {
        //usbCommandVal = Integer.parseInt(usbInstructionDataString);
        char charBuf[30];
        usbInstructionDataString.toCharArray(charBuf, 30);
        usbCommandVal = atoi(charBuf);
      }
      if((USBcommandExecuted == false) && (arduinoSerialData == 13)) {
        delegate(usbCommand, usbCommandVal);
        USBcommandExecuted = true;
        lastcmdtime = millis();
      }
      if((arduinoSerialData != 13) && (arduinoSerialData != 10)) {
        usbCommand = currentChar;
      }
      usbInstructionDataString = "";
    } else {
      //in this case, we're probably receiving a command value.
      //store it
      usbInstructionDataString = usbInstructionDataString + currentChar;
      USBcommandExecuted = false;
    }
  }
}

void loop() {
	serialListen();
	handle_heartbeat();
	read_mb_leds();
	assess_comp_state(); //This needs to come after read_mb_leds
	checkarduinobootstatus();
	printsbuffer();
}  /* end of loop */
