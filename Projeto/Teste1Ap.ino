//-----Bibliotecas Servidor----
 #include <WiFiClient.h>
 #include <AsyncTCP.h>
 #include <ESPAsyncWebServer.h>
 #include "index.h"
 #include <FS.h>
 #include "SPIFFS.h"

//------Bibliotecas Sensores----
#include <driver/adc.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ThingSpeak.h" 


const int PIN_AP = 2;//Pino de Inicio do AP
const int PIN_RELE = 4;//Pino de acionamneto do relê de corte.
 
String rede ;
String senha_wifi; 

const char* PARAM_INPUT_1 = "rede";
const char* PARAM_INPUT_2 = "senha_wifi";

//Rede Local AP  
const char* ssid = "Nicolas";
const char* senha = "12345678";

//Chaves de Acesso ao Thingspeak
unsigned long myChannelNumber = 875859 ; //update
const char * myWriteAPIKey = "MKXZ4EWMOZ4PPIEO"; //update


// GPIO onde o DS18B20 esta conectado
const int oneWireBus = 33;

// Inicia instância da oneWire para comunicar com qquer dispositivo OneWire
OneWire oneWire(oneWireBus);

// Passa as referencia do oneWire para o sensor de temperatura Dallas
DallasTemperature sensors(&oneWire);

//inicia a contador para o intervalo de atualização do thingspeak
long int currentMillis, previousMillis, escrita = 0;

AsyncWebServer server(80);

WiFiClient client;

void setup(){
Serial.begin(115200);
ThingSpeak.begin(client); // Inicializa ThingSpeak
// Definição das GPIO 
pinMode(PIN_AP, INPUT);
pinMode(PIN_RELE,OUTPUT);
inicializa_adc();
//Inicializa o termômetro
sensors.begin();
SPIFFS.format();
openFS();
conectando();
}

void loop(void){
String leiturarede = readFile("/rede.txt");
String leiturasenha = readFile("/senha.txt");

Serial.println(leiturarede);
Serial.println(leiturasenha);

if(digitalRead(PIN_AP) == HIGH){
  AP(1);
  }

  
if(WiFi.status() == WL_CONNECTED){
 //****LEITURA DAS PORTAS DO ESP

  //Media simples de 100 leituras intervaladas com 30ms
  int ch_0 = 0;
  int ch_3 = 0;
  int ch_4 = 0;
  //  int ch_5 = 0;  (usado para leitura digital de temperatura)
  int ch_6 = 0;
  int ch_7 = 0;


  sensors.requestTemperatures();
  float ch_5 = sensors.getTempCByIndex(0);

  for (int i = 0; i < 100; i++)
  {
    ch_0 += adc1_get_raw(ADC1_CHANNEL_0); //Obtem o valor RAW do ADC do GPIO36 canal 1    (Não utilizado)
    ch_3 += adc1_get_raw(ADC1_CHANNEL_3); //Obtem o valor RAW do ADC do GPIO39 canal 3    (TA12-100)
    ch_4 += adc1_get_raw(ADC1_CHANNEL_4); //Obtem o valor RAW do ADC do GPIO32 canal 4    (acs712)
    //    ch_5 += adc1_get_raw(ADC1_CHANNEL_5); //Obtem o valor RAW do ADC do GPIO33 canal 5  (temperatura)
    ch_6 += adc1_get_raw(ADC1_CHANNEL_6); //Obtem o valor RAW do ADC do GPIO34 canal 6    (abertura porta)
    ch_7 += adc1_get_raw(ADC1_CHANNEL_7); //Obtem o valor RAW do ADC do GPIO35 canal 7    (luminosidade)

    delay(3);
  }
  ch_0 /= 100;
  ch_3 /= 100;
  ch_4 /= 100;
  //  ch_5 /= 100; (comentado pq está sendo atribuido valor diretamente pelo método sensors)
  ch_6 /= 100;
  if (ch_6 > 3800) {
    ch_6 = 1;  // (porta fechada = 0 / porta aberta = 1)
  } else {
    ch_6 = 0;
  }
  ch_7 /= 100;

  // Escreve no ThingSpeak. Máximo de 8 campos por canal
  ThingSpeak.setField(1, ch_0); //atualiza campo 1 (sem uso)
  ThingSpeak.setField(2, ch_3); //atualiza campo 2 (TA12-100)
  ThingSpeak.setField(3, ch_4); //atualiza campo 3 (ACS712)
  ThingSpeak.setField(4, ch_5); //atualiza campo 4 (temperatura)
  ThingSpeak.setField(5, ch_6); //atualiza campo 5 (abertura porta)
  ThingSpeak.setField(6, ch_7); //atualiza campo 6 (luminosidade)


  // Escreve no canal do thingspeak a cada 20s

  currentMillis = millis(); //recebe o tempo atual em ms

  if (currentMillis - previousMillis > 17000) {

    digitalWrite(4, !digitalRead(4)); // Inverte o estado da porta do relê
    
    currentMillis = previousMillis; //reinicializa contadores
    
    escrita += 1;
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.print("Canal ");
      Serial.print(myChannelNumber);
      Serial.print(" atualizado com sucesso! (");
      Serial.print(escrita);
      Serial.println(")");
    }
    else {
      Serial.println("Erro ao atualizar o canal. HTTP error code " + String(x));
    }
  }
  Serial.println("-----------------------------------------");
  Serial.print("Canal 0: ");
  Serial.println(ch_0);
  //  Serial.print("Canal 3: ");
  Serial.print("AT12-100: ");
  Serial.println(ch_3);
  //  Serial.print("Canal 4: ");
  Serial.print("ACS712: ");
  Serial.println(ch_4);
  //  Serial.print("Canal 5: ");
  Serial.print("Temperatura: ");
  Serial.println(ch_5);
  //  Serial.print("Canal 6: ");
  Serial.print("Abertura Porta: ");
  Serial.println(ch_6);
  //  Serial.print("Canal 7: ");
  Serial.print("Luminosidade : ");
  Serial.println(ch_7);
  Serial.println("-----------------------------------------");

  delay(2000); // Intervalo de 3 segundos para atualizar novamente o canal  
}
delay(5000);

}

//------- Códigos dos sensores-------
void inicializa_adc(){
adc1_config_width(ADC_WIDTH_BIT_12);  //define resolução de 12-bits (0 a 4095) para as portas do adc1

  //**** DEFINIÇÕES DAS TENSÕES MÁXIMAS EM CADA PORTA
  //adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0); //define atenuação de 0db (1.1v) para o canal 0 GPIO36
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); //define atenuação de 11db (3.3v) para o canal 0 GPIO36
  //adc1_config_channel_atten(ADC1_CHANNEL_3,ADC_ATTEN_DB_0); //define atenuação de 0db (1.1v) para o canal 0 GPIO39
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11); //define atenuação de 11db (3.3v) para o canal 0 GPIO39
  //adc1_config_channel_atten(ADC1_CHANNEL_4,ADC_ATTEN_DB_0); //define atenuação de 0db (1.1v) para o canal 0 GPIO32
  adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11); //define atenuação de 11db (3.3v) para o canal 0 GPIO32
  //adc1_config_channel_atten(ADC1_CHANNEL_5,ADC_ATTEN_DB_0); //define atenuação de 0db (1.1v) para o canal 0 GPIO33
  adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11); //define atenuação de 11db (3.3v) para o canal 0 GPIO33
  //adc1_config_channel_atten(ADC1_CHANNEL_6,ADC_ATTEN_DB_0); //define atenuação de 0db (1.1v) para o canal 0 GPIO34
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); //define atenuação de 11db (3.3v) para o canal 0 GPIO34
  //adc1_config_channel_atten(ADC1_CHANNEL_7,ADC_ATTEN_DB_0); //define atenuação de 0db (1.1v) para o canal 0 GPIO35
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11); //define atenuação de 11db (3.3v) para o canal 0 GPIO35
  
  
}


//---------- Funções do Web Server --------------------
//Função que Faz conexão com a internet
void conectando(){
if(!SPIFFS.exists("/rede.txt")){
  Serial.println("não existe arquivo");
    AP(0);
  }else{
    Serial.println("existe arquivo");
    WiFi.mode(WIFI_STA);
    const String lr = readFile("/rede.txt");
    const String ls = readFile("/senha.txt");
    //const char* lr = "IOT";
    //const char* ls = "12#$56qwIOT";
    WiFi.begin((const char*)lr.c_str(),(const char*)ls.c_str());
    
    while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Conectando ao Wifi");
     if(digitalRead(PIN_AP) == HIGH){
       AP(1);
      }
     }
     Serial.println("Wifi Conectado");

  }
}

//Função que entra no Web Server.
void AP(int estado){
//Formata o Sistema de Arquivos
SPIFFS.format();
openFS();
Serial.printf("Configurando ponto de acesso '%s'n", ssid);
WiFi.mode(WIFI_AP);
WiFi.softAP(ssid,senha);

 

 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    /*if(estado == 1){
      request->send_P(200, "text/html","<h2>Rede ou Senha Errados por favor tente novamente</h2><br><a href=\"/\">Página Inicial</a>");
      }*/
    request->send_P(200, "text/html",MAIN_page);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String redeA;
    String inputParam;
    String sA;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      redeA = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      rede = redeA;
      writeFile(redeA,"/rede.txt");
    }
    if (request->hasParam(PARAM_INPUT_2)) {
      sA = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      senha_wifi = sA;
      writeFile(sA,"/senha.txt");
    }

    if(SPIFFS.exists("/rede.txt")){
    conectando();
  }
    
   });  
   
server.begin();



}

//--------------- Funções do Sistema de Arquivos ---------------
//Escrita de Dados no arquivo
void writeFile(String state, String path) { 
  //Abre o arquivo para escrita ("w" write)
  //Sobreescreve o conteúdo do arquivo
  File rFile = SPIFFS.open(path,"w+"); 
  if(!rFile){
    Serial.println("Erro ao abrir arquivo!");
  } else {
    rFile.println(state);
    Serial.print("gravou estado: ");
    Serial.println(state);
  }
  rFile.close();
}

//Leitura de dados do arquivos.
 String readFile(String path) {
  File rFile = SPIFFS.open(path,"r");
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo!");
  }
  String content = rFile.readStringUntil('\r'); //desconsidera '\r\n'
  Serial.print("leitura de estado: ");
  Serial.println(content);
  rFile.close();
  return content;
}

//Abertura do sistema de arquivos
void openFS(void){
  //Abre o sistema de arquivos
  if(!SPIFFS.begin()){
    Serial.println("\nErro ao abrir o sistema de arquivos");
  } else {
    Serial.println("\nSistema de arquivos aberto com sucesso!");
  }
}
