//Include Approprate Libraries
#include <neopixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

//////////////////////////////
//    CLOUD VARIABLES       //
//////////////////////////////
int lightReading = 0;
long wakeupTime=0;
int currentTime=0;

//////////////////////////////
//    VARS FOR NEOPIXELS    //
//////////////////////////////
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      7
#define PIN 6         
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, WS2812B);

void setup(){
  pinMode(A5,INPUT);  // Photocell
  pinMode(D2, INPUT); // Temp Humidity
  pinMode(A1, INPUT); // Methane
  pinMode(D3, OUTPUT); // PWM Heater for Methan Sensor

  // Iniltize Neopixels
  strip.begin();
  strip.show();

  // Initilize Time (And correct for DST)
  Time.zone(-6);
    if(Time.isDST())
        Time.beginDST();
    else
        Time.endDST();

  // Initilize Cloud Variables 
  Particle.variable("lightReading", lightReading);
  Particle.variable("wakeupTime", wakeupTime);
  Particle.variable("currentTime", currentTime);
  Particle.function("setWakeup",setWakeup);
}

void loop(){
	lightReading = constrain(map(analogRead(A0),1850,250,100,255),0,255);
	currentTime=(Time.hour()+1)*100+Time.minute();

	delay(500);
	if(currentTime > wakeupTime){
		for(int i=0;i<7;i++)
		  	strip.setPixelColor(i, 0, 255-lightReading, 0);	
	}
	else{
		for(int i=0;i<7;i++)
	  		strip.setPixelColor(i, 255-lightReading, 255-lightReading, 255-lightReading);
	}
	strip.show();
}

int setWakeup(String args){
	wakeupTime=args.toInt();
	return wakeupTime;
}
