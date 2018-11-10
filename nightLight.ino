//Include Approprate Libraries
#include <neopixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

//////////////////////////////
//    CLOUD VARIABLES       //
//////////////////////////////
int lightReading = 0;
long wakeupTime=630;
int currentTime=0;
int coReading=0;

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
  pinMode(A1, INPUT); // CO
  pinMode(D3, OUTPUT); // PWM Heater for Methan Sensor

  // Iniltize Neopixels
  strip.begin();
  strip.show();

  // Initilize Time (And correct for DST)
  Time.zone(-7);
    if(Time.isDST())
        Time.beginDST();
    else
        Time.endDST();

  // Initilize Cloud Variables 
  Particle.variable("lightReading", lightReading);
  Particle.variable("coReading", coReading);
  Particle.variable("wakeupTime", wakeupTime);
  Particle.variable("currentTime", currentTime);
  Particle.function("setWakeup",setWakeup);
}

void loop(){
	lightReading = constrain(map(analogRead(A5),1850,250,100,255),0,255);
	currentTime=(Time.hour())*100+Time.minute();
  mq07timers();

	if(currentTime > wakeupTime && currentTime< wakeupTime+200){
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

//////////////////////////////
//    VARS for MQ-07 CO     //
//////////////////////////////
bool heaterOn=false;
bool sensorOn=false;
unsigned long heaterTimer=0;
void mq07timers(){
    //Runs CO sensor every 2 minutes. 
    //Cycle is:
    // off 2 minutes
    // heater on (D3->5V) for 1 minute
    // heater low (D3->1.4V) for 1.5 minutes
    // take reading
    if(heaterOn && millis()-heaterTimer>60000){
    analogWrite(D3,1.4/5*255);
    heaterOn=false;
    sensorOn=true;
    heaterTimer=millis();
  }
  else if(sensorOn && millis()-heaterTimer>90000){
    coReading=analogRead(A1);
    analogWrite(D3,0);
    sensorOn=false;
    heaterTimer=millis();
  }
  else if (millis()-heaterTimer>120000){
    analogWrite(D3,255);
    heaterOn=true;
    heaterTimer=millis();
  }
}
