#include <iostream>
#include <sstream>
#include <string>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Enumerations.H>
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
		
	void ScrollResize::resize(int x, int y, int w, int h) {
		Fl_Scroll::resize(x, y, w, h);
		Fl_Pack * child = dynamic_cast<Fl_Pack *>(this->child(0));
		if (child) {
			child->resize(this->x(), this->y(), this->w(), child->h());
		}
	}
	
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
			if (this->m_process->process_message() == false) { // process_message blocks for up to 100ms
				Logger::warn("Lost connection, shutting down");
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
	
	void ClientUI::refresh_inventory() {
		uuid_t owner_id;
		uuid_t this_id;
		Inventory * inventory = this->m_process->inventory();
		
		this->m_process->copy_client_id(&this_id);
		
		Fl::lock();
		this->m_fl_inventory->clear();
		for (auto & item : *inventory) {
			
			bzero(owner_id, sizeof(uuid_t));
			item->copy_original_owner_id(&owner_id);
			bool is_local = uuid_compare(this_id, owner_id) == 0;
			
			Fl_Pack * pack = new Fl_Pack(0, 0, this->m_fl_inventory->w(), 25);
			pack->type(Fl_Pack::HORIZONTAL);
			pack->resizable(pack);
			
			Fl_Box * box = new Fl_Box(Fl_Boxtype::FL_BORDER_BOX, 0, 0, this->m_fl_inventory->w() - 30, 25, item->desc().c_str());
			box->color(fl_rgb_color(255, 255, 255));
			pack->add(box);
			
			Fl_Box * box2 = new Fl_Box(Fl_Boxtype::FL_BORDER_BOX, 0, this->m_fl_inventory->w() - 30, 30, 25, is_local ? "L" : "X");
			box2->color(is_local ? fl_rgb_color(0, 127, 0) : fl_rgb_color(127, 0, 0));
			pack->add(box2);
			
			this->m_fl_inventory->add(pack);
		}
		this->m_fl_inventory->redraw();
		Fl::awake();
		Fl::unlock();
	}
	
	void ClientUI::run()
	{
		pthread_t sockthread;
		Fl_Text_Buffer textbuffer;
	
		this->m_running = true;
		
		Fl::lock(); // init fltk threading
		
		// begin window
		Fl_Window win(DEFAULT_WIN_W, DEFAULT_WIN_H);
		win.size_range(MIN_WIN_W, MIN_WIN_H);
		win.label("Virtual Escrow Client");
		win.resizable(win);
		
		// echo message controls
		Fl_Input echo_input(10, 10, 250, 25);
		echo_input.value("This is an echo message.");
		this->m_fl_echo_input = &echo_input;
		Fl_Button echo_button(270, 10, 100, 25, "Echo");
		echo_button.callback([](Fl_Widget * widget, void * data) { static_cast<ClientUI *>(data)->echo_button_callback(); }, this);
				
		// inventory display
		Fl_Box inventory_label(10, 45, 250, 25, "Inventory:");
		ScrollResize inventory_scroll(10, 70, 350, 250);
		inventory_scroll.type(Fl_Scroll::VERTICAL);
			Fl_Pack inventory(inventory_scroll.x(), inventory_scroll.y(), inventory_scroll.w(), inventory_scroll.h());
			inventory.resizable(inventory);
			inventory.end();
		inventory_scroll.end();
		this->m_fl_inventory = &inventory;
		
		// output log
		Fl_Text_Display output(10, 340, 730, 150);
		output.buffer(textbuffer);
		output.hide_cursor();
		this->m_fl_output = &output;
				
		// end window
		win.end();
		win.show();
		
		// create processing thread
		int retval = pthread_create(&sockthread, nullptr, [](void * context) -> void * { static_cast<ClientUI *>(context)->socket_thread(); }, this);
		if (retval) {
			Logger::fatal("ERROR creating background thread");
		}
		
		this->refresh_inventory();
		
		// wait until window is closed or connection to server is closed
		while (this->is_running() && Fl::wait() > 0) ;
		
		this->m_running = false;
		
		pthread_join(sockthread, nullptr);
	}
}
