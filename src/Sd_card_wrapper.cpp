#ifdef SD_CARD_WRAPPER_ENABLE
#include <Sd_card_wrapper.h>
#include <cmath>

SD_Card_Wrapper::SD_Card_Wrapper(void (*error_function)(String), String component_name) : Sensor_Wrapper(component_name, error_function)
{
    return;
}
SD_Card_Wrapper::~SD_Card_Wrapper()
{
    if (get_initialized())
    {
        _flash->end();
    }
    return;
}
bool SD_Card_Wrapper::init(const Config &config)
{
#ifdef ARDUINO_ARCH_RP2040
    _flash = &SDFS;
    SDFSConfig sd_config;
    sd_config.setCSPin(config.cs_pin);
    sd_config.setSPI(*config.spi_bus);

    if (!_flash->setConfig(sd_config))
    {
        error("CONFIG != SET");
        return false;
    }

    // Initialize flash
    if (!_flash->begin())
    {
        error("FileSys != BEGIN");
        return false;
    }
#else // ESP32
    _flash = &SD;
    // possible issue with max_files default value - couldn't find the exact meaning of it
    if (!_flash->begin(config.cs_pin, *config.spi_bus))
    {
        error("FileSys != BEGIN");
        return false;
    }
#endif

    // has to be here for init flash to work
    set_initialized(true);

    // find flash files and print header
    if (!init_flash_files(config))
    {
        error("FilePaths != INIT");
        set_initialized(false);
        return false;
    }
    return true;
}

// Open (create) telemetry/info/error files
bool SD_Card_Wrapper::init_flash_files(const Config &config)
{
    if (!get_initialized())
    {
        return false;
    }

    // Determine file name index for final path
    int file_name_nr = 0;
    while (_flash->exists(config.data_file_path_base + String(file_name_nr) + ".csv"))
    {
        file_name_nr++;
    }

    // Get the file full path
    _data_file_path = config.data_file_path_base + String(file_name_nr) + ".csv";
    _info_file_path = config.info_file_path_base + String(file_name_nr) + ".csv";
    _error_file_path = config.error_file_path_base + String(file_name_nr) + ".csv";
    _config_file_path = config.config_file_path + ".csv"; // always the same config file path

    // If required, write the file header. When writing header delete the file if it was there before
#ifdef ARDUINO_ARCH_RP2040
    File data_file = _flash->open(_data_file_path, "w+");
#else // ESP32
    File data_file = _flash->open(_data_file_path, "w", true);
#endif
    if (!data_file)
    {
        error("Data file != OPEN");
        return false;
    }
    else
    {
        data_file.println(config.data_file_header);
        data_file.close();
    }

#ifdef ARDUINO_ARCH_RP2040
    File info_file = _flash->open(_info_file_path, "w+");
#else // ESP32
    File info_file = _flash->open(_info_file_path, "w", true);
#endif
    if (!info_file)
    {
        error("Info file != OPEN");
        return false;
    }
    else
    {
        info_file.println(config.info_file_header);
        info_file.close();
    }

#ifdef ARDUINO_ARCH_RP2040
    File error_file = _flash->open(_error_file_path, "w+");
#else // ESP32
    File error_file = _flash->open(_error_file_path, "w", true);
#endif
    if (!error_file)
    {
        error("Error file != OPEN");
        return false;
    }
    else
    {
        error_file.println(config.error_file_header);
        error_file.close();
    }

    bool config_header_required = true;
    if (_flash->exists(_config_file_path))
    {
        config_header_required = false;
    }

#ifdef ARDUINO_ARCH_RP2040
    File config_file = _flash->open(_config_file_path, "a+");
#else // ESP32
    File config_file = _flash->open(_config_file_path, "a", true);
#endif
    if (!config_file)
    {
        error("Config file != OPEN");
        return false;
    }
    else
    {
        if (config_header_required)
        {
            config_file.println(config.config_file_header);
            config_file.close();
        }
    }
    return true;
}

bool SD_Card_Wrapper::delete_all_files(String dir_name)
{
    if (!get_initialized())
    {
        return false;
    }
#ifdef ARDUINO_ARCH_RP2040
    // not implemented on RP2040 use format...
    return false;
#else

    // Serial.println("DAF: deleting files in: " + dir_name);
    File root = _flash->open(dir_name, "r");
    if (!root)
    {
        error("DAF: File != open: " + dir_name);
        return false;
    }
    if (!root.isDirectory())
    {
        error("DAF: File != directory");
        return false;
    }

    File file = root.openNextFile();
    bool dir_clean_status = true;
    while (file)
    {
        if (file.isDirectory())
        {
            // Serial.println("DAF: going deeper into: " + String(file.path()));
            delete_all_files(String(file.path()));
        }
        else
        {
            // Serial.println("DAF: removing: " + String(file.path()));
            if (!_flash->remove(file.path()))
            {
                error("DAF: File != remove: " + String(file.path()));
            }
            if (_flash->exists(file.path()))
            {
                error("DAF: File != exists: " + String(file.path()));
                dir_clean_status = false;
            }
            else
            {
                // Serial.println("DAF: File removed: " + String(file.path()));
            }
        }
        file = root.openNextFile();
    }
    return dir_clean_status;
#endif
}

bool SD_Card_Wrapper::clean_storage(const Config &config)
{
#ifdef ARDUINO_ARCH_RP2040
    if (get_initialized())
    {
        _flash->end();
        set_initialized(false);
    }

    if (_flash->format())
    {
        if (!init(config))
        {
            error("Failed init() after format");
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        error("Flash != FORMAT");
        return false;
    }
#else // ESP32
    if (!get_initialized())
    {
        return false;
    }

    if (!delete_all_files("/"))
    {
        error("Flash != DELETE ALL FILES");
        return false;
    }
    // quite a brutal approach, better would be to just init only the files again
    _flash->end();
    if (!init(config))
    {
        error("!init() after format");
        return false;
    }
    else
    {
        return true;
    }
#endif
}

bool SD_Card_Wrapper::write_to_file(const String &msg, const String &file_path)
{
    // this function should never call an error -> might cause error_loop
    if (!get_initialized())
    {
        return false;
    }
    // write to flash
    File file = _flash->open(file_path, "a");
    if (!file)
    {
        return false;
    }
    file.println(msg);
    file.close();
    return true;
}
bool SD_Card_Wrapper::read_last_line_from_file(String &msg, const String &file_path)
{
    if (!get_initialized())
    {
        return false;
    }
    File file = _flash->open(file_path, "r");
    if (!file)
    {
        error("File != OPEN");
        return false;
    }
    if (file.size() <= 2)
    {
        file.close();
        error("File TOO SMALL");
        return false;
    }
    // long file optimized method
    unsigned long seek_location = file.size() - 2; // to ignore the last \n
    file.seek(seek_location);
    // locate last \n
    while (file.available())
    {
        char c = file.read();
        if (c == '\n')
        {
            break;
        }
        // will prevent reading the header
        if (seek_location == 0)
        {
            error("Trying to READ HEADER");
            return false;
        }
        seek_location--;
        file.seek(seek_location);
    }

    seek_location++; // move to the start of the line instead of \n
    // Read string
    file.seek(seek_location);
    msg = file.readStringUntil('\n');
    file.close();

    return true;
}

// true if write good
bool SD_Card_Wrapper::write_data(const String &msg)
{
    return write_to_file(msg, _data_file_path);
}
bool SD_Card_Wrapper::write_info(const String &msg)
{
    return write_to_file(msg, _info_file_path);
}
bool SD_Card_Wrapper::write_error(const String &msg)
{
    return write_to_file(msg, _error_file_path);
}
bool SD_Card_Wrapper::write_config(const String &msg, const Config &config)
{
    if (!get_initialized())
    {
        return false;
    }

    _flash->remove(_config_file_path);
#ifdef ARDUINO_ARCH_RP2040
    File config_file = _flash->open(_config_file_path, "a+");
#else // ESP32
    File config_file = _flash->open(_config_file_path, "a", true);
#endif
    if (!config_file)
    {
        error("Failed opening config file");
        return false;
    }
    else
    {
        config_file.println(config.config_file_header);
        config_file.println(msg);
        config_file.close();
    }
    return true;
}

// true if read good
bool SD_Card_Wrapper::read_data(String &data)
{
    return read_last_line_from_file(data, _data_file_path);
}
bool SD_Card_Wrapper::read_info(String &info)
{
    return read_last_line_from_file(info, _info_file_path);
}
bool SD_Card_Wrapper::read_error(String &error)
{
    return read_last_line_from_file(error, _error_file_path);
}
bool SD_Card_Wrapper::read_config(String &config)
{
    return read_last_line_from_file(config, _config_file_path);
}
#endif // SD_CARD_WRAPPER_ENABLE
