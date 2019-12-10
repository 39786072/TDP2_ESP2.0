#include <Arduino.h>
#include "Programador.h"
#include "command.h"


unsigned char strtobyte(char value)
{
  if (value <= '9' && value >= '0') // si el valor está entre 0 y 9
  {
    value -= '0'; //resto el valor del char 0
  }
  if (value <= 'F' && value >= 'A')
  {
    value = value - 'A' + 10;
  }
  if (value <= 'f' && value >= 'a')
  {
    value = value - 'a' + 10;
  }
  return value;
}
bool WaitFor(uint8_t c, uint16_t timeout)
{
  bool found = 0;
  while(!found && timeout--)
  {
    if (Serial.available())
    {
      found = Serial.read() == c;
    }
  }
  return found;
}



void InitProg ()
{
      #ifdef DEBUG
        Serial1.println("Reset Slave");
      #endif
      digitalWrite(RST_Slave,LOW);
      delay(3);
      digitalWrite(RST_Slave,HIGH);
      delay(100);
      
      #ifdef DEBUG
        Serial1.println("Sync");
      #endif
      Serial.write(Cmnd_STK_GET_SYNC);
      Serial.write(Sync_CRC_EOP);
      WaitFor(Resp_STK_INSYNC,65000);
      WaitFor(Resp_STK_OK,65000);
     
      
      #ifdef DEBUG
        Serial1.println("Sync");
      #endif
      Serial.write(Cmnd_STK_GET_SYNC);
      Serial.write(Sync_CRC_EOP);
      WaitFor(Resp_STK_INSYNC,65000);
      WaitFor(Resp_STK_OK,65000);
      
      #ifdef DEBUG
        Serial1.println("GET MAJOR");
      #endif
      Serial.write(Cmnd_STK_GET_PARAMETER);
      Serial.write(Parm_STK_SW_MAJOR);
      Serial.write(Sync_CRC_EOP);
      
      WaitFor(Resp_STK_INSYNC,65000);
      WaitFor(0x4,65000); 
      WaitFor(Resp_STK_OK,65000);

      
      #ifdef DEBUG
        Serial1.println("Get MINOR");
      #endif
      Serial.write(Cmnd_STK_GET_PARAMETER);
      Serial.write(Parm_STK_SW_MINOR);
      Serial.write(Sync_CRC_EOP);
      
      WaitFor(Resp_STK_INSYNC,65000);
      WaitFor(0x4,65000); 
      WaitFor(Resp_STK_OK,65000);
      
      
      #ifdef DEBUG
        Serial1.println("Set Device");
      #endif
      Serial.write(Cmnd_STK_SET_DEVICE);
      Serial.write(device,20);
      Serial.write(Sync_CRC_EOP);
      
      WaitFor(Resp_STK_INSYNC,65000); 
      WaitFor(Resp_STK_OK,65000);

      
      #ifdef DEBUG
        Serial1.println("Set Device Ext");
      #endif
      Serial.write(Cmnd_STK_SET_DEVICE_EXT);
      Serial.write(device_ext,5);
      Serial.write(Sync_CRC_EOP);
      
      WaitFor(Resp_STK_INSYNC,65000); 
      WaitFor(Resp_STK_OK,65000);
      
      
      #ifdef DEBUG
        Serial1.println("Enter ProgMode");
      #endif
      Serial.write(Cmnd_STK_ENTER_PROGMODE);
      Serial.write(Sync_CRC_EOP);
      
      WaitFor(Resp_STK_INSYNC,65000); 
      WaitFor(Resp_STK_OK,65000);
      
      #ifdef DEBUG
        Serial1.println("Read Sign");
      #endif
      Serial.write(Cmnd_STK_READ_SIGN);
      Serial.write(Sync_CRC_EOP);
      //TODO: Falta chekeqar la firma
      WaitFor(Resp_STK_INSYNC,65000); 
      WaitFor(Resp_STK_OK,65000);
      
      #ifdef DEBUG
        Serial1.println("Enable Timer");
      #endif
      timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);
      timer1_write(62500);
}
/**
 * @brief Construct a new Page:: Page object
 * 
 * @param _pageSize 
 */
Page::Page(int _pageSize)
{
    #ifdef DEBUG
      Serial1.print("Creando pagina de tamaño: ");
      Serial1.println(_pageSize);
    #endif
    pageIndex = 0;
    pageSize = _pageSize;
    pagePos = 0;
    page = (uint8_t *) calloc(pageSize,sizeof(uint8_t));
    #ifdef DEBUG
      if (page == NULL)
      {
        Serial1.println("Error al alocar memoria");
      }
    #endif
}
/**
 * @brief Destroy the Page:: Page object
 * 
 */
Page::~Page()
{
  free(page);
}
bool Page::isComplete(){
  return (pagePos + 1) == pageSize;
}
/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool Page::program(){
  #ifdef DEBUG
      Serial1.println("Programando pagina");
  #endif
  int i = 0;
  //si la pagina no está completaCompletar arreglo de datos con 0
  //Esto se podria evitar si funciona el calloc
  if (!this->isComplete())
  {
    //Completar con 1s
  }
    //Parar timer
  timer1_disable();
    //Setear Direccion Ver calculo jeje no is too isi Hay que dividir por dos porque la memoria del arduino tiene celdas de 16 bits 
  Serial.write(Cmnd_STK_LOAD_ADDRESS);
  Serial.write(((pageIndex*pageSize) / 2 ) & 0xFF);
  Serial.write(((pageIndex*pageSize) / 2 ) >> 8 & 0xFF);
  Serial.write(Sync_CRC_EOP);
    //Esperar confirmacion
  WaitFor(Resp_STK_INSYNC,65000); 
  WaitFor(Resp_STK_OK,65000);
    //Enviar Datos
    //Esperar confirmacion
  Serial.write(Cmnd_STK_PROG_PAGE);
  Serial.write(pageSize >> 8 & 0xFF);
  Serial.write(pageSize & 0xFF);
  Serial.write(0x46); //0x46 es la letra F que indica que lo tiene que guardar en flash puede ser E para EEPROM

  for ( i = 0; i < pageSize; i++)
  {
    Serial.write(page[i]);
  }
  Serial.write(Sync_CRC_EOP);
  WaitFor(Resp_STK_INSYNC,65000); 
  WaitFor(Resp_STK_OK,65000);

    //Reiniciar Timer
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);
    //Aumentar index pagina
  pageIndex++;
  pagePos = 0; 
    return true;
}
//Si retorna falso debe reprocesarse la linea, si no está terminada
/**
 * @brief Add a new line to page
 * 
 * @param l is the new line
 * @return true if the line was processed comple
 * @return false 
 */
bool Page::addLine(Line l){
  uint8_t i = 0;
  uint8_t j = 0;
  #ifdef DEBUG
    Serial1.println("Agregar Linea a Pagina ");
    Serial1.print("Posicion Actual: ");
    Serial1.print(ACTUALPOS);
    Serial1.print(" Posicion Fin Pagina Actual: ");
    Serial1.print(PAGEENDS);
    Serial1.print("Tamaño de pagina: ");
    Serial1.print(pageSize);
    Serial1.print("Posicion en pagina: ");
    Serial1.print(pagePos);
    Serial1.print("Numero de pagina: ");
    Serial1.println(pageIndex);
    Serial1.print("Linea address  ");
    Serial1.print(l.getAddress());
    Serial1.print("   Linea length  ");
    Serial1.println(l.getLenght());  
  #endif
  if (l.getAddress() >= ACTUALPOS && l.getAddress() <= PAGEENDS )
  { //si la primera direccion está dentro de la pagina
    #ifdef DEBUG
      Serial1.println("La linea empieza en esta pagina");    
    #endif
    if (l.getAddress() + l.getLenght() <= PAGEENDS)
    {
      #ifdef DEBUG
        Serial1.println("La linea entra completa");    
      #endif
      //Copia la informacion de una linea dentro de la pagina (Entra la linea completa)
      for (i = 0; i < l.getLenght(); i++)
      {
          #ifdef DEBUG
           Serial1.print(l.getData(i), HEX);    
          #endif
         page[pagePos++] = l.getData(i);
      }
      #ifdef DEBUG
        Serial1.println("Termine");    
      #endif
      if (ACTUALPOS == PAGEENDS)
      {
        this->program();
      }
      return true;
    }
    else
    { //Si la linea no entra completa copia hasta completarla
      #ifdef DEBUG
        Serial1.println("La linea no termina en esta pagina");    
      #endif
      j =  pageSize - pagePos;
      for (i = 0; i < j; i++)
      {
        #ifdef DEBUG
          Serial1.print(l.getData(i), HEX);    
        #endif
        page[pagePos++] = l.getData(i);
      }
      #ifdef DEBUG
        Serial1.println("Termine");    
      #endif
      this->program();
      return false;
    }
  } else {
    #ifdef DEBUG
      Serial1.println("La linea no empieza en esta pagina");    
    #endif
    this->program();
    return false;
  }
}
/**
 * @brief Construct a new Line:: Line object
 * 
 * @param linea String with a line of Intel Hex file
 */
Line::Line(String linea) {
  #ifdef DEBUG
    Serial1.println(linea);    
  #endif
  int i = 0 , j = 0;
  while (linea[i++] != ':'); // Busco el primer : y lo descarto, es un caracter que usa la estructura de intel como start
    len       =                                                                 (strtobyte(linea[i++]) << 4) + strtobyte(linea[i++]);
    address   = (strtobyte(linea[i++]) << 12) +  (strtobyte(linea[i++]) << 8) + (strtobyte(linea[i++]) << 4) + strtobyte(linea[i++]);
    type      =                                                                 (strtobyte(linea[i++]) << 4) + strtobyte(linea[i++]);
    if (len != 0){ 
    data = (uint8_t*)malloc(len * sizeof(uint8_t));
      for (j = 0; j < len; j++)
      {
        data[j] = (strtobyte(linea[i++]) << 4) +  strtobyte(linea[i++]);
      }
    }
    checksum = (strtobyte(linea[i++]) << 4) +  strtobyte(linea[i++]);
}
/**
 * @brief Destroy the Line:: Line object
 */
Line::~Line(){
  //free(data);
}
bool Line::isEOF(){
  return type == 1;
}
/**
 * @brief 
 * 
 * @return true si la suma es correcta
 * @return false si la sima es incorrecta
 */
bool Line::verify(){
  uint8_t i = 0;
  uint16_t sum = 0;
  sum += (len + (address & 0xFF) + (address >> 8) + type);
  if (len != 0)
  {
    for(i = 0; i < len; i++)
    {
      sum += data[i];
    }
  }
  sum = ((~sum) + 1) & 0xFF;
  return sum == checksum;
}
/**
 * @brief Getter of line address
 * @return uint16_t 
 */
uint16_t Line::getAddress(){
  return address;
}
/**
 * @brief Getter of line data length
 * @return uint8_t 
 */
uint8_t Line::getLenght(){
  return len;
}
/**
 * @brief Getter of line data
 * @return uint8_t* 
 */
uint8_t Line::getData(uint8_t pos){
  return data[pos];
}
