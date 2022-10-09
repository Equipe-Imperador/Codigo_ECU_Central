/* 
   EQUIPE IMPERADOR DE BAJA-SAE UTFPR
   AUTOR: Juliana Moreira e Matheus Henrique Orsini da Silva
   31/05/2021
   Codigo ECU Central
   INPUTS:   CS-CVT, SCK-CVT, SO-CVT, TRANSDUTOR-1, TRANSDUTOR-2, FREIO_ESTACIONÁRIO
   OUTPUTS:  MsgCAN{TemperaturaCelsius, TemperaturaDecimais, CriticoTemperatura, FreioEstacionario, BateriaVoltz, BateriaMiliVoltz, CriticoBateria, Vazio}
   Método de envio: Utilização de módulo CAN MCP2515
*/

// Include de bibliotecas
#include <max6675.h> // Biblioteca do termopar
#include "mcp2515_can.h" // Biblioteca módulo CAN
#include <SPI.h> // Biblioteca de comunicação do módulo CAN

// Protótipo das funções
float Bateria();
void CalcBat(unsigned short int*, unsigned short int*); // Separar bateria
unsigned short int Freio_Estacionario();
float Temperatura_CVT();
void CalcTemp(unsigned short int*, unsigned short int*);
//void Transdutores();

// Bateria
#define PIN_BAT A0
float Bat = 0;
// Variáveis de separação e crítico
unsigned short int Volts = 0, MiliVolts = 0, Critico_Bat = 0;

// Freio Estacionário
#define PIN_FREIO 4
unsigned short int Freio = 0; // Variável para armazernar o freio estacionário

// Módulo CAN
#define SPI_CS 10
mcp2515_can CAN(SPI_CS); // Cria classe da CAN
uint32_t CAN_ID = 0x01;
unsigned char MsgCAN[8] = {0}; // Vetor da MSG CAN
unsigned long int Tempo = 0;

//Sensor de Temperatura CVT
#define CVT_SCK 9
#define CVT_CS 8
#define CVT_SO 7
float TempCVT = 0; // Variável para armazenar temperatura
// Variáveis de separação e crítico
unsigned short int TempCelc= 0, TempDecm = 0, Critico_Temp = 0;
MAX6675 Termopar(CVT_SCK , CVT_CS, CVT_SO); // Cria classe do sensor

/* Implementação adiada
// Transdutores de pressão
#define PIN_TRANS1 A0
#define PIN_TRANS2 A1
// Variáveis para armazenar tensão dos transdutores
float ValorTrans1 = 0.0;
float ValorTrans2 = 0.0;
*/
void setup() 
{
  
  // Definição dos pinos
  pinMode(PIN_BAT, INPUT);
  pinMode(PIN_FREIO, INPUT);
  /*
  pinMode(PIN_TRANS1, INPUT);
  pinMode(PIN_TRANS2, INPUT);
  */
  // Cria Comunicação Serial
  SERIAL_PORT_MONITOR.begin(115200);
  // Verifica se a Serial foi iniciada
  while(!Serial){};
  // Verifica se a CAN foi iniciada
  while (CAN_OK != CAN.begin(CAN_125KBPS)) 
  {             
      SERIAL_PORT_MONITOR.println("CAN Falhou, tentando novamente...");
      delay(100);
  }
  SERIAL_PORT_MONITOR.println("CAN Iniciada, Tudo OK!");
  
}

void loop() 
{
  Tempo = millis();
  if(Tempo%1000 == 0) // Leitura de dados a cada 1 segundo
  {
    CalcBat(&Volts, &MiliVolts);
    CalcTemp(&TempCelc, &TempDecm);
    Freio = Freio_Estacionario();
    //Transdutores();
    if(TempCVT >= 90)
      Critico_Temp = 1;
    else
      Critico_Temp = 0;
    if(Bat <= 10.9)
      Critico_Bat = 1;
    else
      Critico_Bat = 0;
    // Escreve os dados na mensagem CAN
    MsgCAN[0] = (unsigned char)TempCelc;
    MsgCAN[1] = (unsigned char)TempDecm;
    MsgCAN[2] = (unsigned char)Critico_Temp;
    MsgCAN[3] = (unsigned char)Freio;
    MsgCAN[4] = (unsigned char)Volts;
    MsgCAN[5] = (unsigned char)MiliVolts;
    MsgCAN[6] = (unsigned char)Critico_Bat;
    
    // Envia a Mensagem conforme a forma do cabeçalho
    CAN.sendMsgBuf(CAN_ID, 0, 8, MsgCAN);
   
 
  }
}

/*
    Função para leitura do status da bateria.
    A entrada analógica será utilizada para saber qunato de tensão a bateria está entregando.
    Parâmetros : VOID.
    Return : Float do DDP da bateria.
 */
float Bateria()
{
  /*
      Na entrada analogica a voltagem de entrada é convertina para
      um numero entre 0-1023, para obter o valor em tensão da bateria
      teremos que fazer a seguinte fórmula:
                    (ValorAnalogico*12)/1023
      Obs: A entrada analógica do Arduino suporta somente 5v máximos então
      será necessário um sistema de conversão de 12v para 5v cujo qualquer
      alteração na fonte de 12v altere a entrega de 5v.
   */
   return (float)(analogRead(PIN_BAT) * 12)/ 1023;
}

/*
    Função para separar a Bateria.
    Como um casa do vetor da CAN aceita somente um numero entre 0 e 255 não podemos enviar a Bateria inteira,
    para isso será separado as primeiras duas casas das últimas duas casas em 2 variáveis.
    Parâmetros : Endereço da Casa dos Volts, Endereço da Casa dos miliVolts.
    Return : VOID, Modifica por referência os valores das variáveis de parâmetro.
 */
void CalcBat(unsigned short int* Volts, unsigned short int* MiliVolts)
{
  Bat = Bateria();
  int auxBat = Bat * 100;
  *Volts = (unsigned short int)auxBat/100; // ignora as casas decimais
  *MiliVolts = auxBat % 100; // pega somente as casas decimais
}

/*
    Função para separar a Temperatura.
    Como um casa do vetor da CAN aceita somente um numero entre 0 e 255 não podemos enviar a Temperatura inteira,
    para isso será separado as primeiras duas casas das últimas duas casas em 2 variáveis.
    Parâmetros : Endereço da Casa dos Celcius, Endereço da Casa dos decimais.
    Return : VOID, Modifica por referência os valores das variáveis de parâmetro.
 */
void CalcTemp(unsigned short int* Celcius, unsigned short int* Decimais)
{
  TempCVT = Temperatura_CVT();
  int auxTemp = TempCVT * 100;
  *Celcius = (unsigned short int)auxTemp/100; // ignora as casas decimais
  *Decimais = auxTemp % 100; // pega somente as casas decimais
}

/*
    Função para leitura do freio estacionário.
    A perinha de freio funcio igual um botão, ou seja, permite a passagem
    de sinal somente se fechada(freio acionado), nosso código faz a leitura
    do sinal em 1(Está ativo) ou 0(Está inativo).
    Parâmetros : VOID.
    Return : TRUE(1) se freio precionado, senão FALSE(0).
 */
unsigned short int Freio_Estacionario()
{
  return digitalRead(PIN_FREIO);
}

 /*
    Função para leitura da temperatura da CVT.
    Utilizamos um módulo de arduino(MAX6675) para obter a temperatura ambiente da CVT.
    Parâmetros : VOID.
    Return : Float do valor da temperatura em celsius.
 */
float Temperatura_CVT()
{
  return Termopar.readCelsius();
}

/* Implementação adiada
    Função para leitura dos transdutores de pressão
    Utilizamos das portas analógicas do Arduino para ler um sinal de tensão de até 5V
    recebidos doss transdutores de pressão da linha de freio
    Parâmetros : VOID
    Return : VOID, Tensão recebida pelos transdutores

void Transdutores()
{
  /*
      Na entrada analogica a voltagem de entrada é convertina para
      um numero entre 0-1023, para obter o valor em tensão faça:
                     (ValorAnalogico*5)/1023
   
  ValorTrans1 = (float)(analogRead(PIN_TRANS1) * 5)/ 1023;
  ValorTrans2 = (float)(analogRead(PIN_TRANS2) * 5)/ 1023;
}
*/
