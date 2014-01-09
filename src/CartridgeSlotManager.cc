#include "CartridgeSlotManager.hh"
#include "MSXMotherBoard.hh"
#include "HardwareConfig.hh"
#include "RecordedCommand.hh"
#include "CommandException.hh"
#include "InfoTopic.hh"
#include "FileContext.hh"
#include "TclObject.hh"
#include "MSXException.hh"
#include "StringOp.hh"
#include "CliComm.hh"
#include "unreachable.hh"
#include "memory.hh"
#include "xrange.hh"
#include <cassert>

using std::string;
using std::vector;

namespace openmsx {

class CartCmd : public RecordedCommand
{
public:
	CartCmd(CartridgeSlotManager& manager, MSXMotherBoard& motherBoard,
	        string_ref commandName);
	virtual string execute(const vector<string>& tokens, EmuTime::param time);
	virtual string help(const vector<string>& tokens) const;
	virtual void tabCompletion(vector<string>& tokens) const;
	virtual bool needRecord(const vector<string>& tokens) const;
private:
	const HardwareConfig* getExtensionConfig(string_ref cartname);
	CartridgeSlotManager& manager;
	CliComm& cliComm;
};

class CartridgeSlotInfo : public InfoTopic
{
public:
	CartridgeSlotInfo(InfoCommand& machineInfoCommand,
	                 CartridgeSlotManager& manger);
	virtual void execute(const vector<TclObject>& tokens,
	                     TclObject& result) const;
	virtual string help(const vector<string>& tokens) const;
private:
	CartridgeSlotManager& manager;
};


// CartridgeSlotManager::Slot
CartridgeSlotManager::Slot::Slot()
	: config(nullptr), useCount(0), ps(0), ss(0)
{
}

CartridgeSlotManager::Slot::~Slot()
{
	assert(!config);
	assert(useCount == 0);
}

bool CartridgeSlotManager::Slot::exists() const
{
	return command != nullptr;
}

bool CartridgeSlotManager::Slot::used(const HardwareConfig* allowed) const
{
	assert((useCount == 0) == (config == nullptr));
	return config && (config != allowed);
}


// CartridgeSlotManager
CartridgeSlotManager::CartridgeSlotManager(MSXMotherBoard& motherBoard_)
	: motherBoard(motherBoard_)
	, cartCmd(make_unique<CartCmd>(*this, motherBoard, "cart"))
	, extSlotInfo(make_unique<CartridgeSlotInfo>(
		motherBoard.getMachineInfoCommand(), *this))
{
}

CartridgeSlotManager::~CartridgeSlotManager()
{
	for (auto slot : xrange(MAX_SLOTS)) {
		(void)slot;
		assert(!slots[slot].exists());
		assert(!slots[slot].used());
	}
}

int CartridgeSlotManager::getSlotNum(string_ref slot)
{
	if ((slot.size() == 1) && ('a' <= slot[0]) && (slot[0] <= 'p')) {
		return -(1 + slot[0] - 'a');
	} else if (slot == "any") {
		return -256;
	} else if (slot == "X") {
		return -256;
	} else if ((slot.size() == 2) && (slot[0] == '?')) {
		int result = slot[1] - '0';
		if ((result < 0) || (4 <= result)) {
			throw MSXException(
				"Invalid slot specification: " + slot);
		}
		return result - 128;
	} else {
		int result = stoi(slot);
		if ((result < 0) || (4 <= result)) {
			throw MSXException(
				"Invalid slot specification: " + slot);
		}
		return result;
	}
}

void CartridgeSlotManager::createExternalSlot(int ps)
{
	createExternalSlot(ps, -1);
}

void CartridgeSlotManager::createExternalSlot(int ps, int ss)
{
	if (isExternalSlot(ps, ss, false)) {
		throw MSXException("Slot is already an external slot.");
	}
	for (auto slot : xrange(MAX_SLOTS)) {
		if (!slots[slot].exists()) {
			slots[slot].ps = ps;
			slots[slot].ss = ss;
			char slotName[] = "carta";
			slotName[4] += slot;
			motherBoard.getMSXCliComm().update(
				CliComm::HARDWARE, slotName, "add");
			slots[slot].command = make_unique<CartCmd>(
				*this, motherBoard, slotName);
			return;
		}
	}
	UNREACHABLE;
}


int CartridgeSlotManager::getSlot(int ps, int ss) const
{
	for (auto slot : xrange(MAX_SLOTS)) {
		if (slots[slot].exists() &&
		    (slots[slot].ps == ps) && (slots[slot].ss == ss)) {
			return slot;
		}
	}
	UNREACHABLE; // was not an external slot
	return 0; // avoid warning
}

void CartridgeSlotManager::testRemoveExternalSlot(
	int ps, const HardwareConfig& allowed) const
{
	testRemoveExternalSlot(ps, -1, allowed);
}

void CartridgeSlotManager::testRemoveExternalSlot(
	int ps, int ss, const HardwareConfig& allowed) const
{
	int slot = getSlot(ps, ss);
	if (slots[slot].used(&allowed)) {
		throw MSXException("Slot still in use.");
	}
}

void CartridgeSlotManager::removeExternalSlot(int ps)
{
	removeExternalSlot(ps, -1);
}

void CartridgeSlotManager::removeExternalSlot(int ps, int ss)
{
	int slot = getSlot(ps, ss);
	assert(!slots[slot].used());
	const auto& slotName = slots[slot].command->getName();
	motherBoard.getMSXCliComm().update(
		CliComm::HARDWARE, slotName, "remove");
	slots[slot].command.reset();
}

void CartridgeSlotManager::getSpecificSlot(unsigned slot, int& ps, int& ss) const
{
	assert(slot < MAX_SLOTS);
	if (!slots[slot].exists()) {
		throw MSXException(StringOp::Builder() <<
			"slot-" << char('a' + slot) << " not defined.");
	}
	if (slots[slot].used()) {
		throw MSXException(StringOp::Builder() <<
			"slot-" << char('a' + slot) << " already in use.");
	}
	ps = slots[slot].ps;
	ss = slots[slot].ss;
}

void CartridgeSlotManager::getAnyFreeSlot(int& ps, int& ss) const
{
	// search for the lowest free slot
	ps = 4; // mark no free slot
	for (auto slot : xrange(MAX_SLOTS)) {
		if (slots[slot].exists() && !slots[slot].used()) {
			int p = slots[slot].ps;
			int s = slots[slot].ss;
			if ((p < ps) || ((p == ps) && (s < ss))) {
				ps = p;
				ss = s;
			}
		}
	}
	if (ps == 4) {
		throw MSXException("Not enough free cartridge slots");
	}
}

void CartridgeSlotManager::allocatePrimarySlot(
		int& ps, const HardwareConfig& hwConfig)
{
	for (auto slot : xrange(MAX_SLOTS)) {
		ps = slots[slot].ps;
		if (slots[slot].exists() && (slots[slot].ss == -1) &&
		    !slots[slot].used()) {
			assert(slots[slot].useCount == 0);
			slots[slot].config = &hwConfig;
			slots[slot].useCount = 1;
			return;
		}
	}
	throw MSXException("No free primary slot");
}

void CartridgeSlotManager::freePrimarySlot(
		int ps, const HardwareConfig& hwConfig)
{
	int slot = getSlot(ps, -1);
	assert(slots[slot].config == &hwConfig); (void)hwConfig;
	assert(slots[slot].useCount == 1);
	slots[slot].config = nullptr;
	slots[slot].useCount = 0;
}

void CartridgeSlotManager::allocateSlot(
		int ps, int ss, const HardwareConfig& hwConfig)
{
	for (auto slot : xrange(MAX_SLOTS)) {
		if (!slots[slot].exists()) continue;
		if ((slots[slot].ps == ps) && (slots[slot].ss == ss)) {
			if (slots[slot].useCount == 0) {
				slots[slot].config = &hwConfig;
			} else {
				if (slots[slot].config != &hwConfig) {
					throw MSXException(StringOp::Builder()
						<< "Slot " << ps << '-' << ss
						<< " already in use by "
						<< slots[slot].config->getName());
				}
			}
			++slots[slot].useCount;
		}
	}
	// Slot not found, was not an external slot. No problem.
}

void CartridgeSlotManager::freeSlot(
		int ps, int ss, const HardwareConfig& hwConfig)
{
	for (auto slot : xrange(MAX_SLOTS)) {
		if (!slots[slot].exists()) continue;
		if ((slots[slot].ps == ps) && (slots[slot].ss == ss)) {
			assert(slots[slot].config == &hwConfig); (void)hwConfig;
			assert(slots[slot].useCount > 0);
			--slots[slot].useCount;
			if (slots[slot].useCount == 0) {
				slots[slot].config = nullptr;
			}
			return;
		}
	}
	// Slot not found, was not an external slot. No problem.
}

bool CartridgeSlotManager::isExternalSlot(int ps, int ss, bool convert) const
{
	for (auto slot : xrange(MAX_SLOTS)) {
		int tmp = (convert && (slots[slot].ss == -1)) ? 0 : slots[slot].ss;
		if (slots[slot].exists() &&
		    (slots[slot].ps == ps) && (tmp == ss)) {
			return true;
		}
	}
	return false;
}


// CartCmd
CartCmd::CartCmd(CartridgeSlotManager& manager_, MSXMotherBoard& motherBoard,
                 string_ref commandName)
	: RecordedCommand(motherBoard.getCommandController(),
	                  motherBoard.getStateChangeDistributor(),
	                  motherBoard.getScheduler(),
	                  commandName)
	, manager(manager_)
	, cliComm(motherBoard.getMSXCliComm())
{
}

const HardwareConfig* CartCmd::getExtensionConfig(string_ref cartname)
{
	if (cartname.size() != 5) {
		throw SyntaxError();
	}
	int slot = cartname[4] - 'a';
	return manager.slots[slot].config;
}

string CartCmd::execute(const vector<string>& tokens, EmuTime::param /*time*/)
{
	string result;
	string_ref cartname = tokens[0];

	// strip namespace qualification
	//  TODO investigate whether it's a good idea to strip namespace at a
	//       higher level for all commands. How does that interact with
	//       the event recording feature?
	auto pos = cartname.rfind("::");
	if (pos != string_ref::npos) {
		cartname = cartname.substr(pos + 2);
	}
	if (tokens.size() == 1) {
		// query name of cartridge
		auto* extConf = getExtensionConfig(cartname);
		TclObject object(getInterpreter());
		object.addListElement(cartname + ':');
		object.addListElement(extConf ? extConf->getName() : "");
		TclObject options(getInterpreter());
		if (!extConf) {
			options.addListElement("empty");
		}
		if (options.getListLength() != 0) {
			object.addListElement(options);
		}
		result = object.getString().str();
	} else if ( (tokens[1] == "eject") || (tokens[1] == "-eject") ) {
		// remove cartridge (or extension)
		if (tokens[1] == "-eject") {
			result = "Warning: use of '-eject' is deprecated, "
			         "instead use the 'eject' subcommand";
		}
		if (auto* extConf = getExtensionConfig(cartname)) {
			try {
				manager.motherBoard.removeExtension(*extConf);
				cliComm.update(CliComm::MEDIA, cartname, "");
			} catch (MSXException& e) {
				throw CommandException("Can't remove cartridge: " +
				                       e.getMessage());
			}
		}
	} else {
		// insert cartridge
		auto slotname = (cartname.size() == 5)
			? string(1, cartname[4])
			: "any";
		size_t extensionNameToken = 1;
		if (tokens[1] == "insert") {
			if (tokens.size() > 2) {
				extensionNameToken = 2;
			} else {
				throw CommandException("Missing argument to insert subcommand");
			}
		}
		vector<string> options(tokens.begin() + extensionNameToken + 1,
		                       tokens.end());
		try {
			const auto& romname = tokens[extensionNameToken];
			auto extension = HardwareConfig::createRomConfig(
				manager.motherBoard, romname, slotname, options);
			if (slotname != "any") {
				if (auto* extConf = getExtensionConfig(cartname)) {
					// still a cartridge inserted, (try to) remove it now
					manager.motherBoard.removeExtension(*extConf);
				}
			}
			result = manager.motherBoard.insertExtension(
				"ROM", std::move(extension));
			cliComm.update(CliComm::MEDIA, cartname, romname);
		} catch (MSXException& e) {
			throw CommandException(e.getMessage());
		}
	}
	return result;
}

string CartCmd::help(const vector<string>& tokens) const
{
	return tokens[0] + " eject              : remove the ROM cartridge from this slot\n" +
	       tokens[0] + " insert <filename>  : insert ROM cartridge with <filename>\n" +
	       tokens[0] + " <filename>         : insert ROM cartridge with <filename>\n" +
	       tokens[0] + "                    : show which ROM cartridge is in this slot";
}

void CartCmd::tabCompletion(vector<string>& tokens) const
{
	vector<const char*> extra;
	if (tokens.size() < 3) {
		extra = { "eject", "insert" };
	}
	completeFileName(tokens, UserFileContext(), extra);
}

bool CartCmd::needRecord(const vector<string>& tokens) const
{
	return tokens.size() > 1;
}


// class CartridgeSlotInfo

CartridgeSlotInfo::CartridgeSlotInfo(InfoCommand& machineInfoCommand,
                                   CartridgeSlotManager& manager_)
	: InfoTopic(machineInfoCommand, "external_slot")
	, manager(manager_)
{
}

void CartridgeSlotInfo::execute(const vector<TclObject>& tokens,
                               TclObject& result) const
{
	switch (tokens.size()) {
	case 2: {
		// return list of slots
		string slot = "slotX";
		for (auto i : xrange(CartridgeSlotManager::MAX_SLOTS)) {
			if (!manager.slots[i].exists()) continue;
			slot[4] = char('a' + i);
			result.addListElement(slot);
		}
		break;
	}
	case 3: {
		// return info on a particular slot
		const auto& name = tokens[2].getString();
		if ((name.size() != 5) || (!name.starts_with("slot"))) {
			throw CommandException("Invalid slot name: " + name);
		}
		unsigned num = name[4] - 'a';
		if (num >= CartridgeSlotManager::MAX_SLOTS) {
			throw CommandException("Invalid slot name: " + name);
		}
		auto& slot = manager.slots[num];
		if (!slot.exists()) {
			throw CommandException("Slot '" + name + "' doesn't currently exist in this msx machine.");
		}
		result.addListElement(slot.ps);
		if (slot.ss == -1) {
			result.addListElement("X");
		} else {
			result.addListElement(slot.ss);
		}
		if (slot.config) {
			result.addListElement(slot.config->getName());
		} else {
			result.addListElement("");
		}
		break;
	}
	default:
		throw SyntaxError();
	}
}

string CartridgeSlotInfo::help(const vector<string>& /*tokens*/) const
{
	return "Without argument: show list of available external slots.\n"
	       "With argument: show primary and secondary slot number for "
	       "given external slot.\n";
}

} // namespace openmsx
