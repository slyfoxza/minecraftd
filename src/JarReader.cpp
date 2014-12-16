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
#include <sstream>
#include <stdexcept>

#include "JarReader.h"

using namespace minecraftd;

JarReader::JarReader(const std::string &jarPath) : jarPath_(jarPath) {

	int rc;
	zip_ = zip_open(jarPath.c_str(), 0, &rc);
	if(zip_ == nullptr) {
		throw std::runtime_error{"Failed to open JAR " + jarPath};
	}
}

JarReader::~JarReader() {

	zip_discard(zip_);
}

std::string JarReader::getMainClassName() {

	zip_int64_t manifestIndex = zip_name_locate(zip_, "META-INF/MANIFEST.MF", 0);
	if(manifestIndex == -1) {
		throw std::runtime_error{jarPath_ + " does not contain a JAR manifest"};
	}

	struct zip_stat zip_stat;
	int rc = zip_stat_index(zip_, manifestIndex, 0, &zip_stat);
	if((rc == -1) || ((zip_stat.valid & ZIP_STAT_SIZE) == 0)) {
		throw std::runtime_error{"Unable to determine size of MANIFEST.MF"};
	}

	zip_file *manifest = zip_fopen_index(zip_, manifestIndex, 0);
	if(manifest == nullptr) {
		throw std::runtime_error{"Unable to open MANIFEST.MF"};
	}

	std::stringstream manifestBuffer;
	zip_uint64_t bytesRead = 0;
	while(bytesRead < zip_stat.size) {
		const zip_uint64_t bufferSize = 4096;
		char buffer[bufferSize];
		int thisBytesRead = zip_fread(manifest, buffer, std::min(bufferSize, zip_stat.size - bytesRead));
		if(thisBytesRead == -1) {
			throw std::runtime_error{"Failed to read MANIFEST.MF"};
		}
		manifestBuffer.write(buffer, thisBytesRead);
		bytesRead += thisBytesRead;
	}

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
		return line.substr(value, valueLength);
	}

	throw std::runtime_error{"No 'Main-Class' entry found in manifest"};
}
