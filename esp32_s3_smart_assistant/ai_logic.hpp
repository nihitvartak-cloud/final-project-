#pragma once

#include <cstdint>
#include <string>
#include "esp_err.h"

/**
 * @brief AI Logic for ESP32-S3 Smart Assistant
 * 
 * Handles:
 * - AI state machine (Idle, Listening, Thinking, Speaking)
 * - Placeholder logic for AI operations
 * - Status and mode management
 */

class AILogic {
public:
    // AI states
    enum class AIState {
        IDLE = 0,       // Waiting for activation
        LISTENING = 1,  // Receiving audio input
        THINKING = 2,   // Processing request
        SPEAKING = 3,   // Playing audio response
        ERROR = 4,      // Error state
    };

    // AI modes/capabilities
    enum class AIMode {
        ASSISTANT = 0,  // General assistant
        CONTROL = 1,    // Device control
        INFORMATION = 2, // Information retrieval
    };

    /**
     * @brief Constructor
     */
    AILogic();

    /**
     * @brief Initialize AI logic
     * @return ESP_OK on success
     */
    esp_err_t init();

    /**
     * @brief Activate AI listening
     * @return ESP_OK on success
     */
    esp_err_t activate_listening();

    /**
     * @brief Process received input
     * @param input Input text/data
     * @return ESP_OK on success
     */
    esp_err_t process_input(const std::string& input);

    /**
     * @brief Generate response based on input
     * @param input User input
     * @return Response string
     */
    std::string generate_response(const std::string& input);

    /**
     * @brief Simulate speaking (audio playback)
     * @param response Response text to speak
     * @return ESP_OK on success
     */
    esp_err_t speak_response(const std::string& response);

    /**
     * @brief Stop current AI operation
     * @return ESP_OK on success
     */
    esp_err_t stop();

    /**
     * @brief Get current AI state
     * @return AIState enum value
     */
    AIState get_state() const { return current_state; }

    /**
     * @brief Get AI state as string
     * @return State name
     */
    std::string get_state_string() const;

    /**
     * @brief Check if AI is idle
     * @return true if idle, false otherwise
     */
    bool is_idle() const { return current_state == AIState::IDLE; }

    /**
     * @brief Check if AI is busy (listening/thinking/speaking)
     * @return true if busy, false otherwise
     */
    bool is_busy() const {
        return current_state == AIState::LISTENING ||
               current_state == AIState::THINKING ||
               current_state == AIState::SPEAKING;
    }

    /**
     * @brief Set AI mode
     * @param mode AIMode enum value
     * @return ESP_OK on success
     */
    esp_err_t set_mode(AIMode mode);

    /**
     * @brief Get current AI mode
     * @return AIMode enum value
     */
    AIMode get_mode() const { return current_mode; }

    /**
     * @brief Get AI mode as string
     * @return Mode name
     */
    std::string get_mode_string() const;

    /**
     * @brief Get last response
     * @return Last generated response
     */
    std::string get_last_response() const { return last_response; }

    /**
     * @brief Get confidence level of last processing
     * @return Confidence 0.0-1.0
     */
    float get_confidence() const { return confidence_level; }

    /**
     * @brief Deinitialize AI logic
     * @return ESP_OK on success
     */
    esp_err_t deinit();

private:
    AIState current_state;
    AIMode current_mode;
    std::string last_input;
    std::string last_response;
    float confidence_level;
    uint32_t processing_time_ms;

    /**
     * @brief Process simple predefined commands
     * @param input User input
     * @return Response string
     */
    std::string process_simple_command(const std::string& input);

    /**
     * @brief Simulate AI thinking
     * @param input User input
     */
    void simulate_thinking(const std::string& input);

    /**
     * @brief Simulate audio playback
     * @param text Text to speak
     */
    void simulate_audio_playback(const std::string& text);

    /**
     * @brief Update confidence based on input
     * @param input User input
     */
    void update_confidence(const std::string& input);

    /**
     * @brief Check if input matches keyword
     * @param input User input
     * @param keyword Keyword to match
     * @return true if matches
     */
    bool keyword_match(const std::string& input, const std::string& keyword) const;
};
