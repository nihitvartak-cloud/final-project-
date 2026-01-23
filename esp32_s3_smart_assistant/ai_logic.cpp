#include "ai_logic.hpp"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>
#include <cctype>

static const char* TAG = "AILogic";

AILogic::AILogic()
    : current_state(AIState::IDLE),
      current_mode(AIMode::ASSISTANT),
      confidence_level(0.0f),
      processing_time_ms(0) {
}

esp_err_t AILogic::init() {
    ESP_LOGI(TAG, "Initializing AI Logic");

    current_state = AIState::IDLE;
    current_mode = AIMode::ASSISTANT;
    confidence_level = 0.0f;

    ESP_LOGI(TAG, "AI Logic initialized - State: %s, Mode: %s",
             get_state_string().c_str(), get_mode_string().c_str());

    return ESP_OK;
}

esp_err_t AILogic::activate_listening() {
    if (!is_idle()) {
        ESP_LOGW(TAG, "AI is already busy (State: %s)", get_state_string().c_str());
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Activating listening mode");
    current_state = AIState::LISTENING;

    return ESP_OK;
}

esp_err_t AILogic::process_input(const std::string& input) {
    if (current_state != AIState::LISTENING) {
        ESP_LOGW(TAG, "AI is not in listening state");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Processing input: %s", input.c_str());
    last_input = input;

    // Transition to thinking state
    current_state = AIState::THINKING;

    // Simulate thinking
    simulate_thinking(input);

    // Generate response
    std::string response = generate_response(input);
    last_response = response;

    ESP_LOGI(TAG, "Generated response: %s", response.c_str());

    return ESP_OK;
}

std::string AILogic::generate_response(const std::string& input) {
    // Convert input to lowercase for case-insensitive matching
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::string response;

    // Process simple commands
    if (keyword_match(lower_input, "hello") || keyword_match(lower_input, "hi")) {
        response = "Hello! I'm your Smart Assistant. How can I help you?";
    } else if (keyword_match(lower_input, "time")) {
        response = "I can help you with time information. Use the Time & Date screen.";
    } else if (keyword_match(lower_input, "wifi") || keyword_match(lower_input, "network")) {
        response = "WiFi settings are available in the WiFi screen. Would you like to connect?";
    } else if (keyword_match(lower_input, "status")) {
        response = "System status is normal. All systems operating.";
    } else if (keyword_match(lower_input, "help")) {
        response = "I can help with time, WiFi, and general information. Try saying 'hello' to start.";
    } else if (keyword_match(lower_input, "thanks") || keyword_match(lower_input, "thank")) {
        response = "You're welcome! Is there anything else I can help with?";
    } else if (keyword_match(lower_input, "weather")) {
        response = "Weather information requires internet connection. Please connect to WiFi first.";
    } else {
        response = "I understood: '" + input + "'. However, I need more training for this query. Try saying 'help' for options.";
    }

    // Update confidence based on input
    update_confidence(input);

    return response;
}

esp_err_t AILogic::speak_response(const std::string& response) {
    if (current_state != AIState::THINKING) {
        ESP_LOGW(TAG, "AI is not in thinking state");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Speaking response");
    current_state = AIState::SPEAKING;

    // Simulate audio playback
    simulate_audio_playback(response);

    // Return to idle state
    current_state = AIState::IDLE;
    ESP_LOGI(TAG, "Returned to idle state");

    return ESP_OK;
}

esp_err_t AILogic::stop() {
    ESP_LOGI(TAG, "Stopping AI operation (Current state: %s)", get_state_string().c_str());

    current_state = AIState::IDLE;
    last_input.clear();
    last_response.clear();

    return ESP_OK;
}

std::string AILogic::get_state_string() const {
    switch (current_state) {
        case AIState::IDLE:
            return "Idle";
        case AIState::LISTENING:
            return "Listening";
        case AIState::THINKING:
            return "Thinking";
        case AIState::SPEAKING:
            return "Speaking";
        case AIState::ERROR:
            return "Error";
        default:
            return "Unknown";
    }
}

esp_err_t AILogic::set_mode(AIMode mode) {
    ESP_LOGI(TAG, "Changing AI mode from %s to %s",
             get_mode_string().c_str(), get_mode_string().c_str());

    current_mode = mode;
    confidence_level = 0.0f;

    return ESP_OK;
}

std::string AILogic::get_mode_string() const {
    switch (current_mode) {
        case AIMode::ASSISTANT:
            return "Assistant";
        case AIMode::CONTROL:
            return "Control";
        case AIMode::INFORMATION:
            return "Information";
        default:
            return "Unknown";
    }
}

esp_err_t AILogic::deinit() {
    ESP_LOGI(TAG, "Deinitializing AI Logic");

    current_state = AIState::IDLE;
    last_input.clear();
    last_response.clear();
    confidence_level = 0.0f;

    return ESP_OK;
}

void AILogic::simulate_thinking(const std::string& input) {
    // Simulate thinking time based on input length
    uint32_t think_time = 500 + (input.length() * 10); // 500ms + 10ms per character
    think_time = (think_time > 2000) ? 2000 : think_time; // Max 2 seconds

    ESP_LOGD(TAG, "Simulating thinking for %d ms", think_time);
    vTaskDelay(pdMS_TO_TICKS(think_time));

    processing_time_ms = think_time;
}

void AILogic::simulate_audio_playback(const std::string& text) {
    // Simulate audio playback time based on text length
    // Assuming ~150 words per minute for spoken speech
    uint32_t speak_time = (text.length() / 5) * 400; // ~400ms per word
    speak_time = (speak_time > 5000) ? 5000 : speak_time; // Max 5 seconds

    ESP_LOGD(TAG, "Simulating audio playback for %d ms", speak_time);
    vTaskDelay(pdMS_TO_TICKS(speak_time));
}

void AILogic::update_confidence(const std::string& input) {
    // Simple confidence calculation based on input characteristics
    if (input.empty()) {
        confidence_level = 0.0f;
        return;
    }

    confidence_level = 0.5f; // Base confidence

    // Increase confidence for recognized keywords
    if (keyword_match(input, "hello") || keyword_match(input, "help")) {
        confidence_level = 0.95f;
    } else if (keyword_match(input, "time") || keyword_match(input, "wifi")) {
        confidence_level = 0.85f;
    } else if (input.length() > 20) {
        confidence_level = 0.60f;
    }

    confidence_level = (confidence_level > 1.0f) ? 1.0f : confidence_level;
    ESP_LOGD(TAG, "Confidence level: %.2f", confidence_level);
}

bool AILogic::keyword_match(const std::string& input, const std::string& keyword) const {
    // Simple keyword matching (case-insensitive substring search)
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::string lower_keyword = keyword;
    std::transform(lower_keyword.begin(), lower_keyword.end(), lower_keyword.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return lower_input.find(lower_keyword) != std::string::npos;
}

std::string AILogic::process_simple_command(const std::string& input) {
    // This function can be expanded for more complex command processing
    return generate_response(input);
}
