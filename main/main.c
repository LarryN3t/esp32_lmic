#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"


#include "lmic.h"
#include "bmp280.h"

#include "tcp_server.h"

#define SDA_GPIO GPIO_NUM_21
#define SCL_GPIO GPIO_NUM_22

#define LORA_PIN_SPI_SCLK  CONFIG_LORA_PIN_SPI_SCLK
#define LORA_PIN_SPI_MOSI  CONFIG_LORA_PIN_SPI_MOSI
#define LORA_PIN_SPI_MISO  CONFIG_LORA_PIN_SPI_MISO
#define LORA_PIN_NSS       CONFIG_LORA_PIN_NSS
//##define LORA_PIN_RXTX      TTN_NOT_CONNECTED
#define LORA_PIN_RST       CONFIG_LORA_PIN_RST
#define LORA_PIN_DIO0      CONFIG_LORA_PIN_DIO0
#define LORA_PIN_DIO1      CONFIG_LORA_PIN_DIO1
#define LORA_PIN_DIO2      CONFIG_LORA_PIN_DIO2


const lmic_pinmap lmic_pins = {
    .nss = LORA_PIN_NSS ,
    .rst = LORA_PIN_RST,
    .dio = {LORA_PIN_DIO0, LORA_PIN_DIO1, LORA_PIN_DIO2},
    .spi = {LORA_PIN_SPI_MISO,LORA_PIN_SPI_MOSI,LORA_PIN_SPI_SCLK},
    .rxtx = LMIC_UNUSED_PIN,
};

unsigned int DevAdd;
unsigned char NetKey[16];
unsigned char AppKey[16];


//u1_t NWKSKEY[16] = { 0xa2, 0x25, 0xb5, 0x22, 0x8f, 0xe5, 0xb8, 0x18, 0xb6, 0x19, 0xc1, 0xb0, 0x9b, 0xc8, 0xdb, 0xd2 };
//u1_t APPSKEY[16] = { 0xfa, 0x4c, 0xd4, 0xd2, 0x78, 0x0d, 0x77, 0xc1, 0x03, 0x82, 0xab, 0x77, 0xdb, 0xd8, 0xe7, 0x00 };
//u4_t DEVADDR = 0x07a56a5e;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

//static uint8_t msgData[40] = "Ciao!";
unsigned char mydata[] = "Ciao!";

const unsigned TX_INTERVAL = CONFIG_LORA_TX_INTERVAL;

static osjob_t sendjob;


void lorasetup(void) {
    const char devadd[]= CONFIG_devAdd;
    const char netkey[]= CONFIG_netKey , *pn = netkey;
    const char appkey[]= CONFIG_appKey , *pa = appkey;
/*  
    for (size_t count = 0; count < sizeof DevAdd/sizeof *DevAdd; count++) {
        sscanf(da, "%2hhx", &DevAdd[count]);
        da += 2;
    }
*/
    DevAdd = (int)strtol(devadd,NULL, 16);

    for (size_t count = 0; count < sizeof NetKey/sizeof *NetKey; count++) {
        sscanf(pn, "%2hhx", &NetKey[count]);
        pn += 2;
    }
    for (size_t count = 0; count < sizeof AppKey/sizeof *AppKey; count++) {
        sscanf(pa, "%2hhx", &AppKey[count]);
        pa += 2;
    }
}


void sendMessages(osjob_t* j) {
   lorasetup();
   LMIC_setSession (0x1, DevAdd, NetKey, AppKey);

  if (LMIC.opmode & OP_TXRXPEND) {
      printf("OP_TXRXPEND, not sending");
  } else {
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, msgData, sizeof(msgData)-1, 0);
      printf("Packet sent");
  }

}


void onEvent (ev_t ev) {
    printf("%d", os_getTime());
    printf(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            printf("EV_SCAN_TIMEOUT\n");
            break;
        case EV_BEACON_FOUND:
            printf("EV_BEACON_FOUND\n");
            break;
        case EV_BEACON_MISSED:
            printf("EV_BEACON_MISSED\n");
            break;
        case EV_BEACON_TRACKED:
            printf("EV_BEACON_TRACKED\n");
            break;
        case EV_JOINING:
            printf("EV_JOINING\n");
            break;
        case EV_JOINED:
            printf("EV_JOINED\n");
            break;
        case EV_RFU1:
            printf("EV_RFU1\n");
            break;
        case EV_JOIN_FAILED:
            printf("EV_JOIN_FAILED\n");
            break;
        case EV_REJOIN_FAILED:
            printf("EV_REJOIN_FAILED\n");
            break;
        case EV_TXCOMPLETE:
            printf("EV_TXCOMPLETE (includes waiting for RX windows)\n");
            if (LMIC.txrxFlags & TXRX_ACK)
              printf("Received ack\n");
            if (LMIC.dataLen) {
              printf("Received \n");
              printf(LMIC.dataLen);
              printf(" bytes of payload\n");
            }
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), sendMessages);
	    break;
/*	case EV_TXSTART:
	    printf("EV_TXSTART\n");
            break;*/
        case EV_LOST_TSYNC:
            printf("EV_LOST_TSYNC\n");
            break;
        case EV_RESET:
            printf("EV_RESET\n");
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            printf("EV_RXCOMPLETE\n");
            break;
        case EV_LINK_DEAD:
            printf("EV_LINK_DEAD\n");
            break;
        case EV_LINK_ALIVE:
            printf("EV_LINK_ALIVE\n");
            break;
         default:
            printf("Unknown event: %d\n", ev);
            break;
    }
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void bmp280_test(void *pvParamters)
{
    bmp280_params_t params;
    bmp280_init_default_params(&params);
    bmp280_t dev;

    esp_err_t res;

    while (i2cdev_init() != ESP_OK)
    {
        printf("Could not init I2Cdev library\n");
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    while (bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_1, I2C_NUM_0, SDA_GPIO, SCL_GPIO) != ESP_OK)
    {
        printf("Could not init device descriptor\n");
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    while ((res = bmp280_init(&dev, &params)) != ESP_OK)
    {
        printf("Could not init BMP280, err: %d\n", res);
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    bool bme280p = dev.id == BME280_CHIP_ID;
    printf("BMP280: found %s\n", bme280p ? "BME280" : "BMP280");

    float pressure, temperature, humidity;

    while (1)
    {
	int b = 0; 
        int e = 0;
	int pa = 0;
        vTaskDelay(30 * 1000/ portTICK_PERIOD_MS);
        if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) != ESP_OK)
        {
            printf("Temperature/pressure reading failed\n");
            continue;
        }

        printf("Pressure: %.2f Pa, Temperature: %.2f C", pressure, temperature);
	sprintf((char*)msgData,"t:%.2f,p:%.2f,vb:%i,ve:%i,vp:%i",temperature,pressure,b,e,pa);
	//sprintf((char*)msgData,"|%.2f|%.2f|0.00|%i|%i|%i",temperature,pressure,b,e,pa);
        if (bme280p)
            printf(", Humidity: %.2f\n", humidity);
        else
            printf("\n");
    }
}

void LoraStart(void) {
 // Steup Radio 
  LMIC_reset();
  printf("LMIC RESET\n");
//  lorasetup();
//  LMIC_setSession (0x1, DevAdd, NetKey, AppKey);
   /*
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
*/
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI); // g2-band

  LMIC_setLinkCheckMode(0);
  LMIC.dn2Dr = DR_SF9;
  LMIC.datarate = DR_SF7;
  LMIC_setDrTxpow(DR_SF7,14);
  LMIC.txChnl = 0;
  //LMIC_setDrTxpow(DR_SF7,14);
  int channel = 0;
  for(int i = 0; i <= 8; i++) {
    if (i != channel) { 
	LMIC_disableChannel(i);
	printf("Channel disabled: %i\n", i);
	}
    }
	
  // Task Loop
    sendMessages(&sendjob);
    for (;;) {
        os_runloop_once();
        vTaskDelay(1);
    }


	
}
void app_main(void)
{
  ESP_ERROR_CHECK( nvs_flash_init() );
  os_init();
  initialise_wifi();  
  wait_for_ip();

  xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
  xTaskCreate(LoraStart, "LoraStart", 1024 * 4, (void* )0, 3, NULL);	
}
