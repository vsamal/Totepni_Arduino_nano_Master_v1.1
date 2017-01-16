#include "SPI.h"
#include "Wire.h"
#include "UIPEthernet.h"
#include "SoftwareSerial.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "AM2320.h"
#include "avr/wdt.h"


#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);




AM2320 th;

float teplota;
float vlhkost;

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif




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

uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
char server[] = "topeni.cz"; //server, kam se pripojujeme




void setup()   {                

  wdt_enable(WDTO_8S);
  
  Serial.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  

  // Clear the buffer.
  display.clearDisplay();

  Ethernet.begin(mac, ip, dnServer, gateway, subnet);

  next = 0;

}


void loop() {
  
  readTemp();
  // readGET();
  showtext("Obyvak", "teplota");
  showtext("Loznice", "teplota");
  showtext("Pokojicek", "teplota");
  showtext("Puda", "teplota");
  showtext("Venku", "teplota");


  wdt_reset();

}




void showtext(String radek1, String radek2) {
  display.setTextSize(2);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0,0);
  display.clearDisplay();

  display.println(radek1);
  if(radek2 == "teplota"){
    display.print(teplota);
    display.print(" ");
    display.setTextSize(1);
    display.print(vlhkost);
    display.println("%");
  }else{
    display.println(radek2);
  }
  display.display();
  delay(2100);
  display.clearDisplay();

  wdt_reset();
 
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




void readGET() {

  if (((signed long)(millis() - next)) > 0)
    {
      next = millis() + 11000;

      Serial.println("Connecting to server");
      
      if (client.connect(server,80)){

          Serial.println("Connected");
          
          String myURL = "GET /topeni/topeni.php?time=";
          myURL += millis();

          Serial.println(myURL);
          
          client.print(myURL);
          client.println(" HTTP/1.0");
          client.println("Host: samal.es");
          client.println("Connection: close");
          client.println();

          nalez = false;
          
         while(client.connected()) {
          if(client.available()) {
              char read_buffer = client.read();
              
              // Serial.print(read_buffer); //vypise prijata data
    
              
              if (read_buffer == '>'){nalez = false;}
               
              if (nalez){
                // Serial.print("->");
                Serial.print(read_buffer); //vypise prijata data
                // Serial.print("<-");
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

       }
        
    }

    wdt_reset();
}



