# System vars
BUILD_DIR ?= ./build
CWD = `pwd`

XLA_TARGET ?= cpu
XLA_TARGET_PLATFORM ?= aarch64-darwin

BAZEL_BUILD_FLAGS = --enable_workspace \
	--experimental_cc_static_library
	# --show_progress \
	# --worker_verbose \
	# --verbose_failures \
	# --subcommands

BAZEL_TARGET = xla/extension:tarball
BAZEL_FLAGS = $(BAZEL_BUILD_FLAGS) $(BUILD_FLAGS) $(BAZEL_TARGET)

XLA_REV ?= 2a6015f068e4285a69ca9a535af63173ba92995b
XLA_ARCHIVE = $(XLA_REV).tar.gz
XLA_URL = https://github.com/openxla/xla/archive/$(XLA_ARCHIVE)

XLA_DIR = $(BUILD_DIR)/xla-$(XLA_REV)
XLA_EXT = xla/extension
XLA_EXT_DIR = $(XLA_DIR)/$(XLA_EXT)
XLA_TARBALL = $(XLA_DIR)/bazel-bin/$(XLA_EXT)/xla_extension.tar.gz
TARBALL = xla_extension-$(XLA_TARGET_PLATFORM)-$(XLA_TARGET).tar.gz

$(TARBALL): $(BUILD_DIR) $(XLA_TARBALL)
	cp -f $(XLA_TARBALL) ./$(TARBALL)

$(XLA_TARBALL): $(XLA_DIR) extension/BUILD
	rm -f $(XLA_EXT_DIR) && \
	ln -s `pwd`/extension $(XLA_EXT_DIR) && \
	cd $(XLA_DIR) && \
	./configure.py --backend=$(XLA_TARGET) && \
	bazel build $(BAZEL_FLAGS)

# Clone OpenXLA
$(XLA_DIR):
	cd $(BUILD_DIR) && \
	rm -fr $(XLA_DIR) $(XLA_ARCHIVE) && \
	wget $(XLA_URL) && \
	tar zxf $(XLA_ARCHIVE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	cd $(XLA_DIR) && bazel clean --expunge
	rm -f $(XLA_EXT_DIR)
	rm -rf $(XLA_DIR)
