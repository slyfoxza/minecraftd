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
#include "jvm.h"

using namespace minecraftd;

JavaException::JavaException(JNIEnv *jni, bool clearException) {

	jthrowable throwable = jni->ExceptionOccurred();
	if(throwable == nullptr) {
		throw std::logic_error("JVM does not have exception currently being thrown");
	}

	jclass Throwable = jni->GetObjectClass(throwable);
	jmethodID Throwable_getClass = jni->GetMethodID(Throwable, "getClass", "()Ljava/lang/Class;");
	if(Throwable_getClass == nullptr) {
		throw std::logic_error("Unable to get throwable's getClass method identifier");
	}

	jclass Class = jni->FindClass("java/lang/Class");
	if(Class == nullptr) {
		jni->ExceptionDescribe();
		throw std::logic_error("Unable to find java.lang.Class class");
	}

	jmethodID Class_getName = jni->GetMethodID(Class, "getName", "()Ljava/lang/String;");
	if(Class_getName == nullptr) {
		jni->ExceptionDescribe();
		throw std::logic_error("Unable to get Class.getName method identifier");
	}

	jobject Class_Throwable = jni->CallObjectMethod(throwable, Throwable_getClass);
	jstring simpleName = static_cast<jstring>(jni->CallObjectMethod(Class_Throwable, Class_getName));
	if(simpleName == nullptr) {
		jni->ExceptionDescribe();
		throw std::logic_error("Call to Class.getName failed");
	}

	const char *simpleNameChars = jni->GetStringUTFChars(simpleName, nullptr);
	if(simpleNameChars == nullptr) {
		jni->ExceptionDescribe();
		throw std::logic_error("Failed to retrieve UTF-8 buffer of class' simple name");
	}
	type_ = simpleNameChars;
	jni->ReleaseStringUTFChars(simpleName, simpleNameChars);

	jmethodID Throwable_getMessage = jni->GetMethodID(Throwable, "getMessage", "()Ljava/lang/String;");
	if(Throwable_getMessage == nullptr) {
		jni->ExceptionDescribe();
		throw std::logic_error("Unable to get Throwable.getMessage method identifier");
	}

	jstring message = static_cast<jstring>(jni->CallObjectMethod(throwable, Throwable_getMessage));
	if(message == nullptr) {
		jni->ExceptionDescribe();
		throw std::logic_error("Call to Throwable.getMessage failed");
	}

	const char *messageChars = jni->GetStringUTFChars(message, nullptr);
	if(messageChars == nullptr) {
		jni->ExceptionDescribe();
		throw std::logic_error("Failed to retrieve UTF-8 buffer of throwable message");
	}
	message_ = messageChars;
	jni->ReleaseStringUTFChars(message, messageChars);

	if(clearException) {
		jni->ExceptionClear();
	}
}

const char *JavaException::what() const noexcept(true) {

	return message_.c_str();
}
