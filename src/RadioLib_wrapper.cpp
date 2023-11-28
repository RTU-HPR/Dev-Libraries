#include "RadioLib_wrapper.h"

// Flags for radio interrupt functions
volatile bool action_done = false;

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
RadioLib_Wrapper<T>::RadioLib_Wrapper(RADIO_CONFIG radio_config)
{
    // Create new LoRa object
    // Based on chip family the DIO0 or DIO1 gets sets set as IRQ
    if (radio_config.FAMILY == RADIO_CONFIG::CHIP_FAMILY::SX126X || radio_config.FAMILY == RADIO_CONFIG::CHIP_FAMILY::SX128X)
        radio = new Module(radio_config.CS, radio_config.DIO1, radio_config.RESET, radio_config.DIO0, *(radio_config.SPI_BUS));
    else
    {
        radio = new Module(radio_config.CS, radio_config.DIO0, radio_config.RESET, radio_config.DIO1, *(radio_config.SPI_BUS));
    }
    // Save the name of the radio type
    radio_typename = type_name();

    // Try to initialize communication with LoRa
    state.action_status_code = radio.begin();

    // If initialization failed, print error
    if (state.action_status_code != RADIOLIB_ERR_NONE)
    {
        Serial.println(radio_typename + " initialization failed with status code: " + String(state.action_status_code));
        return;
    }
    // Set interrupt behaviour
    radio.setPacketReceivedAction(RadioLib_interrupts::set_action_done_flag);
    state.action_type = State::Action_Type::STANDBY;

    // Set that radio has been initialized
    state.initialized = true;
}

template <typename T>
bool RadioLib_Wrapper<T>::configure_radio(RADIO_CONFIG radio_config)
{
    if (radio.setFrequency(radio_config.FREQUENCY) == RADIOLIB_ERR_INVALID_FREQUENCY)
    {
        Serial.println(radio_typename + " Frequency is invalid: " + String(radio_config.FREQUENCY));
        return false;
    };

    if (radio.setOutputPower(radio_config.TXPOWER) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
    {
        Serial.println(radio_typename + " Transmit power is invalid: " + String(radio_config.TXPOWER));
        return false;
    };

    if (radio.setSpreadingFactor(radio_config.SPREADING) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
    {
        Serial.println(radio_typename + " Spreading factor is invalid: " + String(radio_config.SPREADING));
        return false;
    };

    if (radio.setCodingRate(radio_config.CODING_RATE) == RADIOLIB_ERR_INVALID_CODING_RATE)
    {
        Serial.println(radio_typename + " Coding rate is invalid: " + String(radio_config.CODING_RATE));
        return false;
    };

    if (radio.setBandwidth(radio_config.SIGNAL_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH)
    {
        Serial.println(radio_typename + " Signal bandwidth is invalid: " + String(radio_config.SIGNAL_BW));
        return false;
    };

    if (radio.setSyncWord(radio_config.SYNC_WORD) == RADIOLIB_ERR_INVALID_SYNC_WORD)
    {
        Serial.println(radio_typename + " Sync word is invalid: " + String(radio_config.SYNC_WORD));
        return false;
    };

    if (radio_config.rf_switching == RADIO_CONFIG::RF_SWITCHING::GPIO)
    {
        if (bool state = configure_tx_rx_switching(radio_config.RX_ENABLE, radio_config.TX_ENABLE != true))
        {
            Serial.println(radio_typename + " rf switching setup is invalid: GPIO");
            return false;
        }
    }
    else if (radio_config.rf_switching == RADIO_CONFIG::RF_SWITCHING::DIO2)
    {
        if (bool state = configure_tx_rx_switching() != true)
        {
            Serial.println(radio_typename + " rf switching setup is invalid: DIO2");
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
bool RadioLib_Wrapper<T>::configure_tx_rx_switching(int RX_ENABLE, int TX_ENABLE)
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
bool RadioLib_Wrapper<SX1268>::configure_tx_rx_switching(int RX_ENABLE, int TX_ENABLE)
{
    radio.setRfSwitchPins(RX_ENABLE, TX_ENABLE);
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
bool RadioLib_Wrapper<SX1262>::configure_tx_rx_switching(int RX_ENABLE, int TX_ENABLE)
{
    radio.setRfSwitchPins(RX_ENABLE, TX_ENABLE);
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
    if (!action_done && state.action_type != State::Action_Type::TRANSMIT)
    {
        radio.standby();
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
        Serial.println(radio_typename + " Starting transmit failed with status code:" + String(state.action_status_code));
        return false;
    }
    // set last action to transmit
    state.action_type = State::Action_Type::TRANSMIT;

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
        // finish transmit from last time
        if (state.action_type == State::Action_Type::TRANSMIT)
        {
            radio.finishTransmit();
        }
        // else reset flag
        action_done = false;
    }

    // Try to read received data
    String str = "";
    state.action_status_code = radio.readData(str);

    if (state.action_status_code != RADIOLIB_ERR_NONE)
    {
        Serial.println(radio_typename + " Receiving failed with status code: " + String(state.action_status_code));
    }

    msg = str;
    rssi = radio.getRSSI();
    snr = radio.getSNR();

    // Restart receiving
    radio.startReceive();

    // If haven't recieved anything return false;
    if (msg == "")
    {
        return false;
    }

    return true;
}

template <typename T>
bool RadioLib_Wrapper<T>::test_transmit()
{
    String msg = radio_typename + " Transmission test";

    // Try to transmit the test message
    if (radio.transmit(msg))
    {
        Serial.println(radio_typename + " Test transmission failed");
        Serial.println("Setting radio as not initialized");
        return false;
    }

    Serial.println(radio_typename + " Test transmission was successful");
    return true;
}