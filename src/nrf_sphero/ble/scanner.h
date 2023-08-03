#ifndef SPHERO_SCANNER_H
#define SPHERO_SCANNER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initalize bluetooth to scan for spheros
 */
int scanner_init(void);

/**
 * @brief Start scanning for spheros
 */
int scanner_start(void);

#ifdef __cplusplus
}
#endif
#endif // SPHERO_SCANNER_H