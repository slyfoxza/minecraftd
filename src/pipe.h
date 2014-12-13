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

#include <cerrno>
#include <system_error>

#include <unistd.h>

namespace minecraftd {
	class PosixPipe {
		public:
			PosixPipe() {
				static_assert(&writeEnd_ == (&readEnd_ + 1), "PosixPipe members must be packed");
				if(pipe(&readEnd_) != 0) {
					throw std::system_error(errno, std::system_category());
				}
			}

			PosixPipe(const PosixPipe&) = delete;

			~PosixPipe() {
				if((close(readEnd_) != 0) || (close(writeEnd_) != 0)) {
					throw std::system_error(errno, std::system_category());
				}
			}

			int readEnd() const { return readEnd_; }
			int writeEnd() const { return writeEnd_; }

		private:
			int readEnd_, writeEnd_;
	};
}
