## Run testcases:
- make NETWORK_MODE=<HOST|CLIENT> t0.test 

### Files:
- /etc/network/interfaces
    - Read by the ifupdown service
    - We don't need anything DHCP related since we manually set all the IPs (static)
- /sbin/networking/ifup
    - uses ifconfig to bring an interface up
- /sbin/networking/ifdown - uses ifconfig
    - uses ifconfig to bring an interface down


- ifupdown service:
    - uses ifconfig
- ifconfig
    - Configure IP Addresses: `ifconfig <interface> <IP_ADDRESS>` --> assign an IP to a network interface
    - Set Netmask: `netmask <NETMASK>`
    - Bring interfaces up / down: `ifconfig eth0 up`, `ifconfig eth0 down`

- No need for systemd-networkd or any other daemon since everything is static 

- Uses ARP to find the MAC address of other machine on the local link