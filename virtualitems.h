#ifndef _VIRTUALITEMS_H
#define _VIRTUALITEMS_H
#include <string>
#include <vector>
#include <uuid/uuid.h>

namespace escrow {
	using namespace std;
	
	class VirtualItem {
	public:
		VirtualItem(const int item_id, const uuid_t owner_id);
		string desc();
		int item_id();
		string instance_id();
		string original_owner_id();
		static string get_item_desc(const int item_id);
	private:
		int m_item_id;
		uuid_t m_instance_id;
		uuid_t m_owner_id;
		string m_desc;
	};

	using Inventory = vector<VirtualItem *>;
	void generate_random_inventory(Inventory * inventory, const int size, const uuid_t owner);
	void free_inventory_items(Inventory * inventory);
}

#endif