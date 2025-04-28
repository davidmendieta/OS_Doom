case SYS_GET_MAC: {
    uint32_t user_buffer = user_sp[1];
    uint8_t mac[6];
    network_interface->get_mac_address(mac);
    memcpy((void* ) user_buffer, mac, 6);
    return 0;
}