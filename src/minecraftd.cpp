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
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

#include <giomm.h>
#include <glibmm.h>
#include <libconfig.h++>
#include <jni.h>

#include "JarReader.h"
#include "jvm.h"
#include "minecraftd-dbus.h"
#include "pipe.h"

namespace {
	const std::string DEFAULT_CONFIG_FILE_NAME{MINECRAFTDCONFDIR "/minecraftd.conf"};

	class EnsureConditionBroadcast {

		public:
			EnsureConditionBroadcast(pthread_cond_t *condition) : condition_(condition), signalled_(false) { }
			~EnsureConditionBroadcast() {

				signal();
			}

			void signal() {

				if(!signalled_) {
					pthread_cond_broadcast(condition_);
					signalled_ = true;
				}
			}

		private:
			pthread_cond_t *condition_;
			bool signalled_;
	};

	void jvmMain(minecraftd::JvmMainArguments *arguments) {

		EnsureConditionBroadcast ensureConditionBroadcast{&arguments->jvmCompleteCondition};

		void *libjvm = dlopen(arguments->libjvmPath.c_str(), RTLD_LAZY);
		if(libjvm == nullptr) {
			throw std::runtime_error{"Failed to load JVM dynamic library: " + std::string{dlerror()}};
		}

		minecraftd::JNI_CreateJavaVM JNI_CreateJavaVM = reinterpret_cast<minecraftd::JNI_CreateJavaVM>(dlsym(libjvm,
					"JNI_CreateJavaVM"));
		if(JNI_CreateJavaVM == nullptr) {
			throw std::runtime_error{"Failed to load JNI_CreateJavaVM function: " + std::string{dlerror()}};
		}

		JavaVMInitArgs jvmArguments;
		jvmArguments.version = JNI_VERSION_1_6;

		std::vector<JavaVMOption> jvmOptions;

		std::string classPath{"-Djava.class.path=" + arguments->jarPath};
		jvmOptions.push_back(JavaVMOption{const_cast<char*>(classPath.c_str()), nullptr});

		// For Oracle-based JVMs, give the process a name for tools such as jcmd and jconsole
		jvmOptions.push_back(JavaVMOption{const_cast<char*>("-Dsun.java.command=minecraftd"), nullptr});

		// jvmOptions.push_back(JavaVMOption{"-Dlog4j.configurationFile=../log4j2.xml", nullptr});

		for(auto argument: arguments->additionalArguments) {
			jvmOptions.push_back(JavaVMOption{const_cast<char*>(argument.c_str()), nullptr});
		}

		jvmArguments.options = jvmOptions.data();
		jvmArguments.nOptions = jvmOptions.size();
		jvmArguments.ignoreUnrecognized = false;

		JavaVM *jvm;
		JNIEnv *jni;
		jint jrc = JNI_CreateJavaVM(&jvm, reinterpret_cast<void**>(&jni), &jvmArguments);
		if(jrc != JNI_OK) {
			throw std::runtime_error{"Failed to create Java virtual machine"};
		}

		std::string mainClassSpec{arguments->mainClassName};
		for(size_t i = mainClassSpec.find('.'); i != std::string::npos; i = mainClassSpec.find('.', i)) {
			mainClassSpec[i] = '/';
		}

		jclass mainClass = jni->FindClass(mainClassSpec.c_str());
		if(mainClass == nullptr) {
			minecraftd::JavaException e{jni};
			if(e.type() == "java.lang.NoClassDefFoundError") {
				throw std::runtime_error{"Could not find main class: " + arguments->mainClassName};
			} else {
				throw e;
			}
		}

		jmethodID mainMethod = jni->GetStaticMethodID(mainClass, "main", "([Ljava/lang/String;)V");
		if(mainMethod == nullptr) {
			minecraftd::JavaException e{jni};
			if(e.type() == "java.lang.NoSuchMethodError") {
				throw std::runtime_error{"Could not find `void main(String[])' method in " + arguments->mainClassName};
			} else {
				throw e;
			}
		}

		jclass stringClass = jni->FindClass("java/lang/String");
		if(stringClass == nullptr) {
			throw minecraftd::JavaException{jni};
		}

		jobjectArray mainArguments = jni->NewObjectArray(1, stringClass, nullptr);
		if(mainArguments == nullptr) {
			throw minecraftd::JavaException{jni};
		}

		jstring noguiString = jni->NewStringUTF("nogui");
		if(noguiString == nullptr) {
			throw minecraftd::JavaException{jni};
		}

		jni->SetObjectArrayElement(mainArguments, 0, noguiString);
		if(jni->ExceptionCheck()) {
			throw minecraftd::JavaException{jni};
		}

		std::cout << "Signalling completion of JVM startup" << std::endl;
		ensureConditionBroadcast.signal();

		jni->CallStaticVoidMethod(mainClass, mainMethod, mainArguments);
		if(jni->ExceptionCheck()) {
			throw minecraftd::JavaException{jni};
		}
	}

	pthread_t mainThread;
	Glib::RefPtr<Glib::MainLoop> mainLoop;

	void onExit() {

		mainLoop->quit();
		std::cout << "Waiting for main thread to exit" << std::endl;
		pthread_join(mainThread, nullptr);
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

	std::string jarPath{MINECRAFTJARDIR "/minecraft_server.jar"};
	configFile.lookupValue("jar", jarPath);

	minecraftd::JarReader jarReader{jarPath};
	std::string mainClassName = jarReader.getMainClassName();

	minecraftd::PosixPipe pipe;

	if(close(STDIN_FILENO) != 0) {
		std::cerr << "Failed to close current standard input file descriptor" << std::endl;
		return 1;
	}

	if(dup2(pipe.readEnd(), STDIN_FILENO) != 0) {
		std::cerr << "Failed to duplicate read end of console pipe as standard input:" << errno << std::endl;
		return 1;
	}

	pthread_mutex_t mutex;
	if(pthread_mutex_init(&mutex, nullptr) != 0) {
		std::cerr << "Failed to create mutex object" << std::endl;
		return 1;
	}

	minecraftd::JvmMainArguments jvmMainArguments{"/etc/alternatives/jre/lib/amd64/server/libjvm.so", jarPath,
		mainClassName};
	if(pthread_cond_init(&jvmMainArguments.jvmCompleteCondition, nullptr) != 0) {
		std::cerr << "Failed to create JVM completion condition" << std::endl;
		return 1;
	}

	try {
		libconfig::Setting &additionalArguments = configFile.lookup("jvm.arguments");
		const int count = additionalArguments.getLength();
		for(int i = 0; i < count; ++i) {
			jvmMainArguments.additionalArguments.push_back(additionalArguments[i]);
		}
	} catch(libconfig::SettingNotFoundException&) {
		// No additional arguments to pass
	}

	mainThread = pthread_self();
	std::atexit(onExit);

	std::thread jvmMainThread(jvmMain, &jvmMainArguments);

	pthread_mutex_lock(&mutex);
	std::cout << "Waiting for JVM to complete startup" << std::endl;
	pthread_cond_wait(&jvmMainArguments.jvmCompleteCondition, &mutex);
	pthread_mutex_unlock(&mutex);

	Gio::init();
	minecraftd::Minecraftd1 dbusObject{"/net/za/slyfox/Minecraftd1", pipe};

	std::cout << "Starting main loop" << std::endl;
	mainLoop = Glib::MainLoop::create();
	mainLoop->run();

	jvmMainThread.join();
	std::cout << "And we're done!" << std::endl;
	return 0;
}

