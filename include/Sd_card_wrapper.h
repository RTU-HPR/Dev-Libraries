#pragma once
#ifdef SD_CARD_WRAPPER_ENABLE

#include <Arduino.h>
#include <SPI.h>
#ifdef ARDUINO_ARCH_RP2040
#include <SDFS.h>
#else // ESP32
#include <SD.h>
// #include "esp_vfs_fat.h"
#endif
#include "Sensor_wrapper.h"

class SD_Card_Wrapper : public Sensor_Wrapper
{
public:
    struct Config
    {
        // spi bus
        SPIClass *spi_bus; // bus must be started before passing it to this class
        int cs_pin;
        // file name bases
        String data_file_path_base;
        String info_file_path_base;
        String error_file_path_base;
        String config_file_path;
        // if new file created will print the header in there
        String data_file_header;
        String info_file_header;
        String error_file_header;
        String config_file_header;
    };

private:
    // SD card object
#ifdef ARDUINO_ARCH_RP2040
    FS *_flash;
#else // ESP32
    SDFS *_flash;
#endif

    String _data_file_path;
    String _info_file_path;
    String _error_file_path;
    String _config_file_path;

    bool init_flash_files(const Config &config);
    bool write_to_file(const String &msg, const String &file_path);
    // ignores header
    bool read_last_line_from_file(String &msg, const String &file_path);
    bool delete_all_files(String dir_name);

public:
    SD_Card_Wrapper(void (*error_function)(String) = nullptr, String component_name = "SD Card");
    ~SD_Card_Wrapper();
    /**
     * @brief Will setup and get all the necessary file names. After this you might want to update the set output error function.
     *
     * @param config
     * @return true init good
     * @return false init bad
     */
    bool init(const Config &config);
    bool clean_storage(const Config &config);

    // true if write good
    bool write_data(const String &msg);
    bool write_info(const String &msg);
    bool write_error(const String &msg);
    // will delete the previous config file and setup a new one
    bool write_config(const String &msg, const Config &config);

    // true if read good
    bool read_data(String &data);
    bool read_info(String &info);
    bool read_error(String &error);
    bool read_config(String &config);
};
#endif // SD_CARD_WRAPPER_ENABLE