#include <iostream>
#include <sstream>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "client/client-ui.h"
#include "shared/shared.h"
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"


namespace escrow {
	int char_if_ready() {
		struct pollfd fds;
		fds.fd = 0; /* STDIN */
		fds.events = POLLIN;
		if (poll(&fds, 1, 100) == 1) {
			int c = std::getchar(); // only care about the first character
			std::cin.ignore(); // discard any remaining chars up to the newline
			return c;
		}
		return 0;
	}
	
	void ClientUI::run()
	{
		Fl_Window win(220,90);
		win.resizable(win);
		win.show();
		Fl::run();
		return;
		
		int c = 0;
		while (c != 'q') {
			c = 0;
			std::cout << "Choose a message to send (select and press ENTER):\n1) echo\n2) available trade partners\ni) display inventory\nq) quit" << std::endl;
			while (c == 0) {
				c = char_if_ready(); // times out after 100ms
				switch (c) {
					case '1':
					{
						std::cout << "Please enter the message: " << std::endl;
						std::string input;
						std::getline(std::cin, input);
						
						this->m_process->cmd_EchoRequest(input);
						break;
					}
					case '2': this->m_process->cmd_AvailableTradePartnersRequest([](const escrow::AvailableTradePartnersResponse * response) {
						uuid_t u_client_id;
						char s_client_id[UUID_STR_SIZE];
						google::protobuf::RepeatedPtrField<std::string>::const_iterator iter;
						
						if (response->client_id_size() > 0) {
							for (iter = response->client_id().begin(); iter < response->client_id().end(); ++iter) {
								std::string client_id = *iter;
								std::stringstream clientmsg;
								
								bzero((char *)u_client_id, sizeof(uuid_t));
								bzero((char *)s_client_id, UUID_STR_SIZE);
								client_id.copy((char *)u_client_id, sizeof(uuid_t));
								uuid_unparse(u_client_id, s_client_id);
								
								clientmsg << "Available trade partner: " << s_client_id << std::endl;
								info(clientmsg.str().c_str());
							}
						} else {
							info("No available trade partners");
						}
					}); break;
					case 'i': this->m_process->show_inventory(); break;
				}
				if (this->m_process->process_message(false) == false) {
					info("Lost connection, shutting down");
					return;
				}
			}
		}
	}
}
