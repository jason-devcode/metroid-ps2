/**
 * @file fps_utils.h
 * @brief Utilities for frame control and Delta Time calculation.
 * 
 * This file provides inline helper functions to measure the time elapsed 
 * between frames (delta time), while capping its maximum value to prevent 
 * sudden spikes in physics or game simulation.
 */

#ifndef LIB_PERFORMANCE_FPS_UTILS_H
#define LIB_PERFORMANCE_FPS_UTILS_H

#include <time.h>
#include <kernel.h>

/** 
 * @brief Time elapsed between the current and previous frame (in seconds).
 */
static float deltatime = 0.0f;

/**
 * @brief Updates the global deltatime value.
 * 
 * Calculates the time difference using the clock() function, updates 
 * the record of the last measured time, and applies an upper limit of 0.1 seconds 
 * to prevent stability issues during severe frame rate drops (lags).
 */
static inline void update_deltatime()
{
    static u_long last_time = 0;

    u_long current_time = clock();
    deltatime = (float)(current_time - last_time) / CLOCKS_PER_SEC;
    last_time = current_time;

    deltatime = deltatime > 0.1f ? 0.1f : deltatime;
}

#endif // LIB_PERFORMANCE_FPS_UTILS_H