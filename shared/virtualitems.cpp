#include <functional>
#include <random>
#include <string>
#include <uuid/uuid.h>
#include "shared/shared.h"
#include "shared/virtualitems.h"

namespace escrow {
	using namespace std;
	
	string VirtualItem::get_item_desc(const int item_id) {
		switch (item_id) {
			// courtesy of: http://www.seventhsanctum.com/generate.php?Genname=sftool
			case 0: return "Antimatter Retrocrowbar";
			case 1: return "Computerized Bioniscrewdriver";
			case 2: return "Covalent Chisel";
			case 3: return "Magnochisel";
			case 4: return "Neutronium Drill";
			case 5: return "Transadaptable Measure";
			case 6: return "Dimensional Stapler";
			case 7: return "Hydrocyclic Hammer";
			case 8: return "Protonic Level";
			case 9: return "Subspace Screwdriver";
			default: return "Unknown";
		}
	}

	VirtualItem::VirtualItem(const int item_id, const uuid_t owner_id) {
		char s_tmp_uuid[UUID_STR_SIZE];
		
		this->m_item_id = item_id;
		uuid_generate(this->m_instance_id);
		uuid_unparse(this->m_instance_id, s_tmp_uuid);
		this->m_str_instance_id = string(s_tmp_uuid, UUID_STR_SIZE);
		
		this->m_desc = VirtualItem::get_item_desc(item_id);
		
		uuid_copy(this->m_owner_id, owner_id);
		bzero(s_tmp_uuid, sizeof(UUID_STR_SIZE));
		uuid_unparse(this->m_owner_id, s_tmp_uuid);
		this->m_str_owner_id = string(s_tmp_uuid, UUID_STR_SIZE);
	}
	
	void generate_random_inventory(Inventory * inventory, const int size, const uuid_t owner) {
		if (size > 0) {
			default_random_engine generator;
			uniform_int_distribution<int> distribution(0, 9);
			auto randitem = bind(distribution, generator);
			
			for (int i = 0; i < size; i++) {
				VirtualItem * item = new VirtualItem(randitem(), owner);
				inventory->push_back(item);
			}
		}
	}
	
	void free_inventory_items(Inventory * inventory) {
		while (inventory->size() > 0) {
			VirtualItem * item = inventory->back();
			delete item;
			inventory->pop_back();
		}
	}
}