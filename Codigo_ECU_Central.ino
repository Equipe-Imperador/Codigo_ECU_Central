/* 
   EQUIPE IMPERADOR DE BAJA-SAE UTFPR
   AUTOR: Juliana Moreira e Matheus Henrique Orsini da Silva
   31/05/2021
   Codigo ECU Central
   INPUTS:   CS-CVT, SCK-CVT, SO-CVT, TRANSDUTOR-1, TRANSDUTOR-2, FREIO ESTACIONÁRIO
   OUTPUTS:  MsgCAN{Temperatura, CriticoTemperatura, FreioEstacionario, Transdutor1, Transdutor2, CAN_ID}
   Método de envio: Utilização de módulo CAN MCP2515
*/

// INCLUDE DAS BIBLIOTECAS
#include <max6675.h> // Biblioteca do termopar
#include "mcp2515_can.h" // Biblioteca módulo CAN
#include <SPI.h> // Biblioteca de comunicação do módulo CAN

// PROTÓTIPO DAS FUNÇÕES
int Freio_Estacionario();
float Temperatura_CVT();
void Transdutores();


//Freio Estacionário
#define PIN_Freio 4
int Freio = 0; // Variável para armazernar o freio estacionário

// Módulo CAN
#define CAN_ID 0x01
#define SPI_CS 10
mcp2515_can CAN(SPI_CS); // Cria classe da CAN
unsigned char MsgCAN[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Vetor da MSG CAN

//Sensor de Temperatura CVT
#define CVT_SCK 9
#define CVT_CS 8
#define CVT_SO 7
float TempCVT = 0; // Variável para armazenar temperatura
int Critico_Temp = 0; // Variável do crítico
MAX6675 Termopar(CVT_SCK , CVT_CS, CVT_SO); // Cria classe do sensor

// Transdutores de pressão
#define PIN_Trans1 A0
#define PIN_Trans2 A1
// Variáveis para armazenar tensão dos transdutores
float ValorTrans1 = 0.0;
float ValorTrans2 = 0.0;

void setup() 
{
  // Definição dos pinos
  pinMode(PIN_Freio, INPUT);
  pinMode(PIN_Trans1, INPUT);
  pinMode(PIN_Trans2, INPUT);

  // Cria Comunicação Serial
  SERIAL_PORT_MONITOR.begin(115200);
  // Verifica se a Serial foi iniciada
  while(!Serial){};
  // Verifica se a CAN foi iniciada
  while (CAN_OK != CAN.begin(CAN_500KBPS)) 
  {             
      SERIAL_PORT_MONITOR.println("CAN Falhou, tentando novamente...");
      delay(100);
  }
  SERIAL_PORT_MONITOR.println("CAN Iniciada, Tudo OK!");
  
}
int Tempo = 0;
void loop() 
{
  Tempo = millis();
  if(Tempo%1000 == 0) // Leitura de dados a cada 1 segundo
  {
    TempCVT = Temperatura_CVT();
    Freio = Freio_Estacionario();
    Transdutores();
    if(TempCVT >= 90)
      Critico_Temp = 1;
    else
      Critico_Temp = 0;
    // Escreve os dados na mensagem CAN
    MsgCAN[0] = TempCVT;
    MsgCAN[1] = Critico_Temp;
    MsgCAN[2] = Freio;
    MsgCAN[3] = ValorTrans1;
    MsgCAN[4] = ValorTrans2;
    MsgCAN[5] = CAN_ID;
    // Envia a Mensagem conforme a forma do cabeçalho
    CAN.sendMsgBuf(CAN_ID, 0, 8, MsgCAN);
  }
}

/*
    Função para leitura da temperatura da CVT
    Utilizamos um módulo de arduino(MAX6675) para obter a temperatura ambiente da CVT
    Parâmetros : VOID
    Return : Float do valor da temperatura em celsius
 */
float Temperatura_CVT()
{
  return Termopar.readCelsius();
}

/*
    Função para leitura do freio estacionário
    A perinha de freio funcio igual um botão, ou seja, permite a passagem
    de sinal somente se fechada(freio acionado), nosso código faz a leitura
    do sinal em 1(Está ativo) ou 0(Está inativo)
    Parâmetros : VOID
    Return : TRUE(1) se freio precionado, senão FALSE(0)
 */
int Freio_Estacionario()
{
  return digitalRead(PIN_Freio);
}

/*
    Função para leitura dos transdutores de pressão
    Utilizamos das portas analógicas do Arduino para ler um sinal de tensão de até 5V
    recebidos doss transdutores de pressão da linha de freio
    Parâmetros : VOID
    Return : VOID, Tensão recebida pelos transdutores
 */
void Transdutores()
{
  /*
      Na entrada analogica a voltagem de entrada é convertina para
      um numero entre 0-1023, para obter o valor em tensão faça:
                     (ValorAnalogico*5)/1024
   */
  ValorTrans1 = (float)(analogRead(PIN_Trans1) * 5)/ 1024;
  ValorTrans2 = (float)(analogRead(PIN_Trans2) * 5)/ 1024;
}
