#include <OneWire.h> 
#include <DallasTemperature.h>// Include the SoftwareSerial library
#include <SoftwareSerial.h>

// Define the RX and TX pins for the software serial port
#define rxPin 5
#define txPin 6

// pins connected on the Arduino 
#define ONE_WIRE_BUS 12
#define Buzzer 9
#define Coil 2
#define Valve 3
#define water_lvl_low 7
#define water_lvl_high 8

// setting up instancefor communication and referencing oneWire
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

// Create a SoftwareSerial object
SoftwareSerial mySerial(rxPin, txPin);

// intitialising global variables
int   water_lvl = 0, water_lvl_snsr_threshold = 300, 
      room_temp = 0, min_temp, max_temp = 46, prev_temp=0;

long  session_start_time=0, 
      session_time=1800,  // time in sec 
      elapsed_time_millis,
      elapsed_time_min,
      prev_elapsed_time_min,
      fill_start_time=0;

bool  coil_on = false, chamber_empty = false;

/////////////////////////////////void setup ///////////////////////////////
void setup()
{
  pinMode(Valve, OUTPUT);
  pinMode(Coil, OUTPUT);
  pinMode(Buzzer,OUTPUT);
  pinMode(water_lvl_low, INPUT_PULLUP);
  pinMode(water_lvl_high, INPUT_PULLUP);
  Serial.begin(9600);
  mySerial.begin(9600);
  min_temp = max_temp-1;
  
  //turns off all relays in the begining
  digitalWrite(Valve, HIGH);
  digitalWrite(Coil, HIGH);
  digitalWrite(Buzzer,HIGH);
  delay(2000);
  digitalWrite(Buzzer,LOW);
  
  session_start_time = millis();
}

////////////////////////////////////// void loop ////////////////////////////
void loop()
{
  elapsed_time_millis = millis() - session_start_time;
  elapsed_time_min = elapsed_time_millis / 60000;
  if(elapsed_time_min != prev_elapsed_time_min) sendData(String("S")+String(elapsed_time_min));
  prev_elapsed_time_min = elapsed_time_min;
  if((elapsed_time_millis) < (session_time*1000))
  {
    room_temp = getRoomTemp(); //read temp
    if(prev_temp!=room_temp)  sendData(String("T")+String(room_temp));
    prev_temp = room_temp;
    changeTemp();
    if(room_temp > max_temp)  // is room hot
    {
      digitalWrite(Coil,HIGH); //turn OFF coil
      coil_on = false;
      delay(1000);
    }
    else if(coil_on) fillChamber();
    else if(room_temp < min_temp)  fillChamber();
  }
  else  endSession();
}

////////////////////////////////////// void fillChamber ///////////////////////
// fill water if chamber is empty and also control the coil
void fillChamber()
{
  // if chamber is empty
  if(!WaterPresent(water_lvl_low)){
    delay(1000);
    // makes sure the sensor reading is not due to fluctuations 
    if(!WaterPresent(water_lvl_low))
    { 
      chamber_empty = true;
      fill_start_time = millis();
      digitalWrite(Valve,LOW);  // starts water
      delay(500);
    }
  }
  // starts filling the chamber until full
  while(chamber_empty)
  {
    if((millis() - fill_start_time) > 3000 && coil_on)
    {
      digitalWrite(Coil,HIGH); //turn OFF coil
      coil_on = false;
    }
    if(WaterPresent(water_lvl_high))
    {
      digitalWrite(Valve,HIGH); //stops water
      chamber_empty = false;
      delay(500);
    }
  }
  delay(100);
  digitalWrite(Coil,LOW); //turn ON coil
  coil_on = true;
  delay(100);

}

//////////////////////////////////////// void endSession //////////////////////////////
void endSession()
{
  digitalWrite(Valve, HIGH);
  digitalWrite(Coil, HIGH);
  //buzzer
  for(int i=0;i<10;i++)
  {
    digitalWrite(Buzzer, HIGH);
    delay(500);
    digitalWrite(Buzzer, LOW); 
    delay(500);
  }
  room_temp = getRoomTemp(); //read temp
  sendData(String(room_temp));
}

//////////////////////////////////////////////////// int getRoomTemp //////////////
// function to read temprature sensor value from ds18b20
int getRoomTemp(){
  sensors.requestTemperatures(); // Send the command to get temperature readings
  Serial.print("room temp: ");
  Serial.println(sensors.getTempCByIndex(0));
  return sensors.getTempCByIndex(0);
}

///////////////////////////////////////// bool WaterLvl ///////////////////////////////
// function that returns true if sensor pin is detects water
bool WaterPresent(int pin)
{
  int val = digitalRead(pin);
  Serial.print("val: ");
  Serial.println(val);
  if(val) return  false;
  else return true;
}

///////////////////////////////////////// void sendData ///////////////////////////////
void sendData(String dataString){
  // Check if there is any data available to read from the Serial Monitor
  // Print the data to the SoftwareSerial port
  mySerial.println(dataString);
}

///////////////////////////////////////// void changeTemp ///////////////////////////////
void changeTemp(){
  // Check if there is any data available to read from the SoftwareSerial port
  if (mySerial.available()) {
    // Read the data from the SoftwareSerial port into a string until a newline character is received
    String dataString = mySerial.readStringUntil('\n');
    Serial.println(dataString);
    
    // Check if 'T' is present in the string
    if (dataString.indexOf('T') != -1) {
      // Find the position of 'T' in the string
      int tPosition = dataString.indexOf('T');
      
      // Extract the substring after 'T'
      String numberString = dataString.substring(tPosition + 1);
      
      // Convert the extracted substring to a float with one decimal point
      float number = numberString.substring(0, 2).toInt() + (numberString.substring(2).toFloat() / 10.0);
      
      // Print the extracted number to the Serial Monitor
      Serial.print("Received 'T', Extracted Number: ");
      Serial.println(number, 1); // Print with one decimal point

      max_temp = number;
      min_temp = max_temp - 2;
      Serial.print("max temp : ");
      Serial.println(max_temp);
      Serial.print("min temp : ");
      Serial.println(min_temp);
    }
    
    // Check if 'S' is present in the string
    if (dataString.indexOf('S') != -1) {
      // Find the position of 'S' in the string
      int tPosition = dataString.indexOf('S');
      
      // Extract the substring after 'S'
      String numberString = dataString.substring(tPosition + 1);
      
      // Convert the extracted substring to a float with one decimal point
      float number = numberString.substring(0, 2).toInt() + (numberString.substring(2).toFloat() / 10.0);
      
      // Print the extracted number to the Serial Monitor
      Serial.print("Received 'S', Extracted Number: ");
      Serial.println(number, 1); // Print with one decimal point

      session_time = number*60;
      Serial.print("session_time : ");
      Serial.println(session_time);
    }
  }
}
