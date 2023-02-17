#pragma once
#include <memory>
#include <string>

namespace Comms {

	class IPDiscovery {
	public:
		IPDiscovery();
		std::string DiscoverIP();

	private:
		std::unique_ptr<juice_agent_t, void(*)(juice_agent_t*)> m_agent;
	};
}
