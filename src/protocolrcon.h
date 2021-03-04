/**
 * The Forgotten Server - a free and open-source MMORPG server emulator
 * Copyright (C) 2019  Mark Samman <mark.samman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef FS_RCON_H_
#define FS_RCON_H_

#include "networkmessage.h"
#include "protocol.h"

class ProtocolRCON final : public Protocol
{
public:
	// static protocol information
	enum { server_sends_first = false };
	enum { protocol_identifier = 0xFA };
	enum { use_checksum = false };
	static const char* protocol_name() {
		return "rcon protocol";
	}

	explicit ProtocolRCON(Connection_ptr conn) : Protocol(conn) {}

	void parsePacket(NetworkMessage& msg) override;
	void onRecvFirstMessage(NetworkMessage& msg) override;

	void sendAuthResult(bool success);

	static const uint64_t start;

private:
	static std::map<uint32_t, int64_t> ipConnectMap;
};

#endif
