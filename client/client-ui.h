#ifndef H_CLIENT_UI
#define H_CLIENT_UI
#include "client/client-process.h"

namespace escrow {
	class ClientUI {
	public:
		ClientUI(ClientProcess * process) { this->m_process = process; };
		void run();
	private:
		ClientProcess * m_process;
	};
}

#endif