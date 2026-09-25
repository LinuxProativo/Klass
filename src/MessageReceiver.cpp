/**
 * @file MessageReceiver.cpp
 * @brief Receive parameters from a secondary instance and forwarding them to the primary instance.
 */

#include <Debug.hpp>
#include <MessageReceiver.hpp>

/**
 * @brief Receives arguments and emits them for use by the primary instance.
 * @param message The received arguments as a byte array.
 */
void MessageReceiver::receivedMessage(int, const QByteArray &message) {
    Debug::msg("Arguments received", "MessageReceiver", {message});
    emit arg(message);
}
