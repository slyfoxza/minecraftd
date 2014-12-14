/*
 * Copyright 2014 Philip Cronje
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */
#include <iostream>
#include <mutex>
#include <unordered_map>

#include "minecraftd-dbus.h"

using namespace minecraftd;

const Glib::ustring Minecraftd1::INTERFACE{"net.za.slyfox.Minecraftd1"};

namespace std {
	template<>
	struct hash<Glib::ustring> {

		size_t operator()(const Glib::ustring &value) const {

			size_t hash = 31;
			for(auto i: value) {
				hash = 31 * hash + i;
			}
			return hash;
		}
	};
}

namespace {
	const Glib::ustring INTROSPECTION_XML{
		"<node>\n"
		"\t<interface name='net.za.slyfox.Minecraftd1'>\n"
		"\t\t<method name='Stop' />\n"
		"\t\t<method name='SaveAll' />\n"
		"\t</interface>\n"
		"</node>"
	};

	std::unordered_map<Glib::ustring, sigc::slot<void, const Glib::RefPtr<Gio::DBus::MethodInvocation>&>> handlerMap{2};

	std::once_flag handlerMapInitFlag;

	void writeAll(const int fd, const std::string &buffer_) {

		const std::string::size_type size = buffer_.size();
		const char *buffer = buffer_.c_str();

		ssize_t totalWritten = 0;
		while(totalWritten < size) {
			ssize_t written = write(fd, buffer + totalWritten, size - totalWritten);
			if(written == -1) {
				throw std::system_error(errno, std::system_category());
			}
			totalWritten += written;
		}
	}
}

Minecraftd1::Minecraftd1(const Glib::ustring &objectName, const PosixPipe &pipe)
	: busName_{Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SYSTEM, INTERFACE,
			sigc::mem_fun(*this, &Minecraftd1::onBusAcquired))},
	introspectionData_{Gio::DBus::NodeInfo::create_for_xml(INTROSPECTION_XML)},
	objectName_{objectName},
	pipe_(pipe),
	vtable_{sigc::mem_fun(*this, &Minecraftd1::onMethodCall)} {

	std::call_once(handlerMapInitFlag, [this]{
		handlerMap["SaveAll"] = sigc::mem_fun(*this, &Minecraftd1::handleSaveAll);
		handlerMap["Stop"] = sigc::mem_fun(*this, &Minecraftd1::handleStop);

		for(size_t i = 0; i < handlerMap.bucket_count(); ++i) {
			if(handlerMap.bucket_size(i) > 1) {
				std::cout << "Warning: Collision detected in handler map bucket " << i << std::endl;
			}
		}
	});
}

Minecraftd1::~Minecraftd1() {

	Gio::DBus::unown_name(busName_);
}

void Minecraftd1::onBusAcquired(const Glib::RefPtr<Gio::DBus::Connection> &connection, const Glib::ustring &name) {

	connection->register_object(objectName_, introspectionData_->lookup_interface(), vtable_);
}

void Minecraftd1::onMethodCall(const Glib::RefPtr<Gio::DBus::Connection> &connection, const Glib::ustring &sender,
		const Glib::ustring &objectPath, const Glib::ustring &interfaceName, const Glib::ustring &methodName,
		const Glib::VariantContainerBase &parameters, const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {

	auto slot = handlerMap.find(methodName);
	if(slot != handlerMap.end()) {
		slot->second(invocation);
	} else {
		invocation->return_error(Gio::DBus::Error{Gio::DBus::Error::UNKNOWN_METHOD, "Method does not exist."});
	}
}

void Minecraftd1::handleSaveAll(const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {

	writeAll(pipe_.writeEnd(), "save-all\n");
	invocation->return_value(Glib::VariantContainerBase{});
}

void Minecraftd1::handleStop(const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {

	writeAll(pipe_.writeEnd(), "stop\n");
	invocation->return_value(Glib::VariantContainerBase{});
}
