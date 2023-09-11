#ifndef DRIVE_H
#define DRIVE_H

#include "../controls/packet.hpp"
#include "../sphero.hpp"
#include "commands.hpp"

#define DRIVE_DID 22

/**
 * @brief Drive flags
 */
enum class DriveFlags : uint8_t {
    FORWARD = 0x0,
    BACKWARD = 0x1,
    TURBO = 0x2,
    FAST_TURN = 0x4,
    LEFT_DIRECTION = 0x8,
    RIGHT_DIRECTION = 0x10,
    ENABLE_DRIFT = 0x20,
};

class Drive : public Commands {
public:
    /**
     * @brief Drive the sphero
     *
     * @param[in] sphero The Sphero to send the command to
     * @param[in] speed The speed to drive at
     * @param[in] heading The heading to drive at
     * @param[in] flags The flags to drive with
     * @param[in] tid The target id for the packet (optional)
     */
    static const Packet drive(Sphero& sphero, uint8_t speed, uint16_t heading, DriveFlags flags, uint8_t tid = 0);

    /**
     * @brief Resets the heading calibration (aim) angle to use the current direction of the robot as 0°.
     * 
     * @note Resets the robots yaw angle
    */
    static const Packet reset_aim(Sphero& sphero, uint8_t tid = 0);
};

#endif