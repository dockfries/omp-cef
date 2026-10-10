#pragma once

#include "packet.hpp"

// The transport is KCP, which fragments a message over as many datagrams as it needs, so a long
// string is not limited by the datagram size. What does limit it is KCP's per-message fragment cap
// (128 * mss, about 176 KB with the default MTU), so strings are capped below that. The old cap of
// 4096 bytes silently cut off the resource manifest of any project with more than ~30 files.
static constexpr size_t kMaxStringBytes = 128 * 1024;

static inline void WriteString(std::ostream& os, const std::string& str)
{
	// Compared as size_t: narrowing to uint16_t first would wrap for strings of 65536 bytes or more.
	size_t length = str.length();

	if (length > kMaxStringBytes) {
		// No format arguments: this header is used by the client (fmt logger) and the server (printf logger).
		LOG_ERROR("String exceeds the protocol limit and was truncated.");
		length = kMaxStringBytes;
	}

	if (length >= 0xFFFF) {
		// Escaped length: uint16 0xFFFF followed by the real length. Readers that predate this only
		// ever received strings that fit in a uint16, so nothing that used to work changes.
		const uint16_t escape = 0xFFFF;
		const uint32_t wide_length = static_cast<uint32_t>(length);
		os.write(reinterpret_cast<const char*>(&escape), sizeof(escape));
		os.write(reinterpret_cast<const char*>(&wide_length), sizeof(wide_length));
	}
	else {
		const uint16_t wire_length = static_cast<uint16_t>(length);
		os.write(reinterpret_cast<const char*>(&wire_length), sizeof(wire_length));
	}

	if (length > 0) {
		os.write(str.data(), length);
	}
}

static inline bool ReadString(std::istream& is, std::string& str)
{
	uint16_t length = 0;

	is.read(reinterpret_cast<char*>(&length), sizeof(length));
	if (is.gcount() != sizeof(length))
		return false;

	uint64_t payload_length = length;

	if (length == 0xFFFF) {
		uint32_t wide_length = 0;
		is.read(reinterpret_cast<char*>(&wide_length), sizeof(wide_length));
		if (is.gcount() != sizeof(wide_length))
			return false;

		payload_length = wide_length;
	}

	if (payload_length > kMaxStringBytes) {
		LOG_WARN("Refusing an over-long string: the protocol limit was exceeded.");
		return false;
	}

	if (payload_length > 0) {
		const std::streamsize available = is.rdbuf()->in_avail();
		if (available < 0 || payload_length > static_cast<uint64_t>(available))
			return false;

		str.resize(static_cast<size_t>(payload_length));
		is.read(&str[0], static_cast<std::streamsize>(payload_length));

		if (is.gcount() != static_cast<std::streamsize>(payload_length))
			return false;
	}
	else {
		str.clear();
	}

	return true;
}

static inline void WriteBytes(std::ostream& os, const std::vector<uint8_t>& bytes)
{
	uint32_t length = static_cast<uint32_t>(bytes.size());

	os.write(reinterpret_cast<const char*>(&length), sizeof(length));

	if (length > 0) {
		os.write(reinterpret_cast<const char*>(bytes.data()), length);
	}
}

static inline bool ReadBytes(std::istream& is, std::vector<uint8_t>& bytes)
{
	uint32_t length = 0;

	is.read(reinterpret_cast<char*>(&length), sizeof(length));
	if (is.gcount() != sizeof(length))
		return false;

	if (length > 0) {
		// A length that exceeds what is left in the packet has to be rejected before the resize:
		// value-initialising a vector to an attacker-chosen 4 GiB length would cost the whole
		// allocation (or throw) before the read can fail.
		const std::streamsize available = is.rdbuf()->in_avail();
		if (available < 0 || static_cast<uint64_t>(length) > static_cast<uint64_t>(available))
			return false;

		bytes.resize(length);

		is.read(reinterpret_cast<char*>(bytes.data()), length);
		if (is.gcount() != length)
			return false;
	}
	else {
		bytes.clear();
	}

	return true;
}

inline bool SerializePacket(const NetworkPacket& packet, std::string& out)
{
	try {
		std::ostringstream os(std::ios::binary);
		os.put(static_cast<uint8_t>(packet.type));

		std::visit([&os](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;

			if constexpr (std::is_same_v<T, RequestJoinPacket>) {
				os.write(reinterpret_cast<const char*>(&arg.playerid), sizeof(arg.playerid));
				os.write(reinterpret_cast<const char*>(&arg.client_version), sizeof(arg.client_version));
			}
			else if constexpr (std::is_same_v<T, HandshakeChallengePacket>) {
				WriteBytes(os, arg.cookie);
				WriteBytes(os, arg.server_public_key);
			}
			else if constexpr (std::is_same_v<T, HandshakeFinalizePacket>) {
				WriteBytes(os, arg.cookie);
				WriteBytes(os, arg.client_public_key);
			}
			else if constexpr (std::is_same_v<T, JoinResponsePacket>) {
				os.put(arg.accepted ? 1 : 0);
				os.write(reinterpret_cast<const char*>(&arg.kcp_conv_id), sizeof(arg.kcp_conv_id));
				WriteString(os, arg.manifest_json);
				os.put(static_cast<char>(arg.reject_reason));
				os.write(reinterpret_cast<const char*>(&arg.server_version), sizeof(arg.server_version));
				os.write(reinterpret_cast<const char*>(&arg.client_version), sizeof(arg.client_version));
				WriteString(os, arg.message);
			}
			else if constexpr (std::is_same_v<T, ServerConfigPacket>) {
				WriteBytes(os, arg.master_resource_key);
				os.put(arg.resources_loader_ui ? 1 : 0);
			}
			else if constexpr (std::is_same_v<T, RequestFilesPacket>) {
				uint16_t count = static_cast<uint16_t>(arg.files.size());
				os.write(reinterpret_cast<const char*>(&count), sizeof(count));

				if (!os.good())
					return;

				for (size_t i = 0; i < arg.files.size(); ++i) {
					const auto& [resourceName, relativePath] = arg.files[i];

					if (resourceName.empty() || relativePath.empty())
						continue;

					WriteString(os, resourceName);

					if (!os.good())
						return;

					WriteString(os, relativePath);

					if (!os.good())
						return;
				}
			}
			else if constexpr (std::is_same_v<T, FileDataPacket>) {
				WriteString(os, arg.resourceName);
				WriteString(os, arg.relativePath);
				WriteString(os, arg.fileHash);
				os.write(reinterpret_cast<const char*>(&arg.chunkIndex), sizeof(arg.chunkIndex));
				os.write(reinterpret_cast<const char*>(&arg.totalChunks), sizeof(arg.totalChunks));
				WriteBytes(os, arg.data);
			}
			else if constexpr (std::is_same_v<T, EmitEventPacket> || std::is_same_v<T, ClientEmitEventPacket>) {
				os.write(reinterpret_cast<const char*>(&arg.browserId), sizeof(arg.browserId));
				WriteString(os, arg.name);

				uint8_t count = static_cast<uint8_t>(arg.args.size());
				os.put(count);

				for (const auto& argument : arg.args) {
					os.put(static_cast<uint8_t>(argument.type));

					switch (argument.type) {
						case ArgumentType::String:
							WriteString(os, argument.stringValue);
							break;
						case ArgumentType::Integer:
							os.write(reinterpret_cast<const char*>(&argument.intValue), sizeof(int));
							break;
						case ArgumentType::Float:
							os.write(reinterpret_cast<const char*>(&argument.floatValue), sizeof(float));
							break;
						case ArgumentType::Bool:
							os.put(argument.boolValue ? 1 : 0);
							break;
					}
				}
			}
		}, packet.payload);

		if (!os.good()) {
			return false;
		}

		out = os.str();
		return true;
	}
	catch (const std::exception& e) {
		LOG_ERROR("[Serialize] Exception: {}", e.what());
		return false;
	}
	catch (...) {
		LOG_ERROR("[Serialize] Unknown exception");
		return false;
	}
}

inline bool DeserializePacket(const char* data, size_t size, NetworkPacket& out)
{
	std::istringstream is(std::string(data, size), std::ios::binary);
	uint8_t packet_type_val = 0;

	is.get(reinterpret_cast<char&>(packet_type_val));
	if (!is.good())
		return false;

	out.type = static_cast<PacketType>(packet_type_val);

	switch (out.type)
	{
		case PacketType::RequestJoin: {
			RequestJoinPacket packet{};

			is.read(reinterpret_cast<char*>(&packet.playerid), sizeof(packet.playerid));
			if (!is.good())
				return false;

			packet.client_version = 0;
			if (is.rdbuf()->in_avail() >= static_cast<std::streamsize>(sizeof(packet.client_version))) {
				is.read(reinterpret_cast<char*>(&packet.client_version), sizeof(packet.client_version));
				if (!is.good()) {
					packet.client_version = 0;
					is.clear();
				}
			}

			out.payload = packet;
			break;
		}
		case PacketType::HandshakeChallenge: {
			HandshakeChallengePacket packet{};

			if (!ReadBytes(is, packet.cookie))
				return false;

			if (!ReadBytes(is, packet.server_public_key))
				return false;

			out.payload = packet;
			break;
		}
		case PacketType::HandshakeFinalize: {
			HandshakeFinalizePacket packet{};

			if (!ReadBytes(is, packet.cookie))
				return false;

			if (!ReadBytes(is, packet.client_public_key))
				return false;

			out.payload = packet;
			break;
		}
		case PacketType::JoinResponse: {
			JoinResponsePacket packet{};

			char accepted;
			is.get(accepted);

			if (!is.good())
				return false;

			packet.accepted = accepted != 0;
			is.read(reinterpret_cast<char*>(&packet.kcp_conv_id), sizeof(packet.kcp_conv_id));

			if (!is.good())
				return false;

			if (!ReadString(is, packet.manifest_json))
				return false;

			packet.reject_reason = static_cast<uint8_t>(JoinRejectReason::None);
			packet.server_version = 0;
			packet.client_version = 0;
			packet.message.clear();

			if (is.rdbuf()->in_avail() > 0) {
				char buf = 0;
				is.get(buf);

				if (is.good()) {
					packet.reject_reason = static_cast<uint8_t>(buf);
					
					if (is.rdbuf()->in_avail() >= static_cast<std::streamsize>(sizeof(packet.server_version)))
						is.read(reinterpret_cast<char*>(&packet.server_version), sizeof(packet.server_version));
					
					if (is.rdbuf()->in_avail() >= static_cast<std::streamsize>(sizeof(packet.client_version)))
						is.read(reinterpret_cast<char*>(&packet.client_version), sizeof(packet.client_version));

					if (is.rdbuf()->in_avail() >= 2) {
						ReadString(is, packet.message);
					}
				}

				if (!is.good()) {
					is.clear();
				}
			}

			out.payload = packet;
			break;
		}
		case PacketType::ServerConfig: {
			ServerConfigPacket packet{};

			if (!ReadBytes(is, packet.master_resource_key))
				return false;

			char b = 0;
			is.get(b);
			if (!is.good())
				return false;

			packet.resources_loader_ui = (b != 0);

			out.payload = packet;
			break;
		}
		case PacketType::RequestFiles: {
			RequestFilesPacket packet{};

			uint16_t count{};
			is.read(reinterpret_cast<char*>(&count), sizeof(count));
			if (is.gcount() != sizeof(count))
				return false;

			for (uint16_t i = 0; i < count; ++i) {
				std::string resourceName;
				std::string relativePath;

				if (!ReadString(is, resourceName) || !ReadString(is, relativePath)) {
					return false;
				}

				packet.files.emplace_back(resourceName, relativePath);
			}
			out.payload = packet;
			break;
		}
		case PacketType::FileData: {
			FileDataPacket packet{};

			if (!ReadString(is, packet.resourceName))
				return false;

			if (!ReadString(is, packet.relativePath))
				return false;

			if (!ReadString(is, packet.fileHash))
				return false;

			is.read(reinterpret_cast<char*>(&packet.chunkIndex), sizeof(packet.chunkIndex));
			if (is.gcount() != sizeof(packet.chunkIndex))
				return false;

			is.read(reinterpret_cast<char*>(&packet.totalChunks), sizeof(packet.totalChunks));
			if (is.gcount() != sizeof(packet.totalChunks))
				return false;

			if (!ReadBytes(is, packet.data))
				return false;

			out.payload = packet;
			break;
		}
		case PacketType::EmitEvent:
		case PacketType::EmitBrowserEvent:
		case PacketType::ClientEmitEvent: {
            EmitEventPacket packet{};

            is.read(reinterpret_cast<char*>(&packet.browserId), sizeof(packet.browserId));
            if (!ReadString(is, packet.name))
                return false;

            uint8_t count{};
            is.get(reinterpret_cast<char&>(count));

            if (!is.good()) 
                return false;

            for (uint8_t i = 0; i < count; ++i) {
                uint8_t type{};
                is.get(reinterpret_cast<char&>(type));

                Argument arg;
                arg.type = static_cast<ArgumentType>(type);

                switch (arg.type) 
                {
                    case ArgumentType::String:  
                        if (!ReadString(is, arg.stringValue)) 
                            return false;
                        break;
                    case ArgumentType::Integer: 
                        is.read(reinterpret_cast<char*>(&arg.intValue), sizeof(int)); 
                        break;
                    case ArgumentType::Float:   
                        is.read(reinterpret_cast<char*>(&arg.floatValue), sizeof(float)); 
                        break;
                    case ArgumentType::Bool:    
                        char boolean; is.get(boolean); arg.boolValue = (boolean != 0);
                        break;
                }

                if (!is.good()) 
                    return false;

                packet.args.push_back(arg);
            }

            if (out.type == PacketType::ClientEmitEvent) {
                ClientEmitEventPacket client_packet;

                client_packet.browserId = packet.browserId;
                client_packet.name = packet.name;
                client_packet.args = packet.args;

                out.payload = client_packet;
            }
            else {
                out.payload = packet;
            }

            break;
        }
	}

	return is.good();
}