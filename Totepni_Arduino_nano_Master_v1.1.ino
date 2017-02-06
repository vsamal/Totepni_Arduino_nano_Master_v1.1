/*
#include <Dhcp.h>
#include <Dns.h>
#include <ethernet_comp.h>
#include <UIPClient.h>
#include <UIPEthernet.h>
#include <UIPServer.h>
#include <UIPUdp.h>
*/


// #include "SPI.h"
#include "Wire.h"
#include "OneWire.h"
#include "UIPEthernet.h"
//#include "SoftwareSerial.h"
#include "DallasTemperature.h"
#include "AM2320.h"
//#include "avr/wdt.h"

// definice cidla teplota a vlhkost
AM2320 th;

float teplota;
float vlhkost;

int val_int;
int val_decimal;

// precteny data z I2C pro textovy vystup na odeslani do displeje
// I2C PINs A4, A5
String dataI2C; 

// tlacitka vstupu
byte tlacitko_modul[5] = {99, A2, A3, A0, A1};
// pamet rele vystupu
byte rele_modul[17] = {99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// detekce alarmu
byte alarm = 8;
boolean is_alarm = false;
byte internet_error = 9;


// nastavení čísla vstupního pinu
const int pinCidlaDS = 3;
// vytvoření instance oneWireDS z knihovny OneWire
OneWire oneWireDS(pinCidlaDS);
// vytvoření instance senzoryDS z knihovny DallasTemperature
DallasTemperature senzoryDS(&oneWireDS);


EthernetClient client;
// Ethernet PINs 10, 11, 12, 13
signed long next;
int next_sd;  
int next_wd;
int data_read;
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

  // wdt_enable(WDTO_8S);
  
  Wire.begin();  
  Serial.begin(9600);  

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

  // info casovac dalsiho precteni dat z webu
  next = 0;

}


void loop() {
  
  // --------- tepomery ----------
  senzoryDS.requestTemperatures();
  readTemp();
  // read_data_topeni();

  
  

   
  setI2Cvals(senzoryDS.getTempCByIndex(0));

  /*
  
  Serial.print(val_int);
  Serial.print(" => ");
  Serial.println(val_decimal);

  */
  
 
  
  // posledem data do SLAVE adruina ------
  next_sd++;
  
  if (next_sd > 27){  

        next_sd = 0;
        
        Wire.beginTransmission(100);
      
        // posleme stav relatek
        for(int i = 1; i <= 16; i++){
              Wire.write(rele_modul[i]);
        }
      
        
        // posleme stav teplomeru a vlhkomeru
        setI2Cvals(teplota);
        Wire.write(val_int); 
        Wire.write(val_decimal); 
      
        setI2Cvals(vlhkost);
        Wire.write(val_int); 
        Wire.write(val_decimal); 
      
       
        // posleme stav cidel
        for(int i = 0; i <= 3; i++){
            setI2Cvals(senzoryDS.getTempCByIndex(i));
            Wire.write(val_int); 
            Wire.write(val_decimal); 
        }  
        
        Wire.endTransmission();
        Wire.flush();
      
        delay(100);
        
        clr_wdt();
        
  }
  // data do SLAVE adruina odeslana ------
  

 // precteme data ze SLAVE

  /*
  
  next_wd++;
                
  if (next_wd > 59){
        
        next_wd = 0;
 
             Wire.flush();
             Wire.requestFrom(100, 1);
             // dataI2C = "";
             data_read = 0;
             while(Wire.available() > 0){
                  data_read = Wire.read();
                  // dataI2C += data_read;
              }
            
              Serial.print("Data readed from SLAVE: ");
              Serial.println(data_read);
             
             Wire.endTransmission();
             Serial.flush();
             Wire.flush();
            
             clr_wdt();

        }

 */
 
 // data ze SLAVE prectena






  // ----------- tlacitka a alarm a Internet ----------


   // zkontrolujeme alarm 
  // Serial.println("Test alarm");  
  if(digitalRead(alarm) == LOW){
             delay(300);
             if(digitalRead(alarm) == LOW){
                if(is_alarm == false){
                        digitalWrite(internet_error, HIGH);
                        setAlarm();
                  }
                is_alarm = true;
             }else{
                is_alarm = false;
             }
        }else{
             is_alarm = false;
        }  
    // Serial.println("OK");  






    // projedeme si stisknuta tlacitka
    // Serial.println("Test key");  
    for(int i = 1; i <= 4; i++){
          if(digitalRead(tlacitko_modul[i]) == LOW){
            Serial.println(i); //vypise stisknute tlacitko
            next_sd = 999;
            setRelayFromKey(i);
          }
          delay(1);
    }
    // Serial.println("OK");  



  // Serial.println("Test internet");  
  if (((signed long)(millis() - next)) > 0){
      
        Serial.println("Read internet ready");  
        next = millis() + 11000;

        digitalWrite(internet_error, HIGH);

        read_data_topeni(0);
        
  }
  // Serial.println("Test internet OK");  


    

}





// alarm zavola na mobil
void setAlarm(){
            Serial.println("Alarm");                   
            
            //makeCall("604833891");
            //makeCall("605906254");
            //makeCall("737226659");
            //sendSMS("737226659");
            //makeCall("737226659");
            
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

  delay(10); // 200
  clr_wdt();
}


void clr_wdt() {

      // wdt_reset();
  
}


void read_data_topeni(int send_relay) {

      
      clr_wdt();

      Serial.print("Connect to ");
      Serial.println(server);

      
      if (client.connect(server,80)){

          Serial.println("Connected");

          // digitalWrite(internet_error, LOW);
          clr_wdt();
          
          String myURL = "GET /topeni/topeni.php?relay=";
          client.print(myURL);
          client.print(send_relay);
          client.print("&status=");
          client.print(rele_modul[send_relay]);
        
                // if (rele_modul[send_relay] == 0){client.print(0);}
                // if (rele_modul[send_relay] == 1){client.print(1);}               
          
          client.print("&temp[1]=");
          client.print(teplota);
          client.print("&humid[1]=");
          client.print(vlhkost);
          client.print("&temp[2]=");
          client.print(senzoryDS.getTempCByIndex(0));
          client.print("&temp[3]=");
          client.print(senzoryDS.getTempCByIndex(1));
          client.print("&temp[4]=");
          client.print(senzoryDS.getTempCByIndex(2));
          client.print("&temp[5]=");
          client.print(senzoryDS.getTempCByIndex(1));
          
          client.println(" HTTP/1.0");
          client.print("Host: ");
          client.println(server);
          client.println("Connection: close");
          client.println();

          Serial.print(myURL);
          Serial.print(send_relay);
          Serial.print("&status=");
          Serial.println(rele_modul[send_relay]);

          nalez = false;
          byte relec = 1;        
          delay(1);
          

          // precteni dat z internetu
                    
          while(client.connected()) {
            if(client.available()) {
                char read_buffer = client.read();
     
                
                if (read_buffer == '>'){nalez = false;}
                 
                if (nalez){

                  digitalWrite(internet_error, LOW);
                  // Serial.print(read_buffer); //vypise prijata data
                  // na tohle mozna udelame funkci  
                  if (read_buffer == '0'){rele_modul[relec] = 0;}
                  if (read_buffer == '1'){rele_modul[relec] = 1;}
                  relec++;                  

                }
  
                if (read_buffer == '<'){nalez = true;}
                         
            }
          }
        
    
         //Serial.println();
         //Serial.println("Odpojuji.");
         delay(1);
         client.flush();
         delay(1);
         //Serial.println();
         client.stop();   
         
       }else{
        
        //Serial.println("Connect failed");

        digitalWrite(internet_error, HIGH);

        clr_wdt();

       }
 
    Serial.println("Internet OK");  
    clr_wdt();
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
            
            // delay(1000);
            clr_wdt();
}









void delayWDT(int delaysec = 30){
            clr_wdt();
       
            int waiting_call = 0;
            while(waiting_call <= delaysec){
                delay (1000);
                clr_wdt();
                waiting_call += 1;
            }            
            clr_wdt();
}

