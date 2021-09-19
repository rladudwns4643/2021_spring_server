#pragma once
#include "pch.h"

class SocketAddress {
public:
	SocketAddress(UINT inAddress, UINT inPort) {
		GetAsSockAddrIn()->sin_family = AF_INET;
		GetIP4Ref() = inAddress;
		GetAsSockAddrIn()->sin_port = htons(inPort);
	}

	SocketAddress(const sockaddr& inSockAddr) {
		memcpy(&m_sockAddr, &inSockAddr, sizeof(sockaddr));
	}

	SocketAddress() {
		GetAsSockAddrIn()->sin_family = AF_INET;
		GetIP4Ref() = INADDR_ANY;
		GetAsSockAddrIn()->sin_port = 0;
	}

	bool operator==(const SocketAddress& rhs) const {
		return (m_sockAddr.sa_family == AF_INET
			&& GetAsSockAddrIn()->sin_port == rhs.GetAsSockAddrIn()->sin_port
			&& (GetIP4Ref() == rhs.GetIP4Ref()));
	}

	int GetSize() const { return sizeof(sockaddr); }

private:
	friend class TCPSocket;

	sockaddr m_sockAddr;
	UINT& GetIP4Ref() { return *reinterpret_cast<UINT*>(&GetAsSockAddrIn()->sin_addr.S_un.S_addr); }
	const UINT& GetIP4Ref() const{ return *reinterpret_cast<const UINT*>(&GetAsSockAddrIn()->sin_addr.S_un.S_addr); }
	
	sockaddr_in* GetAsSockAddrIn() { return reinterpret_cast<sockaddr_in*>(&m_sockAddr); }
	const sockaddr_in* GetAsSockAddrIn() const { return reinterpret_cast<const sockaddr_in*>(&m_sockAddr); }
};