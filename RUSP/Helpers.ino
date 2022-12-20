void serialPassthrough(){	//Taken from Erik Nyquist's example
	if (Serial.available()) {     //Pass anything entered in Serial off to the TOBY 
		Serial1.write(Serial.read());   
	}
	if (Serial1.available()) {     // Pass anything received from TOBY to serial console
		Serial.write(Serial1.read()); 
	}
}

void serialPassthroughOneWay(){	
	if (Serial.available()) {     //Pass anything entered in Serial off to the TOBY 
		Serial1.write(Serial.read());   
	}
}

void ISR_rotary(){	//Interrupt Service Routine for the rotary dial's "pulse" switch, which is a tied-to-GND normlally-open limit switch that rolls against a cam. 	
	interrupt_rot = millis();	//record time of interrupt
	pulseOn = true;
	spinning = true;
	delay(10);
	digitalWrite(LED_STAT, HIGH);
}

void ISR_hall(){	
	digitalWrite(LED_FILAMENT, HIGH);
	delay(500);
	digitalWrite(LED_FILAMENT, LOW);
}


void rotary_accumulator(){ 	// Accumulates digits from rotary dial
	if (pulseOn == true && (millis() - interrupt_rot  > 30)) {	//Debounce detection
		pulseOn = false;
		digitalWrite(LED_STAT, LOW);
		n++;
		if (digitalRead(SW_Alt) == LOW){	//See if we're in alt mode 
			if (digitalRead(SW_Fn) == LOW){	//See if the Function button is depressed while in alt mode
				Speed = true;
			} else {
				Alt = true;
			}
		}
		else if (digitalRead(SW_Fn) == LOW){	//See if the Fn button is being depressed while the dial is turning.
			Fn = true;
		} 
	}
	//ACCUMULATE PULSES
	if (spinning == true && (millis() - interrupt_rot > 700)){	//If more than this many ms have elapsed since the last pulse, assume a number is complete
		if (n % 2 != 0){ 	//Use modulo to see if number is odd. Because we're counting up *and* down, the final result has to be even. If it's not, assume a pulse was missed and add one. 
			n++;
		}
		n = n/2;	//Divide by 2 to get the right number. Above conditional asures n is even.
		if (n >= 10){	//The tenth position on the dial is 0
			n = 0;
		}
		PNumber[k] = n;
		oled.clear();
		for (int j = 0; j <= k; j++){	//DISPLAY ON OLED
			//Serial.print(j);
			Serial.print(PNumber[j]);
			oled.print(itoa(PNumber[j],cbuf,10),(j*lspace),30); //A KLUGE. Should be able to send the whole number as a char array straight to oled.print and let that handle the character spacing. Instead, I'm using the loop index to advance the characters
		}

		Serial.println("");
		if (Fn == true){
			////ADD STAR/POUND DIAL FUNCTIONS
			Fn = false;
			if (n == 2){
				oled.clear();
				oled.print("*",0,30);
			}
			else if (n == 1){
				oled.clear();
				oled.print("#",0,30);
			}
		}
		if (Alt == true){
			pg = epd_displayContacts(n);	
			oled.clear();
			ClearPNum();   //If in Alt mode, don't accumalate these numbers in the PNum buffer
			Alt = false;
		}
		if (Speed == true){
			SDgetContact((pg*9)-9+n);	//Get contact phone number based on the page number from the last time epd_showContacts was called and the rotary dial input
			ClearPNum();   //Clear whatever number has been entered so far
			k = kc;			//Set the current max phone number index to be that of the Contact Numbre
			for (int j = 0; j <= (k-3); j++){	//FIX: I don't understand why I need to subtract from k 	
				PNumber[j] = CNumber[j];	//Set current buffered phone number to be the contact number that SDgetContact provided 
				Serial.print(PNumber[j]);
			}
			Serial.println("");
			oled.clear();
			for (int j = 0; j <= k-3; j++){	//DISPLAY RECALLED CONTACT PHONE NUMBER ON OLED
				Serial.print(PNumber[j]);
				oled.print(itoa(PNumber[j],cbuf,10),(j*lspace),30); //A KLUGE. Should be able to send the whole number as a char array straight to oled.print and let that handle the character spacing. Instead, I'm using the loop index to advance the characters
			}
			k = k-2;	//More annoying incomprehensible corrections
			toby.call(PNumber, k);
			Speed = false;
		}
		else {
			newNum = true;
			k++;	//advance to next number position
		}
		stopwatch_pNum = millis();
		n = 0;	//reset digit
		spinning = false;
	}
}

void pNumAutoClear(uint32_t pndelay){
	if (millis() > (stopwatch_pNum + pndelay)){
		ClearPNum();   //Clear whatever number has been entered so far
		;stopwatch_pNum = millis();
		toby.refresh();		//for good measure
	}
}

void ClearPNum(){  //Clear the currently entered phone number
	for (int j = 0; j < 30; j++){
		PNumber[j] = 0;  //Reset the phone number
	}
	no1s = 0;
	oled.clear();
	newNum = false;
	k = 0;    //Reset the position of the phone number
}

void BackspacePNum(){  //Clear the previously entered digit
	k--;
	PNumber[k] = 0;  //delete prevoius entry
}

long readVcc() {	//Read 1.1V reference against AVcc. Thank you to Scott: https://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); // set the reference to Vcc and the measurement to the internal 1.1V reference
	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA,ADSC)); // measuring
	uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
	uint8_t high = ADCH; // unlocks both
	long mV = (high<<8) | low;
	
	mV = 1125300L / mV; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	return mV;
}

int readBatt() {	//Convert charge in mV (reported from readVcc) to percent	
	int chargeLevel;
	int LiPoRange = LIPOMAX - LIPOMIN;
	
	chargeLevel = ((readVcc() - LIPOMIN)*100)/LiPoRange;
	if (chargeLevel > 100){chargeLevel = 100;}
	if (chargeLevel < 0){chargeLevel = 0;}
	return chargeLevel; 
}

void dispSignal(){
	int k = 24;	//digit in chars returned from TOBY to pull out. KLUGE! Will probably fail in other regions.
	int len = 1;
	oled.clear();
	for (int j = 0; j <= len; j++){
		ibuf[j] = toby.signal(k) - '0';		//convert char to int
		k++;
	}
	oled.print("Signal:",0,30);
	for (int j = 0; j <= len; j++){	//DISPLAY ON OLED
		oled.print(itoa(ibuf[j],cbuf,10),75+(j*lspace),30); //A KLUGE. Should be able to send the whole number as a char array straight to oled.print and let that handle the character spacing. Instead, I'm using the loop index to advance the characters
	}
}

void dispCallID(){
	int k = 20;
	int len = 9;
	for (int j = 0; j <= 20; j++){
		ibuf[j] = 0;
	}
	oled.clear();
		for (int j = 0; j <= len; j++){
			ibuf[j] = toby.callID(k) - '0';		//convert char to int
			k++;
		}
		for (int j = 0; j <= len; j++){	//DISPLAY ON OLED
			oled.print(itoa(ibuf[j],cbuf,10),(j*lspace),30); //A KLUGE. Should be able to send the whole number as a char array straight to oled.print and let that handle the character spacing. Instead, I'm using the loop index to advance the characters
		}
}

void dispBatt(){
	oled.clear();
	Serial.print(F("BATT: "));
	Serial.print(readBatt());
	Serial.println(F("%"));
	oled.print("BATT:",0,30);
	if (battDisplay == 0){
		oled.print(itoa(readBatt(),cbuf,10),65,30);	//use itoa to display readBatt
		oled.print("%",100,30);
	} 
	else {
		oled.print(itoa(readVcc(),cbuf,10),65,30);	//use itoa to display readBatt
	}
}

void blinkRingLEDs() {
	if (ledToggle == true){		//make filament and bell LED flash alternatingly
		digitalWrite(LED_FILAMENT, HIGH);
		digitalWrite(LED_BELL, LOW);
		if (ledCounter % 100 == 0){
			ledToggle = false;
		}
	} else {
		digitalWrite(LED_FILAMENT, LOW);
		digitalWrite(LED_BELL, HIGH);
		if (ledCounter % 100 == 0){
			ledToggle = true;
		}
	}
}

void ringPattern(){
	ledCounter++;
	if (ledCounter <= ledRingPattern_t1){
		blinkRingLEDs();
		if (bellOn == true){	//ring bell only every other cycle
			if (readBatt() > 55){
				ringBell();
			}
		}	
	} 
	else if (ledCounter > ledRingPattern_t1 && ledCounter <= ledRingPattern_t2){
		analogWrite(LED_FILAMENT, 30);
		digitalWrite(LED_BELL, LOW);
		digitalWrite(RINGER_P, LOW);
		digitalWrite(RINGER_N, LOW);
	}
	else if (ledCounter > ledRingPattern_t2 && ledCounter <= ledRingPattern_t3){
		blinkRingLEDs();
		if (bellOn == true){
			if (readBatt() > 55){
				ringBell();
			}
		}
	}
	else if (ledCounter > ledRingPattern_t3 && ledCounter<= ledRingPattern_t4){
		analogWrite(LED_FILAMENT, 30);
		digitalWrite(LED_BELL, LOW);
		digitalWrite(RINGER_P, LOW);
		digitalWrite(RINGER_N, LOW);
	}
	else if (ledCounter > ledRingPattern_t4){
		ledCounter = 0;
		if (bellOn == true){ 	//toggle this bool
			bellOn = false;
		} else {
			bellOn = true;
		}
	}
}

void ringBell(){
	bellDelayCounter++;
	if (bellDelayCounter == bellDutyDelay){ 	//Turn solenoid off for part of the cycle
		digitalWrite(RINGER_P, LOW);
		digitalWrite(RINGER_N, LOW);
	}
	if (bellDelayCounter >= bellDelay){	//Turn solenoid on every full cycle (where bellDelayCounter == the bellDelay
		if (bellFlag == true){			//...and the solenoid polarity will depend on the previous state
			digitalWrite(RINGER_P, HIGH);
			digitalWrite(RINGER_N, LOW);
			bellFlag = false;
		}
		else {
			digitalWrite(RINGER_P, LOW);
			digitalWrite(RINGER_N, HIGH);
			bellFlag = true;
		}
		bellDelayCounter = 0;
	}
}

/*void calibrateBell(){	//pan through a range of bell hammer frequencies to find resonance
	if (bellCalOn == false){	//initialize some stuff the first time we're in this function
		bellCalOn = true;
		stopwatch_bellCal = millis();
		bellDelay = bellMin;
	}

	if (millis() < stopwatch_bellCal + bellPanDwell){
		ringBell();
	}
	else if (bellDelay > bellMax) {	//finish calibration routine at this value for bellDelay 
		bellDelay = bellMin;
		bellCalOn = false;	//main loop won't re-enter calibrateBell if this is false
	}
	else {		//when the bellCalSettleTime is exceeded, reset stopwatch_bellCal, increment the bellDelay, and print the value to the console
		stopwatch_bellCal = millis();	
		bellDelay++;
		Serial.println(bellDelay);
	}
}*/

void ringer(){
	if(toby.ringCheck() == true){
		ringPattern();
		if (ringCnt == 0 || ringCnt == 1){	//Run this twice per ring. For some reason the very first time it runs the text is garbled. 
			dispCallID();
		}
		ringCnt++;
	}
	else{
		digitalWrite(LED_FILAMENT, LOW);
		digitalWrite(LED_BELL, LOW);
		digitalWrite(RINGER_P, LOW);
		digitalWrite(RINGER_N, LOW);
		ringCnt = 0;
	}
}

void testRinger(){
	if(testRing == true){
		ringPattern();
		if (ringCnt == 0 || ringCnt == 1){	//Run this twice per ring. For some reason the very first time it runs the text is garbled. 
			oled.clear();
			oled.print("6313452124",0,30);
		}
		ringCnt++;
	}
	else{
		digitalWrite(LED_FILAMENT, LOW);
		digitalWrite(LED_BELL, LOW);
		digitalWrite(RINGER_P, LOW);
		digitalWrite(RINGER_N, LOW);
		oled.clear();
		ringCnt = 0;
	}
}

void shutdown(){
	//Serial.println("Shutdown signal sent. Waiting for cell powerdown...");
	oled.print("Turning off",0,30);
	digitalWrite(LED1A, HIGH);
	toby.powerdown();
	digitalWrite(RELAY_OFF, HIGH);	
	//POWER KILLED
}


