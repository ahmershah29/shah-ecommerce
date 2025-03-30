#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstring>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class ChatServer {
private:
    std::map<std::string, std::string> responses;
    SOCKET serverSocket;
    bool running;
    

    const char* CORS_HEADERS = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n\r\n";

    void initResponses() {
        responses = {
            {"hello", "Hi! How can I help you today?"},
            {"hi", "Hello! How may I assist you?"},
            {"salam", "Walaikum Asalam! How may I help you?"},
            {"bye", "Thank you for chatting. Have a great day!"},
            {"track order", "Please provide your order number and I'll help you track it."},
            {"shipping", "We typically deliver within 3-5 business days across Pakistan."},
            {"payment", "We accept Cash on Delivery, Bank Transfer, and Credit/Debit cards."},
            {"contact", "You can reach us at +92 331-4413 874 or email at ahmarshah170@gmail.com"},
            {"price", "Which product's price would you like to know?"},
            {"refund", "Refunds are processed within 7 days of return approval."},
            {"return", "You can return items within 7 days of delivery."},
            {"exchange", "Exchanges are free within 7 days of delivery."},
            {"help", "I'm here to help! What do you need assistance with?"},
            {"thanks", "You're welcome! Let me know if you need anything else."},
            {"thank you", "You're welcome! Is there anything else I can help with?"},
            
            // Quick action button responses
            {"📦 track my order", "To track your order, please provide your order number and I'll check its status immediately."},
            {"💳 payment & refunds", "We offer multiple payment options including COD, credit/debit cards, and bank transfers. For refunds, once approved, the amount is credited back within 7 working days."},
            {"🚚 shipping & delivery", "We deliver across Pakistan within 3-5 business days. Free shipping is available on orders above PKR 3,000."},
            {"🔄 returns & exchanges", "You can return or exchange products within 7 days of delivery. Please ensure items are in original condition with tags attached."},
            {"🛒 order assistance", "I can help with placing orders, modifying existing orders, or answering questions about our products. What do you need help with?"},
            {"👤 account & login help", "I can help you with account registration, password recovery, or updating your profile information. What seems to be the issue?"},
            {"📧 contact via email", "You can reach our support team at ahmarshah170@gmail.com. We typically respond within 24 hours."},
            {"📱 contact via whatsapp", "You can message us on WhatsApp at +92 331-4413 874 for instant support during business hours (9 AM - 6 PM PKT)."}
        };
    }

    std::string toLower(std::string text) {
        std::transform(text.begin(), text.end(), text.begin(), ::tolower);
        return text;
    }

    void showTypingAnimation() {
        std::cout << "Bot is typing";
        for(int i = 0; i < 3; i++) {
            std::cout << ".";
            std::cout.flush();
            Sleep(500); // Using Windows Sleep instead of std::this_thread
        }
        std::cout << "\r                \r"; // Clear the typing animation
    }

public:
    ChatServer(int port) : running(false) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Socket creation failed");
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(serverSocket);
            WSACleanup();
            throw std::runtime_error("Bind failed");
        }

        initResponses();
    }

    void start() {
        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error("Listen failed");
        }

        running = true;
        std::cout << "Chat server started on port 8080\n";
        std::cout << "Ready to receive messages from live-chat.html\n";

        while (running) {
            SOCKET clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) continue;

            char buffer[4096] = {0};
            recv(clientSocket, buffer, sizeof(buffer), 0);
            
            std::string request(buffer);
            if (request.find("OPTIONS") != std::string::npos) {
                // Handle preflight CORS request
                send(clientSocket, CORS_HEADERS, strlen(CORS_HEADERS), 0);
            } else {
                // Handle actual message
                std::string message = parseHttpRequest(request);
                std::cout << "Received message: " << message << std::endl;
                
                // Simulate typing delay for more natural interaction
                Sleep(1000); // Using Windows Sleep instead of std::this_thread
                
                std::string response = generateResponse(message);
                std::cout << "Sending response: " << response << std::endl;
                
                std::string fullResponse = std::string(CORS_HEADERS) + response;
                send(clientSocket, fullResponse.c_str(), fullResponse.length(), 0);
            }

            closesocket(clientSocket);
        }
    }

    std::string parseHttpRequest(const std::string& request) {
        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            return request.substr(bodyStart + 4);
        }
        return request;
    }

    std::string generateResponse(const std::string& message) {
        // Special case for server status check
        if (message == "ping") {
            return "pong";
        }
        
        std::string lowerMessage = toLower(message);
        
        // First try exact matches
        for (const auto& pair : responses) {
            if (lowerMessage == pair.first) {
                return pair.second;
            }
        }
        
        // Then try partial matches
        for (const auto& pair : responses) {
            if (lowerMessage.find(pair.first) != std::string::npos) {
                return pair.second;
            }
        }

        // Handle common queries with keyword detection
        if (lowerMessage.find("cost") != std::string::npos || 
            lowerMessage.find("price") != std::string::npos) {
            return "Please let me know which product you're interested in.";
        }
        
        if (lowerMessage.find("delivery") != std::string::npos || 
            lowerMessage.find("shipping") != std::string::npos) {
            return "We deliver within 3-5 business days across Pakistan.";
        }

        if (lowerMessage.find("order") != std::string::npos) {
            return "For order related queries, please provide your order number.";
        }

        if (lowerMessage.find("discount") != std::string::npos || 
            lowerMessage.find("coupon") != std::string::npos || 
            lowerMessage.find("promo") != std::string::npos) {
            return "You can use code 'WELCOME10' for 10% off on your first order!";
        }

        if (lowerMessage.find("product") != std::string::npos || 
            lowerMessage.find("item") != std::string::npos || 
            lowerMessage.find("stock") != std::string::npos) {
            return "We have a wide range of products. Please specify what you're looking for or browse our categories.";
        }

        // Default fallback response
        return "I'm not sure about that. Would you like to:\n"
               "1. Track an order\n"
               "2. Know about shipping\n"
               "3. Ask about payments\n"
               "4. Contact customer service\n"
               "Please type your choice or question.";
    }

    ~ChatServer() {
        if (running) {
            closesocket(serverSocket);
            WSACleanup();
        }
    }
};

int main() {
    try {
        ChatServer server(8080);
        std::cout << "Starting chat server. Press Ctrl+C to stop.\n";
        server.start();
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
