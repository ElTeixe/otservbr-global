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

void ProtocolRCON::onRecvFirstMessage(NetworkMessage& msg)
{
  uint8_t recvbyte = msg.getByte();

  std::cout << "[ProtocolRCON::onRecvFirstMessage] msg:" << msg.getLength() << " recvbyte:" << (int)recvbyte << std::endl;

  switch (recvbyte) {
  case 0x001: {
    std::string password = msg.getString(msg.getLength() - msg.getBufferPosition());
    std::cout << "[ProtocolRCON::onRecvFirstMessage] password:" << password << "/" << g_config.getString(ConfigManager::RCON_PASSWORD).c_str() << std::endl;
    if (password == g_config.getString(ConfigManager::RCON_PASSWORD).c_str()) {
      std::cout << "[ProtocolRCON::onRecvFirstMessage] password OK!" << std::endl;
      g_dispatcher.addTask(createTask(std::bind(&ProtocolRCON::sendStatusString, std::static_pointer_cast<ProtocolRCON>(shared_from_this()))));
      return;
    }
    else {
      disconnect();
    }
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

void ProtocolRCON::parsePacket(NetworkMessage& msg)
{
  uint8_t recvbyte = msg.getByte();

	std::cout << "[ProtocolRCON::parsePacket] msg:" << msg.getLength() << " recvbyte:" << (int)recvbyte << std::endl;

  switch (recvbyte) {
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

void ProtocolRCON::sendStatusString()
{
	auto output = OutputMessagePool::getOutputMessage();

	setRawMessages(true);

	pugi::xml_document doc;

	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";

	pugi::xml_node tsqp = doc.append_child("tsqp");
	tsqp.append_attribute("version") = "1.0";

	pugi::xml_node serverinfo = tsqp.append_child("serverinfo");
	uint64_t uptime = (OTSYS_TIME() - ProtocolRCON::start) / 1000;
	serverinfo.append_attribute("uptime") = std::to_string(uptime).c_str();
	serverinfo.append_attribute("ip") = g_config.getString(ConfigManager::IP).c_str();
	serverinfo.append_attribute("servername") = g_config.getString(ConfigManager::SERVER_NAME).c_str();
	serverinfo.append_attribute("port") = std::to_string(g_config.getNumber(ConfigManager::LOGIN_PORT)).c_str();
	serverinfo.append_attribute("location") = g_config.getString(ConfigManager::LOCATION).c_str();
	serverinfo.append_attribute("url") = g_config.getString(ConfigManager::URL).c_str();
	serverinfo.append_attribute("server") = STATUS_SERVER_NAME;
	serverinfo.append_attribute("version") = STATUS_SERVER_VERSION;
	serverinfo.append_attribute("client") = g_config.getString(ConfigManager::CLIENT_VERSION_STR).c_str();

	pugi::xml_node owner = tsqp.append_child("owner");
	owner.append_attribute("name") = g_config.getString(ConfigManager::OWNER_NAME).c_str();
	owner.append_attribute("email") = g_config.getString(ConfigManager::OWNER_EMAIL).c_str();

	pugi::xml_node players = tsqp.append_child("players");
	uint32_t real = 0;
	std::map<uint32_t, uint32_t> listIP;
	for (const auto& it : g_game.getPlayers()) {
		if (it.second->getIP() != 0) {
			auto ip = listIP.find(it.second->getIP());
			if (ip != listIP.end()) {
				listIP[it.second->getIP()]++;
				if (listIP[it.second->getIP()] < 5) {
					real++;
				}
			}
			else {
				listIP[it.second->getIP()] = 1;
				real++;
			}
		}
	}
	players.append_attribute("online") = std::to_string(real).c_str();
	players.append_attribute("max") = std::to_string(g_config.getNumber(ConfigManager::MAX_PLAYERS)).c_str();
	players.append_attribute("peak") = std::to_string(g_game.getPlayersRecord()).c_str();

	pugi::xml_node monsters = tsqp.append_child("monsters");
	monsters.append_attribute("total") = std::to_string(g_game.getMonstersOnline()).c_str();

	pugi::xml_node npcs = tsqp.append_child("npcs");
	npcs.append_attribute("total") = std::to_string(g_game.getNpcsOnline()).c_str();

	pugi::xml_node rates = tsqp.append_child("rates");
	rates.append_attribute("experience") = std::to_string(g_config.getNumber(ConfigManager::RATE_EXPERIENCE)).c_str();
	rates.append_attribute("skill") = std::to_string(g_config.getNumber(ConfigManager::RATE_SKILL)).c_str();
	rates.append_attribute("loot") = std::to_string(g_config.getNumber(ConfigManager::RATE_LOOT)).c_str();
	rates.append_attribute("magic") = std::to_string(g_config.getNumber(ConfigManager::RATE_MAGIC)).c_str();
	rates.append_attribute("spawn") = std::to_string(g_config.getNumber(ConfigManager::RATE_SPAWN)).c_str();

	pugi::xml_node map = tsqp.append_child("map");
	map.append_attribute("name") = g_config.getString(ConfigManager::MAP_NAME).c_str();
	map.append_attribute("author") = g_config.getString(ConfigManager::MAP_AUTHOR).c_str();

	uint32_t mapWidth, mapHeight;
	g_game.getMapDimensions(mapWidth, mapHeight);
	map.append_attribute("width") = std::to_string(mapWidth).c_str();
	map.append_attribute("height") = std::to_string(mapHeight).c_str();

	pugi::xml_node motd = tsqp.append_child("motd");
	motd.text() = g_config.getString(ConfigManager::MOTD).c_str();

	std::ostringstream ss;
	doc.save(ss, "", pugi::format_raw);

	std::string data = ss.str();
	output->addBytes(data.c_str(), data.size());
	send(output);
	//disconnect();
}
