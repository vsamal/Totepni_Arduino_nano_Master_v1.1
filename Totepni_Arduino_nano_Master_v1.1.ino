#include "SPI.h"
#include "Wire.h"
#include "OneWire.h"
#include "UIPEthernet.h"
#include "SoftwareSerial.h"
#include "DallasTemperature.h"
#include "AM2320.h"
#include "avr/wdt.h"

// definice cidla teplota a vlhkost
AM2320 th;

float teplota;
float vlhkost;

int val_int;
int val_decimal;

// precteny data z I2C pro textovy vystup na odeslani do displeje
String dataI2C; 

// nactena data ze SIMM
char s;

// tlacitka vstupu
byte tlacitko_modul[5] = {99, A3, A2, A1, A0};
// pamet rele vystupu
byte rele_modul[5] = {99, 0, 0, 0, 0};

// detekce alarmu
byte alarm = 6;
boolean is_alarm = false;
byte internet_error = 7;


// nastavení čísla vstupního pinu
const int pinCidlaDS = 3;
// vytvoření instance oneWireDS z knihovny OneWire
OneWire oneWireDS(pinCidlaDS);
// vytvoření instance senzoryDS z knihovny DallasTemperature
DallasTemperature senzoryDS(&oneWireDS);

// SIM na software seriovém portu
SoftwareSerial SIM900(4,5); //RX,TX


EthernetClient client;
signed long next;
char read_buffer;
boolean nalez = false;

// the dns server ip
IPAddress dnServer(193, 85, 1, 12);
// the router's gateway address:
IPAddress gateway(192, 168, 1, 1);
// the subnet:
IPAddress subnet(255, 255, 255, 0);
//the IP address is dependent on your network
IPAddress ip(192, 168, 1, 15);

/*
// --- Dneboh ---
// the dns server ip
IPAddress dnServer(10, 20, 30, 1);
// the router's gateway address:
IPAddress gateway(10, 20, 30, 1);
// the subnet:
IPAddress subnet(255, 255, 255, 0);
//the IP address is dependent on your network
IPAddress ip(10, 20, 30, 21);
*/


// byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   //physical mac address
uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05}; 
char server[] = "www.ipf.cz"; //server, kam se pripojujeme




void setup()   {                

  wdt_enable(WDTO_8S);
  
  Wire.begin();  
  Serial.begin(9600);  
  SIM900.begin(9600); 

  // zapnutí komunikace knihovny s Dallas teplotním čidlem
  senzoryDS.begin();

  // tlacitka relatek jako vstupy s pull up odporem na vstupu
  for(int i = 1; i <= 4; i++){
        pinMode(tlacitko_modul[i], INPUT_PULLUP);
  }

  // vstup z alarmu    
  pinMode(alarm, INPUT_PULLUP);

  // kontrolka ze nejede internet
  pinMode(internet_error, OUTPUT);


  Ethernet.begin(mac, ip, dnServer, gateway, subnet);

  next = 0;

}


void loop() {
  
  // --------- tepomery ----------
  senzoryDS.requestTemperatures();
  readTemp();
  // read_data_topeni();

  setI2Cvals(senzoryDS.getTempCByIndex(0));

  Serial.print(val_int);
  Serial.print(" => ");
  Serial.println(val_decimal);
  
  /*
  
  Wire.beginTransmission(100);
  Wire.write(val_int); 
  Wire.write(val_decimal); 
  Wire.endTransmission();
  Serial.flush();
  
  wdt_reset();
  

 Wire.requestFrom(100, 11);
 dataI2C = "";
 while(Wire.available() > 0){
      int data_read = Wire.read();
      dataI2C += data_read;
      Serial.print("Data: ");
      Serial.println(data_read);
  }
  Serial.flush();

  wdt_reset();

  */



  // ----------- tlacitka a alarm a Internet ----------


    // zkontrolujeme alarm   
  if(digitalRead(alarm) == LOW){
             delay(10);
             if(digitalRead(alarm) == LOW){
                if(is_alarm == false){setAlarm();}
                is_alarm = true;
             }else{
                is_alarm = false;
             }
        }else{
             is_alarm = false;
        }  
  

    // projedeme si stisknuta tlacitka
    for(int i = 1; i <= 4; i++){
          if(digitalRead(tlacitko_modul[i]) == LOW){
            Serial.println(i); //vypise stisknute tlacitko
            setRelayFromKey(i);
          }
          delay(10);
    }




  if (((signed long)(millis() - next)) > 0){
      
        next = millis() + 11000;

        read_data_topeni(0);
        
  }


    

}










// prevede float cislo na dve desetine na prenos po I2C
void setI2Cvals(float teplo_conv){
  
        val_int = teplo_conv;
        val_decimal = (teplo_conv - val_int) * 10;  
}



void readTemp() {
  switch(th.Read()) {
    case 2:
      teplota = 99;
      break;
    case 1:
      teplota = -99;
      break;
    case 0:
      vlhkost = th.h;
      teplota = th.t;
      break;
  }

  delay(200);
  wdt_reset();
}




void read_data_topeni(int send_relay) {

      
      if (client.connect(server,80)){

          Serial.println("Connected");

          digitalWrite(internet_error, LOW);
          
          String myURL = "GET /topeni/topeni.php?relay=";
          Serial.println(myURL);
                    
          client.print(myURL);
          client.print(send_relay);
          client.print("&status=");
        
                if (digitalRead(rele_modul[send_relay]) == LOW){client.print(1);}
                if (digitalRead(rele_modul[send_relay]) == HIGH){client.print(0);}               
          
          client.print("&temp1=");
          client.print(teplota);
          client.print("&humid1=");
          client.print(vlhkost);
          client.print("&temp2=");
          client.print(senzoryDS.getTempCByIndex(0));
          client.print("&temp3=");
          client.print(senzoryDS.getTempCByIndex(1));
          client.print("&temp4=");
          client.print(senzoryDS.getTempCByIndex(2));
          client.print("&temp5=");
          client.print(senzoryDS.getTempCByIndex(3));
          
          
          client.println(" HTTP/1.0");
          client.print("Host: ");
          client.println(server);
          client.println("Connection: close");
          client.println();

          nalez = false;
          byte relec = 1;        
          delay(10);
          

          // precteni dat z internetu
                    
          while(client.connected()) {
            if(client.available()) {
                char read_buffer = client.read();
     
                
                if (read_buffer == '>'){nalez = false;}
                 
                if (nalez){

                  Serial.print(read_buffer); //vypise prijata data
                  // na tohle mozna udelame funkci  
                  if (read_buffer == '0'){rele_modul[relec] = 1;}
                  if (read_buffer == '1'){rele_modul[relec] = 0;}
                  relec++;                  

                }
  
                if (read_buffer == '<'){nalez = true;}
                         
            }
          }
        
    
         Serial.println();
         Serial.println("Odpojuji.");
         delay(10);
         client.flush();
         delay(10);
         Serial.println();
         client.stop();   
         
       }else{
        
        Serial.println("Connect failed");

        digitalWrite(internet_error, HIGH);

       }


    wdt_reset();
}




// nastavi rele a zapise na internet
void setRelayFromKey(int tlac_press){

            Serial.print("Ctu stav rele ");
            Serial.print(tlac_press);
            Serial.print(" ten je nastaven na ");
            Serial.println(rele_modul[tlac_press]);            

            // na tohle mozna udelame funkci  
            // rele nastavime
            rele_modul[tlac_press] = !rele_modul[tlac_press];

            
            Serial.print("Tlacitko ");
            Serial.print(tlac_press);
            Serial.print(" stlaceno, rele vstup ");
            Serial.print(rele_modul[tlac_press]);
            Serial.println(" nastaveno");
            
            read_data_topeni(tlac_press);
            
            delay(1000);
            wdt_reset();
}







// alarm zavola na mobil
void setAlarm(){
            Serial.println("Alarm");                   
            
            //makeCall("604833891");
            //makeCall("605906254");
            makeCall("737226659");
            //sendSMS("737226659");
            //makeCall("737226659");
            
}


void delayWDT(int delaysec = 30){
            wdt_reset();
       
            int waiting_call = 0;
            while(waiting_call <= delaysec){
                delay (1000);
                wdt_reset();
                waiting_call += 1;
            }            
            wdt_reset();
}


void makeCall(char callnumber[]){
            wdt_reset();
            
            SIM900.print("ATD");
            SIM900.print(callnumber);
            SIM900.println(";");
            delay(100);            
             
            while (SIM900.available()){
            s = SIM900.read();
              Serial.print(s);
            }  
        
            delayWDT(29);
                       
            SIM900.println("ATH");                      
            delay (1000);
            wdt_reset();
}




void sendSMS(char smsnumber[]){
          SIM900.println("AT+CMGF=1");  // AT command na odeslani SMS zpravy
          delay(100);
          SIM900.print("AT+CMGS=\"");  // cislo prijemce
          SIM900.print(smsnumber);  // cislo prijemce
          SIM900.println("\"");  // cislo prijemce
          delay(100);
          SIM900.println("Alarm send.");  // zprava k odeslani
          SIM900.print("Status: ");  // zprava k odeslani
          SIM900.print(rele_modul[1]);  // zprava k odeslani
          SIM900.print(rele_modul[2]);  // zprava k odeslani
          SIM900.print(rele_modul[3]);  // zprava k odeslani
          SIM900.println(rele_modul[4]);  // zprava k odeslani
          SIM900.print(" is set.");  // zprava k odeslani
          delay(100);
          SIM900.println((char)26);  //  AT command znak a ^Z, ASCII code 26 pro konec SMS zpravy
          delay(100); 
          SIM900.println();
          delayWDT(11);  // pokej na odesln SMS
          //SIM900power();  // vypni GSM module
          wdt_reset();
        }

