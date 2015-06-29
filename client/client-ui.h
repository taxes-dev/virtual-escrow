#ifndef H_CLIENT_UI
#define H_CLIENT_UI
#include <chrono>
#include <memory>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Pack.H>
#include <FL/Enumerations.H>
#include "client/client-process.h"
#include "trade.pb.h"

namespace escrow {
	using namespace std;
	using namespace std::chrono;
	
	class ScrollResize : public Fl_Scroll {
	public:
		ScrollResize(int x, int y, int w, int h) : Fl_Scroll(x, y, w, h) { };
		void resize(int x, int y, int w, int h);
	};
	
	class TaggedButton : public Fl_Button {
	public:
		TaggedButton(int x, int y, int w, int h, const char * label) : Fl_Button(x, y, w, h, label) { this->m_tag = 0; };
		~TaggedButton() { if (this->m_tag) { free(this->m_tag); } };
		char * tag() const { return this->m_tag; };
		void set_tag(const char * tag) {
			if (this->m_tag) {
				free(this->m_tag);
				this->m_tag = 0;
			}
			if (tag) {
				this->m_tag = strdup(tag);
			}
		};
	private:
		char * m_tag;
	};
	
	class ClientUI {
	public:
		ClientUI(unique_ptr<ClientProcess> & process) { this->m_process = move(process); this->m_running = false; this->m_time = steady_clock::now(); };
		void add_output(const std::string & text);
		bool is_running();
		void refresh_inventory();
		void run();
	private:
		unique_ptr<ClientProcess> m_process;
		Fl_Input * m_fl_echo_input;
		Fl_Text_Display * m_fl_output;
		Fl_Pack * m_fl_inventory;
		Fl_Pack * m_fl_partners;
		volatile bool m_running;
		steady_clock::time_point m_time;
		
		void echo_button_callback();
		void socket_thread();
		void trade_button_callback(const char * client_id);
		void update_trade_partners(const AvailableTradePartnersResponse * message);
	};
}

#endif