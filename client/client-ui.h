#ifndef H_CLIENT_UI
#define H_CLIENT_UI
#include <memory>
#include <FL/Fl_Input.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Pack.H>
#include "client/client-process.h"

namespace escrow {
	using namespace std;
	
	class ScrollResize : public Fl_Scroll {
	public:
		ScrollResize(int x, int y, int w, int h) : Fl_Scroll(x, y, w, h) { };
		void resize(int x, int y, int w, int h);
	};
	
	class ClientUI {
	public:
		ClientUI(unique_ptr<ClientProcess> & process) { this->m_process = move(process); this->m_running = false; };
		void add_output(const std::string & text);
		bool is_running();
		void refresh_inventory();
		void run();
	private:
		unique_ptr<ClientProcess> m_process;
		Fl_Input * m_fl_echo_input;
		Fl_Text_Display * m_fl_output;
		Fl_Pack * m_fl_inventory;
		volatile bool m_running;
		
		void echo_button_callback();
		void socket_thread();
	};
}

#endif