struct IPPacket {
    char VersionIHL[1];
    char TypeOfService[1];
	char Length[2];
	char ID[2];
	char FlagsFragmentOffset[2];
	char TTL[1];
	char Protocol[1];
	char HeaderChecksum[2];
	char SourceIP[4];
	char DestinationIP[4];
};

struct UDPPacket {
	char SourcePort[2];
	char DestinationPort[2];
	char Length[2];
	char Checksum[2];
};

