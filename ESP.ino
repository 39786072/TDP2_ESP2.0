#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "command.h"
#include "Programador.h"


#ifndef STASSID
  #define STASSID "DCS"
  #define STAPSK  "gridcomd"
#endif

const char* ssid      = STASSID;
const char* password  = STAPSK;
const char* url       = "http://tobyteam.computerengineering.me/esps/";


typedef enum{
  Init,       //Inicializa perifericos y establece conexion wifi
  Check,      //Verifica si existe una nueva version
  Download,   //Actualiza el arduino
  Program,
  Wait,       //Espera
  Updated,
  Error       //Error
} State;

typedef enum{
  Ok,                         //No Problems
  //InInit
  I_WifiTimeOut       = 50,   //Can't connect to wifi.
  //InCheck
  C_NoConnected       = 100,  //Wifi is not connected at version check
  C_NoResponse        = 150,  //Server not response
  C_InvalidResponse   = 200,  //Invalid response from server
  //InProgramming
  P_NoConnected       = 300,  //Wifi is not connected at version check
  P_NoResponse        = 350,  //Server not response
  P_InvalidResponse   = 400,  //Invalid response from server
  P_ArduinoNoResponse = 450,  //
  P_ChecksumError      = 500   //CheckSum
} ErrorCode;
//-----------------Fin zona de definiciones--------------------

//------------------Declaracion de variables globales----------
  State CurrentState = Init;
  State OldState;
  ErrorCode CurrentError = Ok;
  uint32_t MEFCount = 0;
  WiFiClient client;
  HTTPClient http;
  String program;
  uint16_t programCounter = 0;
  Page p((device[PageSizeHigh] << 4) + device[PageSizeLow]);

//Funciones Auxiliares
bool WaitForWifi(uint16_t timeout)
{
  while (WiFi.status() != WL_CONNECTED && timeout--) {
    delay(500);
    #ifdef DEBUG
      Serial1.print("Intentando Conectar al Wifi ");
      Serial1.print(ssid);
      Serial1.println(".");
    #endif
  }
  return WiFi.status() == WL_CONNECTED;
}

void inline timerISR()
{
  Serial.write(Cmnd_STK_GET_SYNC);
  Serial.write(Sync_CRC_EOP);
  //WaitFor(Resp_STK_OK);
  timer1_write(62500);
}

//-----------------------------


String getStateText(State state)
{
  switch (state)
  {
    case Init:       //Inicializa perifericos y establece conexion wifi
      return "Init";
      break;
    case Check:      //Verifica si existe una nueva version
      return "Check";
      break;
    case Download://Actualiza el arduino
      return "Downloading Page";
      break;
    case Program://Actualiza el arduino
      return "Processing data and program";
      break;
    case Wait:       //Espera
      return "Wait";
      break;
    case Updated:
      return "Updated";
      break;
    case Error: 
      return "Error";
      break; 
  }
}
String getErrorText(ErrorCode error)
{
  switch(error)
  {
    case Ok:                 //No Problems 
      return "No Error";
      break; 
    //InInit
    case I_WifiTimeOut:           //Can't connect to wifi. 
      return "Wifi Connect Time Out";
      break; 
    //InCheck
    case C_NoConnected:         //Wifi is not connected at version check 
      return "Wifi is not connected at version check";
      break; 
    case C_NoResponse:          //Server not response 
      return "Server not response at version check";
      break; 
    case C_InvalidResponse:     //Invalid response from server 
      return "Invalid response from server at version check";
      break; 
    //InProgramming
    case P_NoConnected:         //Wifi is not connected on programming 
      return "Wifi is not connected on programming";
      break; 
    case P_NoResponse:             //Server not response 
      return "Server not response on programming";
      break; 
    case P_InvalidResponse:     //Invalid response from server 
      return "Invalid response from server on programming";
      break; 
    case P_ArduinoNoResponse:   // 
      return "Arduino no response";
      break;
    case P_ChecksumError:
      return "CheckSum verify error";
      break;
    default:
      return "Unknown";
    break; 
  }
}
//-----------------------------------------------------------------------------
//------------------Fin Declaracion de variables globales----------
void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RST_Slave, OUTPUT);
  digitalWrite(RST_Slave,HIGH);
  Serial.begin(115200);
  #ifdef DEBUG
    Serial1.begin(115200);
    Serial1.println("Reset!");
  #endif
  WiFi.mode(WIFI_STA);
}

void loop() {
  String result = "ERROR";
  String request = "";
  #ifdef DEBUG
    Serial1.print("Estado: ");
    Serial1.print(getStateText(CurrentState));
    if (CurrentState == Error)
    {
      Serial1.print("\tError: ");
      Serial1.print(getErrorText(CurrentError));
    }
    Serial1.print("\tContador: ");
    Serial1.println(MEFCount);
  #endif
  switch (CurrentState)
  {
    case Error:
      programCounter = 0;
      p.init();
      timer1_disable();
      digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
      delay(CurrentError);
      switch(CurrentError)
      {
        case Ok:
          OldState = CurrentState;
          CurrentState = Init;
        break;
        case I_WifiTimeOut:
          if (MEFCount-- == 0){
            OldState = CurrentState;
            CurrentState = Init;
          }
        break;
        case P_ArduinoNoResponse:
          if (MEFCount-- == 0){
            request.concat(url);request.concat(WiFi.macAddress());request.concat("/error/ArduinoNoResponse");// request.concat("45");
            #ifdef DEBUG
              Serial1.println(request);
            #endif
            if(WiFi.status() == WL_CONNECTED){
                if (http.begin(client, request)) {  // HTTP
                  int httpCode = http.POST("Arduino No Response");
                }
            }
            OldState = CurrentState;
            CurrentState = Init;
          }
        break;
        case P_ChecksumError:
          if (MEFCount-- == 0){
            request.concat(url);request.concat(WiFi.macAddress());request.concat("/error/CheckSumError");// request.concat("45");
            #ifdef DEBUG
              Serial1.println(request);
            #endif
            if(WiFi.status() == WL_CONNECTED){
                if (http.begin(client, request)) {  // HTTP
                  int httpCode = http.POST("Arduino No Response");
                }
            }
            OldState = CurrentState;
            CurrentState = Init;
          }
        break;
        default:
          if (MEFCount-- == 0){
            OldState = CurrentState;
            CurrentState = Init;
          }
      }
    break;
    case Init:
      WiFi.begin(ssid, password);
      if (!WaitForWifi(120)){CurrentState = Error; CurrentError = I_WifiTimeOut; MEFCount=6000; return;}
      #ifdef DEBUG
        Serial1.println(WiFi.localIP());
        Serial1.println(WiFi.macAddress());
      #endif
      timer1_isr_init();
      timer1_attachInterrupt(timerISR);
      OldState = CurrentState;
      CurrentState = Check;
    break;
    case Check:
      request.concat(url);request.concat(WiFi.macAddress());request.concat("/version");// request.concat("45");
      #ifdef DEBUG
        Serial1.println(request);
      #endif
      if(WiFi.status() == WL_CONNECTED){
        if (http.begin(client, request)) {  // HTTP
          int httpCode = http.GET();
          if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              result = http.getString();
              if (result == "NewVersion")
              {
                MEFCount = 0;
                OldState = CurrentState;
                CurrentState = Download;
              }else if (result == "Updated"){
                OldState = CurrentState;
                CurrentState = Wait;
                MEFCount = 180;
              } else {
                OldState = CurrentState;
                CurrentState = Error;
                CurrentError = C_InvalidResponse;
                MEFCount = 600;
              }
            }
          } else {
            OldState = CurrentState;
            CurrentState = Error;
            CurrentError = C_InvalidResponse;
            MEFCount = 600;
          }
          http.end();
        } else {
          OldState = CurrentState;
          CurrentState = Error;
          CurrentError = C_NoResponse;
          MEFCount = 600;
        }
      } else {
          OldState = CurrentState;
          CurrentState = Error;
          CurrentError = C_NoConnected;
          MEFCount = 600;
      }
    break;
    case Wait:
      digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
      delay(1000);
      if (MEFCount-- == 0){ CurrentState = Check;}
    break;
    case Download:
      if (programCounter == 0)
      {
        if (!InitProg())
        {
          #ifdef DEBUG
            Serial1.println("Error InitProg");
          #endif
          OldState = CurrentState;
          CurrentState = Error;
          CurrentError = P_ArduinoNoResponse;
          MEFCount = 60;
          break;
        }
      }
      request.concat(url);request.concat(WiFi.macAddress());request.concat("/");request.concat(programCounter++);request.concat("/8/");
      #ifdef DEBUG
        Serial1.println(request);
      #endif
      if(WiFi.status() == WL_CONNECTED){
        if (http.begin(client, request)) {  // HTTP
          int httpCode = http.GET();
          if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              program = http.getString();
              OldState = CurrentState;
              CurrentState = Program;
            }
          } else {
            OldState = CurrentState;
            CurrentState = Error;
            CurrentError = P_InvalidResponse;
            MEFCount = 600;
          }
          http.end();
        } else {
          OldState = CurrentState;
          CurrentState = Error;
          CurrentError = P_NoResponse;
          MEFCount = 600;
        }
      } else {
          OldState = CurrentState;
          CurrentState = Error;
          CurrentError = P_NoConnected;
          MEFCount = 600;
      }
    break;
        case Updated:
      request.concat(url);request.concat(WiFi.macAddress());request.concat("/updated");// request.concat("45");
      #ifdef DEBUG
        Serial1.println(request);
      #endif
      if(WiFi.status() == WL_CONNECTED){
        if (http.begin(client, request)) {  // HTTP
          int httpCode = http.GET();
          if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              result = http.getString();
              if (result == "Updated"){
                OldState = CurrentState;
                CurrentState = Wait;
                MEFCount = 180;
              } else {
                OldState = CurrentState;
                CurrentState = Error;
                CurrentError = C_InvalidResponse;
                MEFCount = 600;
              }
            }
          } else {
            OldState = CurrentState;
            CurrentState = Error;
            CurrentError = C_InvalidResponse;
            MEFCount = 600;
          }
          http.end();
        } else {
          OldState = CurrentState;
          CurrentState = Error;
          CurrentError = C_NoResponse;
          MEFCount = 600;
        }
      } else {
          OldState = CurrentState;
          CurrentState = Error;
          CurrentError = C_NoConnected;
          MEFCount = 600;
      }
    break;
    case Program:
      int aux = program.indexOf(':');
      while(aux != -1){
          #ifdef DEBUG
            Serial1.println("Ingreso While");    
          #endif
          Line *l = new Line(program.substring(aux,program.indexOf(':',aux+1)));
          #ifdef DEBUG
            Serial1.println("Linea Cargada");    
          #endif
          if (l->verify())
          {
            if (l->isEOF())
            {
              p.program();
              //Programar Pagina
              timer1_disable();
              OldState = CurrentState;
              CurrentState = Updated;
              MEFCount = 0;
              programCounter=0;
              p.init();
              break;
            }
            else
            {
              //Agregar linea a la pagina 
              uint8_t result = p.addLine((*l));
              switch (result)
              {
              case 0:
                continue;
                break;
              case -1:
                  OldState = CurrentState;
                  CurrentState = Error;
                  CurrentError = P_ArduinoNoResponse;
                  MEFCount = 60;
                  return;
                  break;
              }
            }
            
          }
          else
          {
            OldState = CurrentState;
            programCounter=0;
            p.init();
            CurrentError = P_ChecksumError;
            CurrentState = Error;
          }
          aux = program.indexOf(':',aux+1);
          //delete l;
      }
      if (CurrentState == Program)
      {
        OldState = CurrentState;
        CurrentState = Download;
      }

    break;
  }
}
