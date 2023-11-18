//Libraries
#include "DFRobot_BMP280.h"     //Barometer library
#include "Wire.h"               //Required for the I2C bus
#include <LiquidCrystal_I2C.h>  //LCD library
#include <dht.h>                //Hygrometer library
#include <Servo.h>              //Servo library

//For timing the humidity measuring
bool MeasureHumidity = false;
int Time = 0;

//Servos
Servo TempServo;    //Temperature pointer servo
Servo HumServo;     //Humidity pointer servo

//Photoresistor variables
int LuxPin = A2;
int LuxResistor = 10000;
float LuxOut = 0, LuxVoltage = 0, LuxResistance = 0;


//Temperature monitoring variables
int TempPin = A3;
int TempResistor = 10000;
float a = 6.4598E-04;          
float b = 2.9419E-04;
float ThermOut = 0, ThermVoltage = 0, ThermResistance = 0, Temperature = 0;


//LCD decleration
LiquidCrystal_I2C lcd(0x20, 16, 2);


//Hygrometer decleration
#define dataPin 7
dht DHT;


//Barometer decleration & error codes 
typedef DFRobot_BMP280_IIC    BMP;
BMP   bmp(&Wire, BMP::eSdoLow);
#define SEA_LEVEL_PRESSURE    1015.0f
void printLastOperateStatus(BMP::eStatus_t eStatus)
{
  switch(eStatus) {
  case BMP::eStatusOK:    lcd.setCursor(0, 0);lcd.print("Barometer Found"); break;
  case BMP::eStatusErr:   lcd.setCursor(0, 0);lcd.print("Unknown Error"); break;
  case BMP::eStatusErrDeviceNotDetected:    lcd.setCursor(0, 0);lcd.print("No Barometer"); break;
  case BMP::eStatusErrParameter:    lcd.setCursor(0, 0);lcd.print("Parameter Error"); break;
  default: lcd.setCursor(0, 0);lcd.print("?"); break;
  }
}


//Function for mapping floats
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void setup()
{
  //Pins
  pinMode(TempPin, INPUT);
  TempServo.attach(6);
  HumServo.attach(5);

  //AnalogRef and baud rate
  Serial.begin(115200);
  analogReference(EXTERNAL); 

  //LCD startup
  lcd.init();                        
  lcd.backlight();                   
  lcd.setCursor(0, 0);      


  //Barometer start and error check
  bmp.reset();
  int BM_Error_Total = 0;
  while(bmp.begin() != BMP::eStatusOK && BM_Error_Total < 10) {
    printLastOperateStatus(bmp.lastOperateStatus);
    BM_Error_Total += 1;
    lcd.setCursor(0,1); lcd.print(BM_Error_Total);
    delay(2000);
  }

  //Startup end
  lcd.clear();
  lcd.setCursor(0, 0);  
  lcd.print("WAIT");
  delay(2000);          //Start delay
  lcd.clear();          //LCD Clear
 
}

void loop()
{
  //Get, calculate (using simplified steinhart-hart equation) and show temperature (ON LCD)
  ThermOut = 0;
  for(int i = 0; i < 10; i++){
    ThermOut = ThermOut + analogRead(TempPin);
    delay(10);
  }
  ThermOut = ThermOut/10;
  ThermVoltage = ThermOut * 5/1023;                   //Convert the ADC value to voltage
  ThermResistance = TempResistor*(5/ThermVoltage-1);  //Get the resistance 
  Temperature = 1/(a+b*log(ThermResistance));         //Simplified steinhart-hart equation
  Temperature = Temperature-273.15;                   //Convert temperature to celsius from kelvin
  int tmp = Temperature*10; float tmp2 = tmp; Temperature = tmp2/10;    //Hate crime
  
  lcd.setCursor(7,1); lcd.print(Temperature); lcd.print("C");         //Show temperature on lcd


  //PhotoResistor
  LuxOut = analogRead(LuxPin);
  LuxVoltage = LuxOut * 5/1023;
  LuxResistance = LuxResistor*(5/LuxVoltage-1);
  lcd.setCursor(14,0);
  if(LuxResistance < 1000){
    lcd.print("H"); 
  } else {
    lcd.print("G");
  }
  

  
  //Get and show barometer data
  uint32_t    press = bmp.getPressure();
  float AirPressure = float(press);
  lcd.setCursor(0,0);
  lcd.print(AirPressure/100,1); lcd.setCursor(7,0); lcd.print("hPa");

  //Get and show hygrometer data every 2000ms or so
  switch(MeasureHumidity)
  {
    case false:
      if(Time >= 2000) {
        MeasureHumidity = true;
        Time = 0;
      } else {
        Time += 100;
      }
      break;
    case true:
      int readData = DHT.read22(dataPin);
      float DHT_Temp = DHT.temperature;
      float DHT_Hum = DHT.humidity;
      lcd.setCursor(0,1);
      lcd.print(DHT_Hum); lcd.print("%");
      //Serial.println("Humidity measured");
      MeasureHumidity = false;

      //Humidity Servo Control;
      float HumAngle = mapf(DHT_Hum, 0.0, 100.0, 180.0,0.0);
      HumServo.write(HumAngle);
      break;
  }



  
  //Temperature Servo Control
  float TempAngle = mapf(Temperature, 15.0, 30.0, 180.0,0.0);
  TempServo.write(TempAngle);
  
}
