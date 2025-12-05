// IFakeCamService.aidl
package com.fakecam.controller;

/**
 * AIDL interface for IPC communication with the Magisk FakeCam module.
 * The module should implement this interface to receive commands from the controller app.
 */
interface IFakeCamService {
    /**
     * Check if the FakeCam module is active
     * @return true if active, false otherwise
     */
    boolean isModuleActive();

    /**
     * Set the media content path for the virtual camera
     * @param path Path to the media file (video or image)
     * @return true if successful, false otherwise
     */
    boolean setMediaPath(String path);

    /**
     * Get the current media content path
     * @return Current media path or empty string if not set
     */
    String getCurrentMediaPath();

    /**
     * Enable or disable the virtual camera
     * @param enabled true to enable, false to disable
     * @return true if successful, false otherwise
     */
    boolean setEnabled(boolean enabled);

    /**
     * Check if the virtual camera is enabled
     * @return true if enabled, false otherwise
     */
    boolean isEnabled();

    /**
     * Set resolution for the virtual camera output
     * @param width Video width
     * @param height Video height
     * @return true if successful, false otherwise
     */
    boolean setResolution(int width, int height);

    /**
     * Refresh/reload the current media content
     * @return true if successful, false otherwise
     */
    boolean refresh();
}
