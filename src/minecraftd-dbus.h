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
#pragma once

#include <giomm.h>
#include <glibmm.h>

#include "pipe.h"

namespace minecraftd {
	class Minecraftd1 {
		public:
			Minecraftd1(const Glib::ustring &objectName, const PosixPipe &pipe);
			~Minecraftd1();

		private:
			void onBusAcquired(const Glib::RefPtr<Gio::DBus::Connection> &connection, const Glib::ustring &name);
			void onMethodCall(const Glib::RefPtr<Gio::DBus::Connection> &connection, const Glib::ustring &sender,
					const Glib::ustring &objectPath, const Glib::ustring &interfaceName,
					const Glib::ustring &methodName, const Glib::VariantContainerBase &parameters,
					const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);

			void handleSaveAll(const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);
			void handleStop(const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);

			static const Glib::ustring INTERFACE;

			guint busName_;
			Glib::RefPtr<Gio::DBus::NodeInfo> introspectionData_;
			Glib::ustring objectName_;
			const PosixPipe &pipe_;
			const Gio::DBus::InterfaceVTable vtable_;
	};
}

