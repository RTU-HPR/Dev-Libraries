#include <Arduino.h>
#include <SPI.h>
#include <RadioLib_wrapper.h>

// Pins for the SPI bus, that the radio module uses
const int SPI_RX = 4; // MISO
const int SPI_TX = 7; // MOSI
const int SPI_SCK = 6;

// Used radio module series
// Our commonly used LoRa module and its series:
//  * RFM96 - Module is SX1272 and family is SX127X
//  * E22-400 - Module is SX1268 and family is SX126X
#define radio_module SX1268
#define radio_module_family SX126X

// RX/TX switching used by the radio module
//  * DIO2 - If RX/TX enable is controlled by radio chip using DIO2
//  * GPIO - If RX/TX enable controlled by micro controller GPIO pins (if this is set define RX_enable TX_enable gpio pins)
//  * DISABLED  - If not using either, set to this
// DIO2 and GPIO currently only set up for SX126X LoRa
#define radio_module_rf_switching DIO2

// Radio module config
RadioLib_Wrapper<radio_module>::RADIO_CONFIG radio_config{
    .FREQUENCY = 434.5, // Frequency
    .CS = 2,            // Chip select
    .DIO0 = 3,          // Busy
    .DIO1 = 5,          // Interrupt action
    .FAMILY = RadioLib_Wrapper<radio_module>::RADIO_CONFIG::CHIP_FAMILY::radio_module_family,
    .rf_switching = RadioLib_Wrapper<radio_module>::RADIO_CONFIG::RF_SWITCHING::radio_module_rf_switching,
    // If using GPIO pins for RX/TX switching, define pins used for RX and TX control
    // .RX_ENABLE = 0,
    // .TX_ENABLE = 0,
    .RESET = 8,
    .SYNC_WORD = 0xF4,
    .TXPOWER = 14,
    .SPREADING = 10,
    .CODING_RATE = 7,
    .SIGNAL_BW = 125,
    .SPI_BUS = &SPI, // SPI bus used by radio
};

// Create radio object
RadioLib_Wrapper<radio_module> radio = RadioLib_Wrapper<radio_module>(radio_config);

// Ping pong
bool should_transmit = true;
int message_index = 1;
int WAIT_FOR_RECEIVE = 5000;
unsigned long last_transmit_time = 0;

void setup()
{
  // Configure and begin SPI bus
  SPI.setRX(SPI_RX);
  SPI.setTX(SPI_TX);
  SPI.setSCK(SPI_SCK);
  SPI.begin();

  // Configure radio module
  if (!radio.configure_radio(radio_config))
  {
    while (true)
    {
      Serial.println("Configuring LoRa failed");
      delay(5000);
    }
  }

  // If required a test message can be transmitted
  // radio.test_transmit();
}

void loop()
{
  // Variables to store the received data
  String msg = "";
  float rssi = 0;
  float snr = 0;

  // Check if anything has been received
  if (radio.receive(msg, rssi, snr))
  {
    // Check if checksum matches
    if (radio.check_checksum(msg))
    {
      Serial.println("LoRa received: " + msg + " | RSSI: " + rssi + "| SNR: " + snr);
    }
    else
    {
      Serial.println("LoRa received with checksum fail: " + msg + " | RSSI: " + rssi + "| SNR: " + snr);
    }
    should_transmit = true;
  }

  // If nothing has been received in the defined time period, transmit a message
  if (should_transmit || millis() - last_transmit_time > WAIT_FOR_RECEIVE)
  {
    String tx_message = "Ping pong message " + String(message_index);
    radio.add_checksum(tx_message);
    radio.transmit(tx_message);
      
    message_index ++;
    should_transmit = false;
    last_transmit_time = millis();
  }
}
