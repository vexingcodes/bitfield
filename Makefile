# Used to run compile-time tests and generate the single header file for the bit field C++20 library.
#
# Targets:
#   bit_field.hpp (default): Generate the single header file version of the library.
#   clean: Remove the generated single header file if it exists.
#   test-multi: The compile-time tests, run against the multiple header version.
#   test-single: The compile-time tests, run against the single header file.
#   test: Run all tests.

# Determine the version number from the git environment.
TAG_COMMIT := $(shell git rev-list --abbrev-commit --tags --max-count=1)
TAG := $(shell git describe --abbrev=0 --tags ${TAG_COMMIT} 2>/dev/null || true)
COMMIT := $(shell git rev-parse --short HEAD)
DATE := $(shell git log -1 --format=%cd --date=format:"%Y%m%d")
VERSION := $(TAG:v%=%)
ifneq ($(COMMIT), $(TAG_COMMIT))
    VERSION := $(VERSION)-next-$(COMMIT)-$(DATE)
endif
ifeq ($(VERSION),)
    VERSION := $(COMMIT)-$(DATE)
endif
ifneq ($(shell git status --porcelain),)
    VERSION := $(VERSION)-dirty
endif

# Build a single header file from the several header files. This involves constructing a header containing some relevant
# information like the version number, the authorship, and the license file. After the comment header, all of the header
# files in the include directory are concatenated in dependency order (omitting local include statements).
bit_field.hpp: include/*.hpp
	@echo "/*"                                                                        > bit_field.hpp
	@echo "File: bit_field.hpp (generated header file)"                              >> bit_field.hpp
	@echo "Version: $(VERSION)"                                                      >> bit_field.hpp
	@echo ""                                                                         >> bit_field.hpp
	@echo "Copyright 2021 WinterWinds Robotics, Inc."                                >> bit_field.hpp
	@echo ""                                                                         >> bit_field.hpp
	@echo "Licensed under the Apache License, Version 2.0 (the "License");"          >> bit_field.hpp
	@echo "you may not use this file except in compliance with the License."         >> bit_field.hpp
	@echo "You may obtain a copy of the License at"                                  >> bit_field.hpp
	@echo ""                                                                         >> bit_field.hpp
	@echo "    http://www.apache.org/licenses/LICENSE-2.0"                           >> bit_field.hpp
	@echo ""                                                                         >> bit_field.hpp
	@echo "Unless required by applicable law or agreed to in writing, software"      >> bit_field.hpp
	@echo "distributed under the License is distributed on an "AS IS" BASIS,"        >> bit_field.hpp
	@echo "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied." >> bit_field.hpp
	@echo "See the License for the specific language governing permissions and"      >> bit_field.hpp
	@echo "limitations under the License."                                           >> bit_field.hpp
	@echo "*/"                                                                       >> bit_field.hpp
	@echo ""                                                                         >> bit_field.hpp
	
	@cat include/config.hpp            \
	     include/bits.hpp              \
	     include/bit_field.hpp         \
	     include/counter.hpp           \
	     include/bit_field_builder.hpp \
	   | sed '/^#include "/d' >> bit_field.hpp

CFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Warith-conversion -Wshadow -Werror
COMPILE_TIME_TESTS = test/bits_test.cpp test/bit_field_test.cpp test/counter_test.cpp test/bit_field_builder_test.cpp

# Run all of the compile-time tests against the separate header files.
.PHONY: test-multi
test-multi:
	$(foreach test_file, $(COMPILE_TIME_TESTS), \
	    $(shell $(CXX) $(CFLAGS) -I./include -o /dev/null -c $(test_file)))

# Run all of the compile-time tests against the separate header files with exception support disabled.
.PHONY: test-multi-noexcept
test-multi-noexcept:
	$(foreach test_file, $(COMPILE_TIME_TESTS), \
	    $(shell $(CXX) $(CFLAGS) -fno-exceptions -I./include -o /dev/null -c $(test_file)))

# Run all of the compile-time tests against the single header file.
.PHONY: test-single
test-single: bit_field.hpp
	$(foreach test_file, $(COMPILE_TIME_TESTS), \
	    $(shell $(CXX) $(CFLAGS) -I. -DBIT_FIELD_TEST_SINGLE_HEADER -o /dev/null -c $(test_file)))

# Run all of the compile-time tests against the single header file with exception support disabled.
.PHONY: test-single-noexcept
test-single-noexcept: bit_field.hpp
	$(foreach test_file, $(COMPILE_TIME_TESTS), \
	    $(shell $(CXX) $(CFLAGS) -fno-exceptions -I. -DBIT_FIELD_TEST_SINGLE_HEADER -o /dev/null -c $(test_file)))

.PHONY: test
test: test-multi test-multi-noexcept test-single test-single-noexcept
	@echo "Tests passed."

.PHONY: clean
clean:
	rm --force bit_field.hpp
