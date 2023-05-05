//Librerías
#include "DHT.h"               
#include <Wire.h>
#include <ESP32Time.h>
#include <WiFi.h>
#include <U8g2lib.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

//Pantalla Display en pixeles
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

//Sensor
#define DHTPIN 23 
#define DHTTYPE DHT11 

//Pines
#define PIN_BOTON_SUBIR 35 
#define PIN_BOTON_BAJAR 34
bool estadoBoton1 = false;
bool estadoBoton2 = false;



//Máquina de Estados
#define PANTALLA_INICIAL 1    
#define LIMPIAR_1 2
#define PANTALLA_CAMBIOS 3
#define LIMPIAR_2 4

//Estado inicial de la máquina
int estado = 1;  

// Inicializar el Sensor
DHT dht(DHTPIN, DHTTYPE);

//Inicializar rtc
int gmt;
struct tm timeinfo;
ESP32Time rtc;

//Reloj
long unsigned int timestamp; 
const char *ntpServer = "south-america.pool.ntp.org";
long gmtOffset_sec = -10800;
const int daylightOffset_sec = 0;

//WiFi
const char* ssid = "ORT-IoT";           
const char* password = "OrtIOTnew22$2";

// Initialize Telegram BOT
#define BOTtoken "6085478906:AAGkmo5pVC1NyXsRvGHJQ10DszGOgBZxsws"  // cambiar por token 

#define CHAT_ID "-1001851624507" /// 

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

//Constructores y variables globales
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int botRequestDelay = 1000; /// intervalo
unsigned long lastTimeBotRan; /// ultimo tiempo

//Funciones
void pedir_lahora(void);
void setup_rtc_ntp(void);
void print_temperatura_humedad(void );
void initWiFi();

void setup() 
{
  
  Serial.begin(9600);
  u8g2.begin();
  pinMode(PIN_BOTON_SUBIR, INPUT_PULLUP);    
  pinMode(PIN_BOTON_BAJAR, INPUT_PULLUP);  
  
  Serial.println("Conectandose a Wi-Fi...");


  //Llamo a las funciones
  initWiFi(); 
  setup_rtc_ntp();

  }
  

void loop() 
{
  
  switch(estado)
  {
    //Pantalla que muestra hora y temperatura
    case PANTALLA_INICIAL: 
   
{      
      Serial.println("PANTALLA INICIAL"); 
      delay(2000);

      pedir_lahora();
    
      
    int hora = timeinfo.tm_hour;
    int minutos = timeinfo.tm_min;
    char stringhoras[7];  
    char stringminutos[7];
    float t = dht.readTemperature();  
    char stringt[5];
    
    u8g2.clearBuffer();  
    u8g2.setFont(u8g_font_5x7); 

    u8g2.drawStr(0,30,"Hora : ");
    u8g2.drawStr(0,15,"Temperatura: ");
    sprintf (stringhoras, "%d" , hora); 
    u8g2.setCursor(50, 30);
    u8g2.print(stringhoras);
    u8g2.drawStr(55,30,":");
    u8g2.setCursor(60, 30);
    sprintf (stringminutos, "%d" , minutos); 
    u8g2.print(stringminutos);
    sprintf (stringt, "%f" , t); 
    u8g2.setCursor(80, 15);
    u8g2.print(stringt);

    u8g2.sendBuffer();  
      
}
    break;
    delay(5000);
        
      if(digitalRead(PIN_BOTON_BAJAR) == LOW && digitalRead(PIN_BOTON_SUBIR) == LOW)
      {
           Serial.println("Boton leído");
        estado = LIMPIAR_1;
      }

    //Espera a que se suelten los botones para pasar a la siguiente pantalla
    case LIMPIAR_1: 
    { 
      Serial.println("PRIMER ESPERA"); 
      if(digitalRead(PIN_BOTON_BAJAR) == HIGH && digitalRead(PIN_BOTON_SUBIR) == HIGH)
      {
        estado = PANTALLA_CAMBIOS;
      }
    }
    break;

    //Pantalla que permite cambiar la hora
    case PANTALLA_CAMBIOS: 
    {

    Serial.println("PANTALLA CAMBIOS");
    float tiempo = gmtOffset_sec;
    char stringtiempo[5];  
    
    u8g2.clearBuffer();  
    u8g2.setFont(u8g_font_5x7); 

    u8g2.drawStr(0,15,"GMT: ");
    sprintf (stringtiempo, "%f" , tiempo);
    u8g2.setCursor(80, 15); 
    u8g2.print(stringtiempo);

    u8g2.sendBuffer();       
      
      if(digitalRead(PIN_BOTON_BAJAR) == LOW) 
      {
        estadoBoton1 = true;
      }
      if(digitalRead(PIN_BOTON_BAJAR) == HIGH && estadoBoton1 == true) 
      {
        estadoBoton1 = false;
        gmtOffset_sec = gmtOffset_sec + 3600;

      }    

      if(digitalRead(PIN_BOTON_SUBIR) == LOW)
      {
        estadoBoton2 = true;
      }
      if(digitalRead(PIN_BOTON_SUBIR) == HIGH && estadoBoton2 == true)
      {
        estadoBoton2 = false;
        gmtOffset_sec = gmtOffset_sec - 3600;
      }    
      
      if(digitalRead(PIN_BOTON_BAJAR) == LOW && digitalRead(PIN_BOTON_SUBIR) == LOW) 
      {
        estado = LIMPIAR_2;
      }
    }
    break;

    //Espera a que se suelten los botones para volver a la pantalla inicial
    case LIMPIAR_2: 
    {
      Serial.println("SEGUNDA ESPERA");
      if(digitalRead(PIN_BOTON_BAJAR) == HIGH && digitalRead(PIN_BOTON_SUBIR) == HIGH) 
      {
        estado = PANTALLA_INICIAL;
        setup_rtc_ntp();      
      }
    }
    break;   
  }

}

//Función conexión WiFi
void initWiFi() 
{       
  WiFi.mode(WIFI_STA);                                
  WiFi.begin(ssid , password );
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
  
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  bot.sendMessage(CHAT_ID, "Bot Hola mundo", "");
  Serial.println(Hola Mundo);
}

//Funciones Tiempo
void setup_rtc_ntp(void)
{
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  timestamp = time(NULL);
  rtc.setTime(timestamp + gmtOffset_sec);
}


void pedir_lahora(void)
{
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Veo la hora del rtc interno ");
    timestamp = rtc.getEpoch() - gmtOffset_sec;
    timeinfo = rtc.getTimeStruct();
  }

  else
  {
    Serial.print("NTP Time:");
    timestamp = time(NULL);
  }

  return;
}

// funcion que se comunica con telegram
void handleNewMessages(int numNewMessages) {
  Serial.println("Mensaje nuevo");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // inicio de verificacion
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){   ////si el id no corresponde da error . en caso de que no se quiera comprobar el id se debe sacar esta parte 
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    ///fin de verificacion

    // imprime el msj recibido 
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    /// si recibe /led on enciende el led 
    if (text == "/led_on") {  
      bot.sendMessage(chat_id, "LED state set to ON", "");
      ledState = HIGH;
      digitalWrite(ledPin, ledState);
    }
    
    if (text == "/led_off") {
      bot.sendMessage(chat_id, "LED state set to OFF", "");
      ledState = LOW;
      digitalWrite(ledPin, ledState);
    }
    
    if (text == "/state") {
      if (digitalRead(ledPin)){
        bot.sendMessage(chat_id, "LED is ON", "");
      }
      else{
        bot.sendMessage(chat_id, "LED is OFF", "");
      }
    }
  }
}

void esperar5segundos(void)
{
   if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("Veo los msj nuevos");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  
  }

  return;
}

void maquinaDeEstados (void)
{
 switch(estado)
  {
    case S1A: 
    {
      
  } 
  }
}
