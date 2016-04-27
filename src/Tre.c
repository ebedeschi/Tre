/*
 ============================================================================
 Name        : Tre.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include "board.h"
#include "radio.h"

#define RF_FREQUENCY                                868750000 // Hz

#define TX_OUTPUT_POWER                             20        // 20 dBm

#define LORA_BANDWIDTH                              2         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       12        // [SF7..SF12]
#define LORA_CODINGRATE                             2         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON						0

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT,
}States_t;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone( void );

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError( void );

void printState(uint32_t );
const char *byte_to_binary(int );
void checkStatusRegister(unsigned long long count);

States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

uint16_t BufferSize = 64;
uint8_t Buffer[64+1];

int main(void) {

	BoardInitMcu();
	BoardInitPeriph();

	GpioWrite( &Led1, 1 );

    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

    printState(0);

    // Radio initialization
    Radio.Init( &RadioEvents );

    printState(1);

    Radio.SetChannel( RF_FREQUENCY );


//    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
//                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
//                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
//                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000000 );
//
//    SX1276Write( REG_LR_PACONFIG, RFLR_PACONFIG_PASELECT_PABOOST );
//
//    printState(2);
//
//    // Send the next PING frame
//    strcpy(((char*)Buffer), "PingPongPingPongPingPongPingPongPingPongPingPongPingPongPingPong");
//
//	DelayMs( 1 );
//	Radio.Send( Buffer, BufferSize );
//
//	printState(3);
//
//    // Sets the radio in Tx mode
//    uint8_t ret = 0; //Radio.Read( 0x12 );
//    int i = 0;
//
//    do
//    {
//    	ret = Radio.Read( 0x12 );
//    	printf("%d: %s\n",i, byte_to_binary(ret));
//
//    }while(i++<1000 && ret == 0);

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

    Radio.Rx( 0 ); // Continuous Rx

    long long unsigned int i = 0;

    do
    {
    	checkStatusRegister(i);

    }while( i++ < 1000 );

	puts("\n!!!Hello World!!!"); /* prints !!!Hello World!!! */
	BoardDeInitMcu();
	GpioWrite( &Led1, 0 );
	bcm2835_close();
	return EXIT_SUCCESS;
}

void checkStatusRegister(long long unsigned int count)
{
	char* ret;
	ret = byte_to_binary( Radio.Read( 0x12 ) );
	printf("%llu: %s\n", count, ret);

	if( ret[1] == '1' )
	{
		printf("RxDone\n");
		SX1276OnDio0Irq();
	}
}

const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

void printState(uint32_t c)
{
	printf("--Data: %d\n", c);
	switch(SX1276.Settings.State)
	{
		case RF_IDLE:
			printf("-State: RF_IDLE\n");
		break;
		case RF_RX_RUNNING:
			printf("-State: RF_RX_RUNNING\n");
		break;
		case RF_TX_RUNNING:
			printf("-State: RF_TX_RUNNING\n");
		break;
		case RF_CAD:
			printf("-State: RF_CAD\n");
		break;
	}
	switch(SX1276.Settings.Modem)
	{
		case MODEM_LORA:
			printf("-Modem: MODEM_LORA\n");
		break;
		case MODEM_FSK:
			printf("-Modem: MODEM_FSK\n");
		break;
	}
}


void OnTxDone( void )
{
    Radio.Sleep( );
    State = TX;
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Sleep( );
    BufferSize = size;
    memcpy( Buffer, payload, BufferSize );
    RssiValue = rssi;
    SnrValue = snr;
    Buffer[BufferSize] = '\0';
    int k;
    for(k =0; k<BufferSize; k++)
    	printf("%c", Buffer[k]);
    printf(", %d, %d\n", RssiValue, SnrValue);
    State = RX;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    State = TX_TIMEOUT;
}

void OnRxTimeout( void )
{
    Radio.Sleep( );
    State = RX_TIMEOUT;
}

void OnRxError( void )
{
    Radio.Sleep( );
    State = RX_ERROR;
}
