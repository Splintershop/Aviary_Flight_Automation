/* Aviary Flight Automation - USB/Serial
 *  V 2016412-16:57
 *  D.R.Daniel
 *  April 2016
 *  
 *  Hardware:
 *    Arduino Uno R3
 *    OSEPP Sensor Shield
 *    Linear Actuator - Windynation 8"  (Amazon B00P4TS694)
 *    Relay board - Sainsmart 2-relay module (Amazon B0057OC6D8)
 *    DHT11 Temperature and Humidity Sensor
 *    
 *    Uses pushbutton to start actuator.  There is a safety timer to backup
 *    built-in acutator limit switch.  External limits are used as primary control.
 *    Since magnetic reed switches aren't exact, a trim timer is provided to fine
 *    tune when actuator shuts off.    
 *    
 *    
 */


//  =====================================================================
#include <DHT.h>


//  =====================================================================
//  Initialize Variables
 int extendRelay = 8;             //extend relay digital output pin
 int retractRelay = 9;            //retract relay digital output pin
 int openLimit = 3;              //door open limit switch
 int closedLimit = 2;            //door closed limit switch
 int openPB = 5;                 //manual open switch
 int closePB = 4;                //manual close switch
 int photoSwitch = 11;            //photocell relay NO input

  #define DHTPIN 10               //!!!!!!note, pin 10 conflicts with wifi board!!!!!!!!
  #define DHTTYPE DHT11 
  DHT dht(DHTPIN, DHTTYPE);
 
 String inputString = "";         //string to hold the incoming serial string
 boolean stringComplete = false;  //init string complete to false
 long safetyTimer = 0;            //this timer provides a max time limit
 long activeTimer = 0;            //this timer counts while the action is in progress
 long tenthsOfSeconds = 235;      //settable value - 235 suitable for WindyNation 8"
 int openTrim=0;                  //x milliseconds additional open time trim amount
 int closeTrim=500;               //x milliseconds additional close time trim amount

 float tempC;                     //temperature in Centigrade
 float tempF;                     //temperature in Farenheit
 float humidity;                  //humidity

//  =====================================================================
void setup() 
{
  Serial.begin(9600);
  dht.begin();

  // Initialize digital I/O
  pinMode(extendRelay, OUTPUT);
  pinMode(retractRelay, OUTPUT);
  
  pinMode(openLimit, INPUT);
  pinMode(closedLimit, INPUT);
  pinMode(openPB, INPUT);
  pinMode(closePB, INPUT);
  pinMode(photoSwitch, INPUT);
  
  digitalWrite(extendRelay, HIGH);
  digitalWrite(retractRelay, HIGH);
  digitalWrite(openLimit, HIGH);    //enable internal pullup resistor
  digitalWrite(closedLimit, HIGH);  //enable internal pullup resistor
  digitalWrite(openPB, HIGH);       //enable internal pullup resistor
  digitalWrite(closePB, HIGH);      //enable internal pullup resistor
  digitalWrite(photoSwitch, HIGH);   //enable internal pullup resistor

  

  //initialize serial string input
  inputString.reserve(20);
  inputString="";
  
  Serial.println("Aviary Flight Automation Program");
  Serial.println("DR Daniel 2016");
  Serial.println("V2016412-16:57");
  Serial.println("Commands");
  Serial.println("___________________________________________________________________________");
  Serial.println("close");
  Serial.println("open:");
  Serial.println("temp");
  Serial.println("status");
  Serial.println("___________________________________________________________________________");   

}


//  =====================================================================
void loop() {

  //Check serial input section
  checkInput();             // go check if the user entered a command

  if (stringComplete)       //check if there is data in the serial buffer
  {
    Serial.println(inputString);
    if (inputString.startsWith("open") || inputString.startsWith("Open")) //check for both upper/lower case
    {
      retractLinearActuator();
    }
    if (inputString.startsWith("close") || inputString.startsWith("Close")) //check both upper/lower case
    {
      extendLinearActuator();
    }
    if (inputString.startsWith("status"))   //if status is entered, then return values
    {
      checkStatus();
    }
    if (inputString.startsWith("temp"))  //check if temperature requested
    {
      getTemperatureAndHumidity();
    }

  inputString = "";
  stringComplete = false;
  }

  //check for pushbutton input

  if (digitalRead(openPB) == false && digitalRead(openLimit) == true)     //check if "Open" button has been pressed
  {
    retractLinearActuator();
  }

  if (digitalRead(closePB) == false && digitalRead(closedLimit) == true)    //check if "Closed" button has been pressed
  {
    extendLinearActuator();
  }
}


//  =====================================================================
/* Procedures and classes section
 *  
 */

void checkInput()
  {
    while (Serial.available()) 
    {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') 
      {
      stringComplete = true;
      }
    }
  }


/*
 * extendLinearActuator
 * Closes door by extending the linear actuator until the closed limit switch closes
 */
void extendLinearActuator()
  {
    activeTimer = 0;
    safetyTimer = tenthsOfSeconds;
    if (digitalRead(closedLimit) == false)        //If door is closed, send msg
    {
      Serial.println("Door is already fully closed");
    }
    else                                        //door is not closed, so have at it
    {
    Serial.println("Closing door by extending linear actuator");
    digitalWrite(retractRelay, HIGH);              //ensure retract power is off
  
    while (digitalRead(closedLimit) != false && activeTimer <= safetyTimer)     //Until the limit switch indicates closed
      {
      digitalWrite(extendRelay, LOW);             //energize the extend relay
      delay(100);                                 //delay .1 second (100ms)
      activeTimer++;                              //increment timer
      }
    delay(closeTrim);                   //wait x milliseconds before de-energizing relay
    digitalWrite(extendRelay, HIGH);           //limit was hit, so turn off relay
    displayInstructionsIfClosed();                //display the instruction set again
    }
  }

/*
 * retractLinearActuator
 * Opens door by retracting the linear actuator until the opened limit switch closes
 */
void retractLinearActuator()
  {
    activeTimer = 0;
    safetyTimer = tenthsOfSeconds;
    if (digitalRead(openLimit) == false)        //If door is open, send msg
    {
      Serial.println("Door is already fully open");
    }
    else                                        //door is not open, so have at it
    {
    Serial.println("opening door by retracting linear actuator");
    digitalWrite(extendRelay, HIGH);              //ensure extend power if off
  
    while (digitalRead(openLimit) != false && activeTimer <= safetyTimer)     //Until the limit switch indicates closed
      {
      digitalWrite(retractRelay, LOW);              //energize the retract relay
      delay(100);
      activeTimer++;                            //increment timer
      }
    delay(openTrim);                      //wait x milliseconds before de-energizing relay
    digitalWrite(retractRelay, HIGH);           //limit was hit, so turn off relay
    displayInstructionsIfOpen();                //display the instruction set again
    }
  }





/*
 * displayInstructionsIfOpen
 * Displays the close door command set to the serial monitor
 */
void displayInstructionsIfOpen()
{
  Serial.println("Door is now open");
  Serial.println("___________________________________________________________________________");
}


/*
 * displayInstructionsIfClosed
 * Displays the open door command set to the serial monitor
 */
void displayInstructionsIfClosed()
{
  Serial.println("Door is now closed");
  Serial.println("___________________________________________________________________________");   
}



/*
 * checkStatus
 * Returns the Limit switch and pushbutton status to the serial monitor
 */
void checkStatus()
{
    Serial.print("Open Limit = ");
    Serial.println(digitalRead(openLimit));
    Serial.print("Open Pushbutton = ");
    Serial.println(digitalRead(openPB));
    Serial.print("Closed Limit = ");
    Serial.println(digitalRead(closedLimit));
    Serial.print("Closed Pushbutton = ");
    Serial.println(digitalRead(closePB));
    Serial.print("Photocell is ");
    Serial.println(digitalRead(photoSwitch));
   
}


/*
 * getTemperatureAndHumidity
 * Returns values from the DHT11 Sensor
 * 
 */
void getTemperatureAndHumidity()
{

 humidity = dht.readHumidity();       //read humidity value
 tempC = dht.readTemperature();       //read temp in C
 tempF = tempC*1.8+32;                 //convert to degrees F

 if (isnan(tempC) || isnan(humidity))
 {
  Serial.println("Error reading from DHT sensor");
 }
 Serial.print("Temp = ");
 Serial.println(tempF);
 Serial.print("Humidity = ");
 Serial.print(humidity);
 Serial.println("% ");
}
 

