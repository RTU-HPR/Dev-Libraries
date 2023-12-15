#ifdef RADIOLIB_WRAPPER_ENABLE

#include "RadioLib_wrapper.h"



// Flags for radio interrupt functions
volatile bool action_done = true;

/*
If compiling for ESP boards, specify that these function are used within interrupt routine
and such should be stored in the RAM and not the flash memory
*/
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void RadioLib_interrupts::set_action_done_flag(void)
{
    action_done = true;
}

template <typename T>
RadioLib_Wrapper<T>::RadioLib_Wrapper(void (*error_function)(String), int check_sum_length, String sensor_name) : Sensor_Wrapper(sensor_name, error_function)
{
    // setup default variables
    // Save the name of the radio type and set error function
    _action_status_code = RADIOLIB_ERR_NONE;
    _action_type = Action_Type::Standby;
}

template <typename T>
bool RadioLib_Wrapper<T>::begin(Radio_Config radio_config)
{
    // Set the used frequency to the inital one
    _used_frequency = radio_config.frequency;
    if (radio_config.frequency_correction)
    {
        _frequency_correction_enabled = true;
    }
    else
    {
        _frequency_correction_enabled = false;
    }
    // Create new LoRa object  !!!! CURRENTLY WILL CAUSE A 4BYTE memory leak
    // Based on chip family the DIO0 or DIO1 gets sets set as IRQ
    if (radio_config.family == Radio_Config::Chip_Family::Sx126x || radio_config.family == Radio_Config::Chip_Family::Sx128x)
        radio = new Module(radio_config.cs, radio_config.dio1, radio_config.reset, radio_config.dio0, *(radio_config.spi_bus));
    else
    {
        radio = new Module(radio_config.cs, radio_config.dio0, radio_config.reset, radio_config.dio1, *(radio_config.spi_bus));
    }

    // Try to initialize communication with LoRa
    _action_status_code = radio.begin();

    // If initialization failed, print error
    if (_action_status_code != RADIOLIB_ERR_NONE)
    {
        error("Initialization failed with status code: " + String(_action_status_code));
        return false;
    }
    // Set interrupt behaviour
    radio.setPacketReceivedAction(RadioLib_interrupts::set_action_done_flag);
    _action_type = Action_Type::Standby;

    if (configure_radio(radio_config) == false)
    {
        error("Radio begin failed!");
        return false;
    }

    // Set that radio has been initialized
    set_initialized(true);
    return true;
}

template <typename T>
bool RadioLib_Wrapper<T>::configure_radio(Radio_Config radio_config)
{
    if (radio.setFrequency(radio_config.frequency) == RADIOLIB_ERR_INVALID_FREQUENCY)
    {
        error("Frequency is invalid: " + String(radio_config.frequency));
        return false;
    };

    if (radio.setOutputPower(radio_config.tx_power) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
    {
        error("Transmit power is invalid: " + String(radio_config.tx_power));
        return false;
    };

    if (radio.setSpreadingFactor(radio_config.spreading) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
    {
        error("Spreading factor is invalid: " + String(radio_config.spreading));
        return false;
    };

    if (radio.setCodingRate(radio_config.coding_rate) == RADIOLIB_ERR_INVALID_CODING_RATE)
    {
        error("Coding rate is invalid: " + String(radio_config.coding_rate));
        return false;
    };

    if (radio.setBandwidth(radio_config.signal_bw) == RADIOLIB_ERR_INVALID_BANDWIDTH)
    {
        error("Signal bandwidth is invalid: " + String(radio_config.signal_bw));
        return false;
    };

    if (radio.setSyncWord(radio_config.sync_word) == RADIOLIB_ERR_INVALID_SYNC_WORD)
    {
        error("Sync word is invalid: " + String(radio_config.sync_word));
        return false;
    };

    if (radio_config.rf_switching == Radio_Config::Rf_Switching::Gpio)
    {
        if (bool state = configure_tx_rx_switching(radio_config.rx_enable, radio_config.tx_enable) != true)
        {
            error("RF switching setup is invalid: GPIO");
            return false;
        }
    }
    else if (radio_config.rf_switching == Radio_Config::Rf_Switching::Dio2)
    {
        if (bool state = configure_tx_rx_switching() != true)
        {
            error("RF switching setup is invalid: DIO2");
            return false;
        }
    }

    return true;
}

// There should be a way to implement this better without copying for each module, but i dont know how. The called functions are a a part of class sx126x that both module inherit
// general implementation
template <typename T>
bool RadioLib_Wrapper<T>::configure_tx_rx_switching()
{
    return false;
}
template <typename T>
bool RadioLib_Wrapper<T>::configure_tx_rx_switching(int rx_enable, int tx_enable)
{
    return false;
}
//  SX1268 implementation
template <>
bool RadioLib_Wrapper<SX1268>::configure_tx_rx_switching()
{
    if (radio.setDio2AsRfSwitch(true) != RADIOLIB_ERR_NONE)
    {
        return false;
    }
    return true;
}
template <>
bool RadioLib_Wrapper<SX1268>::configure_tx_rx_switching(int rx_enable, int tx_enable)
{
    radio.setRfSwitchPins(rx_enable, tx_enable);
    return true;
}
// SX1262 implementation
template <>
bool RadioLib_Wrapper<SX1262>::configure_tx_rx_switching()
{
    if (radio.setDio2AsRfSwitch(true) != RADIOLIB_ERR_NONE)
    {
        return false;
    }
    return true;
}
template <>
bool RadioLib_Wrapper<SX1262>::configure_tx_rx_switching(int rx_enable, int tx_enable)
{
    radio.setRfSwitchPins(rx_enable, tx_enable);
    return true;
}

template <typename T>
bool RadioLib_Wrapper<T>::transmit(String msg)
{
    if (!get_initialized())
    {
        return false;
    }

    // if radio did something that is not sending data before and it hasn't timedout. Time it out
    if (!action_done && _action_type != Action_Type::Transmit)
    {
        action_done = true;
    }

    // If already transmitting, don't continue
    if (action_done == false)
    {
        return false;
    }
    else
    {
        // else reset flag
        action_done = false;
    }

    // Clean up from the previous time
    radio.finishTransmit();

    // Start transmitting
    _action_status_code = radio.startTransmit(msg);

    // If transmit failed, print error
    if (_action_status_code != RADIOLIB_ERR_NONE)
    {
        error(" Starting transmit failed with status code:" + String(_action_status_code));
        return false;
    }
    // set last action to transmit
    _action_type = Action_Type::Transmit;

    return true;
}

// Listen to messages over LoRa. Returns true if received successfully
template <typename T>
bool RadioLib_Wrapper<T>::receive(String &msg, float &rssi, float &snr, double &frequency)
{
    if (!get_initialized())
    {
        return false;
    }

    // If already doing something, don't continue
    if (action_done == false)
    {
        return false;
    }
    else
    {
        // else reset flag
        action_done = false;
    }
    // Put into standby to try reading data
    radio.standby();
    if (_action_type == Action_Type::Receive)
    {
        // Try to read received data
        String str = "";
        _action_status_code = radio.readData(str);

        if (_action_status_code != RADIOLIB_ERR_NONE)
        {
            error("Receiving failed with status code: " + String(_action_status_code));
        }

        msg = str;
        rssi = radio.getRSSI();
        snr = radio.getSNR();
        frequency = _used_frequency;

        if (_frequency_correction_enabled)
        {
            // Frequency correction
            double freq_error = radio.getFrequencyError() / 1000000.0;
            double new_freq = _used_frequency - freq_error;
            // Serial.println("Freq error: " + String(freq_error, 10) + " | Old freq: " + String(used_frequency, 10) + " | New freq: " + String(new_freq, 10));
            if (radio.setFrequency(new_freq) != RADIOLIB_ERR_INVALID_FREQUENCY)
            {
                _used_frequency = new_freq;
                frequency = new_freq;
            }
        }
    }
    // Restart receiving TODO add error check for start recieve
    radio.startReceive();
    _action_type = Action_Type::Receive;
    // If haven't recieved anything return false;
    if (msg == "")
    {
        return false;
    }
    return true;
}
// used for CRC16 checksum
template <typename T>
uint16_t RadioLib_Wrapper<T>::crc_xmodem_update(uint16_t crc, uint8_t data)
{
    int i;
    crc = crc ^ ((uint16_t)data << 8);
    for (i = 0; i < 8; i++)
    {
        if (crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
    }
    return crc;
}

template <typename T>
uint16_t RadioLib_Wrapper<T>::calculate_CRC16_CCITT_checksum(const String &msg)
{
    size_t i;
    uint16_t crc;
    uint8_t c;

    crc = 0xFFFF;

    // Calculate checksum
    for (i = 0; i < msg.length(); i++)
    {
        c = msg.charAt(i);
        crc = crc_xmodem_update(crc, c);
    }

    return crc;
}

template <typename T>
void RadioLib_Wrapper<T>::add_checksum(String &msg)
{
    int crc_index_start = 0;
    if (msg.charAt(0) == '$' && msg.charAt(0) == '$')
    {
        crc_index_start = 2;
    }

    msg += "*" + String(calculate_CRC16_CCITT_checksum(msg.substring(crc_index_start)), HEX) + '\n';
}
template <typename T>
bool RadioLib_Wrapper<T>::check_checksum(String &msg)
{
    int crc_index_start = 0;
    if (msg.charAt(0) == '$' && msg.charAt(0) == '$')
    {
        crc_index_start = 2;
    }

    int crc_index_end = 0;
    for (int i = msg.length(); i > msg.length() - 6;)
    {
        if (msg.charAt(i) == '*')
        {
            crc_index_end = i;
            break;
        }
    }
    // no crc found
    if (crc_index_end = 0)
    {
        return false;
    }

    // Extract the provided checksum from the message
    String provided_checksum = msg.substring(msg.length() - crc_index_end, msg.length());

    // Extract the original content of the message (excluding the checksum)
    String original_msg = msg.substring(crc_index_start, msg.length() - crc_index_end);

    String calculated_checksum = String(calculate_CRC16_CCITT_checksum(original_msg), HEX); // Calculate checksum from the original message

    // Compare the calculated checksum with the provided checksum
    if (calculated_checksum.equals(provided_checksum))
    {
        msg = original_msg; // Remove the checksum from the original message
        return true;        // Checksum verified
    }
    else
    {
        return false; // Checksum couldn't be verified
    }
}

template <typename T>
bool RadioLib_Wrapper<T>::test_transmit()
{
    String msg = get_sensor_name() + " Transmission test";

    // Try to transmit the test message
    if (radio.transmit(msg))
    {
        error("Test transmission failed. Setting radio as not initialized!");
        set_initialized(false);
        return false;
    }
    return true;
}

#endif // RADIOLIB_WRAPPER_ENABLE
