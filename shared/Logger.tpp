namespace escrow {
	
	void Logger::fatal(const char * msg) {
		std::cerr << "[" << pid() << " FATAL] " << msg << std::endl;
		exit(1);
	}
	
	void Logger::warn(const char * msg) {
		std::cerr << "[" << pid() << " WARN] " << msg << std::endl;
	}
	
	void Logger::info(const char * msg) {
		std::cout << "[" << pid() << " INFO] " << msg << std::endl;
	}
}
