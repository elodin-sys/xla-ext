# Makefile for XLA Static Library Test

# Compiler
CXX := clang++

# Find the XLA archive
XLA_ARCHIVE := $(shell find ~/Library/Caches/xla -name "xla_extension-*.tar.gz" | head -1)

# Extract directory
XLA_EXTRACTED := ./xla_extension

# Compiler flags
CXXFLAGS := -std=c++17 -O2 -Wall
CXXFLAGS += -I$(XLA_EXTRACTED)/include
# Suppress warnings from XLA headers
CXXFLAGS += -Wno-deprecated-declarations
CXXFLAGS += -Wno-nullability-completeness
CXXFLAGS += -Wno-invalid-specialization
CXXFLAGS += -Wno-invalid-offsetof

# Linker flags
LDFLAGS := -L$(XLA_EXTRACTED)/lib
LDFLAGS += -lxla_extension

# Additional linker flags from the .link file
LINK_FLAGS := $(shell cat $(XLA_EXTRACTED)/lib/libxla_extension.link 2>/dev/null || echo "")

# Targets
TARGET := test_xla
SIMPLE_TARGET := test_simple
COMPREHENSIVE_TARGET := test_comprehensive

# Source files
SOURCES := test_xla.cpp
SIMPLE_SOURCES := test_simple.cpp
COMPREHENSIVE_SOURCES := test_comprehensive.cpp
OBJECTS := $(SOURCES:.cpp=.o)
SIMPLE_OBJECTS := $(SIMPLE_SOURCES:.cpp=.o)
COMPREHENSIVE_OBJECTS := $(COMPREHENSIVE_SOURCES:.cpp=.o)

.PHONY: all clean extract run run-simple run-comprehensive simple comprehensive

all: extract $(TARGET)

simple: extract $(SIMPLE_TARGET)

comprehensive: extract $(COMPREHENSIVE_TARGET)

# Extract the XLA archive if not already extracted
extract:
	@if [ ! -d "$(XLA_EXTRACTED)" ]; then \
		echo "Extracting XLA archive..."; \
		if [ -z "$(XLA_ARCHIVE)" ]; then \
			echo "ERROR: XLA archive not found. Please run 'XLA_BUILD=true mix' first."; \
			exit 1; \
		fi; \
		tar -xzf $(XLA_ARCHIVE); \
		echo "Extracted to $(XLA_EXTRACTED)"; \
	else \
		echo "XLA already extracted"; \
	fi

# Build the test executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LINK_FLAGS) -o $(TARGET)
	@echo "✓ Build successful!"

# Build the simple test executable
$(SIMPLE_TARGET): $(SIMPLE_OBJECTS)
	@echo "Linking $(SIMPLE_TARGET)..."
	@echo "Link flags: $(LINK_FLAGS)"
	$(CXX) -o $(SIMPLE_TARGET) $(SIMPLE_OBJECTS) $(LDFLAGS) $(LINK_FLAGS)
	@echo "✓ Build successful!"

# Build the comprehensive test executable
$(COMPREHENSIVE_TARGET): $(COMPREHENSIVE_OBJECTS)
	@echo "Linking $(COMPREHENSIVE_TARGET)..."
	$(CXX) -o $(COMPREHENSIVE_TARGET) $(COMPREHENSIVE_OBJECTS) $(LDFLAGS) $(LINK_FLAGS)
	@echo "✓ Build successful!"

# Compile source files
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the test
run: $(TARGET)
	@echo ""
	@echo "Running XLA test..."
	@echo ""
	./$(TARGET)

# Run the simple test
run-simple: $(SIMPLE_TARGET)
	@echo ""
	@echo "Running simple XLA test..."
	@echo ""
	./$(SIMPLE_TARGET)

# Run the comprehensive test
run-comprehensive: $(COMPREHENSIVE_TARGET)
	@echo ""
	@echo "Running comprehensive XLA test..."
	@echo ""
	./$(COMPREHENSIVE_TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(SIMPLE_OBJECTS) $(COMPREHENSIVE_OBJECTS) $(TARGET) $(SIMPLE_TARGET) $(COMPREHENSIVE_TARGET)
	@echo "Cleaned build artifacts"

# Clean everything including extracted XLA
clean-all: clean
	rm -rf $(XLA_EXTRACTED)
	@echo "Cleaned everything"

# Show build configuration
info:
	@echo "Build Configuration:"
	@echo "  CXX:         $(CXX)"
	@echo "  CXXFLAGS:    $(CXXFLAGS)"
	@echo "  LDFLAGS:     $(LDFLAGS)"
	@echo "  LINK_FLAGS:  $(LINK_FLAGS)"
	@echo "  XLA_ARCHIVE: $(XLA_ARCHIVE)"
	@echo "  TARGET:      $(TARGET)"

