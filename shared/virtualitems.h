#ifndef VIRTUALITEMS_H
#define VIRTUALITEMS_H
#include <memory>
#include <string>
#include <vector>
#include <uuid/uuid.h>

namespace escrow {
	using namespace std;
	
	class VirtualItem {
	public:
		VirtualItem(const int item_id, const uuid_t owner_id);
		inline string desc() { return this->m_desc; };
		inline int item_id() { return this->m_item_id; };
		inline string instance_id_parsed() { return this->m_str_instance_id; };
		inline string original_owner_id_parsed() { return this->m_str_owner_id; };
		void copy_instance_id(uuid_t * dst);
		void copy_original_owner_id(uuid_t * dst);
		static string get_item_desc(const int item_id);
	private:
		int m_item_id;
		uuid_t m_instance_id;
		string m_str_instance_id;
		uuid_t m_owner_id;
		string m_str_owner_id;
		string m_desc;
	};

	using Inventory = vector<unique_ptr<VirtualItem>>;
	void generate_random_inventory(Inventory & inventory, const int size, const uuid_t owner);
	
	inline void VirtualItem::copy_instance_id(uuid_t * dst) {
		uuid_copy(*dst, this->m_instance_id);
	}
	
	inline void VirtualItem::copy_original_owner_id(uuid_t * dst) {
		uuid_copy(*dst, this->m_owner_id);
	}
}

#endif