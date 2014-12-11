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

namespace minecraftd {
	typedef jint (JNICALL *JNI_CreateJavaVM)(JavaVM **pvm, void  **penv, void *args);

	struct JvmMainArguments {

		JvmMainArguments(const std::string &libjvmPath_, const std::string &jarPath_, const std::string &mainClassName_)
			: jarPath{jarPath_}, libjvmPath{libjvmPath_}, mainClassName{mainClassName_} { }

		const std::string jarPath;
		pthread_cond_t jvmCompleteCondition;
		const std::string libjvmPath;
		const std::string mainClassName;
	};
}
