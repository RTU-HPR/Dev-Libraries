#pragma once
#ifdef RADIOLIB_WRAPPER_ENABLE
#include <RadioLib.h>
#include <Arduino.h>
#include <SPI.h>
#include "Sensor_wrapper.h"

namespace RadioLib_interrupts
{
    /**
     * @brief Set the flag that a action has finished
     */
    void set_action_done_flag(void);
}

// TODO: Implement timeouts for interrupts
template <typename T>
class RadioLib_Wrapper : public Sensor_Wrapper
{
private:
    bool _frequency_correction_enabled;
    double _used_frequency; // Current frequency used if frequency correction enabled
    int _action_status_code;

    // Usually displays the last action the radio did
    enum Action_Type
    {
        Transmit,
        Receive,
        Standby,
    };
    Action_Type _action_type;
    //used for crc
    uint16_t crc_xmodem_update(uint16_t crc, uint8_t data);
    /**
     * @brief Configure sx126x based radios so that the chip uses DIO2 pin to control the RXEN and TXEN pins
     *
     * @return true Behaviour set
     * @return false Failed to set behaviour
     */
    bool configure_tx_rx_switching();

    /**
     * @brief Configure sx126x based radios so that micro controller controls the RXEN and TXEN pins
     *
     * @param rx_enable RX enable pin
     * @param tx_enable TX enable pin
     * @return true Behaviour set
     * @return false Failed to set behaviour
     */
    bool configure_tx_rx_switching(int rx_enable, int tx_enable);

    uint16_t calculate_CRC16_CCITT_checksum(const String &msg);

public:
    // Config file
    struct Radio_Config
    {
        enum Chip_Family
        {
            Sx126x,
            Sx127x,
            Sx128x,
            Rfm9x
        };
        enum Rf_Switching
        {
            Dio2,    // rx_enable tx_enable controlled by radio chip using DIO2
            Gpio,    // rx_enable tx_enable controlled by micro controller GPIO pins (if this is set define RX_enable TX_enable gpio pins)
            Disabled // rx_enable
        };

        const float frequency;
        const int cs;
        const int dio0;
        const int dio1;
        const Chip_Family family;  // example: CHIP_FAMILY::SX126x
        Rf_Switching rf_switching; // if == GPIO. define RX_enable TX_enable gpio pins. Currently setup only for sx126x loras
        const int rx_enable;       // only needed if rf_switching = gpio
        const int tx_enable;       // only needed if rf_switching = gpio

        const int reset;     //
        const int sync_word; //
        const int tx_power;  // in dBm
        const int spreading;
        const int coding_rate;
        const float signal_bw; // in khz

        bool frequency_correction;

        SPIClass *spi_bus; // Example &SPI
    };
    // Radio object
    T radio = new Module(-1, -1, -1, -1);

    /**
     * @brief Creates a new RadioLib wrapper and initialize the radio module
     *
     * @param error_function the callback that will be called when error occurs. if not needed use ovverride without this
     */
    RadioLib_Wrapper(void (*error_function)(String) = nullptr, int check_sum_length = 5, String sensor_name = "RadioLib_Default");

    /**
     * @brief
     *
     * @param radio_config Radio config file to be used
     * @return true
     * @return false
     */
    bool begin(Radio_Config radio_config);

    /**
     * @brief Configure radio module modulation parameters (frequency, power, etc.) for exact things that are set check the function
     *
     * @param radio_config Radio config file to be used
     * @return true If configured successfully
     * @return false If not configured successfully
     */
    bool configure_radio(Radio_Config radio_config);

    /**
     * @brief Send a message over the radio
     *
     * @param msg Message to send
     * @return true If transmit was successful
     * @return false If transmit failed
     */
    bool transmit(String msg);

    /**
     * @brief Send a message over the radio as bytes
     *
     * @param msg Bytes to send
     * @param length Byte array length
     * @return true If transmit was successful
     * @return false If transmit failed
     */
    bool transmit_bytes(uint8_t* bytes, size_t length);

    /**
     * @brief Read any received data
     *
     * @param msg Reference to variable where to save the message
     * @param rssi Reference to variable where to save the message RSSI
     * @param snr Reference to variable where to save the message SNR
     * @return true If a message was received
     * @return false If receive failed or no message was received
     */
    bool receive(String &msg, float &rssi, float &snr, double &frequency);

    /**
     * @brief Read any received data as bytes
     * @param bytes Reference to uint8_t array where to save the message
     * @param data_length Reference to variable where to save the message length
     * @param rssi Reference to variable where to save the message RSSI
     * @param snr Reference to variable where to save the message SNR
     * @return true If a message was received
     * @return false If receive failed or no message was received
    */
    bool receive_bytes(uint8_t *bytes, uint16_t &data_length, float &rssi, float &snr, double &frequency);

    /**
     * @brief Modifies the original msg to add the checksum
     *
     * @param msg sendable packet msg that will be modified
     */
    void add_checksum(String &msg);
    /**
     * @brief
     *
     * @param msg sendable packet msg that will be checked. If verified the check sum is removed from the original
     * @return true the msg is verified and the check sum is removed from the original
     * @return false the msg couldn't be verified and msg is left as is
     */
    bool check_checksum(String &msg);

    /**
     * @brief Set the error output function object. If not set by default error will be output in serial port.
     * Note that if the function is inside a class then passing it to the set error output might be difficult
     *
     * @param func pointer to a function that you can pass the error string
     */
    void set_error_output_function(void (*error_function)(String));

    /**
     * @brief Transmit a test message
     *
     * @return true If transmit was successful
     * @return false If transmit failed
     */
    bool test_transmit();
};

// Selected SX12xx LoRa types
template class RadioLib_Wrapper<SX1262>;
template class RadioLib_Wrapper<SX1268>;
template class RadioLib_Wrapper<SX1272>;
template class RadioLib_Wrapper<SX1273>;
template class RadioLib_Wrapper<SX1276>;
template class RadioLib_Wrapper<SX1277>;
template class RadioLib_Wrapper<SX1278>;
template class RadioLib_Wrapper<SX1279>;
template class RadioLib_Wrapper<SX1280>;
template class RadioLib_Wrapper<SX1281>;
template class RadioLib_Wrapper<SX1282>;

// Selected RFM9x LoRa types
// template class RadioLib_Wrapper<RFM95>;
// template class RadioLib_Wrapper<RFM96>;

#endif // RADIOLIB_WRAPPER_ENABLE