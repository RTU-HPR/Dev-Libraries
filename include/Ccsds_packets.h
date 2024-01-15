#pragma once
#ifdef CCSDS_PACKETS_ENABLE

// Union for converting between integer/float and byte array
union Converter
{
  uint32_t i32;
  uint16_t i16;
  uint8_t i8;
  float f;
  byte b[4];
};

// FUNCTION DECLARATIONS
/**
 * @brief Create a CCSDS primary header
 * @param apid Application ID
 * @param sequence_count Sequence count
 * @param data_length Length of data in packet
 * @return Pointer to primary header byte array
 * @note The primary header must be deleted after use
*/
byte *create_ccsds_primary_header(uint16_t apid, uint16_t sequence_count, uint16_t data_length);

/**
 * @brief Create a CCSDS secondary header
 * @param gps_epoch_time GPS epoch time
 * @param subseconds Subseconds
 * @return Pointer to secondary header byte array
 * @note The secondary header must be deleted after use
*/
byte *create_ccsds_secondary_header(uint32_t gps_epoch_time, uint16_t subseconds);

/**
 * @brief Create a CCSDS telemetry packet
 * @param apid Application ID
 * @param sequence_count Sequence count
 * @param gps_epoch_time GPS epoch time
 * @param subseconds Subseconds (0-65535 fraction of a second)
 * @param data String of comma seperated values to be converted to byte array
 * @param ccsds_packet_length Length of CCSDS packet
 * @return Pointer to CCSDS packet byte array
 * @note The CCSDS packet must be deleted after use
*/
byte *create_ccsds_packet(uint16_t apid, uint16_t sequence_count, uint32_t gps_epoch_time, uint16_t subseconds, String data, uint16_t &ccsds_packet_length);

/**
 * @brief Parse a CCSDS packet, extracting the primary header, secondary header, and data
 * @param packet Pointer to CCSDS packet byte array
 * @param apid Application ID
 * @param sequence_count Sequence count
 * @param gps_epoch_time GPS epoch time
 * @param subseconds Subseconds
 * @param ccsds_data Pointer to data byte array
 * @param data_length Length of data in packet
 * @note The data must be deleted after use
*/
void parse_ccsds(byte *packet, uint16_t &apid, uint16_t &sequence_count, uint32_t &gps_epoch_time, uint16_t &subseconds, byte *&ccsds_data, uint16_t &data_length);

/**
 * @brief Read a CCSDS packet data, using the data format to convert the data to the correct data type
 * @param ccsds_data Pointer to data byte array
 * @param data_values Pointer to data values array
 * @param data_format String of comma seperated data types. Example: "float,uint8,uint16,uint32"
*/
void extract_ccsds_data_values(byte *ccsds_data, Converter *data_values, String data_format);

/**
 * @brief Read a CCSDS telemetry packet data, extract the position data 
 * @param ccsds_data Pointer to data byte array
 * @param latitude Latitude
 * @param longitude Longitude
 * @param altitude Altitude
*/
void read_position_from_ccsds_telemetry(uint8_t *&ccsds_data, float &latitude, float &longitude, float &altitude);


// FUNCTION DEFINITIONS
byte *create_ccsds_primary_header(uint16_t apid, uint16_t sequence_count, uint16_t data_length)
{
  // Packet version number - 3 bits total
  byte PACKET_VERSION_NUMBER = 0;

  // Packet identification field - 13 bits total
  // Packet type (0 is telemetry, 1 is telecommand)- 1 bit
  // Secondary header flag (Always 1, as we will use it) - 1 bit
  // APID (Application Process Identifier) - 11 bits
  byte PACKET_TYPE = 0;
  byte SECONDARY_HEADER_FLAG = 1;
  int apid_binary = apid & 0x7FF; // keep only 11 bits
  int packet_identification_field = (PACKET_TYPE << 12) | (SECONDARY_HEADER_FLAG << 11) | apid_binary;

  // Packet Sequence Control - 16 bits total
  // Sequence flags (Always 11, as we are sending a single undivided packet) - 2 bits
  // Packet Sequence Count (Packet index)- 14 bits
  byte PACKET_SEQUENCE_FLAG = 3;                       // binary 11 is 3 in decimal
  int packet_sequence_count = sequence_count & 0x3FFF; // keep only 14 bits
  int packet_sequence_control = (PACKET_SEQUENCE_FLAG << 14) | packet_sequence_count;

  // Packet data length - 16 bits total
  // 16-bit field contains a length count that equals one fewer than the length of the data field
  byte *primary_header = new byte[6];
  primary_header[0] = (PACKET_VERSION_NUMBER << 5) | ((packet_identification_field >> 8) & 0x1F);
  primary_header[1] = packet_identification_field & 0xFF;
  primary_header[2] = (packet_sequence_control >> 8) & 0xFF;
  primary_header[3] = packet_sequence_control & 0xFF;
  primary_header[4] = (data_length >> 8) & 0xFF;
  primary_header[5] = data_length & 0xFF;

  return primary_header;
}

byte *create_ccsds_secondary_header(uint32_t gps_epoch_time, uint16_t subseconds)
{
  // GPS epoch time - 4 bytes
  // Subseconds - 2 bytes

  // Create the secondary header
  byte *secondary_header = new byte[6];
  secondary_header[0] = (gps_epoch_time >> 24) & 0xFF;
  secondary_header[1] = (gps_epoch_time >> 16) & 0xFF;
  secondary_header[2] = (gps_epoch_time >> 8) & 0xFF;
  secondary_header[3] = gps_epoch_time & 0xFF;
  secondary_header[4] = (subseconds >> 8) & 0xFF;
  secondary_header[5] = subseconds & 0xFF;

  return secondary_header;
}

// Function to create a CCSDS packet
byte *create_ccsds_packet(uint16_t apid, uint16_t sequence_count, uint32_t gps_epoch_time, uint16_t subseconds, String data, uint16_t &ccsds_packet_length)
{
  // Convert each value in a string to corresponding data type and convert to byte array
  byte packet_data[244]; // LoRa max packet length is 256 bytes, but taking out 12 bytes for primary and secondary headers (256-12=244)

  uint16_t packet_data_length = 0;
  Converter converter;

  uint16_t start = 0;
  while (start < data.length())
  {
    // Find next comma
    int end = data.indexOf(',', start);
    if (end == -1)
    {
      end = data.length();
    }

    // Get value
    String value = data.substring(start, end);
    // Convert value to byte array
    if (value.indexOf('.') != -1) // Float (Simple, but not very robust)
    {
      converter.f = value.toFloat();
    }
    else if (value.toInt() != 0 || value == "0") // Integer
    {
      converter.i32 = value.toInt();
    }
    else // String
    {
      value += "\n"; // Add newline character to end of string
      for (int i = 0; i < value.length(); i++)
      {
        packet_data[packet_data_length++] = value[i]; // Add each character to packet
      }
      // Move on to next value
      start = end + 1;
      continue;
    }

    // Add value to packet as byte array
    for (int i = 3; i >= 0; i--)
    {
      packet_data[packet_data_length++] = converter.b[i]; // MSB first
    }
    start = end + 1;
  }

  // Create full packet
  byte *primary_header = create_ccsds_primary_header(apid, sequence_count, packet_data_length);
  byte *secondary_header = create_ccsds_secondary_header(gps_epoch_time, subseconds);
  byte *packet = new byte[12 + packet_data_length];
  memcpy(packet, primary_header, 6);
  memcpy(packet + 6, secondary_header, 6);
  memcpy(packet + 12, packet_data, packet_data_length);

  ccsds_packet_length = 12 + packet_data_length;

  // Free memory
  delete[] primary_header;   // VERY IMPORTANT, otherwise a significant memory leak will occur
  delete[] secondary_header; // VERY IMPORTANT, otherwise a significant memory leak will occur

  return packet;
}

void parse_ccsds(byte *packet, uint16_t &apid, uint16_t &sequence_count, uint32_t &gps_epoch_time, uint16_t &subseconds, byte *&ccsds_data, uint16_t &data_length)
{
  // Read primary header
  byte packet_version_number = (packet[0] >> 5) & 0x07;
  byte packet_identification_field[2] = {static_cast<byte>(packet[0] & 0x1F), packet[1]};
  byte packet_sequence_control[2] = {packet[2], packet[3]};
  byte packet_data_length[2] = {packet[4], packet[5]};

  // Read secondary header
  byte gps_epoch_time_bytes[4] = {packet[6], packet[7], packet[8], packet[9]};
  byte subseconds_bytes[2] = {packet[10], packet[11]};

  // Convert bytes to integers
  apid = ((packet_identification_field[0] & 0x07) << 8) | packet_identification_field[1];
  sequence_count = ((packet_sequence_control[0] << 8) | packet_sequence_control[1]) & 0x3FFF;
  gps_epoch_time = (gps_epoch_time_bytes[0] << 24) | (gps_epoch_time_bytes[1] << 16) | (gps_epoch_time_bytes[2] << 8) | gps_epoch_time_bytes[3];
  subseconds = (subseconds_bytes[0] << 8) | subseconds_bytes[1];
  data_length = (packet_data_length[0] << 8) | packet_data_length[1];

  // Read data
  // Data starts at byte 12 until the end of the packet
  ccsds_data = new byte[data_length];
  memcpy(ccsds_data, packet + 12, data_length);
}

void extract_ccsds_data_values(byte *ccsds_data, Converter *data_values, String data_format)
{
  // The data_format is a string that contains the data types of each value in the packet
  // The data types are stored in the string in the same order as the values in the packet
  // Example: "float,uint8,uint16,uint32"
  uint16_t start = 0;
  uint16_t value_index = 0;
  uint16_t data_index = 0;
  while (start < data_format.length())
  {
    // Find next comma
    int end = data_format.indexOf(',', start);
    if (end == -1)
    {
      end = data_format.length();
    }

    // Get value
    String value = data_format.substring(start, end);

    // Read value from packet
    if (value == "float") // Float (4 bytes)
    {
      data_values[value_index].b[3] = ccsds_data[data_index++];
      data_values[value_index].b[2] = ccsds_data[data_index++];
      data_values[value_index].b[1] = ccsds_data[data_index++];
      data_values[value_index].b[0] = ccsds_data[data_index++];
    }
    else if (value == "uint8") // Unsigned 8-bit integer (1 byte)
    {
      data_values[value_index].b[0] = ccsds_data[data_index++];
    }
    else if (value == "uint16") // Unsigned 16-bit integer (2 bytes)
    {
      data_values[value_index].b[1] = ccsds_data[data_index++];
      data_values[value_index].b[0] = ccsds_data[data_index++];
    }
    else if (value == "uint32") // Unsigned 32-bit integer (4 bytes)
    {
      data_values[value_index].b[3] = ccsds_data[data_index++];
      data_values[value_index].b[2] = ccsds_data[data_index++];
      data_values[value_index].b[1] = ccsds_data[data_index++];
      data_values[value_index].b[0] = ccsds_data[data_index++];
    }
    start = end + 1;
    value_index++;
  }
}

void read_position_from_ccsds_telemetry(uint8_t *&ccsds_data, float &latitude, float &longitude, float &altitude)
{
  // Create array to store the values
  Converter data_values[3];

  // Read the values from the packet
  extract_ccsds_data_values(ccsds_data, data_values, "float,float,float");

  // Read latitude, longitude, and altitude
  latitude = data_values[0].f;
  longitude = data_values[1].f;
  altitude = data_values[2].f;
}


#endif // CCSDS_PACKETS_ENABLE
