/**
 * @file protocolstatus.cpp
 *
 * The Forgotten Server - a free and open-source MMORPG server emulator
 * Copyright (C) 2019 Mark Samman <mark.samman@gmail.com>
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

#include "otpch.h"

#include "protocolrcon.h"
#include "configmanager.h"
#include "game.h"
#include "outputmessage.h"
#include "iologindata.h"

extern ConfigManager g_config;
extern Game g_game;
extern LuaScriptInterface g_luaScript;

std::map<uint32_t, int64_t> ProtocolRCON::ipConnectMap;
const uint64_t ProtocolRCON::start = OTSYS_TIME();

// Handle first packet (auth packet)
// XX (header: packet lenght) 00 (header: last char) FA (protocol identifier) XX XX XX ... (password)
void ProtocolRCON::onRecvFirstMessage(NetworkMessage& msg)
{
  std::string password = msg.getString(msg.getLength() - msg.getBufferPosition());
  std::cout << "[ProtocolRCON::onRecvFirstMessage] password:" << password << "/" << g_config.getString(ConfigManager::RCON_PASSWORD).c_str() << std::endl;
  if (password == g_config.getString(ConfigManager::RCON_PASSWORD).c_str()) {
    std::cout << "[ProtocolRCON::onRecvFirstMessage] password OK!" << std::endl;
    g_dispatcher.addTask(createTask(std::bind(&ProtocolRCON::sendAuthResult, std::static_pointer_cast<ProtocolRCON>(shared_from_this()), true)));
    return;
  } else {
    disconnect();
  }
}

// Handle next packets (command packets)
// XX (header: packet lenght) 00 (header: last char) XX (command identifier) XX XX XX ... (extra data)
void ProtocolRCON::parsePacket(NetworkMessage& msg)
{
  // Weird fix to proper handle next packets otherwise will read unexpected data from the packet
  msg.setBufferPosition(NetworkMessage::HEADER_LENGTH);

  // Command byte
  uint8_t recvbyte = msg.getByte();

	std::cout << "[ProtocolRCON::parsePacket] msg:" << msg.getLength() << " recvbyte:" << (int)recvbyte << std::endl;

  switch (recvbyte) {
  case 0x01: {
    disconnect();
    return;
  }
  case 0x02: {
    g_game.saveGameState();
    return;
  }
  // START RAID
  case 0x03: {
    const std::string& raidName = "RatsThais";

    Raid* raid = g_game.raids.getRaidByName(raidName);

    std::cout << "[ProtocolRCON::RAID] raid:" << raidName << std::endl;

    if (!raid || !raid->isLoaded()) {
      std::cout << "[ProtocolRCON::RAID] NO EXISTS!" << std::endl;
    }

    if (g_game.raids.getRunning()) {
      std::cout << "[ProtocolRCON::RAID] RUNNING!" << std::endl;
    }

    g_game.raids.setRunning(raid);
    raid->startRaid();
    return;
  }
  // BROADCAST
  case 0x04: {
    Player* p = new Player(nullptr);
    IOLoginData::preloadPlayer(p, "Knight");
    IOLoginData::loadPlayerById(p, p->getGUID());
    const auto& players = g_game.getPlayers();
    for (const auto& it : players) {
      it.second->sendPrivateMessage(p, TALKTYPE_BROADCAST, "TEST MESAGGE!");
    }
    return;
  }
  // PLAYER LIST
  case 0x05: {
    auto output = OutputMessagePool::getOutputMessage();
    const auto& players = g_game.getPlayers();
    output->add<uint32_t>(players.size());
    for (const auto& it : players) {
      output->addString(it.second->getName());
      output->add<uint32_t>(it.second->getLevel());
    }
    send(output);
    return;
  }

  default:
    break;
  }
}

void ProtocolRCON::sendAuthResult(bool success)
{
  auto output = OutputMessagePool::getOutputMessage();

  output->addByte(success);
  output->addString(g_config.getString(ConfigManager::SERVER_NAME));
  output->addString(g_config.getString(ConfigManager::IP));
  output->addString(std::to_string(g_config.getNumber(ConfigManager::LOGIN_PORT)));

  send(output);
  //disconnect();
}
