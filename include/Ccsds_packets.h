#pragma once
#ifdef CCSDS_PACKETS_ENABLE
#include <Arduino.h>

// Union for converting between integer/float and byte array
union Converter
{
  uint32_t i32;
  uint16_t i16;
  uint8_t i8;
  float f;
  byte b[4];
};

/**
 * @brief Calculate the CRC-16-CCITT checksum of a byte array
 * @param data Pointer to data byte array
 * @param length Length of data in packet
 * @return CRC-16-CCITT checksum
 * @note This function should not be used directly. Use add_crc_16_cciit_to_ccsds_packet() and check_crc_16_cciit_of_ccsds_packet() instead
*/
uint16_t calculate_crc_16_ccitt(const uint8_t *data, uint16_t length);

/**
 * @brief Add a CRC-16-CCITT checksum to the end of a CCSDS packet
 * @param ccsds_packet Pointer to CCSDS packet byte array
 * @param ccsds_packet_length Length of CCSDS packet
 * @note This function is already called in create_ccsds_packet(). No need to call it separately
*/
void add_crc_16_cciit_to_ccsds_packet(uint8_t *&ccsds_packet, uint16_t ccsds_packet_length);

/**
 * @brief Check the CRC-16-CCITT checksum of a CCSDS packet
 * @param ccsds_packet Pointer to CCSDS packet byte array
 * @param ccsds_packet_length Length of CCSDS packet
 * @return True if checksum is correct, false if checksum is incorrect
*/
bool check_crc_16_cciit_of_ccsds_packet(uint8_t *ccsds_packet, uint16_t &ccsds_packet_length);

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
 * @brief Create a full CCSDS telemetry packet with checksum
 * @param apid Application ID
 * @param sequence_count Sequence count
 * @param gps_epoch_time GPS epoch time
 * @param subseconds Subseconds (0-65535 fraction of a second)
 * @param data String of comma seperated values to be converted to byte array
 * @param ccsds_packet_length Length of CCSDS packet
 * @return Pointer to CCSDS packet byte array
 * @note The CCSDS packet must be deleted after use
 */
byte *create_ccsds_telemetry_packet(uint16_t apid, uint16_t sequence_count, uint32_t gps_epoch_time, uint16_t subseconds, String data, uint16_t &ccsds_packet_length);

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
void parse_ccsds_telecommand(byte *packet, uint16_t &apid, uint16_t &sequence_count, uint32_t &gps_epoch_time, uint16_t &subseconds, byte *&ccsds_data, uint16_t &data_length);

/**
 * @brief Parse a CCSDS telecommand packet, extracting the primary header, packet id and data
 * @param packet Pointer to CCSDS packet byte array
 * @param apid Application ID
 * @param sequence_count Sequence count
 * @param ccsds_data Pointer to data byte array
 * @param data_length Length of data in packet
 * @note The data must be deleted after use
 */
void parse_ccsds_telecommand(byte *packet, uint16_t &apid, uint16_t &sequence_count, uint16_t &packet_id, byte *&ccsds_data, uint16_t &data_length);

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

#endif // CCSDS_PACKETS_ENABLE
