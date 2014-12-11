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
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

#include <giomm.h>
#include <glibmm.h>
#include <libconfig.h++>
#include <jni.h>
#include <zip.h>

namespace {
	typedef jint (JNICALL *JNI_CreateJavaVM_f)(JavaVM **pvm, void  **penv, void *args);

	const std::string DEFAULT_CONFIG_FILE_NAME{SYSCONFDIR "/minecraftd.conf"};
	const Glib::ustring DBUS_INTROSPECTION_XML{
		"<node>"
		"\t<interface name='net.za.slyfox.Minecraftd1'>"
		"\t\t<method name='Stop' />"
		"\t\t<method name='SaveAll' />"
		"\t</interface>"
		"</node>"
	};

	struct JvmMainArguments {

		JvmMainArguments(const std::string &libjvmPath_, const std::string &jarPath_, const std::string &mainClassName_) : jarPath{jarPath_}, libjvmPath{libjvmPath_}, mainClassName{mainClassName_} { }

		std::string jarPath;
		pthread_cond_t jvmCompleteCondition;
		std::string libjvmPath;
		std::string mainClassName;
	};

	void *jvmMain(void *arguments_) {

		JvmMainArguments *arguments = reinterpret_cast<JvmMainArguments*>(arguments_);

		void *libjvm = dlopen(arguments->libjvmPath.c_str(), RTLD_LAZY);
		if(libjvm == nullptr) {
			std::cerr << "Failed to open libjvm.so: " << dlerror() << std::endl;
			pthread_cond_broadcast(&arguments->jvmCompleteCondition);
			return nullptr;
		}

		JNI_CreateJavaVM_f JNI_CreateJavaVM = reinterpret_cast<JNI_CreateJavaVM_f>(dlsym(libjvm, "JNI_CreateJavaVM"));
		if(JNI_CreateJavaVM == nullptr) {
			std::cerr << "Failed to load JNI_CreateJavaVM function: " << dlerror() << std::endl;
			pthread_cond_broadcast(&arguments->jvmCompleteCondition);
			return nullptr;
		}

		JavaVMInitArgs jvmArguments;
		jvmArguments.version = JNI_VERSION_1_6;

		std::vector<JavaVMOption> jvmOptions;

		std::string classPath{"-Djava.class.path=" + arguments->jarPath};
		jvmOptions.push_back(JavaVMOption{const_cast<char*>(classPath.c_str()), nullptr});

		jvmOptions.push_back(JavaVMOption{"-Dlog4j.configurationFile=../log4j2.xml", nullptr});

		jvmArguments.options = jvmOptions.data();
		jvmArguments.nOptions = jvmOptions.size();
		jvmArguments.ignoreUnrecognized = false;

		JavaVM *jvm;
		JNIEnv *jni;
		jint jrc = JNI_CreateJavaVM(&jvm, reinterpret_cast<void**>(&jni), &jvmArguments);
		if(jrc != JNI_OK) {
			std::cerr << "Failed to create Java virtual machine: " << jrc << std::endl;
			pthread_cond_broadcast(&arguments->jvmCompleteCondition);
			return nullptr;
		}

		std::string mainClassSpec{arguments->mainClassName};
		for(int i = mainClassSpec.find('.'); i != std::string::npos; i = mainClassSpec.find('.', i)) {
			mainClassSpec[i] = '/';
		}

		jclass mainClass = jni->FindClass(mainClassSpec.c_str());
		if(mainClass == nullptr) {
			std::cerr << "Failed to find " << arguments->mainClassName << " class" << std::endl;
			jni->ExceptionDescribe();
			pthread_cond_broadcast(&arguments->jvmCompleteCondition);
			return nullptr;
		}

		jmethodID mainMethod = jni->GetStaticMethodID(mainClass, "main", "([Ljava/lang/String;)V");
		if(mainMethod == nullptr) {
			std::cerr << "Failed to find " << arguments->mainClassName << "#main method" << std::endl;
			jni->ExceptionDescribe();
			pthread_cond_broadcast(&arguments->jvmCompleteCondition);
			return nullptr;
		}

		jclass stringClass = jni->FindClass("java/lang/String");
		if(stringClass == nullptr) {
			std::cerr << "Failed to find java.lang.String class" << std::endl;
			jni->ExceptionDescribe();
			pthread_cond_broadcast(&arguments->jvmCompleteCondition);
			return nullptr;
		}

		jobjectArray mainArguments = jni->NewObjectArray(1, stringClass, nullptr);
		if(mainArguments == nullptr) {
			std::cerr << "Failed to instantiate main arguments array" << std::endl;
			jni->ExceptionDescribe();
			pthread_cond_broadcast(&arguments->jvmCompleteCondition);
			return nullptr;
		}

		jstring noguiString = jni->NewStringUTF("nogui");
		if(noguiString == nullptr) {
			std::cerr << "Failed to create string object for 'nogui'" << std::endl;
			jni->ExceptionDescribe();
			pthread_cond_broadcast(&arguments->jvmCompleteCondition);
			return nullptr;
		}

		jni->SetObjectArrayElement(mainArguments, 0, noguiString);
		if(jni->ExceptionCheck()) {
			std::cerr << "Failed to populate main arguments array" << std::endl;
			jni->ExceptionDescribe();
			pthread_cond_broadcast(&arguments->jvmCompleteCondition);
			return nullptr;
		}

		std::cout << "Signalling completion of JVM startup" << std::endl;
		pthread_cond_broadcast(&arguments->jvmCompleteCondition);

		jni->CallStaticVoidMethod(mainClass, mainMethod, mainArguments);
		if(jni->ExceptionCheck()) {
			std::cerr << "Java main thread threw uncaught exception" << std::endl;
			jni->ExceptionDescribe();
		}

		return nullptr;
	}

	Glib::RefPtr<Glib::MainLoop> mainLoop;

	void onExit() {

		mainLoop->quit();
		while(mainLoop->is_running());
	}

	// [0] - read, [1] - write
	int consolePipe[2];

	void onDBusMethodCall(const Glib::RefPtr<Gio::DBus::Connection> &connection, const Glib::ustring &sender,
			const Glib::ustring &objectPath, const Glib::ustring &interfaceName, const Glib::ustring &methodName,
			const Glib::VariantContainerBase &parameters,
			const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {

		std::cout << "Received method call: sender=" << sender << ", objectPath=" << objectPath << ", interfaceName="
			<< interfaceName << ", methodName=" << methodName << std::endl;
		if(methodName == "SaveAll") {
			std::cout << "Received save-all command" << std::endl;
			static const char buffer[] = "save-all\n";
			ssize_t totalWritten = 0;
			while(totalWritten < sizeof(buffer)) {
				ssize_t written = write(consolePipe[1], buffer + totalWritten, sizeof(buffer) - totalWritten);
				totalWritten += written;
			}
			invocation->return_error(Gio::DBus::Error{Gio::DBus::Error::UNKNOWN_METHOD, "Method does not exist."});
		} else if(methodName == "Stop") {
			std::cout << "Received stop command" << std::endl;
			static const char buffer[] = "stop\n";
			ssize_t totalWritten = 0;
			while(totalWritten < sizeof(buffer)) {
				ssize_t written = write(consolePipe[1], buffer + totalWritten, sizeof(buffer) - totalWritten);
				totalWritten += written;
			}
			invocation->return_error(Gio::DBus::Error{Gio::DBus::Error::UNKNOWN_METHOD, "Method does not exist."});
		} else {
			invocation->return_error(Gio::DBus::Error{Gio::DBus::Error::UNKNOWN_METHOD, "Method does not exist."});
		}
	}

	Glib::RefPtr<Gio::DBus::NodeInfo> introspectionData;
	const Gio::DBus::InterfaceVTable dbusVTable{sigc::ptr_fun(&onDBusMethodCall)};

	void onBusAcquired(const Glib::RefPtr<Gio::DBus::Connection> &connection, const Glib::ustring &name) {

		std::cout << "Acquired bus with name " << name << std::endl;
		try {
			connection->register_object("/net/za/slyfox/Minecraftd1", introspectionData->lookup_interface(),
					dbusVTable);
			std::cout << "Registered object" << std::endl;
		} catch(const Glib::Error &e) {
			std::cerr << "Failed to register object: " << e.what() << std::endl;
		}
	}
}

int main(int argc, char **argv) {

	std::vector<std::string> arguments{argv + 1, argv + argc};
	std::string configFileName{DEFAULT_CONFIG_FILE_NAME};
	for(auto it = arguments.cbegin(); it != arguments.cend(); ++it) {
		if((*it == "--help") || (*it == "-h") || (*it == "-?")) {
			std::cout << "Usage: " << argv[0] << " [options]" << std::endl << std::endl
				<< "  --help/-h/-?\t\tPrints this message" << std::endl
				<< "  --config/-c <path>\tSpecifies an alternate configuration file (default:" << std::endl
				<< "  \t" << DEFAULT_CONFIG_FILE_NAME << ')' << std::endl;
			return 0;
		} else if((*it == "--config") || (*it == "-c")) {
			if(++it == arguments.cend()) {
				std::cerr << "Error: --config requires a path argument" << std::endl;
				return 1;
			}
			configFileName = *it;
		}
	}

	libconfig::Config configFile;
	try {
		configFile.readFile(configFileName.c_str());
	} catch(const libconfig::FileIOException &e) {
		std::cerr << "Failed to read from " << configFileName << ": " << e.what() << std::endl;
		return 1;
	} catch(const libconfig::ParseException &e) {
		std::cerr << "Failed to parse " << configFileName << ": " << e.what() << std::endl;
		return 1;
	}

	std::string jarPath{GAMESDIR "/minecraft_server.jar"};
	configFile.lookupValue("jar", jarPath);

	int rc;
	zip *zip = zip_open(jarPath.c_str(), 0, &rc);
	if(zip == nullptr) {
		std::cerr << "Failed to open JAR " << jarPath << ": " << rc << std::endl;
		return 1;
	}

	zip_int64_t manifestIndex = zip_name_locate(zip, "META-INF/MANIFEST.MF", 0);
	if(manifestIndex == -1) {
		std::cerr << "JAR " << jarPath << " does not contain a manifest file" << std::endl;
		return 1;
	}

	struct zip_stat zip_stat;
	rc = zip_stat_index(zip, manifestIndex, 0, &zip_stat);
	if((rc == -1) || ((zip_stat.valid & ZIP_STAT_SIZE) == 0)) {
		std::cerr << "Could not determine MANIFEST.MF file size" << std::endl;
		return 1;
	}

	zip_file *manifest = zip_fopen_index(zip, manifestIndex, 0);
	if(manifest == nullptr) {
		std::cerr << "Could not open MANIFEST.MF file" << std::endl;
		return 1;
	}

	std::stringstream manifestBuffer;
	zip_uint64_t bytesRead = 0;
	while(bytesRead < zip_stat.size) {
		const zip_uint64_t bufferSize = 4096;
		char buffer[bufferSize];
		int thisBytesRead = zip_fread(manifest, buffer, std::min(bufferSize, zip_stat.size - bytesRead));
		if(thisBytesRead == -1) {
			std::cerr << "Failed to read MANIFEST.MF file" << std::endl;
			return 1;
		}
		manifestBuffer.write(buffer, thisBytesRead);
		bytesRead += thisBytesRead;
	}

	std::string mainClassName;
	for(std::string line; std::getline(manifestBuffer, line);) {
		auto separator = line.find_first_of(':');
		if(line.substr(0, separator) != "Main-Class") {
			continue;
		}

		auto value = line.find_first_not_of(" \t", separator + 1);
		auto valueLength = std::string::npos;
		auto lineEnd = line.find_first_of(" \t\r\n", value);
		if(lineEnd != std::string::npos) {
			valueLength = lineEnd - value;
		}
		mainClassName = line.substr(value, valueLength);
	}

	rc = pipe(consolePipe);
	if(rc != 0) {
		std::cerr << "Failed to create console pipe" << std::endl;
		return 1;
	}

	if(close(STDIN_FILENO) != 0) {
		std::cerr << "Failed to close current standard input file descriptor" << std::endl;
		return 1;
	}

	if(dup2(consolePipe[0], STDIN_FILENO) != 0) {
		std::cerr << "Failed to duplicate read end of console pipe as standard input" << std::endl;
		return 1;
	}

	pthread_mutex_t mutex;
	if(pthread_mutex_init(&mutex, nullptr) != 0) {
		std::cerr << "Failed to create mutex object" << std::endl;
		return 1;
	}

	JvmMainArguments jvmMainArguments{"/etc/alternatives/jre/lib/amd64/server/libjvm.so", jarPath, mainClassName};
	if(pthread_cond_init(&jvmMainArguments.jvmCompleteCondition, nullptr) != 0) {
		std::cerr << "Failed to create JVM completion condition" << std::endl;
		return 1;
	}

	pthread_t jvmMainThread;
	if(pthread_create(&jvmMainThread, nullptr, jvmMain, &jvmMainArguments) != 0) {
		std::cerr << "Failed to create JVM main thread" << std::endl;
		return 1;
	}

	std::atexit(onExit);

	pthread_mutex_lock(&mutex);
	std::cout << "Waiting for JVM to complete startup" << std::endl;
	pthread_cond_wait(&jvmMainArguments.jvmCompleteCondition, &mutex);
	pthread_mutex_unlock(&mutex);

	Gio::init();
	introspectionData = Gio::DBus::NodeInfo::create_for_xml(DBUS_INTROSPECTION_XML);
	guint busName = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SYSTEM, "net.za.slyfox.Minecraftd1",
			sigc::ptr_fun(&onBusAcquired));

	std::cout << "Starting main loop" << std::endl;
	mainLoop = Glib::MainLoop::create();
	mainLoop->run();

	std::cout << "And we're done!" << std::endl;
	return 0;
}

