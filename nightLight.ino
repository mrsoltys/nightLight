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

//////////////////////////////
//       Clock LEDs         //
//////////////////////////////
#define N_LEDS 15
Adafruit_NeoPixel strip0  = Adafruit_NeoPixel(N_LEDS, 0, WS2812B);
Adafruit_NeoPixel strip1  = Adafruit_NeoPixel(N_LEDS, 1, WS2812B);
Adafruit_NeoPixel strip2  = Adafruit_NeoPixel(N_LEDS, 4, WS2812B);
Adafruit_NeoPixel strip3  = Adafruit_NeoPixel(N_LEDS, 5, WS2812B);

//////////////////////////////
//       Traffic Vars       //
//////////////////////////////
#define TRAFFIC_RESP   "hook-response/googleDistanceMatrix"
#define TRAFFIC_PUB    "googleDistanceMatrix"
int updateTrafficTime=0;
int trafficMin=0;
int trafficMax=0;
int traffic0=0;

//////////////////////////////
//       Weather Vars       //
//////////////////////////////
#define CUR_RESP   "hook-response/openWeatherByCity"
#define CUR_PUB    "openWeatherByCity"
int updateCurWeatherTime=0;
int temp0=0;
#define FORECAST_RESP   "hook-response/openWeatherForecast"
#define FORECAST_PUB    "openWeatherForecast"
int updateForecastTime=0;
int tempMin=0;
int tempMax=0;

void setup(){
  //Subscribe to forecast and traffic events
  Particle.subscribe(TRAFFIC_RESP, trafficHandler, MY_DEVICES);
  Particle.subscribe(CUR_RESP, currentTempHandler, MY_DEVICES);
  Particle.subscribe(FORECAST_RESP, forecastHandler, MY_DEVICES);

  //Variables to share to cloud
  Particle.variable("curTraffic", traffic0);
  Particle.variable("tempMin",tempMin);
  Particle.variable("tempMax",tempMax);
  Particle.variable("temp",temp0);
  Particle.variable("lightReading", lightReading);
  Particle.variable("coReading", coReading);
  Particle.variable("wakeupTime", wakeupTime);
  Particle.variable("currentTime", currentTime);
  //Particle.variable("minTraffic", trafficMin);
  //Particle.variable("maxTraffic", trafficMax);
  //Particle.variable("TrafficTemp",trafficTemp);

   // Initilize Cloud Functions
  Particle.function("setWakeup",setWakeup);

  //Set up sensors
  pinMode(A5,INPUT);  // Photocell
  pinMode(D2, INPUT); // Temp Humidity
  pinMode(A1, INPUT); // CO
  pinMode(D3, OUTPUT); // PWM Heater for Methan Sensor

  // Iniltize Neopixels
  strip.begin();
  strip.setBrightness(64);
  strip0.begin();
  strip1.begin();
  strip2.begin();
  strip3.begin();
  strip.show();

  // Initilize Time (And correct for DST)
    Time.zone(-5);
    if(Time.isDST())
        Time.beginDST();
    else
        Time.endDST();
}

void loop(){
  clearStrips();
  
 //Display Time
   showMinute(Time.minute());
   showHour(Time.hour()-1,Time.minute());

 //Update Traffic Every 10 minutes
   if((Time.now()-updateTrafficTime)>=600)
          getTraffic(); 
   //trafficTemp=map(traffic0,trafficMin,trafficMax,60,100);
   dispTemp(map(traffic0,trafficMin,trafficMax,60,100),0);

 //Update Current Temp every 6 minutes
    if((Time.now()-updateCurWeatherTime)>=360)
            getWeather(); 
    dispTemp(temp0,3);dispTemp(temp0,4);

  //Update Forecast every 3 hours and 1 minute
    if((Time.now()-updateForecastTime)>=10860)
            getWeatherForecast(); 
    dispTemp(tempMax,1);dispTemp(tempMax,2);
    dispTemp(tempMin,5);dispTemp(tempMin,6);

  //Update Sensors
    lightReading = constrain(map(analogRead(A5),1850,250,100,255),0,255);
    currentTime=(Time.hour())*100+Time.minute();
    mq07timers();

	//if(currentTime > wakeupTime && currentTime< wakeupTime+200){
//		for(int i=0;i<7;i++)
//		  	strip.setPixelColor(i, 0, 255-lightReading, 0);	
//	}
//	else{
//		for(int i=0;i<7;i++)
//	  		strip.setPixelColor(i, 255-lightReading, 255-lightReading, 255-lightReading);
//	}
	
  showStrips();
  Particle. process();
}

void dispTemp(int temp, byte n){
int rVal=0, gVal=0, bVal=0;
    // constrain temp between 0 and 100 F
    temp=constrain(temp, 0, 100);
    
    // Map Temp to Correct Temp.
    //100 - Red        255,   0,   0
    //    - orange
    //80  - Yellow     255, 255,   0
    //60  - Green        0, 255,   0
    //45  - Blue Green   0, 255, 255
    //30  - Blue         0,   0, 255
    //    - indigo
    //0   - Violet     255,   0, 255

    if (temp>=80){
        rVal=255;
        gVal=map(temp,80,100,255,0);
    }
    else if (temp>=60){
        rVal=map(temp,60,80,0,255);
        gVal=255;
    }
    else if (temp>=45){
        gVal=255;
        bVal=map(temp,45,60,255,0);
    }
    else if (temp>=30){
        gVal=map(temp,30,45,0,255);
        bVal=255;
    }
    else{
        rVal=map(temp,0,30,255,0);
        bVal=255;
    }
    strip.setPixelColor(n, rVal, gVal, bVal);
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

void getTraffic() {
  Particle.publish(TRAFFIC_PUB);
  unsigned long wait = millis();
  //wait for subscribe to kick in or 5 secs
  while ((Time.now()-updateTrafficTime)>=120 && (millis()-wait < 5000))
    Particle. process();
}//End of getWeather function

void trafficHandler(const char *name, const char *data){
    String str = String(data);
    traffic0=str.toInt();

    if(trafficMax==0){
      trafficMax=traffic0+10;
      trafficMin=traffic0-10;
    }
    
    if(traffic0>=trafficMax){
      trafficMax=traffic0+10;
      trafficMin+=5;
    }
    else if(traffic0<=trafficMin){
      trafficMin=traffic0-10;
      trafficMax-=5;
    }
    else{
      trafficMin+=5;
      trafficMax-=5;
    }

    updateTrafficTime = Time.now();
}

void getWeather(){
    Particle.publish(CUR_PUB,"{\"id\":5574991}");
    unsigned long wait = millis();
  //wait for subscribe to kick in or 5 secs
  while ((Time.now()-updateCurWeatherTime)>=120 && (millis()-wait < 5000))
    Particle. process();
}

void currentTempHandler(const char *name, const char *data){
    String str = String(data);
    temp0=str.toInt();
    updateCurWeatherTime = Time.now();
}

void getWeatherForecast(){
    Particle.publish(FORECAST_PUB,"{\"id\":5574991,\"cnt\":1}");
    unsigned long wait = millis();
  //wait for subscribe to kick in or 5 secs
  while ((Time.now()-updateForecastTime)>=120 && (millis()-wait < 5000))
    Particle. process();
}

void forecastHandler(const char *name, const char *data){
    String str = String(data);
    char strBuffer[125] = "";
    str.toCharArray(strBuffer, 125); // example: "1554231600~54.19~43.47~
    int forecastday1 = atoi(strtok(strBuffer, "\"~"));
    tempMax = atoi(strtok(NULL, "~"));
    tempMin = atoi(strtok(NULL, "~"));
    //temp0=str.toInt();
    updateForecastTime = Time.now();
}

void clearStrips(){
   strip0.clear();
   strip1.clear();
   strip2.clear();
   strip3.clear();
}

void showStrips(){
    strip.show();
    strip0.show();
    strip1.show();
    strip2.show();
    strip3.show();
}
 
unsigned long fadeTime = 0;
bool fadeStart=FALSE;
void showMinute(byte min){
    if (Time.second()>=59){
        if (fadeStart==FALSE){
            fadeTime=millis();
            fadeStart=TRUE;
        }
        setMinuteColor(min+1, 0, 0, map(millis()-fadeTime,0,1000,  0,127));
        setMinuteColor(min  , 0, 0, map(millis()-fadeTime,0,1000,255,127));
    }
    else if(Time.second()<1){
        setMinuteColor(min-1, 0, 0, map(millis()-fadeTime,1000,2000,127,  0));
        setMinuteColor(min  , 0, 0, map(millis()-fadeTime,1000,2000,127,255));
    }
    else if(Time.second()>=1){
        fadeStart=FALSE;
        setMinuteColor(min, 0, 0, 255);
    }
 }
 
void showHour(float hr,float min){
    if(hr>11)
     hr=hr-12;
    hr=(hr+min/60)*5;
    //Serial.println(Time.hour());
    //Serial.println(hr);
    setMinuteColor(hr, 0, 255, 0);
 }

 void setMinuteColor(int min, int r, int g, int b){
    if (min<15){
        strip0.setPixelColor(min,r,g,b);
    }
    else if(min<30){ 
        strip1.setPixelColor(min-15,r,g,b);
    }
     else if(min<45) {
        strip2.setPixelColor(min-30,r,g,b);
     }
     else{
        strip3.setPixelColor(min-45,r,g,b);
     }
}