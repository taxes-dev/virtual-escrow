#include <iostream>
#include <sstream>
#include <string>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Display.H>
#include "client/client-ui.h"
#include "shared/shared.h"
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"

#define DEFAULT_WIN_W 750
#define MIN_WIN_W 600
#define DEFAULT_WIN_H 500
#define MIN_WIN_H 400

namespace escrow {
	using namespace std;
		
	void ClientUI::add_output(const string & text)
	{
		Fl::lock();
		this->m_fl_output->insert(text.c_str());
		this->m_fl_output->insert("\n");
		this->m_fl_output->show_insert_position();
		Fl::awake();
		Fl::unlock();
	}
	
	bool ClientUI::is_running()
	{
		return this->m_running;
	}

	void ClientUI::socket_thread() {
		while (this->is_running()) {
			if (this->m_process->process_message(false) == false) { // process_message blocks for up to 100ms
				info("Lost connection, shutting down");
				this->m_running = false;
			}
		}
	}
	
	void ClientUI::echo_button_callback()
	{
		Fl::lock();
		string input = string(this->m_fl_echo_input->value());
		Fl::unlock();
		this->m_process->cmd_EchoRequest(input, [](const EchoResponse * response, void * data) {
			static_cast<ClientUI *>(data)->add_output(response->message());
		}, this);
	}
	
	void ClientUI::run()
	{
		pthread_t sockthread;
		Fl_Text_Buffer textbuffer;
	
		this->m_running = true;
		
		Fl::lock(); // init fltk threading
		Fl_Window win(DEFAULT_WIN_W, DEFAULT_WIN_H);
		win.size_range(MIN_WIN_W, MIN_WIN_H);
		win.label("Virtual Escrow Client");
		win.resizable(win);
		Fl_Input echo_input(10, 10, 250, 25);
		echo_input.value("This is an echo message.");
		this->m_fl_echo_input = &echo_input;
		Fl_Button echo_button(270, 10, 100, 25, "Echo");
		echo_button.callback([](Fl_Widget * widget, void * data) { static_cast<ClientUI *>(data)->echo_button_callback(); }, this);
		Fl_Text_Display output(10, 340, 730, 150);
		output.buffer(textbuffer);
		output.hide_cursor();
		this->m_fl_output = &output;
		win.end();
		win.show();
		
		int retval = pthread_create(&sockthread, nullptr, [](void * context) -> void * { static_cast<ClientUI *>(context)->socket_thread(); }, this);
		if (retval) {
			error("ERROR creating background thread");
		}
		
		while (this->is_running() && Fl::wait() > 0) ;
		
		this->m_running = false;
		
		pthread_join(sockthread, nullptr);
		
		/*int c = 0;
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
		}*/
	}
}
