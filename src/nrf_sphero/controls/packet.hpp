
#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <vector>

uint8_t packet_chk(std::vector<unsigned char> payload);

/**
 * Packet flags
 */
enum class PacketFlags : uint8_t {
    none = 0b0,
    is_response = 0b1,
    requests_response = 0b10,
    requests_only_error_response = 0b100,
    is_activity = 0b1000,
    has_target_id = 0b10000,
    has_source_id = 0b100000,
    unused = 0b1000000,
    extended_flags = 0b10000000,
};

// Define bitwise AND operator for PacketFlags
PacketFlags operator&(PacketFlags lhs, PacketFlags rhs);

// Define bitwise OR operator for PacketFlags
PacketFlags operator|(PacketFlags lhs, PacketFlags rhs);

// Define bitwise OR and assignment operator for PacketFlags
PacketFlags& operator|=(PacketFlags& lhs, PacketFlags rhs);

/**
 * Packet Encoding
 */

enum class PacketEncoding : unsigned char {
    escape = 0xAB,
    start = 0x8D,
    end = 0xD8,
    escaped_escape = 0x23,
    escaped_start = 0x05,
    escaped_end = 0x50
};

/**
 * Packet errors
 */
enum class PacketError : unsigned char {
    success = 0x00,
    bad_device_id = 0x01,
    bad_command_id = 0x02,
    not_yet_implemented = 0x03,
    command_is_restricted = 0x04,
    bad_data_length = 0x05,
    command_failed = 0x06,
    bad_parameter_value = 0x07,
    busy = 0x08,
    bad_target_id = 0x09,
    target_unavailable = 0x0a
};

/**
 * A Sphero BLE Packet
 *
 *  Packet protocol v2, from https://sdk.sphero.com/docs/api_spec/general_api
    [SOP, FLAGS, TID (optional), SID (optional), DID, CID, SEQ, ERR (at response), DATA..., CHK, EOP]
*/
class Packet {
private:
public:
    /* Packet flags - Bit-flags that modify the behaviour of the packet*/
    PacketFlags flags;
    /* Device ID - Command group of the command being sent */
    uint8_t did;
    /* Command ID - The command to execute */
    uint8_t cid;
    /* Sequence Number - The token used to link commands with responses */
    uint8_t seq;
    /* Target ID - Address of the target, expressed as a port ID (upper nibble) and a node id (lower nibble)*/
    uint8_t tid;
    /* Source ID - Address of the source, expressed as a port ID (upper nibble) and a node id (lower nibble)*/
    uint8_t sid;
    /* Data - Zero or more bytes of message data */
    std::vector<unsigned char>* data;
    PacketError err;

    std::vector<unsigned char> build() const;
};

#endif // PACKET_H