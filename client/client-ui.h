#ifndef H_CLIENT_UI
#define H_CLIENT_UI
#include <FL/Fl_Input.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Pack.H>
#include "client/client-process.h"

namespace escrow {
	class ScrollResize : public Fl_Scroll {
	public:
		ScrollResize(int x, int y, int w, int h) : Fl_Scroll(x, y, w, h) { };
		void resize(int x, int y, int w, int h);
	};
	
	class ClientUI {
	public:
		ClientUI(ClientProcess * process) { this->m_process = process; this->m_running = false; };
		void add_output(const std::string & text);
		bool is_running();
		void refresh_inventory();
		void run();
	private:
		ClientProcess * m_process;
		Fl_Input * m_fl_echo_input;
		Fl_Text_Display * m_fl_output;
		Fl_Pack * m_fl_inventory;
		volatile bool m_running;
		
		void echo_button_callback();
		void socket_thread();
	};
}

#endif