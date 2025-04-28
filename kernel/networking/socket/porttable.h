#pragma once
#include "socket.h"

namespace sockets {

struct PortEntry {
    uint16_t port = 0;
    StrongPtr<UDPSocket> socket{};
    PortEntry* next = nullptr;

    PortEntry(uint16_t port, StrongPtr<UDPSocket> socket) : port(port), socket(socket) {}
};

struct PortList {
    PortEntry* first = new PortEntry(0, {});
};

class PortTable {
    uint32_t SLOTS = 1000;

    PortList* table[1000];

   public:
    PortTable() {
        for (uint32_t i = 0; i < SLOTS; i++) {
            table[i] = new PortList();
        }
    }

    ~PortTable() {}

    bool bind(uint16_t port_num, StrongPtr<UDPSocket> socket) {
        Debug::printf("binding %d\n", port_num);
        if (port_num == 0) {
            return false;
        }
        uint32_t index = port_num % SLOTS;
        PortList* list = table[index];
        PortEntry* entry = new PortEntry(port_num, socket);
        entry->next = list->first->next;
        list->first->next = entry;
        return true;
    }

    bool contains(uint16_t port_num) {
        uint32_t index = port_num % SLOTS;
        PortList* list = table[index];
        PortEntry* temp = list->first;
        while (temp != nullptr) {
            if (temp->port == port_num) {
                return true;
            }
            temp = temp->next;
        }
        return false;
    }

    bool remove(uint16_t port_num) {
        if (port_num == 0) {
            return false;
        }
        uint32_t index = port_num % SLOTS;
        PortList* list = table[index];
        PortEntry* curr = list->first;
        PortEntry* prev = nullptr;
        while (curr != nullptr) {
            if (curr->port == port_num) {
                if (prev != nullptr) {
                    prev->next = curr->next;
                    delete curr;
                    return true;
                }
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    StrongPtr<UDPSocket> get(uint16_t port_num) {
        uint32_t index = port_num % SLOTS;
        PortList* list = table[index];
        PortEntry* temp = list->first;
        while (temp != nullptr) {
            if (temp->port == port_num) {
                return temp->socket;
            }
            temp = temp->next;
        }
        return StrongPtr<UDPSocket>{nullptr};
    }
};

extern PortTable port_table;
extern  uint16_t pick_port();



}  // namespace sockets