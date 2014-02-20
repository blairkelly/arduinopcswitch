//Arduino PC Pwr Switch

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
int debounce_delay = 66;
String sBuffer = "";

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

boolean state_mb_pwr_led = false;
boolean last_state_mb_pwr_led = false;
boolean debouncing_mb_pwr_led = false;
unsigned long time_mb_pwr_led_last_state = millis();
boolean state_changed_pwr_led = false;

boolean computer_power_state = false;
boolean sleeping = false;

//com vars
//general
String usbInstructionDataString = "";
int usbCommandVal = 0;
boolean USBcommandExecuted = true;
String usbCommand = "";
unsigned long lastcmdtime = millis();
//specific
boolean report_pwr_led = false;
boolean report_comp_state = true;


void setup() {
	Serial.begin(57600);
  	Serial.println(" ");

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

  	delay(debounce_delay);
}

void loop() {
	serialListen();

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


	//read MB led signals	
	//pwr led
	read_state = digitalRead(pin_mb_led_pwr);
	if((read_state != last_state_mb_pwr_led) && !debouncing_mb_pwr_led) {
		//state changed
		debouncing_mb_pwr_led = true;
		time_mb_pwr_led_last_state = millis();
	} else if ((read_state != last_state_mb_pwr_led) && debouncing_mb_pwr_led) {
		if((millis() - debounce_delay) > time_mb_pwr_led_last_state) {
			state_mb_pwr_led = read_state;
			last_state_mb_pwr_led = read_state;
			state_changed_pwr_led = true;
			debouncing_mb_pwr_led = false;
		}
	}
	if(state_changed_pwr_led) {
		if(state_mb_pwr_led) {
			if(report_pwr_led) {
				addtosbuffer("mbpwrledstate", "on");
			}
		} else {
			if(report_pwr_led) {
				addtosbuffer("mbpwrledstate", "off");
			}
		}
		state_changed_pwr_led = false;
	}
	//ide led
	read_state = digitalRead(pin_mb_led_ide);
	if(read_state) {
		digitalWrite(pin_led_ide, HIGH);
	} else {
		digitalWrite(pin_led_ide, LOW);
	}

	//comp state
	if((millis() - time_mb_pwr_led_last_state) > 750) {
		if(computer_power_state != state_mb_pwr_led) {
			computer_power_state = state_mb_pwr_led;
			if(computer_power_state) {
				addtosbuffer("computerpowerstate", "on");
			} else {
				addtosbuffer("computerpowerstate", "off");
			}
			sleeping = false;
		}
	} else {
		if(!sleeping) {
			addtosbuffer("computerpowerstate", "sleeping");
			sleeping = true;
		}
	}


	//handle stuff coming in from outside.
	if(!under_local_command) {

	}



	//print sBuffer
	if(sBuffer != "") {
		Serial.println(sBuffer);
		sBuffer = "";
	}
	
}  /* end of loop */

















void addtosbuffer (String param, String value) {
	if(sBuffer == "") {
		sBuffer = "t=" + (String)millis() + param + "=" + value;
	} else {
		sBuffer = sBuffer + "&" + param + "=" + value;
	}
}

void delegate(String cmd, int cmdval) {
	if (cmd.equals("p")) {
		digitalWrite(pin_pwr, HIGH);
		delay(cmdval);
	} else if (cmd.equals("s")) {
		digitalWrite(pin_pwr, LOW);
		delay(cmdval);
	}
	if(cmd.equals("f")) {
		if(cmdval == 1) {
			report_pwr_led = true;
		} else {
			report_pwr_led = false;
		}
	}
	if(cmd.equals("c")) {
		if(cmdval == 1) {
			report_comp_state = true;
		} else {
			report_comp_state = false;
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