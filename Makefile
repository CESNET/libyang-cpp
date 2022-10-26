.DEFAULT_GOAL := help
SHELL := /bin/bash
EMSCRIPTEN_VERSION := 3.1.20
LIBPCRE_VERSION := 10.39
LIBYANG_VERSION := 2.0.231
DOCTEST_VERSION := 2.4.9

install: lib-pcre lib-yang

lib-pcre:
	@echo "Downloading and rebuilding PCRE library"
	export PKG_CONFIG_PATH=$(pkg-config --variable pc_path pkg-config) \
		&& cd ./libs/ \
		&& rm -f ./pcre2-${LIBPCRE_VERSION}.zip \
		&& rm -rf ./pcre2-${LIBPCRE_VERSION}/ \
		&& wget https://github.com/PhilipHazel/pcre2/releases/download/pcre2-${LIBPCRE_VERSION}/pcre2-${LIBPCRE_VERSION}.zip \
		&& unzip ./pcre2-${LIBPCRE_VERSION}.zip \
		&& cd ./pcre2-${LIBPCRE_VERSION}/ \
		&& ./configure --enable-utf --enable-unicode-properties \
		&& mkdir ./build/ \
		&& cd ./build/ \
		&& cmake .. \
		&& make \
		&& make install

doctest:
	@echo "Downloading and rebuilding doctest library"
	export PKG_CONFIG_PATH=$(pkg-config --variable pc_path pkg-config) \
		&& cd ./libs/ \
		&& rm -f ./doctest-${DOCTEST_VERSION}.zip \
		&& rm -rf ./doctest-${DOCTEST_VERSION}/ \
		&& wget https://github.com/doctest/doctest/archive/v${DOCTEST_VERSION}.zip --output-document=doctest-${DOCTEST_VERSION}.zip \
		&& unzip ./doctest-${DOCTEST_VERSION}.zip \
		&& cd ./doctest-${DOCTEST_VERSION}/ \
		&& mkdir ./build/ \
		&& cd ./build/ \
		&& cmake .. \
		&& make \
		&& make install

lib-yang:
	@echo "Downloading and rebuilding YANG library (C)"
	export PKG_CONFIG_PATH=$(pkg-config --variable pc_path pkg-config):$(/usr/local/cmake):$(realpath ./libs/pcre2-${LIBPCRE_VERSION}/build/) \
		&& cd ./libs/ \
		&& rm -f ./libyang-${LIBYANG_VERSION}.zip \
		&& rm -rf ./libyang-${LIBYANG_VERSION}/ \
		&& wget https://github.com/CESNET/libyang/archive/v${LIBYANG_VERSION}.zip --output-document=libyang-${LIBYANG_VERSION}.zip \
		&& unzip ./libyang-${LIBYANG_VERSION}.zip \
		&& cd ./libyang-${LIBYANG_VERSION}/ \
		&& mkdir ./build/ \
		&& cd ./build/ \
		&& cmake \
			-DCMAKE_BUILD_TYPE:String="Release" .. \
		&& make \
		&& make install

build-app:
	@echo "Building the app"
		export PKG_CONFIG_PATH=$(pkg-config --variable pc_path pkg-config):$(realpath ./libs/pcre2-${LIBPCRE_VERSION}/build/):$(realpath ./libs/libyang-${LIBYANG_VERSION}/build/) \
			&& rm -rf ./cmake/ \
			&& mkdir ./cmake/ \
		  	&& cd ./cmake/ \
			&& cmake .. \
			&& make \
			&& make install
