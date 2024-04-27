#ifndef SPHERO_SCANNER
#define SPHERO_SCANNER

#include "sphero.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class SpheroScanner {
private:
    bool mInitialized = false;

public:
    SpheroScanner(std::vector<std::string> names);
    ~SpheroScanner();

    /*
     * @brief Find and connect to available spheros
     * @return 0 on success
     *         Otherwise, error code
     *
     * @note This function is async. You should poll for when all spheros have been found
     */
    int start_scanning();

    /**
     * @brief Stopping scanning for spheros
     *
     * @return 0 on success
     *         Otherwise error code
     */
    int stop_scanning();

    /** @brief Check if all spheros have been found
     *
     * @param[in] timeout How long to wait after last Sphero found in milliseconds
     * @returns True if all spheros have been found
     */
    bool found_all_spheros(uint64_t timeout = 5000);

    /**
     * @brief Get the number of spheros found
     *
     * @returns The number of spheros found
     */
    unsigned int get_num_spheros();

    /** @brief Get all spheros
     *
     * @returns spheros The connected spheros
     */
    std::vector<std::shared_ptr<Sphero>> get_spheros();
};

#endif // SPHERO_SCANNER