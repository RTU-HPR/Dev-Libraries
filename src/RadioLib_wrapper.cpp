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
String RadioLib_Wrapper<T>::type_name()
{
    String s = __PRETTY_FUNCTION__;
    int start = s.indexOf("[with T = ") + 10;
    int stop = s.lastIndexOf(']');
    return s.substring(start, stop);
}

template <typename T>
RadioLib_Wrapper<T>::RadioLib_Wrapper(void (*error_function)(String), int check_sum_length)
{
    // setup default variables
    _check_sum_length = check_sum_length; // maximum check sum value for 255 byte msg is 65536 -> 5digits
    set_error_output_function(error_function);
    // Save the name of the radio type
    radio_typename = type_name();
}

template <typename T>
bool RadioLib_Wrapper<T>::begin(Radio_Config radio_config)
{   
    // Set the used frequency to the inital one
    used_frequency = radio_config.frequency;
    // Create new LoRa object  !!!! CURRENTLY WILL CAUSE A 4BYTE memory leak
    // Based on chip family the DIO0 or DIO1 gets sets set as IRQ
    if (radio_config.family == Radio_Config::Chip_Family::Sx126x || radio_config.family == Radio_Config::Chip_Family::Sx128x)
        radio = new Module(radio_config.cs, radio_config.dio1, radio_config.reset, radio_config.dio0, *(radio_config.spi_bus));
    else
    {
        radio = new Module(radio_config.cs, radio_config.dio0, radio_config.reset, radio_config.dio1, *(radio_config.spi_bus));
    }

    // Try to initialize communication with LoRa
    state.action_status_code = radio.begin();

    // If initialization failed, print error
    if (state.action_status_code != RADIOLIB_ERR_NONE)
    {
        error("Initialization failed with status code: " + String(state.action_status_code));
        return false;
    }
    // Set interrupt behaviour
    radio.setPacketReceivedAction(RadioLib_interrupts::set_action_done_flag);
    state.action_type = State::Action_Type::Standby;

    if (configure_radio(radio_config) == false)
    {
        error("Radio begin failed!");
        return false;
    }

    // Set that radio has been initialized
    state.initialized = true;
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
        if (bool state = configure_tx_rx_switching(radio_config.rx_enable, radio_config.tx_enable != true))
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
template <typename T>
void RadioLib_Wrapper<T>::set_error_output_function(void (*error_function)(String))
{
    _error_function = error_function;
}

template <typename T>
void RadioLib_Wrapper<T>::error(String error_msg)
{
    error_msg = "RadioLib " + radio_typename + " Error: " + error_msg;

    if (_error_function == nullptr)
    {
        Serial.println(error_msg);
    }
    else
    {
        _error_function(error_msg);
    }
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
bool RadioLib_Wrapper<T>::status()
{
    return state.initialized;
}

template <typename T>
bool RadioLib_Wrapper<T>::transmit(String msg)
{
    if (!state.initialized)
    {
        return false;
    }

    // if radio did something that is not sending data before and it hasn't timedout. Time it out
    if (!action_done && state.action_type != State::Action_Type::Transmit)
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
    state.action_status_code = radio.startTransmit(msg);

    // If transmit failed, print error
    if (state.action_status_code != RADIOLIB_ERR_NONE)
    {
        error(" Starting transmit failed with status code:" + String(state.action_status_code));
        return false;
    }
    // set last action to transmit
    state.action_type = State::Action_Type::Transmit;

    return true;
}

// Listen to messages over LoRa. Returns true if received successfully
template <typename T>
bool RadioLib_Wrapper<T>::receive(String &msg, float &rssi, float &snr)
{
    if (!state.initialized)
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
    if (state.action_type == State::Action_Type::Receive)
    {
        // Try to read received data
        String str = "";
        state.action_status_code = radio.readData(str);

        if (state.action_status_code != RADIOLIB_ERR_NONE)
        {
            error("Receiving failed with status code: " + String(state.action_status_code));
        }

        msg = str;
        rssi = radio.getRSSI();
        snr = radio.getSNR();
        
        // Frequency correction
        double freq_error = radio.getFrequencyError() / 1000000.0;
        double new_freq = used_frequency - freq_error;
        // Serial.println("Freq error: " + String(freq_error, 10) + " | Old freq: " + String(used_frequency, 10) + " | New freq: " + String(new_freq, 10));
        used_frequency = new_freq;
        radio.setFrequency(new_freq);
    }
    // Restart receiving TODO add error check for start recieve
    radio.startReceive();
    state.action_type = State::Action_Type::Receive;
    // If haven't recieved anything return false;
    if (msg == "")
    {
        return false;
    }
    return true;
}
template <typename T>
void RadioLib_Wrapper<T>::add_checksum(String &msg)
{
    int sum = 0;

    // Calculate the sum of individual character values in the message
    for (size_t i = 0; i < msg.length(); i++)
    {
        sum += msg.charAt(i);
    }

    // Convert the sum to a string with a fixed length of check_sum_length
    String checksum = String(sum);
    while (checksum.length() < _check_sum_length)
    {
        checksum = "0" + checksum; // Padding with leading zeros if needed
    }

    // Append the calculated checksum to the original message
    msg += checksum;
}
template <typename T>
bool RadioLib_Wrapper<T>::check_checksum(String &msg)
{
    // Extract the provided checksum from the message
    String provided_checksum = msg.substring(msg.length() - _check_sum_length);

    // Extract the original content of the message (excluding the checksum)
    String original_msg = msg.substring(0, msg.length() - _check_sum_length);

    int sum = 0;
    for (size_t i = 0; i < original_msg.length(); i++)
    {
        sum += original_msg.charAt(i); // Summing up individual character values
    }

    String calculated_checksum = String(sum); // Calculate checksum from the original message

    // Make sure calculated checksum length is correct
    while (calculated_checksum.length() < _check_sum_length)
    {
        calculated_checksum = "0" + calculated_checksum; // Padding with leading zeros if necessary
    }

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
    String msg = radio_typename + " Transmission test";

    // Try to transmit the test message
    if (radio.transmit(msg))
    {
        error("Test transmission failed. Setting radio as not initialized!");
        state.initialized = false;
        return false;
    }
    return true;
}

#endif // RADIOLIB_WRAPPER_ENABLE
