#include "libjuice/juice.h"

#include "comms/ip_discovery.h"

namespace {
	constexpr const char* STUNServerHost = "stun.l.google.com";
	constexpr int STUNServerPort = 19302;
}

Comms::IPDiscovery::IPDiscovery()
	: m_agent(nullptr, juice_destroy)
{
	juice_config_t stunConfig;
	memset(&stunConfig, 0, sizeof(stunConfig));

	stunConfig.stun_server_host = STUNServerHost;
	stunConfig.stun_server_port = STUNServerPort;

	m_agent = std::unique_ptr<juice_agent_t, decltype(&juice_destroy)>(juice_create(&stunConfig), &juice_destroy);
}

std::string Comms::IPDiscovery::DiscoverIP() {
	return "";
}
